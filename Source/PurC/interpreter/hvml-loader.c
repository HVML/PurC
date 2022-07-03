/*
 * @file hvml-loader.c
 * @author Vincent Wei
 * @date 2022/07/03
 * @brief The impl of the HVML loader.
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

#include "purc.h"

#include "private/hvml.h"
#include "private/map.h"
#include "private/fetcher.h"
#include "../hvml/hvml-gen.h"

#include <time.h>

purc_vdom_t
purc_load_hvml_from_rwstream(purc_rwstream_t stm)
{
    struct pchvml_parser *parser = NULL;
    struct pcvdom_gen *gen = NULL;
    struct pcvdom_document *doc = NULL;
    struct pchvml_token *token = NULL;

    parser = pchvml_create(0, 0);
    if (!parser)
        goto error;

    gen = pcvdom_gen_create();
    if (!gen)
        goto error;

again:
    if (token)
        pchvml_token_destroy(token);

    token = pchvml_next_token(parser, stm);

    if (!token)
        goto error;

    if (pcvdom_gen_push_token(gen, parser, token))
        goto error;

    if (!pchvml_token_is_type(token, PCHVML_TOKEN_EOF)) {
        goto again;
    }

    doc = pcvdom_gen_end(gen);
    goto end;

error:
    doc = pcvdom_gen_end(gen);
    if (doc) {
        pcvdom_document_unref(doc);
        doc = NULL;
    }

end:
    if (token)
        pchvml_token_destroy(token);

    if (gen)
        pcvdom_gen_destroy(gen);

    if (parser)
        pchvml_destroy(parser);

    return doc;
}

static pcutils_map* md5_vdom_map;

/* common functions for string key */
static void* copy_md5_key(const void *key)
{
    void *dst = malloc(MD5_DIGEST_SIZE);
    memcpy(dst, key, MD5_DIGEST_SIZE);
    return dst;
}

static void free_md5_key(void *key)
{
    free(key);
}

static int cmp_md5_keys(const void *key1, const void *key2)
{
    return memcmp((const char*)key1, (const char*)key2, MD5_DIGEST_SIZE);
}

static void free_md5_vdom(void *key, void *val)
{
    free(key);
    pcvdom_document_unref((purc_vdom_t)val);
}

bool init_vdom_map(void)
{
    md5_vdom_map = pcutils_map_create(copy_md5_key, free_md5_key,
            NULL, NULL, cmp_md5_keys, true);
    return md5_vdom_map != NULL;
}

void term_vdom_map(void)
{
    pcutils_map_destroy(md5_vdom_map);
}

static bool cache_doc(unsigned char *md5, purc_vdom_t vdom)
{
    if (pcutils_map_insert_ex(md5_vdom_map, md5, vdom, free_md5_vdom))
        return false;

    pcvdom_document_ref(vdom);
    return true;
}

static purc_vdom_t find_doc_in_cache(unsigned char *md5)
{
    pcutils_map_entry* entry;
    entry = pcutils_map_find(md5_vdom_map, md5);
    if (entry) {
        purc_vdom_t vdom = entry->val;
        pcvdom_document_ref(vdom);
        return entry->val;
    }

    return NULL;
}

purc_vdom_t
purc_load_hvml_from_string(const char* string)
{
    purc_vdom_t vdom;
    unsigned char md5[MD5_DIGEST_SIZE];
    size_t length = strlen(string);

    if (length == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    pcutils_md5digest(string, md5);

    vdom = find_doc_in_cache(md5);
    if (vdom == NULL) {
        purc_rwstream_t in;
        in = purc_rwstream_new_from_mem((void*)string, length);
        if (!in) {
            goto failed;
        }

        if ((vdom = purc_load_hvml_from_rwstream(in))) {
            cache_doc(md5, vdom);
        }

        purc_rwstream_destroy(in);
    }

failed:
    return vdom;
}

static FILE *md5sum(const char *file, unsigned char *md5_buf, size_t *length)
{
    char buf[1024];
    pcutils_md5_ctxt ctx;
    size_t len = 0;
    FILE *f;

    f = fopen(file, "r");
    if (!f)
        return NULL;

    pcutils_md5_begin(&ctx);
    do {
        size_t len = fread(buf, 1, sizeof(buf), f);
        if (!len)
            break;

        pcutils_md5_hash(&ctx, buf, len);
        len += len;
    } while(1);

    pcutils_md5_end(&ctx, md5_buf);
    fseek(f, SEEK_SET, 0);

    if (length)
        *length = len;

    return f;
}

purc_vdom_t
purc_load_hvml_from_file(const char* file)
{
    size_t length;
    purc_vdom_t vdom;
    unsigned char md5[MD5_DIGEST_SIZE];

    FILE *fp = md5sum(file, md5, &length);
    if (fp == NULL || length == 0) {
        purc_set_error(PURC_ERROR_BAD_STDC_CALL);
        return NULL;
    }

    vdom = find_doc_in_cache(md5);
    if (vdom == NULL) {
        purc_rwstream_t in;
        in = purc_rwstream_new_from_fp(fp);
        if (!in) {
            goto failed;
        }

        if ((vdom = purc_load_hvml_from_rwstream(in))) {
            cache_doc(md5, vdom);
        }
        purc_rwstream_destroy(in);
    }

    return vdom;

failed:
    if (fp)
        fclose(fp);

    return NULL;
}

purc_vdom_t
purc_load_hvml_from_url(const char* url)
{
    purc_vdom_t vdom;
    size_t length;
    unsigned char md5[MD5_DIGEST_SIZE];

    length = strlen(url);
    if (length == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    // TODO: time_t expire = purc_monotonic_time_after(60);
    pcutils_md5digest(url, md5);

    vdom = find_doc_in_cache(md5);
    if (vdom == NULL) {
        struct pcfetcher_resp_header resp_header = {0};
        purc_rwstream_t resp = pcfetcher_request_sync(
                url,
                PCFETCHER_REQUEST_METHOD_GET,
                NULL,
                10,
                &resp_header);

        if (resp_header.ret_code == 200) {
            vdom = purc_load_hvml_from_rwstream(resp);
            if (vdom) {
                cache_doc(md5, vdom);
            }
            purc_rwstream_destroy(resp);
        }

        if (resp_header.mime_type) {
            free(resp_header.mime_type);
        }
    }

    return vdom;
}

