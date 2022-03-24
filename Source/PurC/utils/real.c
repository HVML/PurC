/*
 * @file real.c
 * @author Vincent Wei
 * @date 2022/03/24
 * @brief The helper functions to featch real number from byte sequence.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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
 */

#include "purc-utils.h"

#include <string.h>

#include "config.h"

/* copy from MiniGUI */
#define MAKEWORD16(low, high)   \
    ((uint16_t)(((uint8_t)(low)) | (((uint16_t)((uint8_t)(high))) << 8)))

#define MAKEWORD32(first, second, third, fourth)        \
    ((uint32_t)(                                        \
        ((uint8_t)(first)) |                            \
        (((uint32_t)((uint8_t)(second))) << 8) |        \
        (((uint32_t)((uint8_t)(third))) << 16) |        \
        (((uint32_t)((uint8_t)(fourth))) << 24)         \
    ))

#define MAKEWORD64(b0, b1, b2, b3, b4, b5, b6, b7)      \
    ((uint64_t)(                                        \
        ((uint8_t)(b0)) |                               \
        (((uint64_t)((uint8_t)(b1))) << 8)  |           \
        (((uint64_t)((uint8_t)(b2))) << 16) |           \
        (((uint64_t)((uint8_t)(b3))) << 24) |           \
        (((uint64_t)((uint8_t)(b4))) << 32) |           \
        (((uint64_t)((uint8_t)(b5))) << 40) |           \
        (((uint64_t)((uint8_t)(b6))) << 48) |           \
        (((uint64_t)((uint8_t)(b7))) << 56)             \
    ))

purc_real_t
purc_fetch_i8(const unsigned char *bytes)
{
    purc_real_t real;
    real.i64 = (int64_t)(int8_t)*bytes;
    return real;
}

purc_real_t
purc_fetch_i16(const unsigned char *bytes)
{
#if CPU(LITTLE_ENDIAN)
    return purc_fetch_i16le(bytes);
#elif CPU(BIG_ENDIAN)
    return purc_fetch_i16be(bytes);
#else
#error "Unsupported endian"
#endif
}

purc_real_t
purc_fetch_i32(const unsigned char *bytes)
{
#if CPU(LITTLE_ENDIAN)
    return purc_fetch_i32le(bytes);
#elif CPU(BIG_ENDIAN)
    return purc_fetch_i32be(bytes);
#else
#error "Unsupported endian"
#endif
}

purc_real_t
purc_fetch_i64(const unsigned char *bytes)
{
#if CPU(LITTLE_ENDIAN)
    return purc_fetch_i64le(bytes);
#elif CPU(BIG_ENDIAN)
    return purc_fetch_i64be(bytes);
#else
#error "Unsupported endian"
#endif
}

purc_real_t
purc_fetch_i16le(const unsigned char *bytes)
{
    purc_real_t real;
    int16_t i16 = (int16_t)MAKEWORD16(bytes[0], bytes[1]);
    real.i64 = (int64_t)i16;
    return real;
}

purc_real_t
purc_fetch_i32le(const unsigned char *bytes)
{
    purc_real_t real;
    int32_t i32 = (int32_t)MAKEWORD32(bytes[0], bytes[1], bytes[2], bytes[3]);
    real.i64 = (int64_t)i32;
    return real;
}

purc_real_t
purc_fetch_i64le(const unsigned char *bytes)
{
    purc_real_t real;
    real.i64 = (int64_t)MAKEWORD64(bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5], bytes[6], bytes[7]);
    return real;
}

purc_real_t
purc_fetch_i16be(const unsigned char *bytes)
{
    purc_real_t real;
    int16_t i16 = (int16_t)MAKEWORD16(bytes[1], bytes[0]);
    real.i64 = (int64_t)i16;
    return real;
}

purc_real_t
purc_fetch_i32be(const unsigned char *bytes)
{
    purc_real_t real;
    int32_t i32 = (int32_t)MAKEWORD32(bytes[3], bytes[2], bytes[1], bytes[0]);
    real.i64 = (int64_t)i32;
    return real;
}

purc_real_t
purc_fetch_i64be(const unsigned char *bytes)
{
    purc_real_t real;
    real.i64 = (int64_t)MAKEWORD64(bytes[7], bytes[6], bytes[5], bytes[4],
            bytes[3], bytes[2], bytes[1], bytes[0]);
    return real;
}

purc_real_t
purc_fetch_u8(const unsigned char *bytes)
{
    purc_real_t real;
    real.u64 = (uint64_t)(uint8_t)*bytes;
    return real;
}

purc_real_t
purc_fetch_u16(const unsigned char *bytes)
{
#if CPU(LITTLE_ENDIAN)
    return purc_fetch_u16le(bytes);
#elif CPU(BIG_ENDIAN)
    return purc_fetch_u16be(bytes);
#else
#error "Unsupported endian"
#endif
}

purc_real_t
purc_fetch_u32(const unsigned char *bytes)
{
#if CPU(LITTLE_ENDIAN)
    return purc_fetch_u32le(bytes);
#elif CPU(BIG_ENDIAN)
    return purc_fetch_u32be(bytes);
#else
#error "Unsupported endian"
#endif
}

purc_real_t
purc_fetch_u64(const unsigned char *bytes)
{
#if CPU(LITTLE_ENDIAN)
    return purc_fetch_u64le(bytes);
#elif CPU(BIG_ENDIAN)
    return purc_fetch_u64be(bytes);
#else
#error "Unsupported endian"
#endif
}

purc_real_t
purc_fetch_u16le(const unsigned char *bytes)
{
    purc_real_t real;
    uint16_t u16 = MAKEWORD16(bytes[0], bytes[1]);
    real.u64 = (uint64_t)u16;
    return real;
}

purc_real_t
purc_fetch_u32le(const unsigned char *bytes)
{
    purc_real_t real;
    uint32_t u32 = MAKEWORD32(bytes[0], bytes[1], bytes[2], bytes[3]);
    real.u64 = (uint64_t)u32;
    return real;
}

purc_real_t
purc_fetch_u64le(const unsigned char *bytes)
{
    purc_real_t real;
    real.u64 = MAKEWORD64(bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5], bytes[6], bytes[7]);
    return real;
}

purc_real_t
purc_fetch_u16be(const unsigned char *bytes)
{
    purc_real_t real;
    uint16_t u16 = (uint16_t)MAKEWORD16(bytes[1], bytes[0]);
    real.u64 = (uint64_t)u16;
    return real;
}

purc_real_t
purc_fetch_u32be(const unsigned char *bytes)
{
    purc_real_t real;
    uint32_t u32 = (uint32_t)MAKEWORD32(bytes[3], bytes[2], bytes[1], bytes[0]);
    real.u64 = (uint64_t)u32;
    return real;
}

purc_real_t
purc_fetch_u64be(const unsigned char *bytes)
{
    purc_real_t real;
    real.u64 = MAKEWORD64(bytes[7], bytes[6], bytes[5], bytes[4],
            bytes[3], bytes[2], bytes[1], bytes[0]);
    return real;
}

purc_real_t
purc_fetch_f16(const unsigned char *bytes)
{
#if CPU(LITTLE_ENDIAN)
    return purc_fetch_f16le(bytes);
#elif CPU(BIG_ENDIAN)
    return purc_fetch_f16be(bytes);
#else
#error "Unsupported endian"
#endif
}

purc_real_t
purc_fetch_f32(const unsigned char *bytes)
{
#if CPU(LITTLE_ENDIAN)
    return purc_fetch_f32le(bytes);
#elif CPU(BIG_ENDIAN)
    return purc_fetch_f32be(bytes);
#else
#error "Unsupported endian"
#endif
}

purc_real_t
purc_fetch_f64(const unsigned char *bytes)
{
#if CPU(LITTLE_ENDIAN)
    return purc_fetch_f64le(bytes);
#elif CPU(BIG_ENDIAN)
    return purc_fetch_f64be(bytes);
#else
#error "Unsupported endian"
#endif
}

purc_real_t
purc_fetch_f96(const unsigned char *bytes)
{
#if CPU(LITTLE_ENDIAN)
    return purc_fetch_f96le(bytes);
#elif CPU(BIG_ENDIAN)
    return purc_fetch_f96be(bytes);
#else
#error "Unsupported endian"
#endif
}

purc_real_t
purc_fetch_f128(const unsigned char *bytes)
{
#if CPU(LITTLE_ENDIAN)
    return purc_fetch_f128le(bytes);
#elif CPU(BIG_ENDIAN)
    return purc_fetch_f128be(bytes);
#else
#error "Unsupported endian"
#endif
}

/*
           According to IEEE 754
            sign    e      base   offset
        16   1      5       10     15
        32   1      8       23     127      (Single)
        64   1      11      52     1023     (Double)
        96   1      15      64     16383    (Double-Extended)
        128  1      15      112    16383    (Quadruple)
*/

/* Make sure sizeof(float) matches sizeof(uint32_t) */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(f32, sizeof(float) == sizeof(uint32_t));
#undef _COMPILE_TIME_ASSERT

/* Make sure sizeof(double) matches sizeof(uint64_t) */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(f64, sizeof(double) == sizeof(uint64_t));
#undef _COMPILE_TIME_ASSERT

/* Make sure sizeof(long double) matches sizeof(uint64_t) * 2 */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(f128, sizeof(long double) == sizeof(uint64_t) * 2);
#undef _COMPILE_TIME_ASSERT

static inline purc_real_t
assemble_f16(uint8_t b0, uint8_t b1)
{
    purc_real_t real;
    uint16_t u16 = MAKEWORD16(b0, b1);

    uint64_t sign = u16 >> 15;
    uint64_t e = (u16 >> 10) & 0x1F;
    uint64_t base = u16 & 0x3FF;

    real.u64 = sign << 63;
    e = 1023 + e - 15;
    e = e << 52;
    real.u64 |= e;
    base = base << (52 - 10);
    real.u64 |= base;

    return real;
}

purc_real_t
purc_fetch_f16le(const unsigned char *bytes)
{
    return assemble_f16(bytes[0], bytes[1]);
}

purc_real_t
purc_fetch_f32le(const unsigned char *bytes)
{
    purc_real_t real;
    float f32 = (float)MAKEWORD32(bytes[0], bytes[1], bytes[2], bytes[3]);
    real.d = (double)f32;
    return real;
}

purc_real_t
purc_fetch_f64le(const unsigned char *bytes)
{
    purc_real_t real;
    real.d = (double)MAKEWORD64(bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5], bytes[6], bytes[7]);
    return real;
}

purc_real_t
purc_fetch_f96le(const unsigned char *bytes)
{
    purc_real_t real;
    union {
        long double ld;
        uint64_t u64[2];
    } my_real;

    my_real.u64[0] = MAKEWORD64(bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5], bytes[6], bytes[7]);
    my_real.u64[1] = MAKEWORD64(0, 0, 0, 0,
            bytes[8], bytes[9], bytes[10], bytes[11]);

    real.ld = my_real.ld;
    return real;
}

#if 0
static inline void reverse_copy(uint8_t *dst, const uint8_t *src, size_t sz)
{
    for (size_t i = 0; i < sz; i++) {
        dst[i] = src[sz - i  - 1];
    }
}

purc_real_t
purc_fetch_f128le(const unsigned char *bytes)
{
    purc_real_t real;
#if CPU(LITTLE_ENDIAN)
    memcpy(&real.ld, bytes, sizeof(long double));
#elif CPU(BIG_ENDIAN)
    reverse_copy((uint8_t *)&real.ld, bytes, sizeof(long double));
#else
#error "Unsupported endian"
#endif
    return real;
}
#else
purc_real_t
purc_fetch_f128le(const unsigned char *bytes)
{
    purc_real_t real;
    union {
        long double ld;
        uint64_t u64[2];
    } my_real;

    my_real.u64[0] = MAKEWORD64(bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5], bytes[6], bytes[7]);
    my_real.u64[1] = MAKEWORD64(bytes[8], bytes[9], bytes[10], bytes[11],
            bytes[12], bytes[13], bytes[14], bytes[15]);

    real.ld = my_real.ld;
    return real;
}
#endif

purc_real_t
purc_fetch_f16be(const unsigned char *bytes)
{
    return assemble_f16(bytes[1], bytes[0]);
}

purc_real_t
purc_fetch_f32be(const unsigned char *bytes)
{
    purc_real_t real;
    float f32 = (float)MAKEWORD32(bytes[3], bytes[2], bytes[1], bytes[0]);
    real.d = (double)f32;
    return real;
}

purc_real_t
purc_fetch_f64be(const unsigned char *bytes)
{
    purc_real_t real;
    real.d = (double)MAKEWORD64(bytes[7], bytes[6], bytes[5], bytes[4],
            bytes[3], bytes[2], bytes[1], bytes[0]);
    return real;
}

purc_real_t
purc_fetch_f96be(const unsigned char *bytes)
{
    purc_real_t real;
    union {
        long double ld;
        uint64_t u64[2];
    } my_real;

    my_real.u64[0] = MAKEWORD64(bytes[11], bytes[10], bytes[9], bytes[8],
            0, 0, 0, 0);
    my_real.u64[1] = MAKEWORD64(bytes[7], bytes[6], bytes[5], bytes[4],
            bytes[3], bytes[2], bytes[1], bytes[0]);
    real.ld = my_real.ld;
    return real;
}

#if 0
purc_real_t
purc_fetch_f128be(const unsigned char *bytes)
{
    purc_real_t real;
#if CPU(BIG_ENDIAN)
    memcpy(&real.ld, bytes, sizeof(long double));
#elif CPU(LITTLE_ENDIAN)
    reverse_copy((uint8_t *)&real.ld, bytes, sizeof(long double));
#else
#error "Unsupported endian"
#endif
    return real;
}
#else
purc_real_t
purc_fetch_f128be(const unsigned char *bytes)
{
    purc_real_t real;
    union {
        long double ld;
        uint64_t u64[2];
    } my_real;

    my_real.u64[0] = MAKEWORD64(bytes[15], bytes[14], bytes[13], bytes[12],
            bytes[11], bytes[10], bytes[9], bytes[8]);
    my_real.u64[1] = MAKEWORD64(bytes[7], bytes[6], bytes[5], bytes[4],
            bytes[3], bytes[2], bytes[1], bytes[0]);
    real.ld = my_real.ld;
    return real;
}
#endif

