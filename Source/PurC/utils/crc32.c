/*
 * @file crc32.c
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2022/04/01
 * @brief The implementation of CRC32 checksum function.
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

#include "purc-helpers.h"

#include "config.h"
#include "private/utils.h"
#include "private/debug.h"

/*

// program to generate the crc32_table.

#include <stdio.h>

#define POLY    0x04c11db7
#define POSTFIX "04c11db7"

int main (void)
{
    uint32_t i, j;
    uint32_t c;
    int table[256];
    for (i = 0; i < 256; i++)
    {
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

// For more information on CRC, see
// <http://www.ross.net/crc/download/crc_v3.txt>
*/

static const uint32_t crc32_table_04c11db7_reflected[] = {
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
  0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
  0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
  0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
  0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
  0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
  0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
  0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
  0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
  0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
  0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
  0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
  0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
  0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
  0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
  0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
  0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
  0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
  0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
  0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
  0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
  0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
  0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
  0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
  0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
  0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
  0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
  0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
  0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
  0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
  0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
  0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
  0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
  0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
  0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
  0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
  0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
  0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
  0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
  0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
  0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
  0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
  0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
  0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
  0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
  0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
  0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
  0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
  0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
  0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
  0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
  0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
  0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
  0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
  0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
  0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
  0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
  0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
  0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
  0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
  0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
  0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static const uint32_t crc32_table_04c11db7[] = {
  0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
  0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
  0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
  0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
  0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
  0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
  0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
  0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
  0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
  0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
  0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
  0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
  0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
  0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
  0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
  0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
  0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
  0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
  0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
  0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
  0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
  0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
  0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
  0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
  0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
  0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
  0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
  0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
  0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
  0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
  0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
  0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
  0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
  0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
  0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
  0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
  0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
  0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
  0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
  0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
  0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
  0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
  0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
  0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
  0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
  0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
  0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
  0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
  0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
  0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
  0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
  0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
  0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
  0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
  0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
  0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
  0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
  0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
  0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
  0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
  0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
  0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
  0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
  0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

static const uint32_t crc32_table_1edc6f41_reflected[] =
{
  0x00000000, 0xF26B8303, 0xE13B70F7, 0x1350F3F4,
  0xC79A971F, 0x35F1141C, 0x26A1E7E8, 0xD4CA64EB,
  0x8AD958CF, 0x78B2DBCC, 0x6BE22838, 0x9989AB3B,
  0x4D43CFD0, 0xBF284CD3, 0xAC78BF27, 0x5E133C24,
  0x105EC76F, 0xE235446C, 0xF165B798, 0x030E349B,
  0xD7C45070, 0x25AFD373, 0x36FF2087, 0xC494A384,
  0x9A879FA0, 0x68EC1CA3, 0x7BBCEF57, 0x89D76C54,
  0x5D1D08BF, 0xAF768BBC, 0xBC267848, 0x4E4DFB4B,
  0x20BD8EDE, 0xD2D60DDD, 0xC186FE29, 0x33ED7D2A,
  0xE72719C1, 0x154C9AC2, 0x061C6936, 0xF477EA35,
  0xAA64D611, 0x580F5512, 0x4B5FA6E6, 0xB93425E5,
  0x6DFE410E, 0x9F95C20D, 0x8CC531F9, 0x7EAEB2FA,
  0x30E349B1, 0xC288CAB2, 0xD1D83946, 0x23B3BA45,
  0xF779DEAE, 0x05125DAD, 0x1642AE59, 0xE4292D5A,
  0xBA3A117E, 0x4851927D, 0x5B016189, 0xA96AE28A,
  0x7DA08661, 0x8FCB0562, 0x9C9BF696, 0x6EF07595,
  0x417B1DBC, 0xB3109EBF, 0xA0406D4B, 0x522BEE48,
  0x86E18AA3, 0x748A09A0, 0x67DAFA54, 0x95B17957,
  0xCBA24573, 0x39C9C670, 0x2A993584, 0xD8F2B687,
  0x0C38D26C, 0xFE53516F, 0xED03A29B, 0x1F682198,
  0x5125DAD3, 0xA34E59D0, 0xB01EAA24, 0x42752927,
  0x96BF4DCC, 0x64D4CECF, 0x77843D3B, 0x85EFBE38,
  0xDBFC821C, 0x2997011F, 0x3AC7F2EB, 0xC8AC71E8,
  0x1C661503, 0xEE0D9600, 0xFD5D65F4, 0x0F36E6F7,
  0x61C69362, 0x93AD1061, 0x80FDE395, 0x72966096,
  0xA65C047D, 0x5437877E, 0x4767748A, 0xB50CF789,
  0xEB1FCBAD, 0x197448AE, 0x0A24BB5A, 0xF84F3859,
  0x2C855CB2, 0xDEEEDFB1, 0xCDBE2C45, 0x3FD5AF46,
  0x7198540D, 0x83F3D70E, 0x90A324FA, 0x62C8A7F9,
  0xB602C312, 0x44694011, 0x5739B3E5, 0xA55230E6,
  0xFB410CC2, 0x092A8FC1, 0x1A7A7C35, 0xE811FF36,
  0x3CDB9BDD, 0xCEB018DE, 0xDDE0EB2A, 0x2F8B6829,
  0x82F63B78, 0x709DB87B, 0x63CD4B8F, 0x91A6C88C,
  0x456CAC67, 0xB7072F64, 0xA457DC90, 0x563C5F93,
  0x082F63B7, 0xFA44E0B4, 0xE9141340, 0x1B7F9043,
  0xCFB5F4A8, 0x3DDE77AB, 0x2E8E845F, 0xDCE5075C,
  0x92A8FC17, 0x60C37F14, 0x73938CE0, 0x81F80FE3,
  0x55326B08, 0xA759E80B, 0xB4091BFF, 0x466298FC,
  0x1871A4D8, 0xEA1A27DB, 0xF94AD42F, 0x0B21572C,
  0xDFEB33C7, 0x2D80B0C4, 0x3ED04330, 0xCCBBC033,
  0xA24BB5A6, 0x502036A5, 0x4370C551, 0xB11B4652,
  0x65D122B9, 0x97BAA1BA, 0x84EA524E, 0x7681D14D,
  0x2892ED69, 0xDAF96E6A, 0xC9A99D9E, 0x3BC21E9D,
  0xEF087A76, 0x1D63F975, 0x0E330A81, 0xFC588982,
  0xB21572C9, 0x407EF1CA, 0x532E023E, 0xA145813D,
  0x758FE5D6, 0x87E466D5, 0x94B49521, 0x66DF1622,
  0x38CC2A06, 0xCAA7A905, 0xD9F75AF1, 0x2B9CD9F2,
  0xFF56BD19, 0x0D3D3E1A, 0x1E6DCDEE, 0xEC064EED,
  0xC38D26C4, 0x31E6A5C7, 0x22B65633, 0xD0DDD530,
  0x0417B1DB, 0xF67C32D8, 0xE52CC12C, 0x1747422F,
  0x49547E0B, 0xBB3FFD08, 0xA86F0EFC, 0x5A048DFF,
  0x8ECEE914, 0x7CA56A17, 0x6FF599E3, 0x9D9E1AE0,
  0xD3D3E1AB, 0x21B862A8, 0x32E8915C, 0xC083125F,
  0x144976B4, 0xE622F5B7, 0xF5720643, 0x07198540,
  0x590AB964, 0xAB613A67, 0xB831C993, 0x4A5A4A90,
  0x9E902E7B, 0x6CFBAD78, 0x7FAB5E8C, 0x8DC0DD8F,
  0xE330A81A, 0x115B2B19, 0x020BD8ED, 0xF0605BEE,
  0x24AA3F05, 0xD6C1BC06, 0xC5914FF2, 0x37FACCF1,
  0x69E9F0D5, 0x9B8273D6, 0x88D28022, 0x7AB90321,
  0xAE7367CA, 0x5C18E4C9, 0x4F48173D, 0xBD23943E,
  0xF36E6F75, 0x0105EC76, 0x12551F82, 0xE03E9C81,
  0x34F4F86A, 0xC69F7B69, 0xD5CF889D, 0x27A40B9E,
  0x79B737BA, 0x8BDCB4B9, 0x988C474D, 0x6AE7C44E,
  0xBE2DA0A5, 0x4C4623A6, 0x5F16D052, 0xAD7D5351
};

static const uint32_t crc32_table_a833982b_reflected[] =
{
  0x00000000, 0x2bddd04f, 0x57bba09e, 0x7c6670d1,
  0xaf77413c, 0x84aa9173, 0xf8cce1a2, 0xd31131ed,
  0xf6dd1a53, 0xdd00ca1c, 0xa166bacd, 0x8abb6a82,
  0x59aa5b6f, 0x72778b20, 0x0e11fbf1, 0x25cc2bbe,
  0x4589ac8d, 0x6e547cc2, 0x12320c13, 0x39efdc5c,
  0xeafeedb1, 0xc1233dfe, 0xbd454d2f, 0x96989d60,
  0xb354b6de, 0x98896691, 0xe4ef1640, 0xcf32c60f,
  0x1c23f7e2, 0x37fe27ad, 0x4b98577c, 0x60458733,
  0x8b13591a, 0xa0ce8955, 0xdca8f984, 0xf77529cb,
  0x24641826, 0x0fb9c869, 0x73dfb8b8, 0x580268f7,
  0x7dce4349, 0x56139306, 0x2a75e3d7, 0x01a83398,
  0xd2b90275, 0xf964d23a, 0x8502a2eb, 0xaedf72a4,
  0xce9af597, 0xe54725d8, 0x99215509, 0xb2fc8546,
  0x61edb4ab, 0x4a3064e4, 0x36561435, 0x1d8bc47a,
  0x3847efc4, 0x139a3f8b, 0x6ffc4f5a, 0x44219f15,
  0x9730aef8, 0xbced7eb7, 0xc08b0e66, 0xeb56de29,
  0xbe152a1f, 0x95c8fa50, 0xe9ae8a81, 0xc2735ace,
  0x11626b23, 0x3abfbb6c, 0x46d9cbbd, 0x6d041bf2,
  0x48c8304c, 0x6315e003, 0x1f7390d2, 0x34ae409d,
  0xe7bf7170, 0xcc62a13f, 0xb004d1ee, 0x9bd901a1,
  0xfb9c8692, 0xd04156dd, 0xac27260c, 0x87faf643,
  0x54ebc7ae, 0x7f3617e1, 0x03506730, 0x288db77f,
  0x0d419cc1, 0x269c4c8e, 0x5afa3c5f, 0x7127ec10,
  0xa236ddfd, 0x89eb0db2, 0xf58d7d63, 0xde50ad2c,
  0x35067305, 0x1edba34a, 0x62bdd39b, 0x496003d4,
  0x9a713239, 0xb1ace276, 0xcdca92a7, 0xe61742e8,
  0xc3db6956, 0xe806b919, 0x9460c9c8, 0xbfbd1987,
  0x6cac286a, 0x4771f825, 0x3b1788f4, 0x10ca58bb,
  0x708fdf88, 0x5b520fc7, 0x27347f16, 0x0ce9af59,
  0xdff89eb4, 0xf4254efb, 0x88433e2a, 0xa39eee65,
  0x8652c5db, 0xad8f1594, 0xd1e96545, 0xfa34b50a,
  0x292584e7, 0x02f854a8, 0x7e9e2479, 0x5543f436,
  0xd419cc15, 0xffc41c5a, 0x83a26c8b, 0xa87fbcc4,
  0x7b6e8d29, 0x50b35d66, 0x2cd52db7, 0x0708fdf8,
  0x22c4d646, 0x09190609, 0x757f76d8, 0x5ea2a697,
  0x8db3977a, 0xa66e4735, 0xda0837e4, 0xf1d5e7ab,
  0x91906098, 0xba4db0d7, 0xc62bc006, 0xedf61049,
  0x3ee721a4, 0x153af1eb, 0x695c813a, 0x42815175,
  0x674d7acb, 0x4c90aa84, 0x30f6da55, 0x1b2b0a1a,
  0xc83a3bf7, 0xe3e7ebb8, 0x9f819b69, 0xb45c4b26,
  0x5f0a950f, 0x74d74540, 0x08b13591, 0x236ce5de,
  0xf07dd433, 0xdba0047c, 0xa7c674ad, 0x8c1ba4e2,
  0xa9d78f5c, 0x820a5f13, 0xfe6c2fc2, 0xd5b1ff8d,
  0x06a0ce60, 0x2d7d1e2f, 0x511b6efe, 0x7ac6beb1,
  0x1a833982, 0x315ee9cd, 0x4d38991c, 0x66e54953,
  0xb5f478be, 0x9e29a8f1, 0xe24fd820, 0xc992086f,
  0xec5e23d1, 0xc783f39e, 0xbbe5834f, 0x90385300,
  0x432962ed, 0x68f4b2a2, 0x1492c273, 0x3f4f123c,
  0x6a0ce60a, 0x41d13645, 0x3db74694, 0x166a96db,
  0xc57ba736, 0xeea67779, 0x92c007a8, 0xb91dd7e7,
  0x9cd1fc59, 0xb70c2c16, 0xcb6a5cc7, 0xe0b78c88,
  0x33a6bd65, 0x187b6d2a, 0x641d1dfb, 0x4fc0cdb4,
  0x2f854a87, 0x04589ac8, 0x783eea19, 0x53e33a56,
  0x80f20bbb, 0xab2fdbf4, 0xd749ab25, 0xfc947b6a,
  0xd95850d4, 0xf285809b, 0x8ee3f04a, 0xa53e2005,
  0x762f11e8, 0x5df2c1a7, 0x2194b176, 0x0a496139,
  0xe11fbf10, 0xcac26f5f, 0xb6a41f8e, 0x9d79cfc1,
  0x4e68fe2c, 0x65b52e63, 0x19d35eb2, 0x320e8efd,
  0x17c2a543, 0x3c1f750c, 0x407905dd, 0x6ba4d592,
  0xb8b5e47f, 0x93683430, 0xef0e44e1, 0xc4d394ae,
  0xa496139d, 0x8f4bc3d2, 0xf32db303, 0xd8f0634c,
  0x0be152a1, 0x203c82ee, 0x5c5af23f, 0x77872270,
  0x524b09ce, 0x7996d981, 0x05f0a950, 0x2e2d791f,
  0xfd3c48f2, 0xd6e198bd, 0xaa87e86c, 0x815a3823
};

static const uint32_t crc32_table_814141ab[] =
{
  0x00000000, 0x814141ab, 0x83c3c2fd, 0x02828356,
  0x86c6c451, 0x078785fa, 0x050506ac, 0x84444707,
  0x8cccc909, 0x0d8d88a2, 0x0f0f0bf4, 0x8e4e4a5f,
  0x0a0a0d58, 0x8b4b4cf3, 0x89c9cfa5, 0x08888e0e,
  0x98d8d3b9, 0x19999212, 0x1b1b1144, 0x9a5a50ef,
  0x1e1e17e8, 0x9f5f5643, 0x9dddd515, 0x1c9c94be,
  0x14141ab0, 0x95555b1b, 0x97d7d84d, 0x169699e6,
  0x92d2dee1, 0x13939f4a, 0x11111c1c, 0x90505db7,
  0xb0f0e6d9, 0x31b1a772, 0x33332424, 0xb272658f,
  0x36362288, 0xb7776323, 0xb5f5e075, 0x34b4a1de,
  0x3c3c2fd0, 0xbd7d6e7b, 0xbfffed2d, 0x3ebeac86,
  0xbafaeb81, 0x3bbbaa2a, 0x3939297c, 0xb87868d7,
  0x28283560, 0xa96974cb, 0xabebf79d, 0x2aaab636,
  0xaeeef131, 0x2fafb09a, 0x2d2d33cc, 0xac6c7267,
  0xa4e4fc69, 0x25a5bdc2, 0x27273e94, 0xa6667f3f,
  0x22223838, 0xa3637993, 0xa1e1fac5, 0x20a0bb6e,
  0xe0a08c19, 0x61e1cdb2, 0x63634ee4, 0xe2220f4f,
  0x66664848, 0xe72709e3, 0xe5a58ab5, 0x64e4cb1e,
  0x6c6c4510, 0xed2d04bb, 0xefaf87ed, 0x6eeec646,
  0xeaaa8141, 0x6bebc0ea, 0x696943bc, 0xe8280217,
  0x78785fa0, 0xf9391e0b, 0xfbbb9d5d, 0x7afadcf6,
  0xfebe9bf1, 0x7fffda5a, 0x7d7d590c, 0xfc3c18a7,
  0xf4b496a9, 0x75f5d702, 0x77775454, 0xf63615ff,
  0x727252f8, 0xf3331353, 0xf1b19005, 0x70f0d1ae,
  0x50506ac0, 0xd1112b6b, 0xd393a83d, 0x52d2e996,
  0xd696ae91, 0x57d7ef3a, 0x55556c6c, 0xd4142dc7,
  0xdc9ca3c9, 0x5ddde262, 0x5f5f6134, 0xde1e209f,
  0x5a5a6798, 0xdb1b2633, 0xd999a565, 0x58d8e4ce,
  0xc888b979, 0x49c9f8d2, 0x4b4b7b84, 0xca0a3a2f,
  0x4e4e7d28, 0xcf0f3c83, 0xcd8dbfd5, 0x4cccfe7e,
  0x44447070, 0xc50531db, 0xc787b28d, 0x46c6f326,
  0xc282b421, 0x43c3f58a, 0x414176dc, 0xc0003777,
  0x40005999, 0xc1411832, 0xc3c39b64, 0x4282dacf,
  0xc6c69dc8, 0x4787dc63, 0x45055f35, 0xc4441e9e,
  0xcccc9090, 0x4d8dd13b, 0x4f0f526d, 0xce4e13c6,
  0x4a0a54c1, 0xcb4b156a, 0xc9c9963c, 0x4888d797,
  0xd8d88a20, 0x5999cb8b, 0x5b1b48dd, 0xda5a0976,
  0x5e1e4e71, 0xdf5f0fda, 0xdddd8c8c, 0x5c9ccd27,
  0x54144329, 0xd5550282, 0xd7d781d4, 0x5696c07f,
  0xd2d28778, 0x5393c6d3, 0x51114585, 0xd050042e,
  0xf0f0bf40, 0x71b1feeb, 0x73337dbd, 0xf2723c16,
  0x76367b11, 0xf7773aba, 0xf5f5b9ec, 0x74b4f847,
  0x7c3c7649, 0xfd7d37e2, 0xffffb4b4, 0x7ebef51f,
  0xfafab218, 0x7bbbf3b3, 0x793970e5, 0xf878314e,
  0x68286cf9, 0xe9692d52, 0xebebae04, 0x6aaaefaf,
  0xeeeea8a8, 0x6fafe903, 0x6d2d6a55, 0xec6c2bfe,
  0xe4e4a5f0, 0x65a5e45b, 0x6727670d, 0xe66626a6,
  0x622261a1, 0xe363200a, 0xe1e1a35c, 0x60a0e2f7,
  0xa0a0d580, 0x21e1942b, 0x2363177d, 0xa22256d6,
  0x266611d1, 0xa727507a, 0xa5a5d32c, 0x24e49287,
  0x2c6c1c89, 0xad2d5d22, 0xafafde74, 0x2eee9fdf,
  0xaaaad8d8, 0x2beb9973, 0x29691a25, 0xa8285b8e,
  0x38780639, 0xb9394792, 0xbbbbc4c4, 0x3afa856f,
  0xbebec268, 0x3fff83c3, 0x3d7d0095, 0xbc3c413e,
  0xb4b4cf30, 0x35f58e9b, 0x37770dcd, 0xb6364c66,
  0x32720b61, 0xb3334aca, 0xb1b1c99c, 0x30f08837,
  0x10503359, 0x911172f2, 0x9393f1a4, 0x12d2b00f,
  0x9696f708, 0x17d7b6a3, 0x155535f5, 0x9414745e,
  0x9c9cfa50, 0x1dddbbfb, 0x1f5f38ad, 0x9e1e7906,
  0x1a5a3e01, 0x9b1b7faa, 0x9999fcfc, 0x18d8bd57,
  0x8888e0e0, 0x09c9a14b, 0x0b4b221d, 0x8a0a63b6,
  0x0e4e24b1, 0x8f0f651a, 0x8d8de64c, 0x0ccca7e7,
  0x044429e9, 0x85056842, 0x8787eb14, 0x06c6aabf,
  0x8282edb8, 0x03c3ac13, 0x01412f45, 0x80006eee
};

static const uint32_t crc32_table_000000af[] =
{
  0x00000000, 0x000000af, 0x0000015e, 0x000001f1,
  0x000002bc, 0x00000213, 0x000003e2, 0x0000034d,
  0x00000578, 0x000005d7, 0x00000426, 0x00000489,
  0x000007c4, 0x0000076b, 0x0000069a, 0x00000635,
  0x00000af0, 0x00000a5f, 0x00000bae, 0x00000b01,
  0x0000084c, 0x000008e3, 0x00000912, 0x000009bd,
  0x00000f88, 0x00000f27, 0x00000ed6, 0x00000e79,
  0x00000d34, 0x00000d9b, 0x00000c6a, 0x00000cc5,
  0x000015e0, 0x0000154f, 0x000014be, 0x00001411,
  0x0000175c, 0x000017f3, 0x00001602, 0x000016ad,
  0x00001098, 0x00001037, 0x000011c6, 0x00001169,
  0x00001224, 0x0000128b, 0x0000137a, 0x000013d5,
  0x00001f10, 0x00001fbf, 0x00001e4e, 0x00001ee1,
  0x00001dac, 0x00001d03, 0x00001cf2, 0x00001c5d,
  0x00001a68, 0x00001ac7, 0x00001b36, 0x00001b99,
  0x000018d4, 0x0000187b, 0x0000198a, 0x00001925,
  0x00002bc0, 0x00002b6f, 0x00002a9e, 0x00002a31,
  0x0000297c, 0x000029d3, 0x00002822, 0x0000288d,
  0x00002eb8, 0x00002e17, 0x00002fe6, 0x00002f49,
  0x00002c04, 0x00002cab, 0x00002d5a, 0x00002df5,
  0x00002130, 0x0000219f, 0x0000206e, 0x000020c1,
  0x0000238c, 0x00002323, 0x000022d2, 0x0000227d,
  0x00002448, 0x000024e7, 0x00002516, 0x000025b9,
  0x000026f4, 0x0000265b, 0x000027aa, 0x00002705,
  0x00003e20, 0x00003e8f, 0x00003f7e, 0x00003fd1,
  0x00003c9c, 0x00003c33, 0x00003dc2, 0x00003d6d,
  0x00003b58, 0x00003bf7, 0x00003a06, 0x00003aa9,
  0x000039e4, 0x0000394b, 0x000038ba, 0x00003815,
  0x000034d0, 0x0000347f, 0x0000358e, 0x00003521,
  0x0000366c, 0x000036c3, 0x00003732, 0x0000379d,
  0x000031a8, 0x00003107, 0x000030f6, 0x00003059,
  0x00003314, 0x000033bb, 0x0000324a, 0x000032e5,
  0x00005780, 0x0000572f, 0x000056de, 0x00005671,
  0x0000553c, 0x00005593, 0x00005462, 0x000054cd,
  0x000052f8, 0x00005257, 0x000053a6, 0x00005309,
  0x00005044, 0x000050eb, 0x0000511a, 0x000051b5,
  0x00005d70, 0x00005ddf, 0x00005c2e, 0x00005c81,
  0x00005fcc, 0x00005f63, 0x00005e92, 0x00005e3d,
  0x00005808, 0x000058a7, 0x00005956, 0x000059f9,
  0x00005ab4, 0x00005a1b, 0x00005bea, 0x00005b45,
  0x00004260, 0x000042cf, 0x0000433e, 0x00004391,
  0x000040dc, 0x00004073, 0x00004182, 0x0000412d,
  0x00004718, 0x000047b7, 0x00004646, 0x000046e9,
  0x000045a4, 0x0000450b, 0x000044fa, 0x00004455,
  0x00004890, 0x0000483f, 0x000049ce, 0x00004961,
  0x00004a2c, 0x00004a83, 0x00004b72, 0x00004bdd,
  0x00004de8, 0x00004d47, 0x00004cb6, 0x00004c19,
  0x00004f54, 0x00004ffb, 0x00004e0a, 0x00004ea5,
  0x00007c40, 0x00007cef, 0x00007d1e, 0x00007db1,
  0x00007efc, 0x00007e53, 0x00007fa2, 0x00007f0d,
  0x00007938, 0x00007997, 0x00007866, 0x000078c9,
  0x00007b84, 0x00007b2b, 0x00007ada, 0x00007a75,
  0x000076b0, 0x0000761f, 0x000077ee, 0x00007741,
  0x0000740c, 0x000074a3, 0x00007552, 0x000075fd,
  0x000073c8, 0x00007367, 0x00007296, 0x00007239,
  0x00007174, 0x000071db, 0x0000702a, 0x00007085,
  0x000069a0, 0x0000690f, 0x000068fe, 0x00006851,
  0x00006b1c, 0x00006bb3, 0x00006a42, 0x00006aed,
  0x00006cd8, 0x00006c77, 0x00006d86, 0x00006d29,
  0x00006e64, 0x00006ecb, 0x00006f3a, 0x00006f95,
  0x00006350, 0x000063ff, 0x0000620e, 0x000062a1,
  0x000061ec, 0x00006143, 0x000060b2, 0x0000601d,
  0x00006628, 0x00006687, 0x00006776, 0x000067d9,
  0x00006494, 0x0000643b, 0x000065ca, 0x00006565
};

/* For the parameters of different CRC32 algorithms, see
   <https://crccalc.com/> */
void pcutils_crc32_begin(pcutils_crc32_ctxt *ctxt, purc_crc32_algo_t algo)
{
    switch (algo) {
        case PURC_K_ALGO_CRC32:
            ctxt->poly = 0x04C11DB7L;
            ctxt->init = 0xFFFFFFFFL;
            ctxt->xorout = 0xFFFFFFFFL;
            ctxt->refin = false;
            ctxt->refout = true;
            ctxt->table_static = crc32_table_04c11db7_reflected;
            break;

        case PURC_K_ALGO_CRC32_BZIP2:
            ctxt->poly = 0x04C11DB7L;
            ctxt->init = 0xFFFFFFFFL;
            ctxt->xorout = 0xFFFFFFFFL;
            ctxt->refin = false;
            ctxt->refout = false;
            ctxt->table_static = crc32_table_04c11db7;
            break;

        case PURC_K_ALGO_CRC32_XFER:
            ctxt->poly = 0x000000AFL;
            ctxt->init = 0;
            ctxt->xorout = 0;
            ctxt->refin = false;
            ctxt->refout = false;
            ctxt->table_static = crc32_table_000000af;
            break;

        case PURC_K_ALGO_CRC32_MPEG2:
            ctxt->poly = 0x04C11DB7L;
            ctxt->init = 0xFFFFFFFFL;
            ctxt->xorout = 0;
            ctxt->refin = false;
            ctxt->refout = false;
            ctxt->table_static = crc32_table_04c11db7;
            break;

        case PURC_K_ALGO_CRC32_POSIX:
            ctxt->poly = 0x04C11DB7L;
            ctxt->init = 0;
            ctxt->xorout = 0xFFFFFFFFL;
            ctxt->refin = false;
            ctxt->refout = false;
            ctxt->table_static = crc32_table_04c11db7;
            break;

        case PURC_K_ALGO_CRC32_ISCSI:
        case PURC_K_ALGO_CRC32C:
            ctxt->poly = 0x1EDC6F41L;
            ctxt->init = 0xFFFFFFFFL;
            ctxt->xorout = 0xFFFFFFFFL;
            ctxt->refin = false;
            ctxt->refout = true;
            ctxt->table_static = crc32_table_1edc6f41_reflected;
            break;

        case PURC_K_ALGO_CRC32_BASE91_D:
        case PURC_K_ALGO_CRC32D:
            ctxt->poly = 0xA833982BL;
            ctxt->init = 0xFFFFFFFFL;
            ctxt->xorout = 0xFFFFFFFFL;
            ctxt->refin = false;
            ctxt->refout = true;
            ctxt->table_static = crc32_table_a833982b_reflected;
            break;

        case PURC_K_ALGO_CRC32_JAMCRC:
            ctxt->poly = 0x04C11DB7L;
            ctxt->init = 0xFFFFFFFFL;
            ctxt->xorout = 0;
            ctxt->refin = false;
            ctxt->refout = true;
            ctxt->table_static = crc32_table_04c11db7_reflected;
            break;

        case PURC_K_ALGO_CRC32_AIXM:
        case PURC_K_ALGO_CRC32Q:
            ctxt->poly = 0x814141ABL;
            ctxt->init = 0;
            ctxt->xorout = 0;
            ctxt->refin = false;
            ctxt->refout = false;
            ctxt->table_static = crc32_table_814141ab;
            break;

        case PURC_K_ALGO_CRC32_UNKNOWN:
            PC_ASSERT(0);
            break;
    }

    ctxt->crc32 = ctxt->init;
}

void pcutils_crc32_update(pcutils_crc32_ctxt *ctxt,
        const void *data, size_t n)
{
    const uint8_t *buf = data;

    while (n--) {
        uint8_t ch;
        ch = *buf;

        if (ctxt->refout)
            ctxt->crc32 = (((ctxt->crc32) >> 8) ^
                    ctxt->table_static[((ctxt->crc32) ^ (ch)) & 0xFF]);
        else
            ctxt->crc32 = (ctxt->crc32 << 8) ^
                ctxt->table_static[((ctxt->crc32 >> 24) ^ ch) & 0xFF];

        buf++;
    }
}

void pcutils_crc32_end(pcutils_crc32_ctxt *ctxt, uint32_t* crc32)
{
    *crc32 = ctxt->crc32 ^ ctxt->xorout;
}

#if 0
static const uint8_t reflect_table[] = {
    0x0,    // 0x0,
    0x8,    // 0x1,
    0x4,    // 0x2,
    0xC,    // 0x3,
    0x2,    // 0x4,
    0xA,    // 0x5,
    0x6,    // 0x6,
    0xE,    // 0x7,
    0x1,    // 0x8,
    0x9,    // 0x9,
    0x5,    // 0xA,
    0xD,    // 0xB,
    0x3,    // 0xC,
    0xB,    // 0xD,
    0x7,    // 0xE,
    0xF,    // 0xF,
};

static inline uint8_t reflect_uint8(uint8_t u8)
{
    uint8_t half_l = u8 & 0x0F;
    uint8_t half_h = (u8 >> 4) & 0x0F;

    return (reflect_table[half_l] << 4) | reflect_table[half_h];
}
#endif

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

static void calc_crc32_table(uint32_t *table, uint32_t poly, bool refin)
{
    int i, j;
    uint32_t c;

    if (refin) {
        poly = reflect_uint32(poly);
        for (i = 0; i < 256; i++) {
            for (c = i, j = 0; j < 8; j++)
                c = (c >> 1) ^ (poly & (-(int32_t)(c & 1)));
            table[i] = c;
        }
    }
    else {
        for (i = 0; i < 256; i++) {
            for (c = i << 24, j = 8; j > 0; --j)
                c = c & 0x80000000 ? (c << 1) ^ poly : (c << 1);
            table[i] = c;
        }
    }

}

pcutils_crc32_ctxt *
pcutils_crc32_begin_custom(uint32_t poly, uint32_t init, uint32_t xorout,
        bool refin, bool refout)
{
    pcutils_crc32_ctxt *ctxt;

    ctxt = malloc(sizeof(*ctxt));
    if (ctxt) {
        ctxt->table_alloc = malloc(sizeof(uint32_t)*256);
        if (ctxt->table_alloc == NULL) {
            free(ctxt);
            ctxt = NULL;
            goto fatal;
        }

        ctxt->poly = poly;
        ctxt->init = init;
        ctxt->xorout = xorout;
        ctxt->refin = true;
        ctxt->refout = refout;
        if (refin) {
            calc_crc32_table(ctxt->table_alloc, poly, refin);
        }
    }

fatal:
    return ctxt;
}

void
pcutils_crc32_end_custom(pcutils_crc32_ctxt *ctxt, uint32_t* crc32)
{
    *crc32 = ctxt->crc32 ^ ctxt->xorout;
    free(ctxt->table_alloc);
    free(ctxt);
}

