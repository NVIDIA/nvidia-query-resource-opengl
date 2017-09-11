/*
 * Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#if defined (_WIN32)
#include <Windows.h>
#endif
#include <GL/gl.h>

#include "nvidia-query-resource-opengl.h"
#include "nvidia-query-resource-opengl-data.h"

static void print_help(const char *progname)
{
    printf("Query OpenGL resource (vidmem and GPU-mapped sysmem) usage\n\n"
           "Usage: %s -p pid\n"
           "       %s -h\n\n"
           "  -h: print this help message\n"
           "  -p <pid>: select process to query\n",
           progname, progname);
}


//------------------------------------------------------------------------------
// Parse the command line and pass the values of any parsed options.
static nvqrReturn_t parse_commandline(int argc, char * const * const argv,
                                      pid_t *pid, GLenum *queryType)
{
    int i;

    // default values
    *queryType = GL_QUERY_RESOURCE_TYPE_VIDMEM_ALLOC_NV;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            // help
            print_help(argv[0]);
            return NVQR_SUCCESS;
        } else if (strcmp(argv[i], "-p") == 0) {
            // specific pid
            i++;

            if (i < argc) {
                *pid = atoi(argv[i]);
            } else {
                print_help(argv[0]);
                return NVQR_ERROR_INVALID_ARGUMENT;
            }
        } else {
            print_help(argv[0]);
            return NVQR_ERROR_INVALID_ARGUMENT;
        }
    }

    // validation
    if (*pid == 0) {
        // PID 0 on Unix is the scheduler, and on Windows is the System Idle
        // process, neither of which is a valid target for queryResources.
        // If the PID is zero, we may assume that the user did not set one,
        // and if the user actually did set a PID of zero, we can treat that
        // as an invalid request.
        print_help(argv[0]);
        return NVQR_ERROR_INVALID_ARGUMENT;
    }

    return NVQR_SUCCESS;
}


int main (int argc, char * const * const argv)
{
    NVQRConnection connection;
    NVQRQueryDataBuffer buffer;
    pid_t pid = 0;
    GLenum queryType;
    nvqrReturn_t result;

    result = parse_commandline(argc, argv, &pid, &queryType);
    if (result != NVQR_SUCCESS) {
        fprintf(stderr, "%s: invalid command line\n", argv[0]);
        return result;
    }

    result = nvqr_connect(&connection, pid);
    if (result != NVQR_SUCCESS) {
        if (result == NVQR_ERROR_NOT_SUPPORTED &&
            connection.process_name) {
            printf("Resource query not supported for '%s' (pid %ld)\n",
                   connection.process_name, (long) connection.pid);
        } else {
            fprintf(stderr, "Error: failed to open connection to pid %ld\n",
                    (long) connection.pid);
        }
        return result;
    }

    result = nvqr_request_meminfo(connection, queryType, &buffer);
    if (result == NVQR_SUCCESS) {
        NVQRQueryDataHeader *header = (NVQRQueryDataHeader *)&buffer.data;
        if (header->version != NVQR_DATA_FORMAT_VERSION) {
            fprintf(stderr, "Error: unrecognized data format version '%d'. "
                    "(version supported: %d)\n", header->version,
                    NVQR_DATA_FORMAT_VERSION);
            result = nvqr_disconnect(&connection);
            return result;
        }

        if (connection.process_name) {
            printf("%s, pid = %ld, data format version %d\n",
                   connection.process_name, (long) connection.pid, header->version);
        }

        nvqr_print_memory_info(queryType, buffer.data);
    } else {
        fprintf(stderr, "Error: failed to query resource usage information "
                "for pid %ld.\n", (long) connection.pid);
    }

    result = nvqr_disconnect(&connection);
    return result;
}
