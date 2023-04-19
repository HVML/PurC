/*
 * helpers.c -- The global helpers.
 *
 * Copyright (c) 2021 ~ 2023 FMSoft (http://www.fmsoft.cn)
 *
 * Authors:
 *  Vincent Wei (https://github.com/VincentWei), 2021, 2022, 2023
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

#include "config.h"
#include "purc-helpers.h"
#include "private/dvobjs.h"
#include "private/utils.h"
#include "private/kvlist.h"
#include "private/debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <assert.h>

bool purc_is_valid_token(const char* token, int max_len)
{
    int i;

    if (token[0] != '_' && !purc_isalpha(token[0]))
        return false;

    i = 1;
    while (token[i]) {

        if (max_len > 0 && i > max_len)
            return false;

        if (!purc_isalnum(token[i]) && token[i] != '_')
            return false;

        i++;
    }

    return true;
}

bool purc_is_valid_loose_token(const char* token, int max_len)
{
    int i;

    if (token[0] != '_' && !purc_isalpha(token [0]))
        return false;

    i = 1;
    while (token[i]) {

        if (max_len > 0 && i > max_len)
            return false;

        if (!purc_isalnum(token[i]) && token[i] != '_' && token[i] != '-')
            return false;

        i++;
    }

    return true;
}

bool purc_is_valid_endpoint_name(const char* endpoint_name)
{
    char host_name[PURC_LEN_HOST_NAME + 1];
    char app_name[PURC_LEN_APP_NAME + 1];
    char runner_name[PURC_LEN_RUNNER_NAME + 1];

    if (purc_extract_host_name(endpoint_name, host_name) <= 0)
        return false;

    if (purc_extract_app_name(endpoint_name, app_name) <= 0)
        return false;

    if (purc_extract_runner_name(endpoint_name, runner_name) <= 0)
        return false;

    return purc_is_valid_host_name(host_name) &&
        purc_is_valid_app_name(app_name) &&
        purc_is_valid_runner_name(runner_name);
}

static inline const char *check_endpoint_schema (const char *endpoint)
{
    if (strncasecmp(endpoint, PURC_EDPT_SCHEMA, PURC_LEN_EDPT_SCHEMA) == 0)
        return endpoint + PURC_LEN_EDPT_SCHEMA;

    return NULL;
}

/* edpt://<host_name>/<app_name>/<runner_name> */
int purc_extract_host_name(const char* endpoint, char* host_name)
{
    int len;
    char* slash;

    if ((endpoint = check_endpoint_schema(endpoint)) == NULL)
        return 0;

    if ((slash = strchr(endpoint, '/')) == NULL)
        return 0;

    len = (uintptr_t)slash - (uintptr_t)endpoint;
    if (len <= 0 || len > PURC_LEN_APP_NAME)
        return 0;

    strncpy(host_name, endpoint, len);
    host_name[len] = '\0';

    return len;
}

char* purc_extract_host_name_alloc(const char* endpoint)
{
    char* host_name;
    if ((host_name = malloc(PURC_LEN_HOST_NAME + 1)) == NULL)
        return NULL;

    if (purc_extract_host_name(endpoint, host_name) > 0)
        return host_name;

    free(host_name);
    return NULL;
}

/* edpt://<host_name>/<app_name>/<runner_name> */
int purc_extract_app_name(const char* endpoint, char* app_name)
{
    int len;
    char *first_slash, *second_slash;

    if ((endpoint = check_endpoint_schema(endpoint)) == NULL)
        return 0;

    if ((first_slash = strchr(endpoint, '/')) == 0 ||
            (second_slash = strrchr(endpoint, '/')) == 0 ||
            first_slash == second_slash)
        return 0;

    first_slash++;
    len = (uintptr_t)second_slash - (uintptr_t)first_slash;
    if (len <= 0 || len > PURC_LEN_APP_NAME)
        return 0;

    strncpy(app_name, first_slash, len);
    app_name[len] = '\0';

    return len;
}

char* purc_extract_app_name_alloc(const char* endpoint)
{
    char* app_name;

    if ((app_name = malloc(PURC_LEN_APP_NAME + 1)) == NULL)
        return NULL;

    if (purc_extract_app_name(endpoint, app_name) > 0)
        return app_name;

    free(app_name);
    return NULL;
}

/* edpt://<host_name>/<app_name>/<runner_name> */
int purc_extract_runner_name(const char* endpoint, char* runner_name)
{
    int len;
    char *second_slash;

    if ((endpoint = check_endpoint_schema(endpoint)) == NULL)
        return 0;

    if ((second_slash = strrchr(endpoint, '/')) == 0)
        return 0;

    second_slash++;
    len = strlen(second_slash);
    if (len > PURC_LEN_RUNNER_NAME)
        return 0;

    strcpy(runner_name, second_slash);

    return len;
}

char* purc_extract_runner_name_alloc(const char* endpoint)
{
    char* runner_name;

    if ((runner_name = malloc(PURC_LEN_RUNNER_NAME + 1)) == NULL)
        return NULL;

    if (purc_extract_runner_name(endpoint, runner_name) > 0)
        return runner_name;

    free(runner_name);
    return NULL;
}

int purc_assemble_endpoint_name_ex(const char* host_name,
        const char* app_name, const char* runner_name, char* buff, size_t sz)
{
    int host_len, app_len, runner_len;

    if ((host_len = strlen(host_name)) > PURC_LEN_HOST_NAME)
        return 0;

    if ((app_len = strlen(app_name)) > PURC_LEN_APP_NAME)
        return 0;

    if ((runner_len = strlen(runner_name)) > PURC_LEN_RUNNER_NAME)
        return 0;

    size_t len = PURC_LEN_EDPT_SCHEMA + host_len + 1 + app_len + 1 + runner_len;
    if (len >= sz)
        return 0;

    strcpy(buff, PURC_EDPT_SCHEMA);
    strcat(buff, host_name);
    buff += PURC_LEN_EDPT_SCHEMA + host_len;
    buff[0] = '/';
    buff[1] = '\0';

    strcat(buff, app_name);
    buff += app_len + 1;
    buff[0] = '/';
    buff[1] = '\0';

    strcat(buff, runner_name);
    return len;
}

char* purc_assemble_endpoint_name_alloc(const char* host_name,
        const char* app_name, const char* runner_name)
{
    char* endpoint;
    int host_len, app_len, runner_len;

    if ((host_len = strlen(host_name)) > PURC_LEN_HOST_NAME)
        return NULL;

    if ((app_len = strlen(app_name)) > PURC_LEN_APP_NAME)
        return NULL;

    if ((runner_len = strlen(runner_name)) > PURC_LEN_RUNNER_NAME)
        return NULL;

    int len = PURC_LEN_EDPT_SCHEMA + host_len + app_len + runner_len + 3;
    if ((endpoint = malloc(len)) == NULL)
        return NULL;

    purc_assemble_endpoint_name_ex(host_name, app_name, runner_name,
            endpoint, len);
    return endpoint;
}

bool purc_is_valid_host_name(const char* host_name)
{
    // TODO
    (void)host_name;
    return true;
}

/* cn.fmsoft.hybridos.aaa */
bool purc_is_valid_app_name(const char* app_name)
{
    size_t len, left = strlen(app_name);
    const char *start;
    const char *end;

    if (left > PURC_LEN_APP_NAME)
        return false;

    start = app_name;
    while (*start) {
        end = strchr(start, '.');
        if (end == NULL) {
            end += left;
            len = left;
        }
        else {
            len = end - start;
        }

        if (end == start || len == 0)
            return false;

        char token[len + 1];
        strncpy(token, start, len);
        token[len] = 0;
        if (!purc_is_valid_token(token, 0)) {
            return false;
        }

        assert(left >= len);
        left -= len;
        if (left > 0 && *end) {
            start = end + 1;
            left--;
        }
        else {
            break;
        }
    }

    return true;
}

void purc_generate_md5_id(char* id_buff, const char* prefix)
{
    int n;
    char key[256];
    unsigned char md5_digest[MD5_DIGEST_SIZE];
    struct timespec tp;

    clock_gettime(CLOCK_REALTIME, &tp);
    n = snprintf(key, sizeof(key), "%s-%ld-%ld-%ld", prefix,
            tp.tv_sec, tp.tv_nsec, random());

    if (n < 0) {
        PC_INFO("Unexpected call to snprintf.\n");
    }
    else if ((size_t)n >= sizeof(key))
        PC_INFO("The buffer is too small for resultId.\n");

    pcutils_md5digest(key, md5_digest);
    pcutils_bin2hex(md5_digest, MD5_DIGEST_SIZE, id_buff, false);
}

bool purc_is_valid_unique_id(const char* id)
{
    int n = 0;

    while (id [n]) {
        if (n > PURC_LEN_UNIQUE_ID)
            return false;

        if (!purc_isalnum(id [n]) && id [n] != '-')
            return false;

        n++;
    }

    return true;
}

bool purc_is_valid_md5_id(const char* id)
{
    int n = 0;

    while (id [n]) {
        if (n > (MD5_DIGEST_SIZE << 1))
            return false;

        if (!purc_isalnum(id [n]))
            return false;

        n++;
    }

    return true;
}

time_t purc_monotonic_time_after(time_t seconds)
{
    struct timespec ts_curr;

    clock_gettime(CLOCK_MONOTONIC, &ts_curr);
    return ts_curr.tv_sec + seconds;
}

double purc_get_elapsed_seconds(const struct timespec *ts_from,
        const struct timespec *ts_to)
{
    struct timespec ts_curr;
    time_t ds;
    long dns;

    if (ts_to == NULL) {
        clock_gettime(CLOCK_MONOTONIC, &ts_curr);
        ts_to = &ts_curr;
    }

    ds = ts_to->tv_sec - ts_from->tv_sec;
    dns = ts_to->tv_nsec - ts_from->tv_nsec;
    return ds + dns * 1.0E-9;
}

int64_t purc_get_elapsed_milliseconds(const struct timespec *ts_from,
        const struct timespec *ts_to)
{
    struct timespec ts_curr;
    int64_t ds;
    int64_t dns;

    if (ts_to == NULL) {
        clock_gettime(CLOCK_MONOTONIC, &ts_curr);
        ts_to = &ts_curr;
    }

    ds = ts_to->tv_sec - ts_from->tv_sec;
    dns = ts_to->tv_nsec - ts_from->tv_nsec;
    return ds * 1000 + dns / 1000000;
}

int64_t purc_get_elapsed_milliseconds_alt(time_t t_from,
        const struct timespec *ts_to)
{
    struct timespec ts_curr;
    int64_t ds;
    int64_t dns;

    if (ts_to == NULL) {
        clock_gettime(CLOCK_MONOTONIC, &ts_curr);
        ts_to = &ts_curr;
    }

    ds = ts_to->tv_sec - t_from;
    dns = ts_to->tv_nsec - 0;
    return ds * 1000 + dns / 1000000;
}

#define HVML_SCHEMA         "hvml://"
#define COMP_SEPERATOR      '/'
#define QUERY_SEPERATOR     '?'
#define FRAG_SEPERATOR      '#'
#define PAIR_SEPERATOR      '&'
#define KV_SEPERATOR        '='

size_t purc_hvml_uri_assemble(char *uri, const char *host, const char* app,
        const char* runner, const char *group, const char *page)
{
    char *start = uri;

    uri = stpcpy(uri, "hvml://");
    uri = stpcpy(uri, host);
    uri[0] = '/';
    uri++;

    uri = stpcpy(uri, app);
    uri[0] = '/';
    uri++;

    uri = stpcpy(uri, runner);
    uri[0] = '/';
    uri++;

    if (group) {
        uri = stpcpy(uri, group);
        uri[0] = '/';
        uri++;

        if (page) {
            uri = stpcpy(uri, page);
        }
    }

    uri[0] = '\0';
    return uri - start;
}

char* purc_hvml_uri_assemble_alloc(const char* host, const char* app,
        const char* runner, const char *group, const char *page)
{
    char* uri;
    static const int schema_len = sizeof(HVML_SCHEMA) - 1;
    int host_len, app_len, runner_len, group_len = 0, page_len = 0;

    if ((host_len = strlen (host)) > PURC_LEN_HOST_NAME)
        return NULL;

    if ((app_len = strlen (app)) > PURC_LEN_APP_NAME)
        return NULL;

    if ((runner_len = strlen (runner)) > PURC_LEN_RUNNER_NAME)
        return NULL;

    if (group) {
        group_len = strlen(group);
    }
    if (page)
        page_len = strlen(page);

    if ((uri = malloc(schema_len + host_len + app_len + runner_len +
                    group_len + page_len + 8)) == NULL)
        return NULL;


    purc_hvml_uri_assemble(uri, host, app, runner, group, page);
    return uri;
}

static unsigned int get_path_comp_len(const char *str)
{
    unsigned int len = 0;

    while (*str && *str != COMP_SEPERATOR) {
        len++;
        str++;
    }

    return len;
}

static unsigned int get_path_trail_len(const char *str)
{
    unsigned int len = 0;

    while (*str && *str != QUERY_SEPERATOR && *str != FRAG_SEPERATOR) {
        len++;
        str++;
    }

    return len;
}

bool purc_hvml_uri_split(const char *uri,
        char *host, char *app, char *runner, char *group, char *page)
{
    static const unsigned int sz_schema = sizeof(HVML_SCHEMA) - 1;
    unsigned int len;

    if (strncasecmp(uri, HVML_SCHEMA, sz_schema))
        return false;

    uri += sz_schema;
    len = get_path_comp_len(uri);
    if (len == 0 || uri[len] != COMP_SEPERATOR)
        return false;
    if (host) {
        strncpy(host, uri, len);
        host[len] = '\0';
    }

    uri += len + 1;
    len = get_path_comp_len(uri);
    if (len == 0 || uri[len] != COMP_SEPERATOR)
        goto failed;
    if (app) {
        strncpy(app, uri, len);
        app[len] = '\0';
    }

    uri += len + 1;
    len = get_path_comp_len(uri);
    if (len == 0 || uri[len] != COMP_SEPERATOR)
        goto failed;
    if (runner) {
        strncpy(runner, uri, len);
        runner[len] = '\0';
    }

    if (group == NULL && page == NULL) {
        /* stop splitting if the caller was not insterested in group and page */
        return true;
    }

    uri += len + 1;
    len = get_path_comp_len(uri);
    if (len == 0 || uri[len] != COMP_SEPERATOR)
        goto failed;
    if (group) {
        strncpy(group, uri, len);
        group[len] = '\0';
    }

    uri += len + 1;
    len = get_path_trail_len(uri);
    if (len == 0 || uri[0] == COMP_SEPERATOR)
        goto failed;

    if (page) {
        strncpy(page, uri, len);
        page[len] = '\0';
    }

    return true;

failed:
    return false;
}


bool purc_hvml_uri_split_alloc(const char *uri,
        char **host, char **app, char **runner, char **group, char **page)
{
    static const unsigned int sz_schema = sizeof(HVML_SCHEMA) - 1;
    char *my_host = NULL, *my_app = NULL, *my_runner = NULL;
    char *my_group = NULL, *my_page = NULL;
    unsigned int len;

    if (strncasecmp(uri, HVML_SCHEMA, sz_schema))
        return false;

    uri += sz_schema;
    len = get_path_comp_len(uri);
    if (len == 0 || uri[len] != COMP_SEPERATOR)
        return false;
    my_host = strndup(uri, len);

    uri += len + 1;
    len = get_path_comp_len(uri);
    if (len == 0 || uri[len] != COMP_SEPERATOR)
        goto failed;
    my_app = strndup(uri, len);

    uri += len + 1;
    len = get_path_comp_len(uri);
    if (len == 0 || uri[len] != COMP_SEPERATOR)
        goto failed;
    my_runner = strndup(uri, len);

    if (group == NULL && page == NULL) {
        /* stop splitting if the caller was not insterested in group and page */
        goto done;
    }

    uri += len + 1;
    len = get_path_comp_len(uri);
    if (len == 0 || uri[len] != COMP_SEPERATOR)
        goto failed;
    my_group = strndup(uri, len);

    uri += len + 1;
    len = get_path_trail_len(uri);
    if (len == 0 || uri[0] == COMP_SEPERATOR)
        goto failed;
    my_page = strndup(uri, len);

done:
    if (host)
        *host = my_host;
    else
        free(my_host);

    if (app)
        *app = my_app;
    else
        free(my_app);

    if (runner)
        *runner = my_runner;
    else
        free(my_runner);

    if (group)
        *group = my_group;
    else if (my_group)
        free(my_group);

    if (page)
        *page = my_page;
    else
        free(my_page);

    return true;

failed:
    if (my_host)
        free(my_host);
    if (my_app)
        free(my_app);
    if (my_runner)
        free(my_runner);
    if (my_group)
        free(my_group);
    if (my_page)
        free(my_page);

    return false;
}

static size_t get_key_len(const char *str)
{
    size_t len = 0;

    while (*str && *str != KV_SEPERATOR && *str != FRAG_SEPERATOR) {
        len++;
        str++;
    }

    return len;
}

static size_t get_value_len(const char *str)
{
    size_t len = 0;

    while (*str && *str != PAIR_SEPERATOR && *str != FRAG_SEPERATOR) {
        len++;
        str++;
    }

    return len;
}

static const char *locate_query_value(const char *uri, const char *key)
{
    size_t key_len = strlen(key);
    if (key_len == 0)
        return NULL;

    while (*uri && *uri != QUERY_SEPERATOR) {
        uri++;
    }

    if (uri[0] == 0)
        return NULL;

    char my_key[key_len + 2];
    strcpy(my_key, key);
    my_key[key_len] = KV_SEPERATOR;
    key_len++;
    my_key[key_len] = 0;

    const char *left = uri + 1;
    while (*left) {
        if (strncasecmp(left, my_key, key_len) == 0) {
            return left + key_len;
        }
        else {
            const char *value = left + get_key_len(left);
            unsigned int value_len = get_value_len(value);
            left = value + value_len;
            if (*left == PAIR_SEPERATOR)
                left++;

            if (*left == FRAG_SEPERATOR)
                break;
        }
    }

    return NULL;
}

bool purc_hvml_uri_get_query_value(const char *uri, const char *key,
        char *value_buff)
{
    const char *value = locate_query_value(uri, key);

    if (value == NULL) {
        return false;
    }

    size_t value_len = get_value_len(value);
    if (value_len == 0) {
        return false;
    }

    strncpy(value_buff, value, value_len);
    value_buff[value_len] = 0;
    return true;
}

bool purc_hvml_uri_get_query_value_alloc(const char *uri, const char *key,
        char **value_buff)
{
    const char *value = locate_query_value(uri, key);

    if (value == NULL) {
        return false;
    }

    size_t value_len = get_value_len(value);
    if (value_len == 0) {
        return false;
    }

    *value_buff = strndup(value, value_len);
    return true;
}

static purc_variant_t
decode_percent_escaped(const char *str, size_t len, bool rfc1738)
{
    DECL_MYSTRING(mystr);
    int ret = pcdvobj_url_decode(&mystr, str, len,
            rfc1738 ? PURC_K_KW_rfc1738 : PURC_K_KW_rfc3986, true);
    if (ret > 0) {
        pcutils_mystring_free(&mystr);
        goto failed;
    }
    else if (ret < 0) {
        goto failed;
    }

    if (pcutils_mystring_done(&mystr)) {
        goto failed;
    }

    return purc_variant_make_string_reuse_buff(mystr.buff,
            mystr.sz_space, true);

failed:
    return PURC_VARIANT_INVALID;
}

purc_variant_t
purc_make_object_from_query_string(const char *query, bool rfc1738)
{
    purc_variant_t obj = purc_variant_make_object_0();

    if (UNLIKELY(obj == PURC_VARIANT_INVALID))
        return obj;

    const char *left = query;
    while (*left) {
        const char *key = left;
        unsigned int key_len = get_key_len(left);
        if (key_len == 0)
            break;

        purc_variant_t vk = decode_percent_escaped(key, key_len, rfc1738);
        if (vk == PURC_VARIANT_INVALID)
            break;

        const char *value = left + key_len;
        if (*value != KV_SEPERATOR)
            break;

        value++;
        unsigned int value_len = get_value_len(value);
        purc_variant_t vv = decode_percent_escaped(value, value_len, rfc1738);
        if (vv == PURC_VARIANT_INVALID) {
            purc_variant_unref(vk);
            break;
        }

        bool success = purc_variant_object_set(obj, vk, vv);
        purc_variant_unref(vk);
        purc_variant_unref(vv);

        if (!success) {
            break;
        }

        left = value + value_len;
        if (*left == PAIR_SEPERATOR)
            left++;

        if (*left == FRAG_SEPERATOR)
            break;
    }

    return obj;
}

#if HAVE(STDATOMIC_H)

#include <stdatomic.h>

void purc_generate_unique_id(char* id_buff, const char* prefix)
{
    static atomic_ullong atomic_accumulator;
    struct timespec tp;
    int i, n = strlen(prefix);
    char my_prefix[9];

    for (i = 0; i < 8; i++) {
        if (i < n) {
            my_prefix[i] = purc_toupper(prefix [i]);
        }
        else
            my_prefix[i] = 'X';
    }
    my_prefix[8] = '\0';

    clock_gettime(CLOCK_REALTIME, &tp);

    unsigned long long accumulator =atomic_fetch_add(&atomic_accumulator, 1);
    snprintf(id_buff, PURC_LEN_UNIQUE_ID + 1,
            "%s-%016lX-%016lX-%016llX",
            my_prefix, tp.tv_sec, tp.tv_nsec, accumulator);
}

#else /* HAVE(STDATOMIC_H) */

/* see the atomic version at the end of this file */
void purc_generate_unique_id(char* id_buff, const char* prefix)
{
    static unsigned long long accumulator;
    struct timespec tp;
    int i, n = strlen(prefix);
    char my_prefix[9];

    for (i = 0; i < 8; i++) {
        if (i < n) {
            my_prefix[i] = purc_toupper(prefix [i]);
        }
        else
            my_prefix[i] = 'X';
    }
    my_prefix [8] = '\0';

    clock_gettime(CLOCK_REALTIME, &tp);
    snprintf(id_buff, PURC_LEN_UNIQUE_ID + 1,
            "%s-%016lX-%016lX-%016llX",
            my_prefix, tp.tv_sec, tp.tv_nsec, accumulator);
    accumulator++;
}

#endif  /* !HAVE(STDATOMIC_H) */

char *purc_load_file_contents(const char *file, size_t *length)
{
    FILE *f = fopen(file, "r");
    char *buf = NULL;

    if (f) {
        if (fseek(f, 0, SEEK_END))
            goto failed;

        long len = ftell(f);
        if (len < 0)
            goto failed;

        buf = malloc(len + 1);
        if (buf == NULL)
            goto failed;

        fseek(f, 0, SEEK_SET);
        if (fread(buf, 1, len, f) < (size_t)len) {
            free(buf);
            buf = NULL;
        }
        buf[len] = '\0';

        if (length)
            *length = (size_t)len;
failed:
        fclose(f);
    }

    return buf;
}

struct purc_page_ostack {
    const char *id;
    void *page;
    struct purc_page_owner *owners;
    size_t alloc_size;
    size_t nr_owners;
};

#define SZ_INITIAL_OSTACK   2

purc_page_ostack_t
purc_page_ostack_new(pcutils_kvlist_t page_map, const char *id, void *page)
{
    purc_page_ostack_t ostack;
    ostack = calloc(1, sizeof(*ostack));
    if (ostack) {
        ostack->owners = calloc(SZ_INITIAL_OSTACK, sizeof(ostack->owners[0]));
        if (ostack->owners == NULL)
            goto failed;

        ostack->page = page;
        ostack->alloc_size = SZ_INITIAL_OSTACK;
        ostack->nr_owners = 0;
        ostack->id = pcutils_kvlist_set_ex(page_map, id, &ostack);
    }

    return ostack;

failed:
    free(ostack);
    return NULL;
}

void
purc_page_ostack_delete(pcutils_kvlist_t page_map, purc_page_ostack_t ostack)
{
    pcutils_kvlist_remove(page_map, ostack->id);
    free(ostack->owners);
    free(ostack);
}

struct purc_page_owner
purc_page_ostack_register(purc_page_ostack_t ostack,
        struct purc_page_owner owner)
{
    static const struct purc_page_owner nil = { };

    for (size_t i = 0; i < ostack->nr_owners; i++) {
        if (owner.sess == ostack->owners[i].sess &&
                owner.corh == ostack->owners[i].corh) {
            return nil;
        }
    }

    if (ostack->alloc_size < ostack->nr_owners + 1) {
        size_t new_size;
        new_size = pcutils_get_next_fibonacci_number(ostack->alloc_size);
        ostack->owners = realloc(ostack->owners, sizeof(owner) * new_size);

        if (ostack->owners == NULL)
            goto failed;

        ostack->alloc_size = new_size;
    }

    ostack->owners[ostack->nr_owners] = owner;
    ostack->nr_owners++;
    if (ostack->nr_owners > 1)
        return ostack->owners[ostack->nr_owners - 2];
    return nil;

failed:
    purc_log_error("RDR/HEADLESS: Memory failure in %s\n", __func__);
    return nil;
}

struct purc_page_owner
purc_page_ostack_revoke(purc_page_ostack_t ostack, struct purc_page_owner owner)
{
    static const struct purc_page_owner nil = { };

    if (ostack->nr_owners == 0) {
        purc_log_warn("Empty page owner stack\n");
        return nil;
    }

    size_t i;
    for (i = 0; i < ostack->nr_owners; i++) {
        if (owner.sess == ostack->owners[i].sess &&
                owner.corh == ostack->owners[i].corh) {
            break;
        }
    }

    if (i == ostack->nr_owners) {
        purc_log_warn("Not registered page owner (%p/%llu)\n",
                owner.sess, (unsigned long long)owner.corh);
        return nil;
    }

    ostack->nr_owners--;
    if (i == ostack->nr_owners) {
        if (i == 0)
            return nil;

        return ostack->owners[i - 1];
    }

    for (; i < ostack->nr_owners; i++) {
        ostack->owners[i] = ostack->owners[i + 1];
    }

    return nil;
}

struct purc_page_owner
purc_page_ostack_revoke_session(purc_page_ostack_t ostack, void *sess)
{
    static const struct purc_page_owner nil = { };

    if (ostack->nr_owners == 0) {
        purc_log_warn("Empty page owner stack\n");
        return nil;
    }

    size_t revoked = 0;
    for (size_t i = 0; i < ostack->nr_owners; i++) {
        if (sess == ostack->owners[i].sess) {
            ostack->owners[i].corh = 0;
            revoked++;
        }
    }

    if (revoked == ostack->nr_owners) {
        ostack->nr_owners = 0;
        return nil;
    }
    else {
        assert(revoked < ostack->nr_owners);

        size_t n = 0;
        for (size_t i = 0; i < ostack->nr_owners; i++) {
            if (ostack->owners[i].corh) {
                ostack->owners[n] = ostack->owners[i];
                n++;
            }
        }
        ostack->nr_owners -= revoked;
    }

    return ostack->owners[ostack->nr_owners - 1];
}

const char *
purc_page_ostack_get_id(purc_page_ostack_t ostack)
{
    return ostack->id;
}

void *
purc_page_ostack_get_page(purc_page_ostack_t ostack)
{
    return ostack->page;
}

const char *purc_check_and_make_plainwin_id(char *id_buf, char *name_buf,
        const char *name_group)
{
    const char *group = strchr(name_group, PURC_SEP_GROUP_NAME);
    if (group && !purc_is_valid_identifier(group + 1))
        goto failed;

    size_t n;
    if (group) {
        n = group - name_group;
    }
    else {
        n = strlen(name_group);
    }

    if (n == 0 || n > PURC_LEN_IDENTIFIER)
        goto failed;

    strncpy(name_buf, name_group, n);
    name_buf[n] = 0;
    if (!purc_is_valid_identifier(name_buf))
        goto failed;

    strcpy(id_buf, PURC_PREFIX_PLAINWIN);
    strcat(id_buf, name_group);

    return group ? group + 1 : NULL;

failed:
    return PURC_INVPTR;
}

const char *purc_check_and_make_widget_id(char *id_buf, char *name_buf,
        const char *name_group)
{
    const char *group = strchr(name_group, PURC_SEP_GROUP_NAME);
    if (group == NULL || !purc_is_valid_identifier(group + 1))
        goto failed;

    size_t n = group - name_group;
    if (n == 0 || n > PURC_LEN_IDENTIFIER)
        goto failed;

    strncpy(name_buf, name_group, n);
    name_buf[n] = 0;
    if (!purc_is_valid_identifier(name_buf))
        goto failed;

    strcpy(id_buf, PURC_PREFIX_WIDGET);
    strcat(id_buf, name_group);

    return group + 1;

failed:
    return NULL;
}

