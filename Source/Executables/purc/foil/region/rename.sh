#!/bin/bash

sed -i 's/PBLOCKHEAP/foil_block_heap_p/g' *.h *.c
sed -i 's/BLOCKHEAP/foil_block_heap/g' *.h *.c
sed -i 's/PCLIPRECT/foil_rgnrc_p/g' *.h *.c
sed -i 's/PCLIPRGN/foil_region_p/g' *.h *.c
sed -i 's/CLIPRECT/foil_rgnrc/g' *.h *.c
sed -i 's/CLIPRGN/foil_region/g' *.h *.c
sed -i 's/PRECT/foil_rect_p/g' *.h *.c
sed -i 's/RECT/foil_rect/g' *.h *.c
sed -i 's/ClipRgn/Region/g' *.h *.c
sed -i 's/FALSE/false/g' *.h *.c
sed -i 's/TRUE/true/g' *.h *.c
sed -i 's/BOOL/bool/g' *.h *.c
sed -i 's/BYTE/uint8_t/g' *.h *.c
sed -i 's/GUIAPI //g' *.h *.c
sed -i 's/MG_EXPORT //g' *.h *.c

sed -i 's/\<pRgn\>/region/g' *.h
sed -i 's/\<pDstRgn\>/dst_rgn/g' *.h
sed -i 's/\<pSrcRgn\>/src_rgn/g' *.h
sed -i 's/\<pFreeList\>/rgnrc_heap/g' *.h
sed -i 's/\<pRect\>/rect/g' *.h

sed -i 's/DestroyBlockDataHeap/foil_block_heap_cleanup/g' *.c *.h
sed -i 's/DeleteBlockDataHeap/foil_block_heap_delete/g' *.c *.h
sed -i 's/InitBlockDataHeap/foil_block_heap_init/g' *.c *.h
sed -i 's/NewBlockDataHeap/foil_block_heap_new/g' *.c *.h

sed -i 's/BlockDataAlloc/foil_block_heap_alloc/g' *.c *.h
sed -i 's/BlockDataFree/foil_block_heap_free/g' *.c *.h

sed -i 's/DeleteFreeClipRectList/foil_region_heap_rect_delete/g' *.c *.h
sed -i 's/DestroyFreeClipRectList/foil_region_rect_heap_cleanup/g' *.c *.h
sed -i 's/InitFreeClipRectList/foil_region_rect_heap_init/g' *.c *.h
sed -i 's/NewFreeClipRectList/foil_region_heap_rect_new/g' *.c *.h

sed -i 's/ClipRectAlloc/foil_region_rect_alloc/g' *.c *.h
sed -i 's/FreeClipRect/foil_region_rect_free/g' *.c *.h

sed -i 's/DestroyRegion/foil_region_delete/g' *.c *.h
sed -i 's/CreateRegion/foil_region_new/g' *.c *.h
sed -i 's/InitRegion/foil_region_init/g' *.c *.h

sed -i 's/AreRegionsIntersected/foil_region_does_intersect/g' *.c *.h
sed -i 's/GetRegionBoundRect/foil_region_get_bound_rect/g' *.c *.h
sed -i 's/IntersectClipRect/foil_region_intersect_rect/g' *.c *.h
sed -i 's/SubtractClipRect/foil_region_subtract_rect/g' *.c *.h
sed -i 's/RegionIntersect/foil_region_intersect/g' *.c *.h
sed -i 's/OffsetRegionEx/foil_region_offset_ex/g' *.c *.h
sed -i 's/RectInRegion/foil_region_is_rect_in/g' *.c *.h
sed -i 's/SubtractRegion/foil_region_subtract/g' *.c *.h
sed -i 's/IsEmptyRegion/foil_region_is_empty/g' *.c *.h
sed -i 's/PtInRegion/foil_region_is_point_in/g' *.c *.h
sed -i 's/AddClipRect/foil_region_add_rect/g' *.c *.h
sed -i 's/OffsetRegion/foil_region_offset/g' *.c *.h
sed -i 's/UnionRegion/foil_region_union/g' *.c *.h
sed -i 's/EmptyRegion/foil_region_empty/g' *.c *.h
sed -i 's/CopyRegion/foil_region_copy/g' *.c *.h
sed -i 's/XorRegion/foil_region_xor/g' *.c *.h
sed -i 's/SetRegion/foil_region_set/g' *.c *.h

sed -i 's/InflateRectToPt/foil_rect_inflate_to_point/g' *.c *.h
sed -i 's/DoesIntersect/foil_rect_does_intersect/g' *.c *.h
sed -i 's/NormalizeRect/foil_rect_normalize/g' *.c *.h
sed -i 's/IntersectRect/foil_rect_intersect/g' *.c *.h
sed -i 's/IsCovered/foil_rect_is_covered_by/g' *.c *.h
sed -i 's/GetBoundRect/foil_rect_get_bound/g' *.c *.h
sed -i 's/SubtractRect/foil_rect_subtract/g' *.c *.h
sed -i 's/PtInRect/foil_rect_is_point_in/g' *.c *.h
sed -i 's/IsRectEmpty/foil_rect_is_empty/g' *.c *.h
sed -i 's/InflateRect/foil_rect_inflate/g' *.c *.h
sed -i 's/EqualRect/foil_rect_is_equal/g' *.c *.h
sed -i 's/SetRectEmpty/foil_rect_empty/g' *.c *.h
sed -i 's/OffsetRect/foil_rect_offset/g' *.c *.h
sed -i 's/UnionRect/foil_rect_union/g' *.c *.h
sed -i 's/CopyRect/foil_rect_copy/g' *.c *.h
sed -i 's/SetRect/foil_rect_set/g' *.c *.h


