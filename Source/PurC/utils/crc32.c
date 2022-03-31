/*
 * crc32.c - Implementation of CRC32 checksum function.
 *
 * This is a simple public domain implementation of CRC32 from
 * <http://home.thep.lu.se/~bjorn/crc/>
 *
 * Modify the source code with Unix C style by Vincent Wei
 *  - Mar. 2022
 *
 * Copyright (C) 2020, 2021 FMSoft <https://www.fmsoft.cn>
 *
 * Here is a simple implementation of the commonly used CRC32 checksum using
 * the reverse polynomial 0xEDB88320. The algorithm is consistent with setting
 * of all bits in the initial CRC, along with negation of the bit pattern of
 * the final running CRC.
 */

#include "config.h"
#include "private/utils.h"

static uint32_t crc32_for_byte(uint32_t r)
{
    for(int j = 0; j < 8; ++j)
        r = (r & 1? 0: (uint32_t)0xEDB88320L) ^ r >> 1;
    return r ^ (uint32_t)0xFF000000L;
}

void pcutils_crc32(const void *data, size_t nr_bytes, uint32_t* crc)
{
    static uint32_t table[0x100];

    if (table[0] == 0) {
        for (size_t i = 0; i < 0x100; ++i)
            table[i] = crc32_for_byte(i);
    }

    for (size_t i = 0; i < nr_bytes; ++i)
        *crc = table[(uint8_t)*crc ^ ((uint8_t*)data)[i]] ^ *crc >> 8;
}

