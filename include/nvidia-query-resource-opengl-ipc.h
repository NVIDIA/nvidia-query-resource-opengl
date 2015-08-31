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

#ifndef __NVIDIA_QUERY_RESOURCE_OPENGL_IPC_H__
#define __NVIDIA_QUERY_RESOURCE_OPENGL_IPC_H__

#include "nvidia-query-resource-opengl-data.h"

// Data types used by the IPC between queryResource client and server

typedef enum {
    NVQR_QUERY_CONNECT = 1,
    NVQR_QUERY_MEMORY_INFO,
    NVQR_QUERY_DISCONNECT
} NVQRqueryOp;

typedef struct NVQRQueryCmdBufferRec {
    NVQRqueryOp op;
    int         queryType;
    int         pid;
} NVQRQueryCmdBuffer;

typedef struct NVQRQueryDataBufferRec {
    NVQRqueryOp     op;
    int             cnt;
    NVQRQueryData_t data[NVQR_MAX_DATA_BUFFER_LEN];
} NVQRQueryDataBuffer;

#endif
