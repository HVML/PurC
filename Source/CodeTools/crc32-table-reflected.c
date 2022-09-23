#include <stdio.h>
#include <stdint.h>

#if 0
#define CRCPOLY     0x04c11db7
#define POSTFIX      "04c11db7"
#define CRCPOLY     0x1EDC6F41
#define POSTFIX      "1edc6f41"
#define CRCPOLY     0xA833982B
#define POSTFIX      "a833982b"
#define CRCPOLY     0x814141AB
#define POSTFIX      "814141ab"
#endif

#define CRCPOLY      0xA833982B
#define POSTFIX      "a833982b_reflected"

static uint32_t reflect_uint32(uint32_t u32)
{
    uint32_t result = 0;

    for (int i = 0; i < 32; i++) {
        if ((u32 >> i) & 0x01L) {
            result |= 0x01L << (31 - i);
        }
    }

    return result;
}

int main (void)
{
    uint32_t i;
    uint32_t table[256];

    uint32_t crcpoly = reflect_uint32(CRCPOLY);

    for (uint32_t i = 0; i <= 0xFF; i++) {
        uint32_t x = i;
        for (uint32_t j = 0; j < 8; j++)
            x = (x>>1) ^ (crcpoly & (-(int32_t)(x & 1)));
        table[i] = x;
    }

#if 0
    for (uint32_t i = 0; i <= 0xFF; i++) {
        uint32_t c = g_crc_slicing[0][i];
        for (uint32_t j = 1; j < 8; j++) {
            c = table[0][c & 0xFF] ^ (c >> 8);
            g_crc_slicing[j][i] = c;
        }
    }
#endif

    printf ("static const uint32_t crc32_table_" POSTFIX "[] =\n{\n");
    for (i = 0; i < 256; i += 4)
    {
        printf ("  0x%08x, 0x%08x, 0x%08x, 0x%08x",
                table[i + 0], table[i + 1], table[i + 2], table[i + 3]);
        if (i + 4 < 256)
            putchar (',');
        putchar ('\n');
    }
    printf ("};\n");
    return 0;
}

