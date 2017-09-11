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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdbool.h>
#include <signal.h>
#include <stdarg.h>
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include "nvidia-query-resource-opengl-ipc.h"
#include "nvidia-query-resource-opengl-ipc-util.h"

__attribute__((constructor)) void queryResourcePreloadInit(void);
__attribute__((destructor)) void queryResourcePreloadExit(void);

#define NVQR_QUEUE_MAX 8

/* XXX GL_NV_query_resource defines - these should be removed once the
 * extension has been finalized and these values become part of real 
 * OpenGL header files. */
#ifndef GL_NV_query_resource
#define GL_NV_query_resource 1

#ifdef GL_GLEXT_PROTOTYPES
GLAPI GLint GLAPIENTRY glQueryResourceNV (GLenum queryType, GLuint pname, GLuint bufSize, GLint *buffer);
#endif /* GL_GLEXT_PROTOTYPES */
typedef GLint (GLAPIENTRYP PFNGLQUERYRESOURCENVPROC) (GLenum queryType, GLuint pname, GLuint bufSize, GLint *buffer);

#endif

#define NVQR_EXTENSION ((const GLubyte *)"glQueryResourceNV")

#define SOCKET_NAME_MAX_LENGTH sizeof(((struct sockaddr_un *)0)->sun_path)
static char socket_name[SOCKET_NAME_MAX_LENGTH];
static int socket_fd = -1;

static int clientsConnected = 0;
// Mutex around connect/disconnect requests from clients
static pthread_mutex_t connect_lock;
// Mutex around query requests from clients: the GLXContext needs to be made
// current to the thread that is making the query
static pthread_mutex_t query_lock;

static PFNGLQUERYRESOURCENVPROC glQueryResourceNV = NULL;

static Display *dpy = NULL;
static GLXContext ctx = NULL;


//------------------------------------------------------------------------------
// Release any X11/GLX resources that have been created
static void cleanup_glx_resources(void) {
    if (ctx) {
        glXDestroyContext(dpy, ctx);
        ctx = NULL;
    }
    if (dpy) {
        XCloseDisplay(dpy);
        dpy = NULL;
    }
}


//------------------------------------------------------------------------------
// Wrapper around vfprintf(3) that prepends a header and appends a newline
static void print_msg(FILE *stream, const char *type, const char *fmt,
                      va_list vargs)
{
    fprintf(stream, "NVIDIA QUERY RESOURCE %s: ", type);
    vfprintf(stream, fmt, vargs);
    fprintf(stream, "\n");
}


//------------------------------------------------------------------------------
// Print an error message to stderr with the NVQR header
static void error_msg(const char *fmt, ...)
{
    va_list vargs;

    va_start(vargs, fmt);
    print_msg(stderr, "ERROR", fmt, vargs);
    va_end(vargs);
}


//------------------------------------------------------------------------------
// Print a warning message to stderr with the NVQR header
static void warning_msg(const char *fmt, ...)
{
    va_list vargs;

    va_start(vargs, fmt);
    print_msg(stderr, "WARNING", fmt, vargs);
    va_end(vargs);
}


//------------------------------------------------------------------------------
// Lazily create the GLX context that will be used to service query requests,
// if no clients are currently connected, and increment the connected client
// refcount if the context was successfully created.
static bool connectToClient(void)
{
    bool success = false;

    pthread_mutex_lock(&connect_lock);

    if (clientsConnected == 0) {
        int screen;
        XVisualInfo *visual;
        static int attribs[] = { GLX_RGBA, None };

        // connect to X and create a GLX context
        // XOpenDisplay(NULL) + DefaultScreen(dpy) may not give same display app
        // is using: may need to revisit this if issues come up
        dpy = XOpenDisplay(NULL);
        if (dpy == NULL) {
            error_msg("failed to open X11 display");
            goto done;
        }
        screen = DefaultScreen(dpy);

        visual = glXChooseVisual(dpy, screen, attribs);
        if (visual == NULL) {
            error_msg("failed to choose a GLX visual");
            goto done;
        }

        ctx = glXCreateContext(dpy, visual, NULL, True);
        XFree(visual);

        if (ctx == NULL) {
            error_msg("failed to create GLX context");
            goto done;
        }
    }

    clientsConnected++;
    success = true;

  done:

    if (!success) {
        cleanup_glx_resources();
    }

    pthread_mutex_unlock(&connect_lock);

    return success;
}

//------------------------------------------------------------------------------
// Decrement the connected clients refcount and clean up the GLX context and
// X11 resources if the last client has disconnected.
static void disconnectFromClient(void)
{
    pthread_mutex_lock(&connect_lock);

    clientsConnected--;

    if (clientsConnected == 0) {
        cleanup_glx_resources();
    }

    pthread_mutex_unlock(&connect_lock);
}


//------------------------------------------------------------------------------
// Make the GLX context current to the current thread and perform the resource
// query. Returns 0 on failure, and passes along the return value of the
// resource query operation on success.
static int do_query(GLenum queryType, size_t len, int *data)
{
    int ret = 0;

    pthread_mutex_lock(&query_lock);

    if (glXMakeCurrent(dpy, None, ctx)) {
        ret = glQueryResourceNV(queryType, -1, len, data);
        if (!glXMakeCurrent(dpy, None, NULL)) {
            ret = 0;
        }
    }

    pthread_mutex_unlock(&query_lock);

    return ret;
}

//------------------------------------------------------------------------------
// Handle client requests over an accept(2)ed (accept(3socket) on Solaris)
// domain socket connection. Keep the connection open until a disconnect
// request is received from the client or an error occurs.
static void *process_client_commands(void *fdp)
{
    NVQRQueryCmdBuffer readBuffer;
    NVQRQueryDataBuffer writeBuffer;
    int fd = *(int*)fdp;
    bool connected = false, connection_successful = false;
    sigset_t block_signals;

    // Suppress SIGPIPE in this thread, in case a client closes its connection
    // before the server can respond to a request.
    sigemptyset(&block_signals);
    sigaddset(&block_signals, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &block_signals, NULL);

    do {
        memset(&writeBuffer, 0, sizeof(writeBuffer));

        // read a command from the query tool
        read(fd, &readBuffer, sizeof(readBuffer));
        // by default, echo back the command op to the caller
        writeBuffer.op = readBuffer.op;

        // handle query commands appropriately
        switch(readBuffer.op) {
            // connect the client
            case NVQR_QUERY_CONNECT:
                    connected = connection_successful = connectToClient();

                    if (connected) {
                        break; // fall through to error if connection fails
                    }

            // perform the resource query
            case NVQR_QUERY_MEMORY_INFO:
                if (connected) {
                    writeBuffer.cnt =  do_query(readBuffer.queryType,
                                                sizeof(writeBuffer.data),
                                                writeBuffer.data);
                    if (writeBuffer.cnt) {
                        break; // fall through to error if the query fails
                    }
                } // also fall through if the client was not connected

            // Handle connection/query errors and unknown commands
            default:
                writeBuffer.op = 0; // tell the client there was an error
                // fall through to disconnect

            // disconnect the client
            case NVQR_QUERY_DISCONNECT:
                connected = false;
                break;
        }

        // write a response to the client: if the client is already disconnected
        // the write will fail with EPIPE. handle all write failures by closing
        // the connection.
        if (write(fd, &writeBuffer, sizeof(writeBuffer)) !=
            sizeof(writeBuffer)) {
            connected = false;
        }
    } while (connected);

    if (connection_successful) {
        disconnectFromClient();
    }

    close(fd);
    return NULL;
}

//------------------------------------------------------------------------------
// Bind the domain socket and begin accepting client connections, spawning
// a new thread for each connection.
static void *queryResourcePreloadThread(void *ptr)
{
    struct sockaddr_un addr;
    socklen_t addrlen = sizeof(addr);
    int accept_fd;
    pid_t my_pid = getpid();

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    // socket_name may begin with '\0', so use memcpy(3) instead of strncpy(3)
    memcpy(addr.sun_path, socket_name, SOCKET_NAME_MAX_LENGTH);

    if (bind(socket_fd, (struct sockaddr*) &addr, sizeof(addr)) != 0) {
        error_msg("failed to bind socket for pid %ld.", (long) my_pid);
        return NULL;
    }

    if (listen(socket_fd, NVQR_QUEUE_MAX) != 0) {
        error_msg("failed to listen on pid %ld's socket.", (long) my_pid);
        return NULL;
    }

    while ((accept_fd = accept(socket_fd, (struct sockaddr*) &addr, &addrlen))
           != -1) {
        pthread_t thread;

        pthread_create(&thread, NULL, process_client_commands, &accept_fd);
    }

    return NULL;
}

//------------------------------------------------------------------------------
// Create a domain socket and spawn a thread to accept connections over it.
__attribute__((constructor)) void queryResourcePreloadInit(void)
{
    pthread_t queryThreadId;
    pid_t my_pid = getpid();

    pthread_mutex_init(&connect_lock, NULL);

    glQueryResourceNV =
        (PFNGLQUERYRESOURCENVPROC) glXGetProcAddressARB(NVQR_EXTENSION);

    if (glQueryResourceNV == NULL) {
        // XXX should check extension string once extension is exported there
        error_msg("failed to load %s", NVQR_EXTENSION);
        return;
    }

    socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        error_msg("failed to create socket.");
        return;
    }

    if (nvqr_ipc_get_socket_name(socket_name, SOCKET_NAME_MAX_LENGTH, my_pid) >=
        SOCKET_NAME_MAX_LENGTH) {
        warning_msg("socket name for pid %ld truncated - "
                    "name collision may be possible.", (long) my_pid);
    }

    // create the thread
    if (!XInitThreads()) {
        error_msg("failed to initialize X threads.");
        return;
    }
    pthread_create(&queryThreadId, NULL, &queryResourcePreloadThread, NULL);
}

//------------------------------------------------------------------------------
// Clean up resources
__attribute__((destructor)) void queryResourcePreloadExit(void)
{
    pthread_mutex_destroy(&connect_lock);
    pthread_mutex_destroy(&query_lock);

    if (socket_fd != -1) {
        close(socket_fd);
        unlink(socket_name);
    }
}
