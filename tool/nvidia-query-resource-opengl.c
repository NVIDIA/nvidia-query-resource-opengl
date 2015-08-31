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
#include <stdarg.h>

#if defined(_WIN32)
#include <Windows.h>
#include <TlHelp32.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdbool.h>
#endif

#include <GL/gl.h>

#if defined (__sun) && defined (__SVR4)
#include <procfs.h>
#endif

#include "nvidia-query-resource-opengl.h"
#include "nvidia-query-resource-opengl-ipc.h"
#include "nvidia-query-resource-opengl-ipc-util.h"


#if defined (_WIN32)
// Older versions of MSVC didn't support stdbool.h
typedef int bool;
#ifndef true
#define true 1
#endif // true
#ifndef false
#define false 0
#endif // false
typedef HANDLE nvqr_handle_t;
typedef DWORD iosize_t;
char *(*nvqr_strdup)(const char *src) = _strdup;
#else
typedef int nvqr_handle_t;
typedef size_t iosize_t;
char *(*nvqr_strdup)(const char *src) = strdup;
#endif // _WIN32


static pid_t get_my_pid(void)
{
#if defined(_WIN32)
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}


//------------------------------------------------------------------------------
// Functions to abstract away the OS-specific IPC interface. The client_*()
// functions are no-ops on Unix, where bidirectional sockets are used.


static bool open_server_connection(nvqr_handle_t *handle, pid_t pid)
{
#if defined(_WIN32)
    char *name = nvqr_ipc_server_pipe_name(pid);
    int i;
    const int max_retries = 3;

    for (i = 0, *handle = INVALID_HANDLE_VALUE;
         i < max_retries && *handle == INVALID_HANDLE_VALUE;
         i++) {
        *handle = CreateFile(
          (LPCSTR)(LPCTSTR)name,
          GENERIC_WRITE,  // write access
          0,              // no sharing
          NULL,           // default security attributes
          OPEN_EXISTING,  // opens existing pipe
          0,              // default attributes
          NULL);          // no template file

        if (*handle == INVALID_HANDLE_VALUE &&
            GetLastError() == ERROR_PIPE_BUSY) {
            WaitNamedPipe((LPCSTR)(LPCSTR)name, 10000);
        }
    }
    free(name);

    return *handle != INVALID_HANDLE_VALUE;
#else
    bool connected = false;

    *handle = socket(PF_UNIX, SOCK_STREAM, 0);
    if (*handle != -1) {
        struct sockaddr_un addr;
        int len;

        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        len = nvqr_ipc_get_socket_name(addr.sun_path,
                                       sizeof(addr.sun_path), pid);
        if (len >= sizeof(addr.sun_path)) {
            fprintf(stderr, "Warning: socket name for pid %ld truncated - "
                            "name collision may be possible.", (long) pid);
        }
        connected = connect(*handle, (struct sockaddr *) &addr,
                            sizeof(addr)) == 0;
    }

    return connected;
#endif
}


static bool create_client(NVQRConnection *c)
{
#if defined(_WIN32)
    char *name = nvqr_ipc_client_pipe_name(get_my_pid());
    c->client_handle = CreateNamedPipe(
          (LPCSTR)(LPCTSTR)name,    // pipe name
          PIPE_ACCESS_INBOUND,      // read access
          PIPE_TYPE_MESSAGE |       // message type pipe
          PIPE_READMODE_MESSAGE |   // message-read mode
          PIPE_WAIT,                // blocking mode
          PIPE_UNLIMITED_INSTANCES, // max. instances
          512,                      // output buffer size
          512,                      // input buffer size
          0,                        // client time-out
          NULL);                    // default security attribute
    free(name);
    return c->client_handle != INVALID_HANDLE_VALUE;
#else
    return true;
#endif
}


static bool open_client_connection(NVQRConnection *c)
{
#if defined(_WIN32)
    return ConnectNamedPipe(c->client_handle, NULL) ||
           GetLastError() == ERROR_PIPE_CONNECTED;
#else
    return true;
#endif
}


static iosize_t write_file(nvqr_handle_t handle, const void *buf, iosize_t len)
{
    iosize_t bytes_written;
#if defined(_WIN32)

    if (!WriteFile(handle, buf, len, &bytes_written, NULL)) {
        bytes_written = 0;
    }
#else
    bytes_written = write(handle, buf, len);
#endif
    return bytes_written;
}


static void close_file(nvqr_handle_t handle)
{
#if defined (_WIN32)
    CloseHandle(handle);
#else
    close(handle);
#endif
}


static void flush_file(nvqr_handle_t handle)
{
#if defined (_WIN32)
    FlushFileBuffers(handle);
#else
    fsync(handle);
#endif
}


static void close_client_connection(NVQRConnection c)
{
#if defined (_WIN32)
    DisconnectNamedPipe(c.client_handle);
#else
#endif
}

static void destroy_client(NVQRConnection c)
{
#if defined(_WIN32)
    CloseHandle(c.client_handle);
#else
#endif
}


//-----------------------------------------------------------------------------
// Read a response from the server. This is done through the client handle
// (a named pipe) on Windows, and the server handle (a domain socket) on Unix.
static bool read_server_response(NVQRConnection c, NVQRQueryDataBuffer *buf) {
    iosize_t bytesRead;

    memset(buf, 0, sizeof(*buf));
#if defined (_WIN32)
    return ReadFile(c.client_handle, buf, sizeof(*buf), &bytesRead, NULL);
#else
    bytesRead = read(c.server_handle, buf, sizeof(*buf));
    return bytesRead == sizeof(*buf);
#endif
}


static bool write_server_command(NVQRConnection c, NVQRqueryOp op,
                                 int queryType, pid_t pid)
{
    NVQRQueryCmdBuffer buf;
    bool ret;

    memset(&buf, 0, sizeof(buf));
    buf.op = op;
    buf.queryType = queryType;
    buf.pid = pid;

    ret = write_file(c.server_handle, &buf, sizeof(buf)) == sizeof(buf);

    flush_file(c.server_handle);

    return ret;
}


//-----------------------------------------------------------------------------
// Send NVQR_QUERY_CONNECT to the server and verify that it ACKs with
// NVQR_QUERY_CONNECT. Returns TRUE on success; FALSE on failure.
static bool connect_to_server(NVQRConnection *conn)
{
    NVQRQueryDataBuffer data;
    bool ret = write_server_command(*conn, NVQR_QUERY_CONNECT, 0, get_my_pid());

    if (ret) {
        ret = open_client_connection(conn);

        if (ret) {
            if (!read_server_response(*conn, &data) ||
                data.op != NVQR_QUERY_CONNECT) {
                close_client_connection(*conn);
            }
        }
    }

    return ret;
}


static void close_server_connection(nvqr_handle_t handle)
{
    close_file(handle);
}


//-----------------------------------------------------------------------------
// Send NVQR_QUERY_MEMINFO to the server and verify that it ACKs with
// NVQR_QUERY_MEMINFO. Pass the meminfo read from the server back to the caller.
nvqrReturn_t nvqr_request_meminfo(NVQRConnection c, GLenum queryType,
                                         NVQRQueryDataBuffer *buf)
{
    if (write_server_command(c, NVQR_QUERY_MEMORY_INFO, queryType, 0) &&
        read_server_response(c, buf) &&
        buf->op == NVQR_QUERY_MEMORY_INFO)
    {
        return NVQR_SUCCESS;
    }

    return NVQR_ERROR_UNKNOWN;
}


//-----------------------------------------------------------------------------
// Send NVQR_QUERY_DISCONNECT to the server and verify that it ACKs with
// NVQR_QUERY_DISCONNECT. Returns TRUE on success; FALSE on failure.
static bool disconnect_from_server(NVQRConnection c)
{
    NVQRQueryDataBuffer data;

    return write_server_command(c, NVQR_QUERY_DISCONNECT, 0, 0) &&
                            read_server_response(c, &data) &&
                            data.op == NVQR_QUERY_DISCONNECT;
}


//-----------------------------------------------------------------------------
// Use the Toolhelp API on Win32 or the /proc virtual filesystem on Unix to
// determine the name of the process with the given PID. Strip all file path
// components except the last and return the process name to the caller in a
// newly heap-allocated buffer, or return NULL on error.
static char *process_name_from_pid(int pid)
{
    char *name = NULL;
#if defined (_WIN32)
    static const char directory_separator = '\\';
    HANDLE hTHSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hTHSnap) {
        PROCESSENTRY32 pe;
        int ret;

        pe.dwSize=(sizeof(pe));
        for (ret = Process32First(hTHSnap, &pe);
             ret;
             ret = Process32Next(hTHSnap, &pe)) {
            if (pe.th32ProcessID == pid) {
                name = nvqr_strdup(pe.szExeFile);
                break;
            }
        }
        CloseHandle(hTHSnap);
    }
#else
// Format string to determine the name of a procfs file to read
#if defined (__sun) && defined (__SVR4)
// Solaris exposes process info through a psinfo_t structure in a "psinfo" file
#define PROCFILE_FMT_STRING "/proc/%ld/psinfo"
#else
// FreeBSD and Linux expose the command line of a process in a "cmdline" file
#define PROCFILE_FMT_STRING "/proc/%ld/cmdline"
#endif

    static const char directory_separator = '/';
    int fd = -1;
    char *procfile;
    size_t len = strlen(PROCFILE_FMT_STRING) + NVQR_IPC_MAX_DIGIT_LENGTH;

    procfile = calloc(len + 1, 1);
    if (!procfile) {
        return NULL;
    }
    snprintf(procfile, len, PROCFILE_FMT_STRING, (long) pid);
    fd = open(procfile, O_RDONLY);
    free(procfile);

    if (fd != -1) {
#if defined (__sun) && defined (__SVR4)
        psinfo_t psinfo;

        if (read(fd, &psinfo, sizeof(psinfo)) == sizeof(psinfo)) {
            name = strdup(psinfo.pr_fname);
        }
#else
        int namelen;
        char c;

        // neither fstat(2) nor lseek(fd, 0, SEEK_END) seem to work for finding
        // the length of a /proc/$pid/cmdline file, so just crawl the file to
        // find the length of argv[0] for the target process id.
        for (namelen = 0; read(fd, &c, 1) == 1 && c; namelen++);
        lseek(fd, 0, SEEK_SET);

        name = calloc(namelen + 1, 1);
        if (name && read(fd, name, namelen) != namelen) {
            free(name);
            name = NULL;
        }
#endif // __sun && __SVR4
        close(fd);
    }
#endif // _WIN32
    if (name) {
        char *shortname, *fullname = name;

        shortname = strrchr(fullname, directory_separator);
        if (shortname) {
            shortname++;
        } else {
            shortname = fullname;
        }

        name = nvqr_strdup(shortname);
        free(fullname);
    }

    return name;
}


nvqrReturn_t nvqr_connect(NVQRConnection *connection, pid_t pid)
{
    memset(connection, 0, sizeof(*connection));
    connection->pid = pid;
    connection->process_name = process_name_from_pid(pid);

    if (!create_client(connection)) {
        return NVQR_ERROR_UNKNOWN;
    }

    if (!open_server_connection(&(connection->server_handle), pid)) {
        destroy_client(*connection);
        return NVQR_ERROR_NOT_SUPPORTED;
    }

    if (!connect_to_server(connection)) {
        destroy_client(*connection);
        close_server_connection(connection->server_handle);
        return NVQR_ERROR_UNKNOWN;
    }

    return NVQR_SUCCESS;
}


nvqrReturn_t nvqr_disconnect(NVQRConnection *connection)
{
    if (disconnect_from_server(*connection)) {
        close_client_connection(*connection);
        destroy_client(*connection);
        close_server_connection(connection->server_handle);
        free(connection->process_name);
        return NVQR_SUCCESS;
    }

    return NVQR_ERROR_UNKNOWN;
}
