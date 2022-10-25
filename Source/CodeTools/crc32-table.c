#include <stdio.h>
#include <stdint.h>

#if 0
#define POLY        0x04c11db7
#define POSTFIX      "04c11db7"
#define POLY        0x1EDC6F41
#define POSTFIX      "1edc6f41"
#define POLY        0xA833982B
#define POSTFIX      "a833982b"
#define POLY        0x000000AF
#define POSTFIX      "000000af"
#endif

#define POLY        0x814141AB
#define POSTFIX      "814141ab"

int main (void)
{
    uint32_t i, j;
    uint32_t c;
    uint32_t table[256];

    for (i = 0; i < 256; i++) {
        for (c = i << 24, j = 8; j > 0; --j)
            c = c & 0x80000000 ? (c << 1) ^ POLY : (c << 1);
        table[i] = c;
    }

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

