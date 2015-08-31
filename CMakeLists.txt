# Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

cmake_minimum_required (VERSION 2.6)

project (nvidia-query-resource-opengl)

include_directories(include)

# The query tool executable

add_executable (nvqrgl-bin
    tool/main.c
)
set_target_properties (nvqrgl-bin PROPERTIES
    OUTPUT_NAME nvidia-query-resource-opengl
)

# Static library for custom clients

add_library (nvqrgl-lib STATIC
    common/nvidia-query-resource-opengl-ipc-util.c
    tool/nvidia-query-resource-opengl.c
    tool/nvidia-query-resource-opengl-data.c
)
set_target_properties (nvqrgl-lib PROPERTIES
    OUTPUT_NAME nvidia-query-resource-opengl
)

# Socket functionality is in a separate libsocket library on Solaris

include (CheckLibraryExists)

CHECK_LIBRARY_EXISTS (socket bind "" SOCKET_LIBRARY)
if (SOCKET_LIBRARY)
    set(LINK_SOCKET socket)
endif ()

target_link_libraries (nvqrgl-bin nvqrgl-lib ${LINK_SOCKET})

# Build the preload library on Unix

if (NOT WIN32)
    add_library (nvidia-query-resource-opengl-preload SHARED
        common/nvidia-query-resource-opengl-ipc-util.c
        preload/nvidia-query-resource-opengl-preload.c
    )

    # Find GL and X11 include / link paths

    # XXX find_path() doesn't seem to work on Solaris, but it's okay,
    # since the GL and X11 headers tend to be in /usr/include there.
    if (NOT "${CMAKE_SYSTEM_NAME}" MATCHES "SunOS")
        find_path (GL_INCLUDE_DIR GL/gl.h)
        find_path (X11_INCLUDE_DIR X11/Xlib.h)

        include_directories("${GL_INCLUDE_DIR}" "${X11_INCLUDE_DIR}")
    endif ()

    find_library (LIBGL_PATH GL)
    find_library (LIBX11_PATH X11)

    target_link_libraries (nvidia-query-resource-opengl-preload
        ${LIBGL_PATH} ${LIBX11_PATH} pthread ${LINK_SOCKET}
    )
endif ()
