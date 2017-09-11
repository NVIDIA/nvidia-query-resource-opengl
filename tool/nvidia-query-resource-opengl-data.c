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
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#if defined (_WIN32)
#include <Windows.h>
#endif
#include <GL/gl.h>

#include "nvidia-query-resource-opengl.h"
#include "nvidia-query-resource-opengl-data.h"

//------------------------------------------------------------------------------
// print the tag info
//
static void print_tag_info(int tagCount, NVQRQueryData_t *tagBuffer)
{
    int i;
    NVQRQueryData_t *ptr = tagBuffer;

    printf("\n  number of tags = %d\n", tagCount);
    for (i = 0; i < tagCount; i++) {
        NVQRTagBlock *tag = (NVQRTagBlock *)ptr;
        ptr += (tag->tagBlkSize + tag->tagLength); // skip over to next tag
        printf("    device %d: tagId %d - %s\n", tag->deviceId, tag->tagId, tag->tag);
        if (tag->numAllocs != 0) {
            printf("      number of vidmem allocations = %d, vidmem allocated = %d kb\n", 
                    tag->numAllocs, tag->vidmemUsedkiB);
        }
    }
}

//------------------------------------------------------------------------------
// print the detailed memory info for a single device
//
static void print_detail_info(int detailCount, NVQRQueryData_t *detailBuffer)
{
    int i;
    NVQRQueryData_t *ptr = detailBuffer;
    NVQRQueryDetailInfo *nextDetailBlk = (NVQRQueryDetailInfo *)ptr;

    for (i = 0; i < detailCount; i++) {
        NVQRQueryDetailInfo *detailBlk;

        detailBlk = nextDetailBlk;
        ptr += detailBlk->detailBlkSize; // step to next detail block
        nextDetailBlk = (NVQRQueryDetailInfo *)ptr;

        printf("    %6d kiB ", detailBlk->memUsedkiB);
        switch (detailBlk->objectType) {
            case GL_QUERY_RESOURCE_SYS_RESERVED_NV:     printf("SYSTEM RESERVED"); break;
            case GL_QUERY_RESOURCE_TEXTURE_NV:          printf("TEXTURE"); break;
            case GL_QUERY_RESOURCE_RENDERBUFFER_NV:     printf("RENDERBUFFER"); break;
            case GL_QUERY_RESOURCE_BUFFEROBJECT_NV:     printf("BUFFEROBJ_ARRAY"); break;
            default:                                    printf("UNKNOWN ALLOCATION TYPE"); break;
        } 
        printf(", number of allocations = %d\n", detailBlk->numAllocs);
    }
}


//------------------------------------------------------------------------------
// print the memory info for a single device
//
static void print_device_info(GLenum queryType, NVQRQueryDeviceInfo *devInfo)
{
    printf("number of memory resource allocations = %d\n", devInfo->totalAllocs);

    if (devInfo->totalAllocs > 0) {
        printf("    vidmem allocated = %d kiB (in use = %d kiB, free = %d kiB\n",
                devInfo->vidMemUsedkiB + devInfo->vidMemFreekiB, 
                devInfo->vidMemUsedkiB, 
                devInfo->vidMemFreekiB);
        if (devInfo->numDetailBlocks > 0) {
            NVQRQueryData_t *detailInfo = ((NVQRQueryData_t *)devInfo + devInfo->summaryBlkSize);
            print_detail_info(devInfo->numDetailBlocks, detailInfo);
        }
    }
}

void nvqr_print_memory_info(GLenum queryType, NVQRQueryData_t *buffer)
{
    NVQRQueryData_t *ptr = buffer;
    NVQRQueryDataHeader *header;
    NVQRQueryDeviceInfo *nextDeviceBlk;
    int num_devices, num_tags;
    int i;

    header = (NVQRQueryDataHeader *)ptr;
    ptr += header->headerBlkSize; // step over header to first device block
    nextDeviceBlk = (NVQRQueryDeviceInfo *)ptr;

    num_devices = header->numDevices;
    for (i = 0; i < num_devices; i++) {
        NVQRQueryDeviceInfo *deviceBlk;

        deviceBlk = nextDeviceBlk;
        ptr += deviceBlk->deviceBlkSize; // step over to next device block
        nextDeviceBlk = (NVQRQueryDeviceInfo *)ptr;

        printf("  Device %d: ", i);
        print_device_info(queryType, deviceBlk);
    }

    num_tags = *ptr++;
    if (num_tags > 0) {
        print_tag_info(num_tags, ptr);
    }
}
