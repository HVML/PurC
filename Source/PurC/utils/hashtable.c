/*
 * @file hashtable.c
 * @author Vincent Wei <https://github.com/VincentWei>
 * @date 2021/07/07
 *
 * Copyright (C) 2021 ~ 2023 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Note that the code is derived from json-c which is licensed under MIT Licence.
 *
 * Author: Michael Clark <michael@metaparadigm.com>
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Copyright (c) 2009 Hewlett-Packard Development Company, L.P.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#undef NDEBUG

#include "purc-utils.h"
#include "private/hashtable.h"
#include "config.h"

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_ENDIAN_H
#include <endian.h> /* attempt to define endianness */
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h> /* Get InterlockedCompareExchange */
#endif

#if HAVE(GLIB)
#include <gmodule.h>
#endif

/**
 * golden prime used in hash functions
 */
#define PCHASH_PRIME 0x9e370001UL

/**
 * The fraction of filled hash buckets until an insert will cause the table
 * to be resized.
 * This can range from just above 0 up to 1.0.
 */
#define PCHASH_LOAD_FACTOR 0.66

unsigned long pchash_ptr_hash(const void *k)
{
    /* CAW: refactored to be 64bit nice */
    return (unsigned long)((((ptrdiff_t)k * PCHASH_PRIME) >> 4) & ULONG_MAX);
}

int pchash_ptr_equal(const void *k1, const void *k2)
{
    if (k1 == k2)
        return 0;
    else if (k1 > k2)
        return 1;

    return -1;
}

/*
 * hashlittle from lookup3.c, by Bob Jenkins, May 2006, Public Domain.
 * http://burtleburtle.net/bob/c/lookup3.c
 * minor modifications to make functions static so no symbols are exported
 * minor mofifications to compile with -Werror
 */

/*
-------------------------------------------------------------------------------
lookup3.c, by Bob Jenkins, May 2006, Public Domain.

These are functions for producing 32-bit hashes for hash table lookup.
hashword(), hashlittle(), hashlittle2(), hashbig(), mix(), and final()
are externally useful functions.  Routines to test the hash are included
if SELF_TEST is defined.  You can use this free for any purpose.  It's in
the public domain.  It has no warranty.

You probably want to use hashlittle().  hashlittle() and hashbig()
hash byte arrays.  hashlittle() is is faster than hashbig() on
little-endian machines.  Intel and AMD are little-endian machines.
On second thought, you probably want hashlittle2(), which is identical to
hashlittle() except it returns two 32-bit hashes for the price of one.
You could implement hashbig2() if you wanted but I haven't bothered here.

If you want to find a hash of, say, exactly 7 integers, do
  a = i1;  b = i2;  c = i3;
  mix(a,b,c);
  a += i4; b += i5; c += i6;
  mix(a,b,c);
  a += i7;
  final(a,b,c);
then use c as the hash value.  If you have a variable length array of
4-byte integers to hash, use hashword().  If you have a byte array (like
a character string), use hashlittle().  If you have several byte arrays, or
a mix of things, see the comments above hashlittle().

Why is this so big?  I read 12 bytes at a time into 3 4-byte integers,
then mix those integers.  This is fast (you can do a lot more thorough
mixing with 12*3 instructions on 3 integers than you can with 3 instructions
on 1 byte), but shoehorning those bytes into integers efficiently is messy.
-------------------------------------------------------------------------------
*/

#if 0
/*
 * My best guess at if you are big-endian or little-endian.  This may
 * need adjustment.
 */
#if (defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) &&               \
        __BYTE_ORDER == __LITTLE_ENDIAN) ||                             \
    (defined(i386) || defined(__i386__) || defined(__i486__) ||         \
     defined(__i586__) || defined(__i686__) || defined(__x86_64__) ||   \
     defined(__ia64__) || defined(vax) || defined(MIPSEL) ||            \
     defined(__loongarch__) || defined(__riscv))
#   define HASH_LITTLE_ENDIAN 1
#   define HASH_BIG_ENDIAN 0
#elif (defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) &&                \
        __BYTE_ORDER == __BIG_ENDIAN) ||                                \
    (defined(sparc) || defined(POWERPC) || defined(mc68000) ||          \
     defined(sel))
#   define HASH_LITTLE_ENDIAN 0
#   define HASH_BIG_ENDIAN 1
#else
#   define HASH_LITTLE_ENDIAN 0
#   define HASH_BIG_ENDIAN 0
#endif
#endif  /* deprecated */

#if CPU(LITTLE_ENDIAN)
#   define HASH_LITTLE_ENDIAN 1
#   define HASH_BIG_ENDIAN 0
#elif CPU(BIG_ENDIAN)
#   define HASH_LITTLE_ENDIAN 0
#   define HASH_BIG_ENDIAN 1
#else
#   define HASH_LITTLE_ENDIAN 0
#   define HASH_BIG_ENDIAN 0
#endif

#define hashsize(n) ((uint32_t)1 << (n))
#define hashmask(n) (hashsize(n) - 1)
#define rot(x, k) (((x) << (k)) | ((x) >> (32 - (k))))

/*
-------------------------------------------------------------------------------
mix -- mix 3 32-bit values reversibly.

This is reversible, so any information in (a,b,c) before mix() is
still in (a,b,c) after mix().

If four pairs of (a,b,c) inputs are run through mix(), or through
mix() in reverse, there are at least 32 bits of the output that
are sometimes the same for one pair and different for another pair.
This was tested for:
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or
  all zero plus a counter that starts at zero.

Some k values for my "a-=c; a^=rot(c,k); c+=b;" arrangement that
satisfy this are
    4  6  8 16 19  4
    9 15  3 18 27 15
   14  9  3  7 17  3
Well, "9 15 3 18 27 15" didn't quite get 32 bits diffing
for "differ" defined as + with a one-bit base and a two-bit delta.  I
used http://burtleburtle.net/bob/hash/avalanche.html to choose
the operations, constants, and arrangements of the variables.

This does not achieve avalanche.  There are input bits of (a,b,c)
that fail to affect some output bits of (a,b,c), especially of a.  The
most thoroughly mixed value is c, but it doesn't really even achieve
avalanche in c.

This allows some parallelism.  Read-after-writes are good at doubling
the number of bits affected, so the goal of mixing pulls in the opposite
direction as the goal of parallelism.  I did what I could.  Rotates
seem to cost as much as shifts on every machine I could lay my hands
on, and rotates are much kinder to the top and bottom bits, so I used
rotates.
-------------------------------------------------------------------------------
*/
/* clang-format off */
#define mix(a,b,c) \
{ \
    a -= c;  a ^= rot(c, 4);  c += b; \
    b -= a;  b ^= rot(a, 6);  a += c; \
    c -= b;  c ^= rot(b, 8);  b += a; \
    a -= c;  a ^= rot(c,16);  c += b; \
    b -= a;  b ^= rot(a,19);  a += c; \
    c -= b;  c ^= rot(b, 4);  b += a; \
}
/* clang-format on */

/*
-------------------------------------------------------------------------------
final -- final mixing of 3 32-bit values (a,b,c) into c

Pairs of (a,b,c) values differing in only a few bits will usually
produce values of c that look totally different.  This was tested for
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or
  all zero plus a counter that starts at zero.

These constants passed:
 14 11 25 16 4 14 24
 12 14 25 16 4 14 24
and these came close:
  4  8 15 26 3 22 24
 10  8 15 26 3 22 24
 11  8 15 26 3 22 24
-------------------------------------------------------------------------------
*/
/* clang-format off */
#define final(a,b,c) \
{ \
    c ^= b; c -= rot(b,14); \
    a ^= c; a -= rot(c,11); \
    b ^= a; b -= rot(a,25); \
    c ^= b; c -= rot(b,16); \
    a ^= c; a -= rot(c,4);  \
    b ^= a; b -= rot(a,14); \
    c ^= b; c -= rot(b,24); \
}
/* clang-format on */

/*
-------------------------------------------------------------------------------
hashlittle() -- hash a variable-length key into a 32-bit value
  k       : the key (the unaligned variable-length array of bytes)
  length  : the length of the key, counting by bytes
  initval : can be any 4-byte value
Returns a 32-bit value.  Every bit of the key affects every bit of
the return value.  Two keys differing by one or two bits will have
totally different hash values.

The best hash table sizes are powers of 2.  There is no need to do
mod a prime (mod is sooo slow!).  If you need less than 32 bits,
use a bitmask.  For example, if you need only 10 bits, do
  h = (h & hashmask(10));
In which case, the hash table should have hashsize(10) elements.

If you are hashing n strings (uint8_t **)k, do it like this:
  for (i=0, h=0; i<n; ++i) h = hashlittle( k[i], len[i], h);

By Bob Jenkins, 2006.  bob_jenkins@burtleburtle.net.  You may use this
code any way you wish, private, educational, or commercial.  It's free.

Use for hash table lookup, or anything where one collision in 2^^32 is
acceptable.  Do NOT use for cryptographic purposes.
-------------------------------------------------------------------------------
*/

/* clang-format off */
static uint32_t hashlittle(const void *key, size_t length, uint32_t initval)
{
    uint32_t a,b,c; /* internal state */
    union
    {
        const void *ptr;
        size_t i;
    } u; /* needed for Mac Powerbook G4 */

    /* Set up the internal state */
    a = b = c = 0xdeadbeef + ((uint32_t)length) + initval;

    u.ptr = key;
    if (HASH_LITTLE_ENDIAN && ((u.i & 0x3) == 0)) {
        const uint32_t *k = (const uint32_t *)key; /* read 32-bit chunks */

        /* all but last block: aligned reads and affect 32 bits of (a,b,c) */
        while (length > 12)
        {
            a += k[0];
            b += k[1];
            c += k[2];
            mix(a,b,c);
            length -= 12;
            k += 3;
        }

        /* handle the last (probably partial) block */
        /*
         * "k[2]&0xffffff" actually reads beyond the end of the string, but
         * then masks off the part it's not allowed to read.  Because the
         * string is aligned, the masked-off tail is in the same word as the
         * rest of the string.  Every machine with memory protection I've seen
         * does it on word boundaries, so is OK with this.  But VALGRIND will
         * still catch it and complain.  The masking trick does make the hash
         * noticably faster for short strings (like English words).
         * AddressSanitizer is similarly picky about overrunning
         * the buffer. (http://clang.llvm.org/docs/AddressSanitizer.html
         */
#ifdef VALGRIND
#define PRECISE_MEMORY_ACCESS 1
#elif defined(__SANITIZE_ADDRESS__) /* GCC's ASAN */
#define PRECISE_MEMORY_ACCESS 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer) /* Clang's ASAN */
#define PRECISE_MEMORY_ACCESS 1
#endif
#endif
#ifndef PRECISE_MEMORY_ACCESS

        switch(length)
        {
        case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
        case 11: c+=k[2]&0xffffff; b+=k[1]; a+=k[0]; break;
        case 10: c+=k[2]&0xffff; b+=k[1]; a+=k[0]; break;
        case 9 : c+=k[2]&0xff; b+=k[1]; a+=k[0]; break;
        case 8 : b+=k[1]; a+=k[0]; break;
        case 7 : b+=k[1]&0xffffff; a+=k[0]; break;
        case 6 : b+=k[1]&0xffff; a+=k[0]; break;
        case 5 : b+=k[1]&0xff; a+=k[0]; break;
        case 4 : a+=k[0]; break;
        case 3 : a+=k[0]&0xffffff; break;
        case 2 : a+=k[0]&0xffff; break;
        case 1 : a+=k[0]&0xff; break;
        case 0 : return c; /* zero length strings require no mixing */
        }

#else /* make valgrind happy */

        const uint8_t  *k8 = (const uint8_t *)k;
        switch(length)
        {
        case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
        case 11: c+=((uint32_t)k8[10])<<16;  /* fall through */
        case 10: c+=((uint32_t)k8[9])<<8;    /* fall through */
        case 9 : c+=k8[8];                   /* fall through */
        case 8 : b+=k[1]; a+=k[0]; break;
        case 7 : b+=((uint32_t)k8[6])<<16;   /* fall through */
        case 6 : b+=((uint32_t)k8[5])<<8;    /* fall through */
        case 5 : b+=k8[4];                   /* fall through */
        case 4 : a+=k[0]; break;
        case 3 : a+=((uint32_t)k8[2])<<16;   /* fall through */
        case 2 : a+=((uint32_t)k8[1])<<8;    /* fall through */
        case 1 : a+=k8[0]; break;
        case 0 : return c;
        }

#endif /* !valgrind */

    }
    else if (HASH_LITTLE_ENDIAN && ((u.i & 0x1) == 0))
    {
        const uint16_t *k = (const uint16_t *)key; /* read 16-bit chunks */
        const uint8_t  *k8;

        /* all but last block: aligned reads and different mixing */
        while (length > 12)
        {
            a += k[0] + (((uint32_t)k[1])<<16);
            b += k[2] + (((uint32_t)k[3])<<16);
            c += k[4] + (((uint32_t)k[5])<<16);
            mix(a,b,c);
            length -= 12;
            k += 6;
        }

        /* handle the last (probably partial) block */
        k8 = (const uint8_t *)k;
        switch(length)
        {
        case 12: c+=k[4]+(((uint32_t)k[5])<<16);
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
        case 11: c+=((uint32_t)k8[10])<<16;     /* fall through */
        case 10: c+=k[4];
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
        case 9 : c+=k8[8];                      /* fall through */
        case 8 : b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
        case 7 : b+=((uint32_t)k8[6])<<16;      /* fall through */
        case 6 : b+=k[2];
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
        case 5 : b+=k8[4];                      /* fall through */
        case 4 : a+=k[0]+(((uint32_t)k[1])<<16);
             break;
        case 3 : a+=((uint32_t)k8[2])<<16;      /* fall through */
        case 2 : a+=k[0];
             break;
        case 1 : a+=k8[0];
             break;
        case 0 : return c;      /* zero length requires no mixing */
        }

    }
    else
    {
        /* need to read the key one byte at a time */
        const uint8_t *k = (const uint8_t *)key;

        /* all but the last block: affect some 32 bits of (a,b,c) */
        while (length > 12)
        {
            a += k[0];
            a += ((uint32_t)k[1])<<8;
            a += ((uint32_t)k[2])<<16;
            a += ((uint32_t)k[3])<<24;
            b += k[4];
            b += ((uint32_t)k[5])<<8;
            b += ((uint32_t)k[6])<<16;
            b += ((uint32_t)k[7])<<24;
            c += k[8];
            c += ((uint32_t)k[9])<<8;
            c += ((uint32_t)k[10])<<16;
            c += ((uint32_t)k[11])<<24;
            mix(a,b,c);
            length -= 12;
            k += 12;
        }

        /* last block: affect all 32 bits of (c) */
        switch(length) /* all the case statements fall through */
        {
        case 12: c+=((uint32_t)k[11])<<24; /* FALLTHRU */
        case 11: c+=((uint32_t)k[10])<<16; /* FALLTHRU */
        case 10: c+=((uint32_t)k[9])<<8; /* FALLTHRU */
        case 9 : c+=k[8]; /* FALLTHRU */
        case 8 : b+=((uint32_t)k[7])<<24; /* FALLTHRU */
        case 7 : b+=((uint32_t)k[6])<<16; /* FALLTHRU */
        case 6 : b+=((uint32_t)k[5])<<8; /* FALLTHRU */
        case 5 : b+=k[4]; /* FALLTHRU */
        case 4 : a+=((uint32_t)k[3])<<24; /* FALLTHRU */
        case 3 : a+=((uint32_t)k[2])<<16; /* FALLTHRU */
        case 2 : a+=((uint32_t)k[1])<<8; /* FALLTHRU */
        case 1 : a+=k[0];
             break;
        case 0 : return c;
        }
    }

    final(a,b,c);
    return c;
}
/* clang-format on */

/* a simple hash function similiar to what perl does for strings.
 * for good results, the string should not be excessivly large.
 */
unsigned long pchash_perlish_str_hash(const void *k)
{
    const char *rkey = (const char *)k;
    unsigned hashval = 1;

    while (*rkey)
        hashval = hashval * 33 + *rkey++;

    return hashval;
}

unsigned long pchash_default_str_hash(const void *k)
{
#if defined _MSC_VER || defined __MINGW32__
#define RANDOM_SEED_TYPE LONG
#else
#define RANDOM_SEED_TYPE int
#endif
    static volatile RANDOM_SEED_TYPE random_seed = -1;

    if (random_seed == -1)
    {
        RANDOM_SEED_TYPE seed;
        /* we can't use -1 as it is the unitialized sentinel */
        while ((seed = pcutils_get_random_seed()) == -1) {}
/* hacking: sizeof can't be used when pre-processing,
 * because it's compiler-level calculation
 * https://stackoverflow.com/questions/2319519/why-cant-i-use-sizeof-in-a-if
 * better in config.h */
#define SIZEOF_INT 4

#if SIZEOF_INT == 8 && defined __GCC_HAVE_SYNC_COMPARE_AND_SWAP_8
#define USE_SYNC_COMPARE_AND_SWAP 1
#endif
#if SIZEOF_INT == 4 && defined __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4
#define USE_SYNC_COMPARE_AND_SWAP 1
#endif
#if SIZEOF_INT == 2 && defined __GCC_HAVE_SYNC_COMPARE_AND_SWAP_2
#define USE_SYNC_COMPARE_AND_SWAP 1
#endif

#undef SIZEOF_INT

#if defined USE_SYNC_COMPARE_AND_SWAP
        (void)__sync_val_compare_and_swap(&random_seed, -1, seed);
#elif defined _MSC_VER || defined __MINGW32__
        InterlockedCompareExchange(&random_seed, seed, -1);
#else
        //#warning "racy random seed initializtion if used by multiple threads"
        random_seed = seed; /* potentially racy */
#endif
    }

    return hashlittle((const char *)k, strlen((const char *)k), random_seed);
}

int pchash_str_equal(const void *k1, const void *k2)
{
    return strcmp((const char *)k1, (const char *)k2);
}

#define WRLOCK_INIT(t)                        \
        purc_rwlock_init(&(t)->rwlock)

#define WRLOCK_CLEAR(t)                       \
    if ((t)->rwlock.native_impl)              \
        purc_rwlock_clear(&(t)->rwlock)

#define RDLOCK_TABLE(t)                         \
    if ((t)->rwlock.native_impl)              \
        purc_rwlock_reader_lock(&(t)->rwlock)

#define RDUNLOCK_TABLE(t)                       \
    if ((t)->rwlock.native_impl)              \
        purc_rwlock_reader_unlock(&(t)->rwlock)

#define WRLOCK_TABLE(t)                         \
    if ((t)->rwlock.native_impl)              \
        purc_rwlock_writer_lock(&(t)->rwlock)

#define WRUNLOCK_TABLE(t)                       \
    if ((t)->rwlock.native_impl)              \
        purc_rwlock_writer_unlock(&(t)->rwlock)

#if HAVE(GLIB)
static inline UNUSED_FUNCTION pchash_entry *alloc_entry(void) {
    return (pchash_entry *)g_slice_alloc(sizeof(pchash_entry));
}

static inline UNUSED_FUNCTION pchash_entry *alloc_entry_0(void) {
    return (pchash_entry *)g_slice_alloc0(sizeof(pchash_entry));
}

static inline void free_entry(pchash_entry *v) {
    return g_slice_free1(sizeof(pchash_entry), (gpointer)v);
}
#else
static inline UNUSED_FUNCTION pchash_entry *alloc_entry(void) {
    return (pchash_entry *)malloc(sizeof(pchash_entry));
}

static inline UNUSED_FUNCTION pchash_entry *alloc_entry_0(void) {
    return (pchash_entry *)calloc(1, sizeof(pchash_entry));
}

static inline void free_entry(pchash_entry *v) {
    return free(v);
}
#endif

struct pchash_table *pchash_table_new(size_t size,
        pchash_copy_key_fn copy_key, pchash_free_key_fn free_key,
        pchash_copy_val_fn copy_val, pchash_free_val_fn free_val,
        pchash_hash_fn hash_fn, pchash_equal_fn equal_fn, bool threads)
{
    struct pchash_table *t;

    if (size == 0)
        size = PCHASH_DEFAULT_SIZE;

    t = (pchash_table *)calloc(1, sizeof(pchash_table));
    if (!t)
        return NULL;

    t->count = 0;
    t->size = size;
    t->table = (struct list_head *)calloc(size, sizeof(struct list_head));
    if (!t->table) {
        free(t);
        return NULL;
    }

    for (size_t i = 0; i < t->size; i++) {
        list_head_init(t->table + i);
    }

    t->copy_key = copy_key;
    t->free_key = free_key;
    t->copy_val = copy_val;
    t->free_val = free_val;
    t->hash_fn = hash_fn;
    t->equal_fn = equal_fn;

    if (threads)
        WRLOCK_INIT(t);

    return t;
}

static void move_entry(struct pchash_table *dst, struct pchash_entry *ent)
{
    list_del(&ent->list);
    ent->slot = ent->hash % dst->size;
    list_add_tail(&ent->list, dst->table + ent->slot);
    dst->count++;
}

int pchash_table_resize(struct pchash_table *t, size_t new_size)
{
    struct pchash_table *new_t;
    struct pchash_entry *ent;

    new_t = pchash_table_new(new_size, NULL, NULL, NULL, NULL,
            t->hash_fn, t->equal_fn, false);
    if (new_t == NULL)
        return -1;

    for (size_t i = 0; i < t->size; i++) {
        struct list_head *p, *n;
        list_for_each_safe(p, n, t->table + i) {
            ent = list_entry(p, pchash_entry, list);
            move_entry(new_t, ent);
        }
    }

    t->size = new_size;
    free(t->table);
    t->table = new_t->table;
    free(new_t);

    return 0;
}

void pchash_table_reset(struct pchash_table *t)
{
    struct pchash_entry *c;

    for (size_t i = 0; i < t->size; i++) {
        struct list_head *p, *n;
        list_for_each_safe(p, n, t->table + i) {
            c = list_entry(p, pchash_entry, list);
            if (c->free_kv_alt) {
                c->free_kv_alt(c->key, c->val);
            }
            else {
                if (t->free_key) {
                    t->free_key(c->key);
                }

                if (t->free_val) {
                    t->free_val(c->val);
                }
            }

            free_entry(c);
        }
    }

    t->count = 0;
}

void pchash_table_delete(struct pchash_table *t)
{
    pchash_table_reset(t);
    WRLOCK_CLEAR(t);
    free(t->table);
    free(t);
}

static int insert_entry(struct pchash_table *t,
        const void *k, const void *v, const unsigned long h,
        pchash_free_kv_fn free_kv_alt)
{
    if (t->count >= t->size * PCHASH_LOAD_FACTOR) {
        /* Avoid signed integer overflow with large tables. */
        size_t new_size = (t->size > INT_MAX / 2) ? INT_MAX : (t->size * 2);
        if (t->size == INT_MAX || pchash_table_resize(t, new_size) != 0) {
            return -1;
        }
    }

    pchash_entry *ent = alloc_entry_0();
    if (ent == NULL)
        return -1;

    ent->key = (t->copy_key != NULL) ? t->copy_key(k) : (void *)k;
    ent->val = (t->copy_val != NULL) ? t->copy_val(v) : (void *)v;
    ent->free_kv_alt = free_kv_alt;
    ent->hash = h;
    ent->slot = h % t->size;

    list_add_tail(&ent->list, t->table + ent->slot);
    t->count++;

    return 0;
}

int pchash_table_insert_w_hash(struct pchash_table *t,
        const void *k, const void *v, const unsigned long h,
        pchash_free_kv_fn free_kv_alt)
{
    int retv;

    WRLOCK_TABLE(t);
    retv = insert_entry(t, k, v, h, free_kv_alt);
    WRUNLOCK_TABLE(t);
    return retv;
}

int pchash_table_insert_ex(struct pchash_table *t,
        const void *k, const void *v, pchash_free_kv_fn free_kv_alt)
{
    return pchash_table_insert_w_hash(t, k, v,
            pchash_get_hash(t, k), free_kv_alt);
}

static pchash_entry_t find_entry(struct pchash_table *t,
        const void *k, const unsigned long h)
{
    unsigned long slot = h % t->size;
    pchash_entry_t found = NULL;

    struct list_head *p;
    list_for_each(p, t->table + slot) {
        pchash_entry_t ent;
        ent = list_entry(p, pchash_entry, list);
        if (t->equal_fn(ent->key, k) == 0) {
            found = ent;
            break;
        }
    }

    return found;
}

pchash_entry_t pchash_table_lookup_entry_w_hash(struct pchash_table *t,
        const void *k, const unsigned long h)
{
    pchash_entry_t found = NULL;

    RDLOCK_TABLE(t);
    found = find_entry(t, k, h);
    RDUNLOCK_TABLE(t);

    return found;
}

pchash_entry_t pchash_table_lookup_entry(struct pchash_table *t,
        const void *k)
{
    return pchash_table_lookup_entry_w_hash(t, k, pchash_get_hash(t, k));
}

pchash_entry_t pchash_table_lookup_and_lock_w_hash(
        struct pchash_table *t, const void *k, const unsigned long h)
{
    pchash_entry_t found = NULL;

    WRLOCK_TABLE(t);
    found = find_entry(t, k, h);
    if (found == NULL)
        WRUNLOCK_TABLE(t);

    return found;
}

pchash_entry_t pchash_table_lookup_and_lock(struct pchash_table *t,
        const void *k)
{
    return pchash_table_lookup_and_lock_w_hash(t, k, pchash_get_hash(t, k));
}

bool pchash_table_lookup_ex(struct pchash_table *t, const void *k, void **v)
{
    pchash_entry_t e = pchash_table_lookup_entry(t, k);

    if (e != NULL) {
        if (v != NULL)
            *v = e->val;
        return true; /* key found */
    }

    if (v != NULL)
        *v = NULL;
    return false; /* key not found */
}

static int erase_entry(struct pchash_table *t, pchash_entry_t e)
{
    assert(e->slot < t->size);

    if (e->free_kv_alt) {
        e->free_kv_alt(e->key, e->val);
    }
    else {
        if (t->free_key) {
            t->free_key(e->key);
        }

        if (t->free_val) {
            t->free_val(e->val);
        }
    }

    t->count--;

    list_del(&e->list);
    free_entry(e);

    return 0;
}

int pchash_table_erase_entry(struct pchash_table *t, pchash_entry_t e)
{
    int retv;

    WRLOCK_TABLE(t);
    retv = erase_entry(t, e);
    WRUNLOCK_TABLE(t);
    return retv;
}

int pchash_table_erase(struct pchash_table *t, const void *k)
{
    int retv = -1;
    pchash_entry_t e;

    WRLOCK_TABLE(t);

    e = find_entry(t, k, pchash_get_hash(t, k));
    if (e) {
        retv = erase_entry(t, e);
        assert(retv == 0);
    }

    WRUNLOCK_TABLE(t);
    return retv;
}

int pchash_table_erase_nolock(struct pchash_table *t, pchash_entry_t e)
{
    return erase_entry(t, e);
}

int pchash_table_replace(struct pchash_table *t,
        const void *k, const void *v, pchash_free_kv_fn free_kv_alt)
{
    int retv = -1;
    pchash_entry_t e;

    WRLOCK_TABLE(t);

    e = find_entry(t, k, pchash_get_hash(t, k));
    if (e == NULL) {
        goto ret;
    }

    retv = 0;

    if (e->free_kv_alt) {
        e->free_kv_alt(NULL, e->val);
    }
    else if (t->free_val) {
        t->free_val(e->val);
    }

    if (t->copy_val) {
        e->val = t->copy_val(v);
    }
    else
        e->val = (void*)v;

    e->free_kv_alt = free_kv_alt;

ret:
    WRUNLOCK_TABLE(t);
    return retv;
}

int pchash_table_replace_or_insert(struct pchash_table *t,
        const void *k, const void *v, pchash_free_kv_fn free_kv_alt)
{
    int retv = -1;
    pchash_entry_t e;
    unsigned long h = pchash_get_hash(t, k);

    WRLOCK_TABLE(t);

    e = find_entry(t, k, h);
    if (e) {
        retv = 0;

        if (e->free_kv_alt) {
            e->free_kv_alt(NULL, e->val);
        }
        else if (t->free_val) {
            t->free_val(e->val);
        }

        if (t->copy_val) {
            e->val = t->copy_val(v);
        }
        else
            e->val = (void*)v;

        e->free_kv_alt = free_kv_alt;
    }
    else {
        retv = insert_entry(t, k, v, h, free_kv_alt);
    }

    WRUNLOCK_TABLE(t);
    return retv;
}

