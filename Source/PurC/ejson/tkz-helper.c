/*
 * @file tkz-helper.c
 * @author XueShuming
 * @date 2022/04/22
 * @brief The impl of tkz helper.
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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "purc-utils.h"
#include "purc-errors.h"
#include "private/errors.h"
#include "private/tkz-helper.h"

#if HAVE(GLIB)
#include <gmodule.h>
#else
#include <stdlib.h>
#endif

#define NR_CONSUMED_LIST_LIMIT   128
#define MIN_BUFFER_CAPACITY      32

#if HAVE(GLIB)
#define    PCHVML_ALLOC(sz)   g_slice_alloc0(sz)
#define    PCHVML_FREE(p)     g_slice_free1(sizeof(*p), (gpointer)p)
#else
#define    PCHVML_ALLOC(sz)   calloc(1, sz)
#define    PCHVML_FREE(p)     free(p)
#endif

enum tkz_reader_data_source_type {
    TKA_READER_DATA_SOURCE_TYPE_UNDEFINED = 0,
    TKA_READER_DATA_SOURCE_TYPE_RWS,
    TKA_READER_DATA_SOURCE_TYPE_UCS
};

struct tkz_reader {
    enum tkz_reader_data_source_type data_source_type;
    union {
        purc_rwstream_t rws;
        struct tkz_ucs *ucs;
    };

    struct tkz_ucs *reconsume_ucs;
    struct tkz_ucs *consumed_ucs;
    size_t nr_consumed_list;

    struct tkz_uc curr_uc;
    struct tkz_lc *lc;

    int line;
    int column;
    int consumed;
};


struct tkz_unihan_area {
    uint32_t begin;
    uint32_t end;
} unihan_areas[] = {
    {0x4E00, 0x9FFC},
    {0xF900, 0xFAD9},
    {0x3400, 0x4DBF},
    {0x20000, 0x2A6DD},
    {0x2A700, 0x2B734},
    {0x2B740, 0x2B81D},
    {0x2B820, 0x2CEA1},
    {0x2CEB0, 0x2EBE0},
    {0x2F800, 0x2FA1D},
    {0x30000, 0x3134A}
};

bool is_unihan(uint32_t uc)
{
    static ssize_t max = sizeof(unihan_areas)/sizeof(unihan_areas[0]) - 1;
    for (ssize_t i = 0; i < max; i++) {
        if (uc >= unihan_areas[i].begin && uc <= unihan_areas[i].end) {
            return true;
        }
    }
    return false;
}

struct tkz_uc *tkz_uc_new(void)
{
    return PCHVML_ALLOC(sizeof(struct tkz_uc));
}

void tkz_uc_destroy(struct tkz_uc *uc)
{
    if (uc) {
        PCHVML_FREE(uc);
    }
}

struct tkz_reader *tkz_reader_new(void)
{
    struct tkz_reader *reader = PCHVML_ALLOC(sizeof(struct tkz_reader));
    if (!reader) {
        PC_ERROR("Failed to allocate memory for tkz_reader (%zu)\n",
                sizeof(*reader));
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    reader->data_source_type = TKA_READER_DATA_SOURCE_TYPE_UNDEFINED;
    reader->reconsume_ucs = tkz_ucs_new();
    if (!reader->reconsume_ucs) {
        goto failed_clear_reader;
    }

    reader->consumed_ucs = tkz_ucs_new();
    if (!reader->consumed_ucs) {
        goto failed_clear_reconsume_ucs;
    }

    reader->line = 1;
    reader->column = 0;
    reader->consumed = 0;

    return reader;

failed_clear_reconsume_ucs:
    tkz_ucs_destroy(reader->reconsume_ucs);

failed_clear_reader:
    free(reader);

failed:
    return NULL;
}

void tkz_reader_set_data_source_rws(struct tkz_reader *reader,
        purc_rwstream_t rws)
{
    reader->data_source_type = TKA_READER_DATA_SOURCE_TYPE_RWS;
    reader->rws = rws;
}

void tkz_reader_set_data_source_ucs(struct tkz_reader *reader,
        struct tkz_ucs *ucs)
{
    reader->data_source_type = TKA_READER_DATA_SOURCE_TYPE_UCS;
    reader->ucs = ucs;
}

void tkz_reader_set_lc(struct tkz_reader *reader, struct tkz_lc *lc)
{
    reader->lc = lc;
}

static struct tkz_uc*
tkz_reader_read_from_rwstream(struct tkz_reader *reader)
{
    char c[8] = {0};
    uint32_t uc = 0;
    int nr_c = purc_rwstream_read_utf8_char(reader->rws, c, &uc);
    if (nr_c < 0) {
        uc = TKZ_INVALID_CHARACTER;
    }
    reader->column++;
    reader->consumed++;

    reader->curr_uc.character = uc;
    reader->curr_uc.line = reader->line;
    reader->curr_uc.column = reader->column;
    reader->curr_uc.position = reader->consumed;
    strcpy((char *)reader->curr_uc.utf8_buf, c);

    if (uc == '\n') {
        if (reader->lc) {
            tkz_lc_commit(reader->lc, reader->line);
        }
        reader->line++;
        reader->column = 0;
    }
    else {
        if (reader->lc && uc > 0) {
            tkz_lc_append_bytes(reader->lc, c, strlen(c));
        }
    }
    return &reader->curr_uc;
}

static struct tkz_uc*
tkz_reader_read_from_ucs(struct tkz_reader *reader)
{
    reader->curr_uc = tkz_ucs_read_head(reader->ucs);
    reader->line = reader->curr_uc.line;
    reader->column = reader->curr_uc.column;
    reader->consumed++;
    return &reader->curr_uc;
}

static struct tkz_uc*
tkz_reader_read_from_data_source(struct tkz_reader *reader)
{
    if (reader->data_source_type == TKA_READER_DATA_SOURCE_TYPE_RWS) {
        return tkz_reader_read_from_rwstream(reader);
    }
    return tkz_reader_read_from_ucs(reader);
}

static struct tkz_uc*
tkz_reader_read_from_reconsume_list(struct tkz_reader *reader)
{
    reader->curr_uc = tkz_ucs_read_head(reader->reconsume_ucs);
    return &reader->curr_uc;
}

static bool
tkz_reader_add_consumed(struct tkz_reader *reader, struct tkz_uc *uc)
{
    if (tkz_ucs_add_tail(reader->consumed_ucs, *uc)) {
        return false;
    }
    reader->nr_consumed_list++;

    if (reader->nr_consumed_list > NR_CONSUMED_LIST_LIMIT) {
        tkz_ucs_read_head(reader->consumed_ucs);
        reader->nr_consumed_list--;
    }
    return true;
}

bool tkz_reader_reconsume_last_char(struct tkz_reader *reader)
{
    if (!reader->nr_consumed_list) {
        return true;
    }

    struct tkz_uc uc = tkz_ucs_read_tail(reader->consumed_ucs);
    reader->nr_consumed_list--;

    tkz_ucs_add_head(reader->reconsume_ucs, uc);
    return true;
}

struct tkz_uc *tkz_reader_current(struct tkz_reader *reader)
{
    return &reader->curr_uc;
}

struct tkz_uc *tkz_reader_next_char(struct tkz_reader *reader)
{
    struct tkz_uc *ret = NULL;
    if (tkz_ucs_is_empty(reader->reconsume_ucs)) {
        ret = tkz_reader_read_from_data_source(reader);
    }
    else {
        ret = tkz_reader_read_from_reconsume_list(reader);
    }

    if (tkz_reader_add_consumed(reader, ret)) {
        return ret;
    }
    return NULL;
}

void tkz_reader_destroy(struct tkz_reader *reader)
{
    if (reader) {
        if (reader->reconsume_ucs) {
            tkz_ucs_destroy(reader->reconsume_ucs);
        }

        if (reader->consumed_ucs) {
            tkz_ucs_destroy(reader->consumed_ucs);
        }

        PCHVML_FREE(reader);
    }
}

struct tkz_buffer *tkz_reader_get_line_from_cache(struct tkz_reader *reader,
        int line_num)
{
    if (reader->lc && line_num >= 0) {
        return tkz_lc_get_line(reader->lc, line_num);
    }
    return NULL;
}

struct tkz_buffer *tkz_reader_get_curr_line(struct tkz_reader *reader)
{
    return reader->lc ? tkz_lc_get_current(reader->lc) : NULL;
}

int tkz_reader_get_line_number(struct tkz_reader *reader)
{
    return reader->line;
}

/* tkz uc list */
struct tkz_ucs *tkz_ucs_new(void)
{
    struct tkz_ucs *ucs = (struct tkz_ucs*) calloc(1, sizeof(*ucs));
    if (!ucs) {
        PC_ERROR("Failed to allocate memory for tkz_ucs (%zu)\n", sizeof(*ucs));
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }
    INIT_LIST_HEAD(&ucs->list);

out:
    return ucs;
}

bool tkz_ucs_is_empty(struct tkz_ucs *ucs)
{
    return list_empty(&ucs->list);
}

struct tkz_uc tkz_ucs_read_head(struct tkz_ucs *ucs)
{
    struct tkz_uc ret = {0};
    if (tkz_ucs_is_empty(ucs)) {
        goto out;
    }

    struct tkz_uc *uc = list_first_entry(&ucs->list, struct tkz_uc, ln);

    ret = *uc;
    list_del(&uc->ln);
    tkz_uc_destroy(uc);

out:
    return ret;
}

struct tkz_uc tkz_ucs_read_tail(struct tkz_ucs *ucs)
{
    struct tkz_uc ret = {0};
    if (tkz_ucs_is_empty(ucs)) {
        goto out;
    }
    struct tkz_uc *uc = list_last_entry(&ucs->list, struct tkz_uc, ln);

    ret = *uc;
    list_del(&uc->ln);
    tkz_uc_destroy(uc);

out:
    return ret;
}

int tkz_ucs_delete_tail(struct tkz_ucs *ucs, size_t sz)
{
    struct tkz_uc *p, *n;
    list_for_each_entry_reverse_safe(p, n, &ucs->list, ln) {
        if (sz == 0) {
            break;
        }
        list_del(&p->ln);
        tkz_uc_destroy(p);
        sz--;
    }
    return 0;
}

int tkz_ucs_add_head(struct tkz_ucs *ucs, struct tkz_uc uc)
{
    struct tkz_uc *p = tkz_uc_new();
    if (!p) {
        PC_ERROR("Failed to allocate memory for tkz_uc (%zu)\n", sizeof(*p));
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    *p = uc;
    list_add(&p->ln, &ucs->list);

    return 0;
out:
    return -1;
}

int tkz_ucs_add_tail(struct tkz_ucs *ucs, struct tkz_uc uc)
{
    struct tkz_uc *p = tkz_uc_new();
    if (!p) {
        PC_ERROR("Failed to allocate memory for tkz_uc (%zu)\n", sizeof(*p));
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    *p = uc;
    list_add_tail(&p->ln, &ucs->list);

    return 0;
out:
    return -1;
}

int tkz_ucs_dump(struct tkz_ucs *ucs)
{
    fprintf(stderr, "dump tkz ucs begin\n");
    struct tkz_uc *p, *n;
    list_for_each_entry_safe(p, n, &ucs->list, ln) {
        fprintf(stderr, "%s", p->utf8_buf);
    }
    fprintf(stderr, "|\ndump tkz ucs end\n");
    return 0;
}

int tkz_ucs_reset(struct tkz_ucs *ucs)
{
    if (tkz_ucs_is_empty(ucs)) {
        goto out;
    }
    struct tkz_uc *p, *n;
    list_for_each_entry_safe(p, n, &ucs->list, ln) {
        list_del(&p->ln);
        tkz_uc_destroy(p);
    }
    INIT_LIST_HEAD(&ucs->list);

out:
    return 0;
}

void tkz_ucs_destroy(struct tkz_ucs *ucs)
{
    if (tkz_ucs_is_empty(ucs)) {
        goto clear_ucs;
    }

    struct tkz_uc *p, *n;
    list_for_each_entry_safe(p, n, &ucs->list, ln) {
        list_del(&p->ln);
        tkz_uc_destroy(p);
    }

clear_ucs:
    free(ucs);
}

// tokenizer buffer

static size_t get_buffer_size(size_t sz)
{
    size_t sz_buf = pcutils_get_next_fibonacci_number(sz);
    return sz_buf < MIN_BUFFER_CAPACITY ? MIN_BUFFER_CAPACITY : sz_buf;
}

struct tkz_buffer *tkz_buffer_new(void)
{
    struct tkz_buffer *buffer = (struct tkz_buffer*) calloc(
            1, sizeof(struct tkz_buffer));
    size_t sz_init = get_buffer_size(MIN_BUFFER_CAPACITY);
    buffer->base = (uint8_t*) calloc(1, sz_init + 1);
    buffer->here = buffer->base;
    buffer->stop = buffer->base + sz_init;
    buffer->nr_chars = 0;
    return buffer;
}

static bool is_utf8_leading_byte(char c)
{
    return (c & 0xC0) != 0x80;
}

static uint32_t utf8_to_uint32_t(const unsigned char *utf8_char,
        int utf8_char_len)
{
    uint32_t wc = *((unsigned char *)(utf8_char++));
    int n = utf8_char_len;
    int t = 0;

    if (wc & 0x80) {
        wc &= (1 <<(8-n)) - 1;
        while (--n > 0) {
            t = *((unsigned char *)(utf8_char++));
            wc = (wc << 6) | (t & 0x3F);
        }
    }

    return wc;
}

static void tkz_buffer_append_inner(struct tkz_buffer *buffer,
        const char *bytes, size_t nr_bytes)
{
    uint8_t *newpos = buffer->here + nr_bytes;
    if ( newpos > buffer->stop ) {
        size_t new_size = get_buffer_size(newpos - buffer->base);
        off_t here_offset = buffer->here - buffer->base;

        uint8_t *newbuf = (uint8_t*) realloc(buffer->base, new_size + 1);
        if (newbuf == NULL) {
            pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return;
        }

        buffer->base = newbuf;
        buffer->here = buffer->base + here_offset;
        buffer->stop = buffer->base + new_size;
    }

    memcpy(buffer->here, bytes, nr_bytes);
    buffer->here += nr_bytes;
    *buffer->here = 0;
}

void tkz_buffer_append_bytes(struct tkz_buffer *buffer, const char *bytes,
        size_t nr_bytes)
{
    tkz_buffer_append_inner(buffer, bytes, nr_bytes);
    const uint8_t *p = (const uint8_t*)bytes;
    const uint8_t *end = p + nr_bytes;
    while (p != end) {
        if (is_utf8_leading_byte(*p)) {
            buffer->nr_chars++;
        }
        p++;
    }
}

size_t uc_to_utf8(uint32_t c, char *outbuf)
{
    size_t len = 0;
    int first;
    int i;

    if (c < 0x80) {
        first = 0;
        len = 1;
    }
    else if (c < 0x800) {
        first = 0xc0;
        len = 2;
    }
    else if (c < 0x10000) {
        first = 0xe0;
        len = 3;
    }
    else if (c < 0x200000) {
        first = 0xf0;
        len = 4;
    }
    else if (c < 0x4000000) {
        first = 0xf8;
        len = 5;
    }
    else {
        first = 0xfc;
        len = 6;
    }

    if (outbuf) {
        for (i = len - 1; i > 0; --i) {
            outbuf[i] = (c & 0x3f) | 0x80;
            c >>= 6;
        }
        outbuf[0] = c | first;
    }

    return len;
}

void tkz_buffer_append(struct tkz_buffer *buffer,
        uint32_t uc)
{
    char buf[8] = {0};
    size_t len = uc_to_utf8(uc, buf);
    tkz_buffer_append_bytes(buffer, buf, len);
}

void tkz_buffer_append_chars(struct tkz_buffer *buffer,
        const uint32_t *ucs, size_t nr_ucs)
{
    for (size_t i = 0; i < nr_ucs; i++) {
        tkz_buffer_append(buffer, ucs[i]);
    }
}

void tkz_buffer_delete_head_chars(
        struct tkz_buffer *buffer, size_t sz)
{
    uint8_t *p = buffer->base;
    size_t nr = 0;
    while (p < buffer->here && nr <= sz) {
        if (is_utf8_leading_byte(*p)) {
            nr++;
        }
        p = p + 1;
    }
    p = p - 1;
    size_t n = buffer->here - p;
    memmove(buffer->base, p, n);
    buffer->here = buffer->base + n;
    memset(buffer->here, 0, buffer->stop - buffer->here);
}

void tkz_buffer_delete_tail_chars(
        struct tkz_buffer *buffer, size_t sz)
{
    uint8_t *p = buffer->here - 1;
    while (p >= buffer->base && sz > 0) {
        if (is_utf8_leading_byte(*p)) {
            sz--;
        }
        p = p - 1;
    }
    buffer->here = p + 1;
    memset(buffer->here, 0, buffer->stop - buffer->here);
}

bool
tkz_buffer_start_with(struct tkz_buffer *buffer, const char *bytes,
        size_t nr_bytes)
{
    size_t sz = tkz_buffer_get_size_in_bytes(buffer);
    return (sz >= nr_bytes
            && memcmp(buffer->base, bytes, nr_bytes) == 0);
}

bool tkz_buffer_end_with(struct tkz_buffer *buffer,
        const char *bytes, size_t nr_bytes)
{
    size_t sz = tkz_buffer_get_size_in_bytes(buffer);
    return (sz >= nr_bytes
            && memcmp(buffer->here - nr_bytes, bytes, nr_bytes) == 0);
}

bool tkz_buffer_equal_to(struct tkz_buffer *buffer,
        const char *bytes, size_t nr_bytes)
{
    size_t sz = tkz_buffer_get_size_in_bytes(buffer);
    return (sz == nr_bytes && memcmp(buffer->base, bytes, sz) == 0);
}

uint32_t tkz_buffer_get_last_char(struct tkz_buffer *buffer)
{
    if (tkz_buffer_is_empty(buffer)) {
        return 0;
    }

    uint8_t *p = buffer->here - 1;
    while (p >= buffer->base) {
        if (is_utf8_leading_byte(*p)) {
            break;
        }
        p = p - 1;
    }
    return utf8_to_uint32_t(p, buffer->here - p);
}

bool tkz_buffer_is_int(struct tkz_buffer *buffer)
{
    char *p = NULL;
    strtol((const char*)buffer->base, &p, 10);
    return (p == (char*)buffer->here);
}

bool tkz_buffer_is_number(struct tkz_buffer *buffer)
{
    char *p = NULL;
    strtold((const char*)buffer->base, &p);
    return (p == (const char*)buffer->here);
}

bool tkz_buffer_is_whitespace(struct tkz_buffer *buffer)
{
    uint8_t *p = buffer->base;
    while (p !=  buffer->here) {
        if (*p == ' ' || *p == '\x0A' || *p == '\x09' || *p == '\x0C') {
            p++;
            continue;
        }
        return false;
    }
    return true;
}

void tkz_buffer_reset(struct tkz_buffer *buffer)
{
    memset(buffer->base, 0, buffer->stop - buffer->base);
    buffer->here = buffer->base;
    buffer->nr_chars = 0;
}

void tkz_buffer_destroy(struct tkz_buffer *buffer)
{
    if (buffer) {
        free(buffer->base);
        free(buffer);
    }
}

/* line cache begin */
struct tkz_lc *
tkz_lc_new(size_t max_size)
{
    struct tkz_lc *lc = (struct tkz_lc *)calloc(1, sizeof(*lc));
    if (!lc) {
        PC_ERROR("Failed to allocate memory for line cache (%zu)\n", sizeof(*lc));
        goto out;
    }

    lc->current = tkz_buffer_new();
    if (!lc) {
        PC_ERROR("Failed to allocate memory for line cache current (%zu)\n",
                sizeof(*lc->current));
        goto clear_lc;
    }

    lc->size = 0;
    lc->max_size = max_size;
    INIT_LIST_HEAD(&lc->cache);

    return lc;

clear_lc:
    PCHVML_FREE(lc);

out:
    return lc;
}

void
tkz_lc_destroy(struct tkz_lc *lc)
{
    if (!lc) {
        return;
    }

    if (lc->current) {
        tkz_buffer_destroy(lc->current);
        lc->current = NULL;
    }

    struct tkz_lc_node *p, *n;
    list_for_each_entry_safe(p, n, &lc->cache, ln) {
        list_del(&p->ln);
        if (p->buf) {
            tkz_buffer_destroy(p->buf);
        }
        free(p);
    }

    free(lc);
}

void
tkz_lc_reset(struct tkz_lc *lc)
{
    assert(lc);
    tkz_buffer_reset(lc->current);
    struct tkz_lc_node *p, *n;
    list_for_each_entry_safe(p, n, &lc->cache, ln) {
        list_del(&p->ln);
        if (p->buf) {
            tkz_buffer_destroy(p->buf);
        }
        free(p);
    }
    lc->size = 0;
    INIT_LIST_HEAD(&lc->cache);
}

int
tkz_lc_append(struct tkz_lc *lc, char c)
{
    assert(lc);
    tkz_buffer_append_bytes(lc->current, &c, 1);
    return 0;
}

int
tkz_lc_append_bytes(struct tkz_lc *lc, const char *bytes, size_t nr_bytes)
{
    assert(lc && bytes && nr_bytes);
    tkz_buffer_append_bytes(lc->current, bytes, nr_bytes);
    return 0;
}

int
tkz_lc_commit(struct tkz_lc *lc, int line_num)
{
    assert(lc && line_num >= 0);

    int ret = -1;
    size_t nr_bytes = tkz_buffer_get_size_in_bytes(lc->current);
    if (nr_bytes == 0) {
        ret = 0;
        goto out;
    }

    /* create new line cache node */
    struct tkz_lc_node *node = (struct tkz_lc_node *)calloc(1, sizeof(*node));
    if (!node) {
        PC_ERROR("Failed to allocate memory for line cache node(%zu)\n",
                sizeof(*node));
        ret = -1;
        goto out;
    }

    node->buf = tkz_buffer_new();
    if (!node->buf) {
        PC_ERROR("Failed to allocate memory for line cache node buf (%zu)\n",
                sizeof(*node->buf));
        goto out_clear_node;
    }

    tkz_buffer_append_another(node->buf, lc->current);
    node->line = line_num;

    list_add(&node->ln, &lc->cache);
    lc->size++;

    /* clear old node */
    while (lc->size > lc->max_size) {
        struct tkz_lc_node *last =
            list_last_entry(&lc->cache, struct tkz_lc_node, ln);
        list_del(&last->ln);
        tkz_buffer_destroy(last->buf);
        free(last);
        lc->size--;
    }

    tkz_buffer_reset(lc->current);
    ret = 0;

    return ret;

out_clear_node:
    free(node);

out:
    return ret;
}

struct tkz_buffer *
tkz_lc_get_line(const struct tkz_lc *lc, int line_num)
{
    struct tkz_lc_node *node;
    list_for_each_entry(node, &lc->cache, ln) {
        if (node->line == line_num)
            return node->buf;
    }
    return NULL;
}

struct tkz_buffer *
tkz_lc_get_current(const struct tkz_lc *lc)
{
    return lc ? lc->current : NULL;
}

/* line cache end */

struct tkz_sbst {
    const pcutils_sbst_entry_static_t *strt;
    const pcutils_sbst_entry_static_t *root;
    const pcutils_sbst_entry_static_t *match;
    struct pcutils_arrlist *ucs;
};

static
struct tkz_sbst *tkz_sbst_new(const pcutils_sbst_entry_static_t *strt)
{
    struct tkz_sbst *sbst = (struct tkz_sbst*)
        PCHVML_ALLOC(sizeof(struct tkz_sbst));
    if (!sbst) {
        return NULL;
    }
    sbst->strt = strt;
    sbst->root = strt + 1;
    sbst->ucs = pcutils_arrlist_new(NULL);
    return sbst;
}

void tkz_sbst_destroy(struct tkz_sbst *sbst)
{
    if (sbst) {
        pcutils_arrlist_free(sbst->ucs);
        PCHVML_FREE(sbst);
    }
}

bool tkz_sbst_advance_ex(struct tkz_sbst *sbst,
        uint32_t uc, bool case_insensitive)
{
    pcutils_arrlist_append(sbst->ucs, (void*)(uintptr_t)uc);
    if (uc > 0x7F) {
        return false;
    }

    if (case_insensitive && uc >= 'A' && uc <= 'Z') {
        uc = uc | 0x20;
    }
    const pcutils_sbst_entry_static_t *ret =
                pcutils_sbst_entry_static_find(sbst->strt, sbst->root, uc);
    if (ret) {
        if (ret->value) {
            sbst->match = ret;
        }
        sbst->root = sbst->strt + ret->next;
        return true;
    }
    sbst->root = NULL;
    sbst->match = NULL;
    return false;
}

const char *tkz_sbst_get_match(struct tkz_sbst *sbst)
{
    if (sbst->match) {
        return (char*)sbst->match->value;
    }
    return NULL;
}

struct pcutils_arrlist *tkz_sbst_get_buffered_ucs(
        struct tkz_sbst *sbst)
{
    return sbst->ucs;
}

static void
free_error_info(void *key, void *local_data)
{
    free_key_string(key);

    struct purc_parse_error_info *info =
        (struct purc_parse_error_info *) local_data;

    if (info->extra) {
        free(info->extra);
    }

    if (info->code_snippets) {
        free(info->code_snippets);
    }

    free(info);
}

int
tkz_set_error_info(struct tkz_reader *reader, struct tkz_uc *uc, int error,
        const char *type, const char *extra)
{
    if (!uc) {
        purc_set_error(error);
        goto out;
    }

    struct purc_parse_error_info *info = (struct purc_parse_error_info *)calloc(1,
            sizeof(*info));
    if (!info) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }
    int line = uc->line;
    int column = uc->column;
    info->character = uc->character;
    info->line = line;
    info->column = column;
    info->position = uc->position;
    info->error = error;

    if (extra) {
        info->extra = strdup(extra);
        purc_set_error_with_info(error, "E%d:%s:%d:%d:%s:%s",
                error, type, info->line, info->column,
                purc_get_error_message(error), extra);
    }
    else {
        const char *err_msg = purc_get_error_message(error);
        /* type: err_msg */
        size_t nr = strlen(type) + strlen(err_msg) + 3;
        info->extra = malloc(nr);
        sprintf(info->extra, "%s: %s", type, err_msg);
        purc_set_error_with_info(error, "E%d:%s:%d:%d:%s",
                error, type, info->line, info->column,
                purc_get_error_message(error));
    }

    if (reader->lc) {
        purc_rwstream_t rws = purc_rwstream_new_buffer(1024, 0);
        purc_rwstream_write(rws, "<<<<\n", 5);

        int curr_ln = uc->line;
        if (curr_ln > 0) {
            struct tkz_buffer *line = tkz_reader_get_line_from_cache(reader,
                    curr_ln - 1);
            if (line) {
                const char *buf = tkz_buffer_get_bytes(line);
                size_t nr_buf = tkz_buffer_get_size_in_bytes(line);
                purc_rwstream_write(rws, buf, nr_buf);
                purc_rwstream_write(rws, "\n", 1);
            }
        }

        struct tkz_buffer *line = tkz_reader_get_line_from_cache(reader, curr_ln);
        if (!line) {
            line = tkz_reader_get_curr_line(reader);
        }
        if (line) {
            const char *buf = tkz_buffer_get_bytes(line);
            size_t nr_buf = tkz_buffer_get_size_in_bytes(line);
            purc_rwstream_write(rws, buf, nr_buf);
            purc_rwstream_write(rws, "\n", 1);

            int column = uc->column;
            int chars = 0;
            for (const char *p = buf; *p && chars < column - 1; ) {
                char c = *p;
                int nr_c = (*p & 0x80) == 0 ? 1 : ((*p & 0xE0) == 0xC0 ? 2 : 3);

                purc_rwstream_write(rws, " ", 1);
                if ((c & 0xF0) == 0xE0) {
                    purc_rwstream_write(rws, " ", 1);
                }

                p += nr_c;
                chars++;
            }

            purc_rwstream_write(rws, "^", 1);
            purc_rwstream_write(rws, "\n", 1);
        }
        purc_rwstream_write(rws, ">>>>\n", 5);

        size_t sz_content, sz_buff;
        bool res_buff = true;
        char *p;
        p = (char*)purc_rwstream_get_mem_buffer_ex(rws,
                &sz_content, &sz_buff, res_buff);

        info->code_snippets = p;
        purc_rwstream_destroy(rws);
    }
    purc_set_local_data(PURC_LDNAME_PARSE_ERROR, (uintptr_t)info,
            free_error_info);
out:
    return 0;
}

struct tkz_sbst *tkz_sbst_new_char_ref(void)
{
    return tkz_sbst_new(pchtml_html_tokenizer_res_entities_sbst);
}

static const pcutils_sbst_entry_static_t markup_declaration_open_state_sbst[] =
{
    {0x00, NULL, 0, 0, 0, 0},
    {0x44, NULL, 0, 3, 2, 4},
    {0x5b, NULL, 0, 0, 0, 10},
    {0x2d, NULL, 0, 0, 0, 16},
    {0x4f, NULL, 0, 0, 0, 5},
    {0x43, NULL, 0, 0, 0, 6},
    {0x54, NULL, 0, 0, 0, 7},
    {0x59, NULL, 0, 0, 0, 8},
    {0x50, NULL, 0, 0, 0, 9},
    {0x45, (char*)"\x44\x4f\x43\x54\x59\x50\x45", 7, 0, 0, 0},
    {0x43, NULL, 0, 0, 0, 11},
    {0x44, NULL, 0, 0, 0, 12},
    {0x41, NULL, 0, 0, 0, 13},
    {0x54, NULL, 0, 0, 0, 14},
    {0x41, NULL, 0, 0, 0, 15},
    {0x5b, (char*)"\x5b\x43\x44\x41\x54\x41\x5b", 7, 0, 0, 0},
    {0x2d, (char*)"\x2d\x2d", 2, 0, 0, 0}
};

struct tkz_sbst *tkz_sbst_new_markup_declaration_open_state(void)
{
    return tkz_sbst_new(markup_declaration_open_state_sbst);
}

static const pcutils_sbst_entry_static_t after_doctype_name_state_sbst[] =
{
    {0x00, NULL, 0, 0, 0, 0},
    {0x73, NULL, 0, 2, 0, 3},
    {0x70, NULL, 0, 0, 0, 8},
    {0x79, NULL, 0, 0, 0, 4},
    {0x73, NULL, 0, 0, 0, 5},
    {0x74, NULL, 0, 0, 0, 6},
    {0x65, NULL, 0, 0, 0, 7},
    {0x6d, (char*)"\x53\x59\x53\x54\x45\x4d", 6, 0, 0, 0},
    {0x75, NULL, 0, 0, 0, 9},
    {0x62, NULL, 0, 0, 0, 10},
    {0x6c, NULL, 0, 0, 0, 11},
    {0x69, NULL, 0, 0, 0, 12},
    {0x63, (char*)"\x50\x55\x42\x4c\x49\x43", 6, 0, 0, 0}
};

struct tkz_sbst *tkz_sbst_new_after_doctype_name_state(void)
{
    return tkz_sbst_new(after_doctype_name_state_sbst);
}

/* true false null undefined */
static const pcutils_sbst_entry_static_t ejson_keywords_sbst[] =
{
    {0x00, NULL, 0, 0, 0, 0},
    {0x74, NULL, 0, 3, 2, 5},
    {0x75, NULL, 0, 0, 0, 8},
    {0x6e, NULL, 0, 4, 0, 16},
    {0x66, NULL, 0, 0, 0, 19},
    {0x72, NULL, 0, 0, 0, 6},
    {0x75, NULL, 0, 0, 0, 7},
    {0x65, (char*)"\x74\x72\x75\x65", 4, 0, 0, 0},
    {0x6e, NULL, 0, 0, 0, 9},
    {0x64, NULL, 0, 0, 0, 10},
    {0x65, NULL, 0, 0, 0, 11},
    {0x66, NULL, 0, 0, 0, 12},
    {0x69, NULL, 0, 0, 0, 13},
    {0x6e, NULL, 0, 0, 0, 14},
    {0x65, NULL, 0, 0, 0, 15},
    {0x64, (char*)"\x75\x6e\x64\x65\x66\x69\x6e\x65\x64", 9, 0, 0, 0},
    {0x75, NULL, 0, 0, 0, 17},
    {0x6c, NULL, 0, 0, 0, 18},
    {0x6c, (char*)"\x6e\x75\x6c\x6c", 4, 0, 0, 0},
    {0x61, NULL, 0, 0, 0, 20},
    {0x6c, NULL, 0, 0, 0, 21},
    {0x73, NULL, 0, 0, 0, 22},
    {0x65, (char*)"\x66\x61\x6c\x73\x65", 5, 0, 0, 0}
};

struct tkz_sbst *tkz_sbst_new_ejson_keywords(void)
{
    return tkz_sbst_new(ejson_keywords_sbst);
}

