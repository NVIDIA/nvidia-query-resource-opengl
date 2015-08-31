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

#ifndef __NVIDIA_QUERY_RESOURCE_IPC_UTIL_H__
#define __NVIDIA_QUERY_RESOURCE_IPC_UTIL_H__

// Windows IPC uses a pair of named pipes: one for each of the client and server
#if defined (_WIN32)
#include <Windows.h>

//------------------------------------------------------------------------------
// Return the pipe name for the client with the given PID in a newly allocated
// buffer. The server should call this function with the PID provided by the
// client, and the client should call this function with its own PID. The caller
// is responsible for freeing the buffer.

char *nvqr_ipc_client_pipe_name(DWORD pid);

//------------------------------------------------------------------------------
// Return the pipe name for the server with the given PID in a newly allocated
// buffer. The server should call this function with its own PID, and the client
// should call this function with the PID of the target server. The caller is
// responsible for freeing the buffer.

char *nvqr_ipc_server_pipe_name(DWORD pid);

// Unix IPC uses a domain socket created by the server
#else
#include <sys/types.h>

//------------------------------------------------------------------------------
// Write the socket name for the given pid into the provided buffer. On Linux,
// use the abstract namespace for domain sockets. On other Unixen, create the
// socket file in /tmp. Returns the length of the socket name, which may exceed
// the provided storage length. The socket name will be truncated if its length
// exceeds the available storage, so the caller should check the return value of
// this function to verify that the socket name was not truncated.

int nvqr_ipc_get_socket_name(char *dest, size_t len, pid_t pid);

#endif

//------------------------------------------------------------------------------
// Both ULONG_MAX and LONG_MIN (including the negative sign '-') are twenty
// characters long in decimal representation. It shouldn't be necessary to
// reserve more than twenty characters to represent any integer as a string.

#define NVQR_IPC_MAX_DIGIT_LENGTH 20

#endif
