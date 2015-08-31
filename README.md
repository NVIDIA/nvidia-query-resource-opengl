nvidia-query-resource-opengl
============================

About
-----

nvidia-query-resource-opengl queries the NVIDIA OpenGL driver to determine the
OpenGL resource usage of an application. OpenGL applications may query their
own resource usage using the GL\_NVX\_query\_resource extension, but the
nvidia-query-resource-opengl tool allows users to perform resource queries
externally, against unmodified OpenGL applications.

Requirements
------------

* A Windows, Linux, Solaris, or FreeBSD system with an NVIDIA GPU, running a
  version of the NVIDIA OpenGL driver supporting the GL\_NVX\_query\_resource
  extension. Support for this extension was introduced with the 355.xx driver
  release. **Note that this extension is still under development and subject to
  change, so applications developed against it, including this query resource
  tool, may need to be updated for compatibility with future driver versions.**
* CMake 2.6 or later, and a suitable build system (e.g. Windows SDK and/or
  Microsoft Visual Studio on Windows; make and cc/gcc on Unix-like systems)
  that is supported by the CMake generators on the target platform. (Not needed
  when using precompiled executables on Windows.)

Building
--------

nvidia-query-resource-opengl uses [CMake](http://www.cmake.org) to support
building on multiple platforms. In order to build nvidia-query-resource-opengl,
you will need to first use the CMake graphical or command line interface to
generate a build system that will work on your platform, then use the generated
build system to build the project. For example, on a typical Unix-like system,
the following commands run from within the top level directory of this source
code repository will create a "build" directory and build within it:

    mkdir build
    cd build
    cmake ..
    make

On Windows, run `nmake` instead of `make` from the Visual Studio command line
when using the nmake build system generator with the Windows SDK, or choose a
Visual Studio solution generator to create a solution that can be built within
Microsoft Visual Studio. Windows users may also download precompiled executable
files for convenience.

A successful build will produce the following items:

* The resource query tool, 'nvidia-query-resource-opengl' (on Windows, the .exe
  file extension is appended to the executable name.)
* A static library, 'libnvidia-query-resource-opengl.a' on Unix-like systems,
  or 'nvidia-query-resource-opengl.lib' on Windows. This can be used together
  with the API defined in include/nvidia-query-resource-opengl.h to add OpenGL
  resource query functionality to your own monitoring tools.
* On Unix-like systems only, the 'libnvidia-query-resource-preload.so' DSO,
  which must be preloaded into any OpenGL applications that will be the target
  of resource queries. (See "Usage" section below for more details.)

Usage
-----

You can query an application's OpenGL resource usage by executing the command:

    nvidia-query-resource-opengl -p <pid> [-qt <query_type>]

* pid: the process ID of the target OpenGL application of the query
* query\_type: this may be 'summary' or 'detailed'. The default is 'summary'.
  + summary: reports a summary, per device, of allocated video and system
     memory, the total amount of memory in use by the the driver, and the
     total amount of allocated but unused memory.
  + detailed: includes the summary information, and additionally reports
     separate allocation amounts for various object types. The current set
     of reported object types includes:
       - SYSTEM RESERVED - driver allocated memory
       - TEXTURE - memory in use by 1D/2D/3D textures
       - RENDERBUFFER - render buffer memory
       - BUFFEROBJ_ARRAY - buffer object memory

Resource queries are handled asynchronously to the OpenGL applications being
queried. Due to this, and other factors, including object migration between
video and system memory, it is possible for subsequent queries to yield
different results.

On Windows, nvidia-query-resource-opengl will communicate directly with any
OpenGL application to perform resource queries; however, on Unix-like systems,
the libnvidia-query-resource-preload.so DSO must be preloaded into the target
application before a resource query can be performed. This is achieved by
setting a relative or absolute path to the preload DSO in the LD\_PRELOAD
variable of the target application's environment, e.g.:

    $ LD_PRELOAD=path/to/libnvidia-query-resource-opengl-preload.so app
