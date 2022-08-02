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

bool
purc_dump_i8(unsigned char *dst, purc_real_t real, bool force)
{
    int8_t i8;

    if (real.i64 > INT8_MAX) {
        if (force)
            i8 = INT8_MAX;
        else
            return false;
    }
    else if (real.i64 < INT8_MIN) {
        if (force)
            i8 = INT8_MIN;
        else
            return false;
    }
    else {
        i8 = (int8_t)real.i64;
    }

    dst[0] = (uint8_t)i8;
    return true;
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

bool
purc_dump_i16(unsigned char *dst, purc_real_t real, bool force)
{
#if CPU(LITTLE_ENDIAN)
    return purc_dump_i16le(dst, real, force);
#elif CPU(BIG_ENDIAN)
    return purc_dump_i16be(dst, real, force);
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

bool
purc_dump_i32(unsigned char *dst, purc_real_t real, bool force)
{
#if CPU(LITTLE_ENDIAN)
    return purc_dump_i32le(dst, real, force);
#elif CPU(BIG_ENDIAN)
    return purc_dump_i32be(dst, real, force);
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

bool
purc_dump_i64(unsigned char *dst, purc_real_t real, bool force)
{
#if CPU(LITTLE_ENDIAN)
    return purc_dump_i64le(dst, real, force);
#elif CPU(BIG_ENDIAN)
    return purc_dump_i64be(dst, real, force);
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

#define LOBYTE(w)       ((uint8_t)(w))
#define HIBYTE(w)       ((uint8_t)(((uint16_t)(w) >> 8) & 0xFF))
#define LOWORD(dw)      ((uint16_t)(uint32_t)(dw))
#define HIWORD(dw)      ((uint16_t)((((uint32_t)(dw)) >> 16) & 0xFFFF))
#define LODWORD(dw)     ((uint32_t)(uint64_t)(dw))
#define HIDWORD(dw)     ((uint32_t)((((uint64_t)(dw)) >> 32) & 0xFFFFFFFF))

bool
purc_dump_i16le(unsigned char *dst, purc_real_t real, bool force)
{
    int16_t i16;

    if (real.i64 > INT16_MAX) {
        if (force)
            i16 = INT16_MAX;
        else
            return false;
    }
    else if (real.i64 < INT16_MIN) {
        if (force)
            i16 = INT16_MIN;
        else
            return false;
    }
    else {
        i16 = (int16_t)real.i64;
    }

    dst[0] = LOBYTE(i16);
    dst[1] = HIBYTE(i16);
    return true;
}

purc_real_t
purc_fetch_i32le(const unsigned char *bytes)
{
    purc_real_t real;
    int32_t i32 = (int32_t)MAKEWORD32(bytes[0], bytes[1], bytes[2], bytes[3]);
    real.i64 = (int64_t)i32;
    return real;
}

bool
purc_dump_i32le(unsigned char *dst, purc_real_t real, bool force)
{
    int32_t i32;

    if (real.i64 > INT32_MAX) {
        if (force)
            i32 = INT32_MAX;
        else
            return false;
    }
    else if (real.i64 < INT32_MIN) {
        if (force)
            i32 = INT32_MIN;
        else
            return false;
    }
    else {
        i32 = (int32_t)real.i64;
    }

    dst[0] = LOBYTE(LOWORD(i32));
    dst[1] = HIBYTE(LOWORD(i32));
    dst[2] = LOBYTE(HIWORD(i32));
    dst[3] = HIBYTE(HIWORD(i32));
    return true;
}

purc_real_t
purc_fetch_i64le(const unsigned char *bytes)
{
    purc_real_t real;
    real.i64 = (int64_t)MAKEWORD64(bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5], bytes[6], bytes[7]);
    return real;
}

bool
purc_dump_i64le(unsigned char *dst, purc_real_t real, bool force)
{
    UNUSED_PARAM(force);

    dst[0] = LOBYTE(LOWORD(LODWORD(real.i64)));
    dst[1] = HIBYTE(LOWORD(LODWORD(real.i64)));
    dst[2] = LOBYTE(HIWORD(LODWORD(real.i64)));
    dst[3] = HIBYTE(HIWORD(LODWORD(real.i64)));
    dst[4] = LOBYTE(LOWORD(HIDWORD(real.i64)));
    dst[5] = HIBYTE(LOWORD(HIDWORD(real.i64)));
    dst[6] = HIBYTE(HIWORD(HIDWORD(real.i64)));
    dst[7] = HIBYTE(HIWORD(HIDWORD(real.i64)));
    return true;
}

purc_real_t
purc_fetch_i16be(const unsigned char *bytes)
{
    purc_real_t real;
    int16_t i16 = (int16_t)MAKEWORD16(bytes[1], bytes[0]);
    real.i64 = (int64_t)i16;
    return real;
}

bool
purc_dump_i16be(unsigned char *dst, purc_real_t real, bool force)
{
    int16_t i16;

    if (real.i64 > INT16_MAX) {
        if (force)
            i16 = INT16_MAX;
        else
            return false;
    }
    else if (real.i64 < INT16_MIN) {
        if (force)
            i16 = INT16_MIN;
        else
            return false;
    }
    else {
        i16 = (int16_t)real.i64;
    }

    dst[1] = LOBYTE(i16);
    dst[0] = HIBYTE(i16);
    return true;
}

purc_real_t
purc_fetch_i32be(const unsigned char *bytes)
{
    purc_real_t real;
    int32_t i32 = (int32_t)MAKEWORD32(bytes[3], bytes[2], bytes[1], bytes[0]);
    real.i64 = (int64_t)i32;
    return real;
}

bool
purc_dump_i32be(unsigned char *dst, purc_real_t real, bool force)
{
    int32_t i32;

    if (real.i64 > INT32_MAX) {
        if (force)
            i32 = INT32_MAX;
        else
            return false;
    }
    else if (real.i64 < INT32_MIN) {
        if (force)
            i32 = INT32_MIN;
        else
            return false;
    }
    else {
        i32 = (int32_t)real.i64;
    }

    dst[3] = LOBYTE(LOWORD(i32));
    dst[2] = HIBYTE(LOWORD(i32));
    dst[1] = LOBYTE(HIWORD(i32));
    dst[0] = HIBYTE(HIWORD(i32));
    return true;
}

purc_real_t
purc_fetch_i64be(const unsigned char *bytes)
{
    purc_real_t real;
    real.i64 = (int64_t)MAKEWORD64(bytes[7], bytes[6], bytes[5], bytes[4],
            bytes[3], bytes[2], bytes[1], bytes[0]);
    return real;
}

bool
purc_dump_i64be(unsigned char *dst, purc_real_t real, bool force)
{
    UNUSED_PARAM(force);

    dst[7] = LOBYTE(LOWORD(LODWORD(real.i64)));
    dst[6] = HIBYTE(LOWORD(LODWORD(real.i64)));
    dst[5] = LOBYTE(HIWORD(LODWORD(real.i64)));
    dst[4] = HIBYTE(HIWORD(LODWORD(real.i64)));
    dst[3] = LOBYTE(LOWORD(HIDWORD(real.i64)));
    dst[2] = HIBYTE(LOWORD(HIDWORD(real.i64)));
    dst[1] = HIBYTE(HIWORD(HIDWORD(real.i64)));
    dst[0] = HIBYTE(HIWORD(HIDWORD(real.i64)));
    return true;
}

purc_real_t
purc_fetch_u8(const unsigned char *bytes)
{
    purc_real_t real;
    real.u64 = (uint64_t)(uint8_t)*bytes;
    return real;
}

bool
purc_dump_u8(unsigned char *dst, purc_real_t real, bool force)
{
    uint8_t u8;

    if (real.u64 > UINT8_MAX) {
        if (force)
            u8 = UINT8_MAX;
        else
            return false;
    }
    else {
        u8 = (uint8_t)real.u64;
    }

    dst[0] = u8;
    return true;
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

bool
purc_dump_u16(unsigned char *dst, purc_real_t real, bool force)
{
#if CPU(LITTLE_ENDIAN)
    return purc_dump_u16le(dst, real, force);
#elif CPU(BIG_ENDIAN)
    return purc_dump_u16be(dst, real, force);
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

bool
purc_dump_u32(unsigned char *dst, purc_real_t real, bool force)
{
#if CPU(LITTLE_ENDIAN)
    return purc_dump_u32le(dst, real, force);
#elif CPU(BIG_ENDIAN)
    return purc_dump_u32be(dst, real, force);
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

bool
purc_dump_u64(unsigned char *dst, purc_real_t real, bool force)
{
#if CPU(LITTLE_ENDIAN)
    return purc_dump_u64le(dst, real, force);
#elif CPU(BIG_ENDIAN)
    return purc_dump_u64be(dst, real, force);
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

bool
purc_dump_u16le(unsigned char *dst, purc_real_t real, bool force)
{
    uint16_t u16;

    if (real.u64 > UINT16_MAX) {
        if (force)
            u16 = UINT16_MAX;
        else
            return false;
    }
    else {
        u16 = (uint16_t)real.u64;
    }

    dst[0] = LOBYTE(u16);
    dst[1] = HIBYTE(u16);
    return true;
}

purc_real_t
purc_fetch_u32le(const unsigned char *bytes)
{
    purc_real_t real;
    uint32_t u32 = MAKEWORD32(bytes[0], bytes[1], bytes[2], bytes[3]);
    real.u64 = (uint64_t)u32;
    return real;
}

bool
purc_dump_u32le(unsigned char *dst, purc_real_t real, bool force)
{
    uint32_t u32;

    if (real.u64 > UINT32_MAX) {
        if (force)
            u32 = UINT32_MAX;
        else
            return false;
    }
    else {
        u32 = (uint32_t)real.u64;
    }

    dst[0] = LOBYTE(LOWORD(u32));
    dst[1] = HIBYTE(LOWORD(u32));
    dst[2] = LOBYTE(HIWORD(u32));
    dst[3] = HIBYTE(HIWORD(u32));
    return true;
}

purc_real_t
purc_fetch_u64le(const unsigned char *bytes)
{
    purc_real_t real;
    real.u64 = MAKEWORD64(bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5], bytes[6], bytes[7]);
    return real;
}

bool
purc_dump_u64le(unsigned char *dst, purc_real_t real, bool force)
{
    UNUSED_PARAM(force);

    dst[0] = LOBYTE(LOWORD(LODWORD(real.u64)));
    dst[1] = HIBYTE(LOWORD(LODWORD(real.u64)));
    dst[2] = LOBYTE(HIWORD(LODWORD(real.u64)));
    dst[3] = HIBYTE(HIWORD(LODWORD(real.u64)));
    dst[4] = LOBYTE(LOWORD(HIDWORD(real.u64)));
    dst[5] = HIBYTE(LOWORD(HIDWORD(real.u64)));
    dst[6] = HIBYTE(HIWORD(HIDWORD(real.u64)));
    dst[7] = HIBYTE(HIWORD(HIDWORD(real.u64)));
    return true;
}

purc_real_t
purc_fetch_u16be(const unsigned char *bytes)
{
    purc_real_t real;
    uint16_t u16 = (uint16_t)MAKEWORD16(bytes[1], bytes[0]);
    real.u64 = (uint64_t)u16;
    return real;
}

bool
purc_dump_u16be(unsigned char *dst, purc_real_t real, bool force)
{
    uint16_t u16;

    if (real.u64 > UINT16_MAX) {
        if (force)
            u16 = UINT16_MAX;
        else
            return false;
    }
    else {
        u16 = (uint16_t)real.u64;
    }

    dst[1] = LOBYTE(u16);
    dst[0] = HIBYTE(u16);
    return true;
}

purc_real_t
purc_fetch_u32be(const unsigned char *bytes)
{
    purc_real_t real;
    uint32_t u32 = (uint32_t)MAKEWORD32(bytes[3], bytes[2], bytes[1], bytes[0]);
    real.u64 = (uint64_t)u32;
    return real;
}

bool
purc_dump_u32be(unsigned char *dst, purc_real_t real, bool force)
{
    uint32_t u32;

    if (real.u64 > UINT32_MAX) {
        if (force)
            u32 = UINT32_MAX;
        else
            return false;
    }
    else {
        u32 = (uint32_t)real.u64;
    }

    dst[3] = LOBYTE(LOWORD(u32));
    dst[2] = HIBYTE(LOWORD(u32));
    dst[1] = LOBYTE(HIWORD(u32));
    dst[0] = HIBYTE(HIWORD(u32));
    return true;
}

purc_real_t
purc_fetch_u64be(const unsigned char *bytes)
{
    purc_real_t real;
    real.u64 = MAKEWORD64(bytes[7], bytes[6], bytes[5], bytes[4],
            bytes[3], bytes[2], bytes[1], bytes[0]);
    return real;
}

bool
purc_dump_u64be(unsigned char *dst, purc_real_t real, bool force)
{
    UNUSED_PARAM(force);

    dst[7] = LOBYTE(LOWORD(LODWORD(real.u64)));
    dst[6] = HIBYTE(LOWORD(LODWORD(real.u64)));
    dst[5] = LOBYTE(HIWORD(LODWORD(real.u64)));
    dst[4] = HIBYTE(HIWORD(LODWORD(real.u64)));
    dst[3] = LOBYTE(LOWORD(HIDWORD(real.u64)));
    dst[2] = HIBYTE(LOWORD(HIDWORD(real.u64)));
    dst[1] = HIBYTE(HIWORD(HIDWORD(real.u64)));
    dst[0] = HIBYTE(HIWORD(HIDWORD(real.u64)));
    return true;
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

bool
purc_dump_f16(unsigned char *dst, purc_real_t real, bool force)
{
#if CPU(LITTLE_ENDIAN)
    return purc_dump_f16le(dst, real, force);
#elif CPU(BIG_ENDIAN)
    return purc_dump_f16be(dst, real, force);
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

bool
purc_dump_f32(unsigned char *dst, purc_real_t real, bool force)
{
#if CPU(LITTLE_ENDIAN)
    return purc_dump_f32le(dst, real, force);
#elif CPU(BIG_ENDIAN)
    return purc_dump_f32be(dst, real, force);
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

bool
purc_dump_f64(unsigned char *dst, purc_real_t real, bool force)
{
#if CPU(LITTLE_ENDIAN)
    return purc_dump_f64le(dst, real, force);
#elif CPU(BIG_ENDIAN)
    return purc_dump_f64be(dst, real, force);
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

bool
purc_dump_f96(unsigned char *dst, purc_real_t real, bool force)
{
#if CPU(LITTLE_ENDIAN)
    return purc_dump_f96le(dst, real, force);
#elif CPU(BIG_ENDIAN)
    return purc_dump_f96be(dst, real, force);
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

bool
purc_dump_f128(unsigned char *dst, purc_real_t real, bool force)
{
#if CPU(LITTLE_ENDIAN)
    return purc_dump_f128le(dst, real, force);
#elif CPU(BIG_ENDIAN)
    return purc_dump_f128be(dst, real, force);
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

/* VW: unnecessary.
   Make sure sizeof(long double) matches sizeof(uint64_t) * 2 
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(f128, sizeof(long double) == sizeof(uint64_t) * 2);
#undef _COMPILE_TIME_ASSERT
*/

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

bool
purc_dump_f16le(unsigned char *dst, purc_real_t real, bool force)
{
    UNUSED_PARAM(force);
    union {
        float f32;
        uint8_t u8[4];
    } my_real;

    my_real.f32 = (float)real.d;

    dst[0] = my_real.u8[0];
    dst[1] = my_real.u8[1];
    return true;
}

purc_real_t
purc_fetch_f32le(const unsigned char *bytes)
{
    purc_real_t real;
    float f32;
    uint8_t *dst = (uint8_t *)&f32;

    dst[0] = bytes[0];
    dst[1] = bytes[1];
    dst[2] = bytes[2];
    dst[3] = bytes[3];
    real.d = (double)f32;
    return real;
}

bool
purc_dump_f32le(unsigned char *dst, purc_real_t real, bool force)
{
    UNUSED_PARAM(force);
    float f32 = (float)real.d;
    uint8_t *src = (uint8_t *)&f32;

    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
    return false;
}

purc_real_t
purc_fetch_f64le(const unsigned char *bytes)
{
    double f64;
    uint8_t *dst = (uint8_t *)&f64;

    for (int i = 0; i < 8; i++) {
        dst[i] = bytes[i];
    }

    purc_real_t real;
    real.d = f64;
    return real;
}

bool
purc_dump_f64le(unsigned char *dst, purc_real_t real, bool force)
{
    UNUSED_PARAM(force);
    uint8_t *src = (uint8_t *)&real.d;

    for (int i = 0; i < 8; i++) {
        dst[i] = src[i];
    }

    return false;
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

bool
purc_dump_f96le(unsigned char *dst, purc_real_t real, bool force)
{
    UNUSED_PARAM(force);
    uint8_t *src = (uint8_t *)&real.ld;

    for (int i = 0; i < 8; i++) {
        dst[i] = src[i];
    }

    for (int i = 0; i < 4; i++) {
        dst[8 + i] = src[12 + i];
    }

    return true;
}

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

bool
purc_dump_f128le(unsigned char *dst, purc_real_t real, bool force)
{
    UNUSED_PARAM(force);
    uint8_t *src = (uint8_t *)&real.ld;

    for (int i = 0; i < 16; i++) {
        dst[i] = src[i];
    }

    return true;
}

purc_real_t
purc_fetch_f16be(const unsigned char *bytes)
{
    return assemble_f16(bytes[1], bytes[0]);
}

bool
purc_dump_f16be(unsigned char *dst, purc_real_t real, bool force)
{
    UNUSED_PARAM(force);
    union {
        float f32;
        uint8_t u8[4];
    } my_real;

    my_real.f32 = (float)real.d;

    dst[1] = my_real.u8[0];
    dst[0] = my_real.u8[1];
    return true;
}

purc_real_t
purc_fetch_f32be(const unsigned char *bytes)
{
    purc_real_t real;
    float f32;
    uint8_t *dst = (uint8_t *)&f32;

    dst[3] = bytes[0];
    dst[2] = bytes[1];
    dst[1] = bytes[2];
    dst[0] = bytes[3];
    real.d = (double)f32;
    return real;
}

bool
purc_dump_f32be(unsigned char *dst, purc_real_t real, bool force)
{
    UNUSED_PARAM(force);
    float f32 = (float)real.d;
    uint8_t *src = (uint8_t *)&f32;

    dst[3] = src[0];
    dst[2] = src[1];
    dst[1] = src[2];
    dst[0] = src[3];
    return false;
}

purc_real_t
purc_fetch_f64be(const unsigned char *bytes)
{
    double f64;
    uint8_t *dst = (uint8_t *)&f64;

    for (int i = 0; i < 8; i++) {
        dst[7 - i] = bytes[i];
    }

    purc_real_t real;
    real.d = f64;
    return real;
}

bool
purc_dump_f64be(unsigned char *dst, purc_real_t real, bool force)
{
    UNUSED_PARAM(force);
    uint8_t *src = (uint8_t *)&real.d;

    for (int i = 0; i < 8; i++) {
        dst[i] = src[i];
    }

    return false;
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

bool
purc_dump_f96be(unsigned char *dst, purc_real_t real, bool force)
{
    UNUSED_PARAM(force);
    uint8_t *src = (uint8_t *)&real.ld;

    for (int i = 0; i < 8; i++) {
        dst[i] = src[15 - i];
    }

    for (int i = 0; i < 4; i++) {
        dst[8 + i] = src[15 - 12 - i];
    }

    return true;
}

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

bool
purc_dump_f128be(unsigned char *dst, purc_real_t real, bool force)
{
    UNUSED_PARAM(force);
    uint8_t *src = (uint8_t *)&real.ld;

    for (int i = 0; i < 16; i++) {
        dst[i] = src[15 - i];
    }

    return true;
}

