/*
 * @file printbuf.c
 * @author gengyue 
 * @date 2021/07/02
 * @brief The API for print buffer.
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

#ifndef PURC_PRIVATE_PRINTBUF_H 
#define PURC_PRIVATE_PRINTBUF_H

#ifdef __cplusplus
extern "C" {
#endif

struct purc_printbuf
{
	char *buf;
	int bpos;
	int size;
};
typedef struct purc_printbuf purc_printbuf;

int purc_printbuf_init(struct purc_printbuf *pb);

struct purc_printbuf * purc_printbuf_new(void);

/* As an optimization, printbuf_memappend_fast() is defined as a macro
 * that handles copying data if the buffer is large enough; otherwise
 * it invokes printbuf_memappend() which performs the heavy
 * lifting of realloc()ing the buffer and copying data.
 *
 * Your code should not use printbuf_memappend() directly unless it
 * checks the return code. Use printbuf_memappend_fast() instead.
 */
int purc_printbuf_memappend(struct purc_printbuf *p, const char *buf, int size);

#define purc_printbuf_memappend_fast(p, bufptr, bufsize)         \
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
			purc_printbuf_memappend(p, (bufptr), bufsize);       \
		}                                                        \
	} while (0)

#define purc_printbuf_length(p) ((p)->bpos)

#define _purc_printbuf_check_literal(mystr) ("" mystr)

#define purc_printbuf_strappend(pb, str) \
	purc_printbuf_memappend((pb), _purc_printbuf_check_literal(str), sizeof(str) - 1)

int purc_printbuf_memset(struct purc_printbuf *pb, int offset, int charvalue, int len);

int purc_printbuf_shrink(struct purc_printbuf *pb, int len);

int purc_sprintbuf(struct purc_printbuf *p, const char *msg, ...);

void purc_printbuf_reset(struct purc_printbuf *p);

void purc_printbuf_free(struct purc_printbuf *p);

#ifdef __cplusplus
}
#endif

#endif  /* PURC_PRIVATE_PRINTBUF_H */
