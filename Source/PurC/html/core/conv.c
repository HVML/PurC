/**
 * @file conv.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of data convertion.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
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


#include <math.h>
#include <float.h>

#include "html/core/conv.h"
#include "html/core/dtoa.h"
#include "html/core/strtod.h"


size_t
pchtml_conv_float_to_data(double num, unsigned char *buf, size_t len)
{
    return pchtml_dtoa(num, buf, len);
}

double
pchtml_conv_data_to_double(const unsigned char **start, size_t len)
{
    int exponent, exp, insignf;
    unsigned char c, *pos;
    bool minus;
    const unsigned char  *e, *p, *last, *end;
    unsigned char data[128];

    end = *start + len;

    exponent = 0;
    insignf = 0;

    pos = data;
    last = data + sizeof(data);

    for (p = *start; p < end; p++) {
        /* Values less than '0' become >= 208. */
        c = *p - '0';

        if (c > 9) {
            break;
        }

        if (pos < last) {
            *pos++ = *p;
        }
        else {
            insignf++;
        }
    }

    /* Do not emit a '.', but adjust the exponent instead. */
    if (p < end && *p == '.') {

        for (p++; p < end; p++) {
            /* Values less than '0' become >= 208. */
            c = *p - '0';

            if (c > 9) {
                break;
            }

            if (pos < last) {
                *pos++ = *p;
                exponent--;
            }
            else {
                /* Ignore insignificant digits in the fractional part. */
            }
        }
    }

    e = p + 1;

    if (e < end && (*p == 'e' || *p == 'E')) {
        minus = 0;

        if (e + 1 < end) {
            if (*e == '-') {
                e++;
                minus = 1;
            }
            else if (*e == '+') {
                e++;
            }
        }

        /* Values less than '0' become >= 208. */
        c = *e - '0';

        if (c <= 9) {
            exp = c;

            for (p = e + 1; p < end; p++) {
                /* Values less than '0' become >= 208. */
                c = *p - '0';

                if (c > 9) {
                    break;
                }

                exp = exp * 10 + c;
            }

            exponent += minus ? -exp : exp;
        }
    }

    *start = p;

    exponent += insignf;

    return pchtml_strtod_internal(data, pos - data, exponent);
}

unsigned long
pchtml_conv_data_to_ulong(const unsigned char **data, size_t length)
{
    const unsigned char *p = *data;
    const unsigned char *end = p + length;
    unsigned long last_number = 0, number = 0;

    for (; p < end; p++) {
        if (*p < '0' || *p > '9') {
            goto done;
        }

        number = (*p - '0') + number * 10;

        if (last_number > number) {
            *data = p - 1;
            return last_number;
        }

        last_number = number;
    }

done:

    *data = p;

    return number;
}

unsigned
pchtml_conv_data_to_uint(const unsigned char **data, size_t length)
{
    const unsigned char *p = *data;
    const unsigned char *end = p + length;
    unsigned last_number = 0, number = 0;

    for (; p < end; p++) {
        if (*p < '0' || *p > '9') {
            goto done;
        }

        number = (*p - '0') + number * 10;

        if (last_number > number) {
            *data = p - 1;
            return last_number;
        }

        last_number = number;
    }

done:

    *data = p;

    return number;
}
