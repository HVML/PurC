///////////////////////////////////////////////////////////////////////////////
//
//                          IMPORTANT NOTICE
//
// The following open source license statement does not apply to any
// entity in the Exception List published by FMSoft.
//
// For more information, please visit:
//
// https://www.fmsoft.cn/exception-list
//
//////////////////////////////////////////////////////////////////////////////
/*
 *   This file is part of MiniGUI, a mature cross-platform windowing
 *   and Graphics User Interface (GUI) support system for embedded systems
 *   and smart IoT devices.
 *
 *   Copyright (C) 2002~2020, Beijing FMSoft Technologies Co., Ltd.
 *   Copyright (C) 1998~2002, WEI Yongming
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Or,
 *
 *   As this program is a library, any link to this program must follow
 *   GNU General Public License version 3 (GPLv3). If you cannot accept
 *   GPLv3, you need to be licensed from FMSoft.
 *
 *   If you have got a commercial license of this program, please use it
 *   under the terms and conditions of the commercial license.
 *
 *   For more information about the commercial license, please refer to
 *   <http://www.minigui.com/blog/minigui-licensing-policy/>.
 */

#include "region.h"

bool foil_block_heap_init (foil_block_heap_p heap, size_t sz_block, size_t sz_heap)
{
    heap->sz_usage_bmp = (sz_heap + 7) >> 3;
    heap->sz_block = ROUND_TO_MULTIPLE (sz_block, SIZEOF_PTR);
    heap->sz_heap = heap->sz_usage_bmp << 3;
    heap->nr_alloc = 0;

    if (heap->sz_heap == 0 || heap->sz_block == 0)
        goto failed;

    if ((heap->heap = calloc (heap->sz_heap, heap->sz_block)) == NULL)
        goto failed;

    if ((heap->usage_bmp = calloc (heap->sz_usage_bmp, sizeof (char))) == NULL) {
        free (heap->heap);
        goto failed;
    }

    memset (heap->usage_bmp, 0xFF, heap->sz_usage_bmp);
    return true;

failed:
    heap->heap = NULL;
    heap->usage_bmp = NULL;
    return false;
}

foil_block_heap_p foil_block_heap_new(size_t sz_block, size_t sz_heap)
{
    foil_block_heap_p heap = calloc(1, sizeof(*heap));
    if (heap) {
        if (!foil_block_heap_init(heap, sz_block, sz_heap)) {
            free(heap);
            return NULL;
        }
    }

    return heap;
}

void* foil_block_heap_alloc (foil_block_heap_p heap)
{
    int free_slot;
    unsigned char* block_data = NULL;

    free_slot = foil_lookfor_unused_slot (heap->usage_bmp, heap->sz_usage_bmp, 1);
    if (free_slot >= 0) {
        block_data = heap->heap + heap->sz_block * free_slot;
        goto ret;
    }

    if ((block_data = calloc (1, heap->sz_block)) == NULL)
        goto ret;

    heap->nr_alloc++;

ret:

    return block_data;
}

void foil_block_heap_free (foil_block_heap_p heap, void* data)
{
    unsigned char* block_data = data;

    if (block_data < heap->heap ||
            block_data > (heap->heap + heap->sz_block * heap->sz_heap)) {
        free (block_data);
        heap->nr_alloc--;
    }
    else {
        int slot;
        slot = (block_data - heap->heap) / heap->sz_block;
        foil_slot_clear_use (heap->usage_bmp, slot);
    }
}

void foil_block_heap_cleanup (foil_block_heap_p heap)
{
    size_t nr_free_slots;

    if (heap->nr_alloc > 0) {
        LOG_WARN ("There are still not freed extra blocks in the block heap: %p (%zu)\n",
                heap, heap->nr_alloc);
    }

    nr_free_slots = foil_get_nr_idle_slots (heap->usage_bmp, heap->sz_usage_bmp);
    if (nr_free_slots != heap->sz_heap) {
        LOG_WARN ("There are still not freed blocks in the block heap: %p (%zu)\n",
                heap, heap->sz_heap - nr_free_slots);
    }

    free (heap->heap);
    heap->heap = NULL;

    free (heap->usage_bmp);
    heap->usage_bmp = NULL;
}

void foil_block_heap_delete (foil_block_heap_p heap)
{
    foil_block_heap_cleanup(heap);
    free(heap);
}

