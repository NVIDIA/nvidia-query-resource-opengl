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

#ifndef __NVIDIA_QUERY_RESOURCE_OPENGL_DATA_H__
#define __NVIDIA_QUERY_RESOURCE_OPENGL_DATA_H__

// Data types for interpreting data returned by the queryResource extension

#define NVQR_DATA_FORMAT_VERSION    2
#define NVQR_MAX_DATA_BUFFER_LEN    1024

typedef int NVQRQueryData_t;

typedef struct NVQRQueryDataHeaderRec {
    NVQRQueryData_t headerBlkSize;
    NVQRQueryData_t version;
    NVQRQueryData_t numDevices;
} NVQRQueryDataHeader;

typedef struct NVQRQueryDeviceInfoRec {
    NVQRQueryData_t deviceBlkSize;
    NVQRQueryData_t summaryBlkSize;
    NVQRQueryData_t totalAllocs;
    NVQRQueryData_t vidMemUsedkiB;
    NVQRQueryData_t vidMemFreekiB;
    NVQRQueryData_t numDetailBlocks;
} NVQRQueryDeviceInfo;

typedef struct NVQRQueryDetailInfoRec {
    NVQRQueryData_t detailBlkSize;
    NVQRQueryData_t memType;
    NVQRQueryData_t objectType;
    NVQRQueryData_t numAllocs;
    NVQRQueryData_t memUsedkiB;
} NVQRQueryDetailInfo;

typedef struct NVQRTagBlockRec {
    NVQRQueryData_t tagBlkSize;
    NVQRQueryData_t tagId;
    NVQRQueryData_t deviceId;
    NVQRQueryData_t numAllocs;
    NVQRQueryData_t vidmemUsedkiB;
    NVQRQueryData_t tagLength;
    char tag[4];
} NVQRTagBlock;

#endif
