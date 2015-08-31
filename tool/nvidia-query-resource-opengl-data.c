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
#include <string.h>

#include "nvidia-query-resource-opengl-data.h"

// Returns the data format version of the provided data array. The version
// field should always be the first array element, so this helper doesn't
// need versioning like the others.

int nvqr_get_data_format_version (NVQRQueryData_t *data) {
    NVQRQueryDataHeader *header = (NVQRQueryDataHeader*) data;
    return header->version;
}

// Returns the number of devices whose information is listed in the provided
// data array, or a negative number on error.

static int get_num_devices_v1 (NVQRQueryData_t *data) {
    NVQRQueryDataHeader_v1 *header = (NVQRQueryDataHeader_v1*) data;

    return header->numDevices;
}

int nvqr_get_num_devices (NVQRQueryData_t *data) {
    switch (nvqr_get_data_format_version(data)) {
        case 1: return get_num_devices_v1(data);
        default: return -1;
    }
}

// Returns a pointer to a device info structure for the desired device within
// the provided data array, or NULL on error.

NVQRQueryDeviceInfo_v1 *nvqr_get_device_v1(NVQRQueryData_t *data, int device)
{
    void *ptr = (char *) data + sizeof(NVQRQueryDataHeader_v1);
    int num_devices, i;

    num_devices = get_num_devices_v1(data);

    if (num_devices <= 0) {
        return NULL;
    }

    if (device > NVQR_MAX_DEVICES || device > num_devices) {
        return NULL;
    }

    for (i = 0; i < device; i++) {
        NVQRQueryDeviceInfo_v1 *device = ptr;
        size_t offset = sizeof(*device) +
            device->numDetailedBlocks * sizeof(device->detailed[0]);
        ptr = (char *) ptr + offset;
    }

    return ptr;
}

NVQRQueryDeviceInfo *nvqr_get_device(NVQRQueryData_t *data, int device)
{
    NVQRQueryDeviceInfo *ret = NULL;

    switch (nvqr_get_data_format_version(data)) {
        int i;
        NVQRQueryDeviceInfo_v1 *info_v1;

        case 1:
            info_v1 = nvqr_get_device_v1(data, device);
            if (info_v1 == NULL) {
                break;
            }

            ret = calloc (sizeof(*ret) + info_v1->numDetailedBlocks *
                          sizeof(info_v1->detailed[0]), 1);
            if (ret == NULL) {
                break;
            }

            memcpy(&ret->summary, &info_v1->summary, sizeof(info_v1->summary));
            ret->numDetailedBlocks = info_v1->numDetailedBlocks;
            for (i = 0; i < info_v1->numDetailedBlocks; i++) {
                memcpy(&ret->detailed[i], &info_v1->detailed[i],
                       sizeof(info_v1->detailed[i]));
            }
        break;
        default: break;
    }

    return ret;
}

