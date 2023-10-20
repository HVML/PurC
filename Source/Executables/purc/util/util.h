/** \file util/util.h
 *  \brief Header: various utilities
 */

#ifndef _purc_util_util_h
#define _purc_util_util_h

#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>           /* uintmax_t */
#include <unistd.h>

/*
 * calloc_a(size_t len, [void **addr, size_t len,...], NULL)
 *
 * allocate a block of memory big enough to hold multiple aligned objects.
 * the pointer to the full object (starting with the first chunk) is returned,
 * all other pointers are stored in the locations behind extra addr arguments.
 * the last argument needs to be a NULL pointer
 */

#define calloc_a(len, ...) mc_calloc_a(len, ##__VA_ARGS__, NULL)
void *mc_calloc_a(size_t len, ...);

#endif /* not defined _purc_util_util_h */
