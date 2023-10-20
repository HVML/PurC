/*
** @file region.h
** @author Vincent Wei
** @date 2022/11/21
** @brief Interface of region consisting of multiple rectangles.
**  Note that we copied most of code from GPL'd MiniGUI:
**
**      <https://github.com/VincentWei/MiniGUI/>
**
** Copyright (C) 2002~2022, Beijing FMSoft Technologies Co., Ltd.
** Copyright (C) 1998~2002, WEI Yongming
**
** This file is a part of purc, which is an HVML interpreter with
** a command line interface (CLI).
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef purc_foil_region_h
#define purc_foil_region_h

#include "rect.h"

typedef struct foil_block_heap {
    /** Size of one block element in bytes. */
    size_t          sz_block;
    /** Size of the heap in blocks. */
    size_t          sz_heap;
    /** The number of blocks extra allocated. */
    size_t          nr_alloc;
    /** The size of usage bitmap in bytes. */
    size_t          sz_usage_bmp;

    /** The pointer to the pre-allocated heap. */
    unsigned char*  heap;
    /** The pointer to the usage bitmap. */
    unsigned char*  usage_bmp;
} foil_block_heap;

/**
 * \var typedef foil_block_heap* foil_block_heap_p
 * \brief Data type of the pointer to a foil_block_heap.
 *
 * \sa foil_block_heap
 */
typedef foil_block_heap* foil_block_heap_p;

/**
 * Region rectangle structure.
 */
typedef struct _foil_rgnrc {
    /** The region rectangle itself.  */
    foil_rect rc;

    /** The pointer to the next region rectangle. */
    struct _foil_rgnrc* next;
    /** The pointer to the previous region rectangle. */
    struct _foil_rgnrc* prev;
} foil_rgnrc;
typedef foil_rgnrc* foil_rgnrc_p;

/* Region Type */
#define NULLREGION      0x00
#define SIMPLEREGION    0x01
#define COMPLEXREGION   0x02

/**
 * Region structure, alos used for general regions.
 */
typedef struct foil_region {
   /**
    * Type of the region, can be one of the following value:
    *   - NULLREGION\n
    *     A null region.
    *   - SIMPLEREGION\n
    *     A simple region.
    *   - COMPLEXREGION\n
    *     A complex region.
    */
    uint8_t             type;
   /**
    * Reserved for alignment.
    */
    uint8_t             reserved[3];
   /**
    * The bounding rect of the region.
    */
    foil_rect           rcBound;
   /**
    * Head of the region rectangle list.
    */
    foil_rgnrc_p        head;
   /**
    * Tail of the region rectangle list.
    */
    foil_rgnrc_p        tail;
   /**
    * The private block data heap used to allocate region rectangles.
    * \sa foil_block_heap
    */
    foil_block_heap_p   heap;
} foil_region;

/**
 * \var typedef foil_region* foil_region_p
 * \brief Data type of the pointer to a foil_region.
 *
 * \sa foil_region
 */
typedef foil_region* foil_region_p;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \fn bool foil_block_heap_init (foil_block_heap_p heap,
                size_t bd_size, size_t heap_size)
 * \brief Initializes a private block data heap.
 *
 * This function initializes a block data heap pointed to by \a heap.
 * It will allocate the buffer used by the heap from system heap by using
 * \a malloc(3). Initially, the heap has \a heap_size blocks free, and each
 * is \a bd_size bytes long.
 *
 * \param heap The pointer to the heap structure.
 * \param bd_size The size of one block in bytes.
 * \param heap_size The size of the heap in blocks.
 *
 * \return true on success, false on error.
 *
 * \sa foil_block_heap
 */
bool foil_block_heap_init (foil_block_heap_p heap,
                size_t bd_size, size_t heap_size);

foil_block_heap_p foil_block_heap_new (size_t bd_size, size_t heap_size);

/**
 * \fn void* foil_block_heap_alloc (foil_block_heap_p heap)
 * \brief Allocates a data block from private heap.
 *
 * This function allocates a data block from an initialized
 * block data heap. The allocated block will have the size of \a heap->bd_size.
 * If there is no free block in the heap, this function will try to allocate
 * the block from the system heap by using \a malloc(3) function.
 *
 * \param heap The pointer to the initialized heap.
 * \return Pointer to the allocated data block, NULL on error.
 *
 * \sa foil_block_heap_init, foil_block_heap_free
 */
void* foil_block_heap_alloc (foil_block_heap_p heap);

/**
 * \fn void foil_block_heap_free (foil_block_heap_p heap, void* data)
 * \brief Frees an allocated data block.
 *
 * This function frees the specified data block pointed to by \a data to
 * the heap \a heap. If the block was allocated by using \a malloc function,
 * this function will free the element by using \a free(3) function.
 *
 * \param heap The pointer to the heap.
 * \param data The pointer to the element to be freed.
 *
 * \sa foil_block_heap_init, foil_block_heap_alloc
 */
void foil_block_heap_free (foil_block_heap_p heap, void* data);

/**
 * \fn void foil_block_heap_cleanup (foil_block_heap_p heap)
 * \brief Cleans up a private block data heap.
 *
 * \param heap The pointer to the heap to be cleaned up.
 *
 * \sa foil_block_heap_init, foil_block_heap_delete
 */
void foil_block_heap_cleanup (foil_block_heap_p heap);

/**
 * \fn void foil_block_heap_delete (foil_block_heap_p heap)
 * \brief Deletes a private block data heap.
 *
 * \param heap The pointer to the heap to be deleted.
 *
 * \sa foil_block_heap_init, foil_block_heap_cleanup
 */
void foil_block_heap_delete (foil_block_heap_p heap);

/**
 * \def foil_region_rect_heap_init(heap, size)
 * \brief Initializes the private block data heap used to allocate
 * region rectangles.
 *
 * \param heap The pointer to a foil_block_heap structure.
 * \param size The size of the heap.
 *
 * \note This macro is defined to call \a foil_block_heap_init function
 *       with \a bd_size set to \a sizeof(foil_rgnrc).
 *
 * \sa foil_block_heap_init
 */
#define foil_region_rect_heap_init(heap, size)    \
                foil_block_heap_init (heap, sizeof (foil_rgnrc), size)

#define foil_region_heap_rect_new(size)    \
                foil_block_heap_new (sizeof (foil_rgnrc), size)

/**
 * \def foil_region_rect_alloc(heap)
 * \brief Allocates a region rectangles from the private block data heap.
 *
 * \param heap The pointer to the initialized foil_block_heap structure.
 *
 * \note This macro is defined to call \a foil_block_heap_alloc function.
 *
 * \sa foil_block_heap_alloc
 */
#define foil_region_rect_alloc(heap)     foil_block_heap_alloc (heap)

/**
 * \def foil_region_rect_free(heap, cr)
 * \brief Frees a region rectangle which is allocated from the private
 *        block data heap.
 *
 * \param heap The pointer to the initialized foil_block_heap structure.
 * \param cr The pointer to the region rectangle to be freed.
 *
 * \note This macro is defined to call \a foil_block_heap_free function.
 *
 * \sa foil_block_heap_free
 */
#define foil_region_rect_free(heap, cr)  foil_block_heap_free (heap, cr);

/**
 * \def foil_region_rect_heap_cleanup(heap)
 * \brief Destroys the private block data heap used to allocate region
 *        rectangles.
 *
 * \param heap The pointer to the foil_block_heap structure.
 *
 * \note This macro is defined to call \a foil_block_heap_cleanup function.
 *
 * \sa foil_block_heap_cleanup
 */
#define foil_region_rect_heap_cleanup(heap)   foil_block_heap_cleanup (heap);

#define foil_region_rect_heap_delete(heap)   foil_block_heap_delete (heap);

/**
 * \fn void foil_region_init (foil_region_p region, foil_block_heap_p rgnrc_heap)
 * \brief Initializes a region region.
 *
 * Before intializing a region region, you should initialize a private
 * block data heap first. The region operations, such as \a foil_region_union
 * function, will allocate/free the region rectangles from/to the heap.
 * This function will set the \a heap field of \a region to be \a rgnrc_heap,
 * and empty the region.
 *
 * \param region The pointer to the foil_region structure to be initialized.
 * \param rgnrc_heap The pointer to the initialized private block data heap.
 *
 * \sa foil_region_rect_heap_init, foil_region_empty.
 *
 * Example:
 *
 * \include initcliprgn.c
 */

void foil_region_init (foil_region_p region, foil_block_heap_p rgnrc_heap);

/**
 * \fn void foil_region_empty (foil_region_p region)
 * \brief Empties a region region.
 *
 * This function empties a region region pointed to by \a region.
 *
 * \param region The pointer to the region.
 *
 * \sa foil_region_init
 */
void foil_region_empty (foil_region_p region);

/**
 * \fn foil_region_p foil_region_new (void)
 * \brief Creates a region region.
 *
 * \return The pointer to the clip region.
 *
 * \sa foil_region_init, foil_region_empty, foil_region_delete.
 */

foil_region_p foil_region_new (foil_block_heap_p rgnrc_heap);

/**
 * \fn void foil_region_delete (foil_region_p region)
 * \brief Empties and destroys a region region.
 *
 * This function empties and destroys a region region pointed to by \a region.
 *
 * \param region The pointer to the region.
 *
 * \sa foil_region_init, foil_region_new
 */
void foil_region_delete (foil_region_p region);

/**
 * \fn bool foil_region_copy (foil_region_p dst_rgn, const foil_region* src_rgn)
 * \brief Copies one region to another.
 *
 * This function copies the region pointed to by \a src_rgn to the region
 * pointed to by \a dst_rgn.
 *
 * \param dst_rgn The destination region.
 * \param src_rgn The source region.
 *
 * \return true on success, otherwise false.
 *
 * \note This function will empty the region \a dst_rgn first.
 *
 * \sa foil_region_empty, foil_region_intersect, foil_region_union, foil_region_subtract, foil_region_xor
 */
bool foil_region_copy (foil_region_p dst_rgn, const foil_region* src_rgn);

/**
 * \fn bool foil_region_intersect (foil_region_p pRstRgn, \
                const foil_region* pRgn1, const foil_region* pRgn2)
 * \brief Intersects two region.
 *
 * This function gets the intersection of two regions pointed to by \a pRgn1
 * and \a pRgn2 respectively and puts the result to the region pointed to
 * by \a pRstRgn.
 *
 * \param pRstRgn The intersected result region.
 * \param pRgn1 The first region.
 * \param pRgn2 The second region.
 *
 * \return true on success, otherwise false.
 *
 * \note If \a pRgn1 does not intersected with \a pRgn2, the result region
 *       will be an emgty region.
 *
 * \sa foil_region_empty, foil_region_copy, foil_region_union, foil_region_subtract, foil_region_xor
 */
bool foil_region_intersect (foil_region_p dst_rgn,
                       const foil_region* pRgn1, const foil_region* pRgn2);

/**
 * \fn void foil_region_get_bound_rect (foil_region_p region, foil_rect *rect)
 * \brief Get the bounding rectangle of a region.
 *
 * This function gets the bounding rect of the region pointed to by \a region,
 * and returns the rect in the rect pointed to by \a rect.
 *
 * \param region The pointer to the region.
 * \param rect The pointer to the result rect.
 *
 * \sa foil_region_is_empty
 */
static inline void
foil_region_get_bound_rect (foil_region_p region, foil_rect *rect)
{
     *rect = region->rcBound;
}

/**
 * \fn bool foil_region_set (foil_region_p region, const foil_rect* rect)
 * \brief Set a region to contain only one rect.
 *
 * This function sets the region \a region to contain only a rect pointed to
 * by \a rect.
 *
 * \param region The pointer to the region.
 * \param rect The pointer to the rect.
 *
 * \return true on success, otherwise false.
 *
 * \note This function will empty the region \a region first.
 *
 * \sa foil_region_empty
 */
bool foil_region_set (foil_region_p region, const foil_rect* rect);

/**
 * \fn bool foil_region_is_empty (const foil_region* region)
 * \brief Determine whether a region is an empty region.
 *
 * This function determines whether the region pointed to by \a region is
 * an empty region.
 *
 * \param region The pointer to the region.
 *
 * \return true for empty one, else for not empty region.
 *
 * \sa foil_region_empty
 */
static inline bool foil_region_is_empty (const foil_region* region) {
     if (region->head == NULL)
         return true;

     return false;
}

/**
 * \fn bool foil_region_add_rect (foil_region_p region, const foil_rect* rect)
 * \brief Unions one rectangle to a region.
 *
 * This function unions a rectangle to the region pointed to by \a region.
 *
 * \param region The pointer to the region.
 * \param rect The pointer to the rectangle.
 *
 * \return true on success, otherwise false.
 *
 * \sa foil_region_intersect_rect, foil_region_subtract_rect
 */
bool foil_region_add_rect (foil_region_p region, const foil_rect* rect);

/**
 * \fn bool foil_region_intersect_rect (foil_region_p region, const foil_rect* rect)
 * \brief Intersects a rectangle with a region.
 *
 * This function intersects the region pointed to by \a region with a
 * rect pointed to by \a rect.
 *
 * \param region The pointer to the region.
 * \param rect The pointer to the rectangle.
 *
 * \return true on success, otherwise false.
 *
 * \sa foil_region_add_rect, foil_region_subtract_rect
 */
bool foil_region_intersect_rect (foil_region_p region, const foil_rect* rect);

/**
 * \fn bool foil_region_subtract_rect (foil_region_p region, const foil_rect* rect)
 * \brief Subtracts a rectangle from a region.
 *
 * This function subtracts a rect pointed to by \a rect from the region
 * pointed to by \a region.
 *
 * \param region The pointer to the region.
 * \param rect The pointer to the rect.
 *
 * \return true on success, otherwise false.
 *
 * \sa foil_region_add_rect, foil_region_intersect_rect
 */
bool foil_region_subtract_rect (foil_region_p region, const foil_rect* rect);

/**
 * \fn bool foil_region_is_point_in (foil_region_p region, int x, int y)
 * \brief Determine whether a point is in a region.
 *
 * This function determines whether a point \a (x,y) is in the region pointed
 * to by \a region.
 *
 * \param region The pointer to the region.
 * \param x x,y: The point.
 * \param y x,y: The point.
 *
 * \return true for in the region, otherwise false.
 *
 * \sa foil_region_is_rect_in
 */
bool foil_region_is_point_in (const foil_region_p region, int x, int y);

/**
 * \fn bool foil_region_is_rect_in (foil_region_p region, const foil_rect* rect)
 * \brief Determine whether a rectangle is intersected with a region.
 *
 * This function determines whether the rect \a rect is intersected with
 * the region pointed to by \a region.
 *
 * \param region The pointer to the region.
 * \param rect The pointer to the rect.
 *
 * \return true for in the region, otherwise false.
 *
 * \sa foil_region_is_point_in
 */
bool foil_region_is_rect_in (const foil_region_p region, const foil_rect* rect);

/**
 * \fn bool foil_region_does_intersect (const foil_region_p s1, const foil_region_p s2)
 * \brief Determine whether two regions are intersected.
 *
 * This function determines whether the region \a s1 and the region \a s2
 * are intersected.
 *
 * \param s1 The pointer to the first region.
 * \param s2 The pointer to the second region.
 *
 * \return true if intersected, otherwise false.
 *
 * \sa foil_region_is_rect_in
 */
static inline
bool foil_region_does_intersect (const foil_region_p s1, const foil_region_p s2)
{
    foil_rgnrc_p crc = s1->head;
    while (crc) {
        if (foil_region_is_rect_in (s2, &crc->rc))
            return true;
        crc = crc->next;
    }

    return false;
}

/**
 * \fn void foil_region_offset_ex (foil_region_p region, const foil_rect *rcClient, \
                            const foil_rect *rcScroll, int x, int y)
 * \brief Offsets the region in the specified window's scroll area.
 *
 * This function offsets a given region pointed to by region in the specified
 * window's scroll area.
 *
 * \param region The pointer to the region.
 * \param rcClient The client area which the region belongs to.
 * \param rcScroll The rectangle of the area in which the region will be offset.
 * \param x x,y: Offsets on x and y coodinates.
 * \param y x,y: Offsets on x and y coodinates.
 *
 */
void foil_region_offset_ex (foil_region_p region, const foil_rect *rcClient,
                            const foil_rect *rcScroll, int x, int y);

/**
 * \fn void foil_region_offset (foil_region_p region, int x, int y)
 * \brief Offsets the region.
 *
 * This function offsets a given region pointed to by \a region.
 *
 * \param region The pointer to the region.
 * \param x x,y: Offsets on x and y coodinates.
 * \param y x,y: Offsets on x and y coodinates.
 *
 */
void foil_region_offset (foil_region_p region, int x, int y);

/**
 * \fn bool foil_region_union (foil_region_p dst, \
                const foil_region* src1, const foil_region* src2)
 * \brief Unions two regions.
 *
 * This function unions two regions pointed to by \a src1 and \a src2
 * respectively and puts the result to the region pointed to by \a dst.
 *
 * \param dst The pointer to the result region.
 * \param src1 src1,src2: Two regions will be unioned.
 * \param src2 src1,src2: Two regions will be unioned.
 *
 * \return true on success, otherwise false.
 *
 * \sa foil_region_subtract, foil_region_xor
 */
bool foil_region_union (foil_region_p dst,
                const foil_region* src1, const foil_region* src2);

/**
 * \fn bool foil_region_subtract (foil_region* rgnD, \
                const foil_region* rgnM, const foil_region* rgnS)
 * \brief Substrcts a region from another.
 *
 * This function subtracts \a rgnS from \a rgnM and leave the result in \a rgnD.
 *
 * \param rgnD The pointer to the difference region.
 * \param rgnM The pointer to the minuend region.
 * \param rgnS The pointer to the subtrahend region.
 *
 * \return true on success, otherwise false.
 *
 * \sa foil_region_union, foil_region_xor
 */
bool foil_region_subtract (foil_region* rgnD,
                const foil_region* rgnM, const foil_region* rgnS);

/**
 * \fn bool foil_region_xor (foil_region *dst, \
                const foil_region *src1, const foil_region *src2)
 * \brief Does the XOR operation between two regions.
 *
 * This function does the XOR operation between two regions pointed to by
 * \a src1 and \a src2 and puts the result to the region pointed to by \a dst.
 *
 * \param dst The pointer to the result region.
 * \param src1 src1,src2: Two regions will be xor'ed.
 * \param src2 src1,src2: Two regions will be xor'ed.
 *
 * \return true on success, otherwise false.
 *
 * \sa foil_region_union, foil_region_subtract
 */
bool foil_region_xor (foil_region *dst,
                const foil_region *src1, const foil_region *src2);

size_t foil_lookfor_unused_slot (unsigned char* bitmap, size_t len_bmp, int set);
size_t foil_get_nr_idle_slots (unsigned char* bitmap, size_t len_bmp);

#ifdef __cplusplus
}
#endif

static inline
void foil_slot_set_use (unsigned char* bitmap, size_t index) {
    bitmap += index >> 3;
    *bitmap &= (~(0x80 >> (index % 8)));
}

static inline
int foil_slot_clear_use (unsigned char* bitmap, size_t index) {
    bitmap += index >> 3;
    *bitmap |= (0x80 >> (index % 8));
    return 1;
}

static inline
int foil_slot_is_used (unsigned char* bitmap, size_t index) {
    bitmap += index >> 3;
    if (*bitmap & (0x80 >> (index % 8)))
        return 0;
    return 1;
}

#endif /* purc_foil_region_h */
