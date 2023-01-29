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
#include "private/ports.h"
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

/*
 * TODO:
 * When total_orig_size reaches a number (say 64KB), we can shrink the cached
 * by emoving some vDOMs according to LRU.
 *
 * By now, we keep all vDOMs until the program exits.
 */
static size_t total_orig_size;
static pcutils_map* md5_vdom_map;

struct vdom_entry {
    time_t expire;
    size_t length;
    purc_vdom_t vdom;
};

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

static void *copy_entry(const void *val)
{
    const struct vdom_entry *entry = val;
    total_orig_size += entry->length;
    return (void *)val;
}

static void free_entry(void *val)
{
    struct vdom_entry *entry = val;
    total_orig_size -= entry->length;
    pcvdom_document_unref(entry->vdom);
    free(val);
}

static void cleanup_loader_once(void)
{
#ifndef NDEBUG
    size_t n = pcutils_map_get_size(md5_vdom_map);
    fprintf(stderr, "Totally cached vdom: %llu/%llu\n",
            (unsigned long long)total_orig_size,
            (unsigned long long)n);
#endif
    pcutils_map_destroy(md5_vdom_map);
}

int pcintr_init_loader_once(void)
{
    md5_vdom_map = pcutils_map_create(copy_md5_key, free_md5_key,
            copy_entry, free_entry, cmp_md5_keys, true);
    if (md5_vdom_map == NULL)
        goto failed;

    if (atexit(cleanup_loader_once))
        goto failed;

    return 0;

failed:
    if (md5_vdom_map)
        pcutils_map_destroy(md5_vdom_map);
    return -1;
}

static bool
cache_vdom(unsigned char *md5, unsigned expire_after, size_t length,
        purc_vdom_t vdom)
{
    struct vdom_entry *entry = calloc(1, sizeof(*entry));

    time_t expire;
    if (expire_after)
       expire = purc_monotonic_time_after(expire_after);
    else
       expire = purc_monotonic_time_after(3600);

    entry->expire = expire;
    entry->length = length;
    entry->vdom = vdom;

    pcvdom_document_ref(vdom);
    if (pcutils_map_replace_or_insert(md5_vdom_map, md5, entry, NULL)) {
        pcvdom_document_unref(vdom);
        return false;
    }

    return true;
}

static purc_vdom_t find_vdom_in_cache(unsigned char *md5)
{
    purc_vdom_t vdom = NULL;

    pcutils_map_entry* entry;
    entry = pcutils_map_find_and_lock(md5_vdom_map, md5);
    if (entry) {
        time_t t = purc_get_monotoic_time();
        struct vdom_entry *vdom_entry = entry->val;
        if (t >= vdom_entry->expire) {
            pcutils_map_erase_entry_nolock(md5_vdom_map, entry);
        }
        else {
            struct vdom_entry *vdom_entry = entry->val;
            vdom = vdom_entry->vdom;
        }

        pcutils_map_unlock(md5_vdom_map);
    }

    return vdom;
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

    vdom = find_vdom_in_cache(md5);
    if (vdom == NULL) {
        purc_rwstream_t in;
        in = purc_rwstream_new_from_mem((void*)string, length);
        if (!in) {
            goto failed;
        }

        if ((vdom = purc_load_hvml_from_rwstream(in))) {
            cache_vdom(md5, 0, length, vdom);
        }

        purc_rwstream_destroy(in);
    }

failed:
    return vdom;
}

purc_vdom_t
purc_load_hvml_from_file(const char* file)
{
    size_t length;
    purc_vdom_t vdom;
    unsigned char md5[MD5_DIGEST_SIZE];

    if (!pcutils_file_md5(file, md5, &length) || length == 0) {
        purc_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
        return NULL;
    }

    vdom = find_vdom_in_cache(md5);
    if (vdom == NULL) {
        purc_rwstream_t in;
        in = purc_rwstream_new_from_file(file, "r");
        if (!in) {
            goto failed;
        }

        if ((vdom = purc_load_hvml_from_rwstream(in))) {
            cache_vdom(md5, 0, length, vdom);
        }
        purc_rwstream_destroy(in);
    }

    return vdom;

failed:
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

    pcutils_md5digest(url, md5);

    vdom = find_vdom_in_cache(md5);
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
                size_t length = purc_rwstream_tell(resp);
                cache_vdom(md5, 60, length, vdom);
            }
            purc_rwstream_destroy(resp);
        }

        if (resp_header.mime_type) {
            free(resp_header.mime_type);
        }
    }

    return vdom;
}

