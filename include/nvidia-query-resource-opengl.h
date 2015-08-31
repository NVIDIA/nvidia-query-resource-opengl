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

#ifndef __NVIDIA_QUERY_RESOURCE_OPENGL_H__
#define __NVIDIA_QUERY_RESOURCE_OPENGL_H__

#include <sys/types.h>
#include <GL/gl.h>

#include "nvidia-query-resource-opengl-ipc.h"

#if defined (_WIN32)
#include <windows.h>
typedef DWORD pid_t;
#endif

typedef enum {
    NVQR_SUCCESS = 0,
    NVQR_ERROR_INVALID_ARGUMENT = 2,
    NVQR_ERROR_NOT_SUPPORTED = 3,
    NVQR_ERROR_UNKNOWN = 999,
} nvqrReturn_t;

typedef struct {
    pid_t pid;
    char *process_name;
#if defined(_WIN32)
    HANDLE server_handle;
    HANDLE client_handle;
#else
    int server_handle;
#endif
} NVQRConnection;

//------------------------------------------------------------------------------
// Open a connection to an OpenGL process. This process must be using an NVIDIA
// driver that supports the GL_query_resource_NVX extension. On Unix, this
// process must additionally have libnvidia-query-resource-opengl-preload.so
// preloaded into it with LD_PRELOAD. The connection may be reused for any
// number of query operations as long as it is left open; however, Windows only
// supports one client connection per OpenGL process.

nvqrReturn_t nvqr_connect(NVQRConnection *connection, pid_t pid);

//------------------------------------------------------------------------------
// Close an existing connection to an OpenGL process.

nvqrReturn_t nvqr_disconnect(NVQRConnection *connection);

//------------------------------------------------------------------------------
// Perform a glQueryResourceNVX() query in the remote OpenGL process. The
// process must be in the connected state when performing the query.

nvqrReturn_t nvqr_request_meminfo(NVQRConnection c, GLenum queryType,
                                  NVQRQueryDataBuffer *buf);

/* XXX GL_NVX_query_resource defines - these should be removed once the
 * extension has been finalized and promoted to GL_NV_query_resource, and
 * these values become part of real OpenGL header files. */
#ifndef GL_QUERY_RESOURCE_TYPE_SUMMARY_NVX

#define GL_QUERY_RESOURCE_TYPE_SUMMARY_NVX                  0x9540
#define GL_QUERY_RESOURCE_TYPE_DETAILED_NVX                 0x9541
#define GL_QUERY_RESOURCE_MEMTYPE_VIDMEM_NVX                0x9542
#define GL_QUERY_RESOURCE_MEMTYPE_SYSMEM_NVX                0x9543
#define GL_QUERY_RESOURCE_SYS_RESERVED_NVX                  0x9544
#define GL_QUERY_RESOURCE_TEXTURE_NVX                       0x9545
#define GL_QUERY_RESOURCE_RENDERBUFFER_NVX                  0x9546
#define GL_QUERY_RESOURCE_BUFFEROBJECT_NVX                  0x9547

#endif

#endif
