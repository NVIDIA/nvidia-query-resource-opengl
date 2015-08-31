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

#define NVQR_MIN_DATA_FORMAT_VERSION 1
#define NVQR_MAX_DATA_FORMAT_VERSION 1
#define NVQR_VERSIONED_IDENT(IDENT, VERSION) IDENT ## _v ## VERSION
#define NVQR_VERSION_TYPEDEF(TYPE, VERSION) \
    typedef NVQR_VERSIONED_IDENT(TYPE, VERSION) TYPE

typedef int NVQRQueryData_t;

typedef struct NVQRQueryDetailedInfoRec_v1 {
    NVQRQueryData_t memType;
    NVQRQueryData_t objectType;
    NVQRQueryData_t memUsedkiB;
    NVQRQueryData_t numAllocs;
} NVQRQueryDetailedInfo_v1;
NVQR_VERSION_TYPEDEF(NVQRQueryDetailedInfo, NVQR_MAX_DATA_FORMAT_VERSION);

typedef struct NVQRQuerySummaryInfoRec_v1 {
    NVQRQueryData_t totalAllocs;
    NVQRQueryData_t vidMemUsedkiB;
    NVQRQueryData_t sysMemUsedkiB;
    NVQRQueryData_t vidMemFreekiB;
    NVQRQueryData_t sysMemFreekiB;
} NVQRQuerySummaryInfo_v1;
NVQR_VERSION_TYPEDEF(NVQRQuerySummaryInfo, NVQR_MAX_DATA_FORMAT_VERSION);

typedef struct NVQRQueryDeviceInfoRec_v1 {
    NVQRQuerySummaryInfo_v1 summary;
    NVQRQueryData_t numDetailedBlocks;
    NVQRQueryDetailedInfo_v1 detailed[];
} NVQRQueryDeviceInfo_v1;
NVQR_VERSION_TYPEDEF(NVQRQueryDeviceInfo, NVQR_MAX_DATA_FORMAT_VERSION);

typedef struct NVQRQueryDataHeaderRec_v1 {
    NVQRQueryData_t version;
    NVQRQueryData_t numDevices;
} NVQRQueryDataHeader_v1;
NVQR_VERSION_TYPEDEF(NVQRQueryDataHeader, NVQR_MAX_DATA_FORMAT_VERSION);

#define NVQR_MAX_DEVICES 16
#define NVQR_MAX_DETAILED_INFOS 8
#define NVQR_MAX_DATA_BUFFER_LEN ( \
    ( \
        sizeof(NVQRQueryDataHeader) + ( \
            NVQR_MAX_DEVICES * ( \
                sizeof(NVQRQueryDeviceInfo) + \
                NVQR_MAX_DETAILED_INFOS * sizeof(NVQRQueryDetailedInfo) \
            ) \
        ) \
    ) / sizeof(NVQRQueryData_t) \
)

// Helper functions for extracting data from raw int array

int nvqr_get_data_format_version (NVQRQueryData_t *data);
int nvqr_get_num_devices (NVQRQueryData_t *data);

// Returns a pointer to a DeviceInfo structure. The versioned functions
// (e.g. nvqr_get_device_v1()) must only be used with the correct data format
// version, and will return a pointer within the already allocated data
// array passed into the function. This pointer must not be freed.
// The unversioned nvqr_get_device() function will return a pointer to a *newly
// allocated* structure of the highest type version supported by this header,
// and copy data from the source array into this structure. The caller is
//responsible for freeing the memory allocated by nvqr_get_device().

NVQRQueryDeviceInfo_v1 *nvqr_get_device_v1(NVQRQueryData_t *data, int device);
NVQRQueryDeviceInfo *nvqr_get_device(NVQRQueryData_t *data, int device);

#endif
