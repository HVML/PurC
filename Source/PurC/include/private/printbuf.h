/*
 * @file printbuf.h
 * @author gengyue
 * @date 2021/07/02
 * @brief The implementation of print buffer.
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
 *
 * Note that the original code come from json-c, which is licensed under
 * MIT License (<http://www.opensource.org/licenses/mit-license.php>).
 *
 * The copying annoucements are as follow:
 *
 * Copyright (c) 2008-2009 Yahoo! Inc.  All rights reserved.
 *
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Author: Michael Clark <michael@metaparadigm.com>
 */

#ifndef PURC_PRIVATE_PRINTBUF_H
#define PURC_PRIVATE_PRINTBUF_H

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

struct pcutils_printbuf
{
    char *buf;
    int bpos;
    int size;
};
typedef struct pcutils_printbuf pcutils_printbuf;

int pcutils_printbuf_init(struct pcutils_printbuf *pb);

struct pcutils_printbuf * pcutils_printbuf_new(void);

/* As an optimization, printbuf_memappend_fast() is defined as a macro
 * that handles copying data if the buffer is large enough; otherwise
 * it invokes printbuf_memappend() which performs the heavy
 * lifting of realloc()ing the buffer and copying data.
 *
 * Your code should not use printbuf_memappend() directly unless it
 * checks the return code. Use printbuf_memappend_fast() instead.
 */
int pcutils_printbuf_memappend(struct pcutils_printbuf *p, const char *buf, int size);

#define pcutils_printbuf_memappend_fast(p, bufptr, bufsize)      \
    do                                                           \
    {                                                            \
        if ((p->size - p->bpos) > bufsize)                       \
        {                                                        \
            memcpy(p->buf + p->bpos, (bufptr), bufsize);         \
            p->bpos += bufsize;                                  \
            p->buf[p->bpos] = '\0';                              \
        }                                                        \
        else                                                     \
        {                                                        \
            pcutils_printbuf_memappend(p, (bufptr), bufsize);    \
        }                                                        \
    } while (0)

#define pcutils_printbuf_length(p) ((p)->bpos)

#define _printbuf_check_literal(mystr) ("" mystr)

#define pcutils_printbuf_strappend(pb, str) \
    pcutils_printbuf_memappend((pb), _printbuf_check_literal(str), sizeof(str) - 1)

int pcutils_printbuf_memset(struct pcutils_printbuf *pb, int offset, int charvalue, int len);

int pcutils_printbuf_shrink(struct pcutils_printbuf *pb, int len);

int pcutils_sprintbuf(struct pcutils_printbuf *p, const char *msg, ...)
    WTF_ATTRIBUTE_PRINTF(2, 3);

void pcutils_printbuf_reset(struct pcutils_printbuf *p);

void pcutils_printbuf_free(struct pcutils_printbuf *p);

#ifdef __cplusplus
}
#endif

#endif  /* PURC_PRIVATE_PRINTBUF_H */
