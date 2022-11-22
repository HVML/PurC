/*
** @file slotset.h
** @author Vincent Wei
** @date 2022/11/21
** @brief Implementation of bitmap slot set.
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


#include <stdlib.h>

size_t foil_lookfor_unused_slot (unsigned char* bitmap, size_t len_bmp, int set)
{
    size_t unused = 0;
    size_t i, j;

    for (i = 0; i < len_bmp; i++) {
        if (*bitmap) {
            unsigned char half_byte = *bitmap & 0xF0;
            unsigned char test_byte = 0x80;

            if (half_byte) {
                for (j = 0; j < 4; j++) {
                    if (half_byte & test_byte) {
                        if (set)
                            *bitmap ^= test_byte;
                        return unused + j;
                    }

                    test_byte = test_byte >> 1;
                }
            }

            half_byte = *bitmap & 0x0F;
            test_byte = 0x08;
            for (j = 4; j < 8; j++) {
                if (half_byte & test_byte) {
                    if (set)
                        *bitmap ^= test_byte;
                    return unused + j;
                }

                test_byte = test_byte >> 1;
            }
        }

        unused += 8;
        bitmap++;
    }

    return -1;
}

/* get the number of idle mask rectangles */
size_t foil_get_nr_idle_slots (unsigned char* bitmap, size_t len_bmp)
{
    size_t idle = 0;
    size_t i, j;

    for (i = 0; i < len_bmp; i++) {
        if (*bitmap) {
            if (*bitmap == 0xFF) {
                idle += 8;
            }
            else {
                if (*bitmap & 0xF0) {
                    for (j = 0; j < 4; j++) {
                        if (*bitmap & (0x80 >> j))
                            idle++;
                    }
                }
                if (*bitmap & 0x0F) {
                    for (j = 0; j < 4; j++) {
                        if (*bitmap & (0x08 >> j))
                            idle++;
                    }
                }
            }
        }

        bitmap++;
    }

    return idle;
}

