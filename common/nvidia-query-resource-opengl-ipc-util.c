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
#include <stdlib.h>
#include <string.h>

#include "nvidia-query-resource-opengl-ipc-util.h"

#if defined (_WIN32)
static char *construct_name(const char *basename, DWORD pid)
{
    static const char basedir[] = "\\\\.\\pipe\\";
    size_t len = strlen(basedir) + strlen(basename) +
        NVQR_IPC_MAX_DIGIT_LENGTH;
    char *buf = calloc(len + 1, 1);

    if (!buf) {
        fprintf(stderr, "Failed to allocate memory for connection name!");
        exit(1);
    }

    _snprintf_s(buf, len + 1, len, "%s%s%d", basedir, basename, pid);
    return buf;
}

char *nvqr_ipc_client_pipe_name(DWORD pid)
{
    return construct_name("clientpipe", pid);
}

char *nvqr_ipc_server_pipe_name(DWORD pid)
{
    return construct_name("serverpipe", pid);
}

#else

int nvqr_ipc_get_socket_name(char *dest, size_t len, pid_t pid)
{
    int total_len;
    static const char *basename = "nvidia-query-resource-opengl-socket.";

#if __linux
    // Socket names in the abstract namespace are not strings; rather, the
    // entire contents of the sun_path field are considered. Zero out the
    // whole buffer to avoid surprises.
    memset(dest, 0, len);

    total_len = snprintf(dest, len, "0%s%d", basename, pid);
    dest[0] = '\0';
#else
    const char *basedir = "/tmp";

// Uncomment the below code if you want socket files to be created under
// XDG_RUNTIME_DIR on FreeBSD and Solaris, when that variable is set. The
// hardcoded default to "/tmp" is intentional, to facilitate use cases where
// the user performing the query may not be the same as the user running the
// queried process.

/*
    basedir = getenv("XDG_RUNTIME_DIR");
    if (!basedir) {
        basedir = "/tmp";
    }
 */

    total_len = snprintf(dest, len, "%s/%s%ld", basedir, basename, (long) pid);
#endif // __linux

    // Terminate the string properly if truncation occurred.
    if (total_len >= len) {
        dest[len - 1] = '\0';
    }

    return total_len;
}
#endif // _WIN32
