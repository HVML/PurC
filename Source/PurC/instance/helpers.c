/*
 * helpers.c -- The global helpers.
 *
 * Copyright (c) 2021, 2022 FMSoft (http://www.fmsoft.cn)
 *
 * Authors:
 *  Vincent Wei (https://github.com/VincentWei), 2021, 2022
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
#include "private/utils.h"
#include "private/debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

bool purc_is_valid_token (const char* token, int max_len)
{
    int i;

    if (!purc_isalpha (token [0]))
        return false;

    i = 1;
    while (token [i]) {

        if (max_len > 0 && i > max_len)
            return false;

        if (!purc_isalnum (token [i]) && token[i] != '_')
            return false;

        i++;
    }

    return true;
}

bool purc_is_valid_loose_token (const char* token, int max_len)
{
    int i;

    if (!purc_isalpha (token [0]))
        return false;

    i = 1;
    while (token [i]) {

        if (max_len > 0 && i > max_len)
            return false;

        if (!purc_isalnum (token [i]) && token[i] != '_' && token[i] != '-')
            return false;

        i++;
    }

    return true;
}

bool purc_is_valid_endpoint_name (const char* endpoint_name)
{
    char host_name [PURC_LEN_HOST_NAME + 1];
    char app_name [PURC_LEN_APP_NAME + 1];
    char runner_name [PURC_LEN_RUNNER_NAME + 1];

    if ( purc_extract_host_name (endpoint_name, host_name) <= 0)
        return false;

    if ( purc_extract_app_name (endpoint_name, app_name) <= 0)
        return false;

    if ( purc_extract_runner_name (endpoint_name, runner_name) <= 0)
        return false;

    return purc_is_valid_host_name (host_name) &&
        purc_is_valid_app_name (app_name) &&
        purc_is_valid_runner_name (runner_name);
}

/* @<host_name>/<app_name>/<runner_name> */
int purc_extract_host_name (const char* endpoint, char* host_name)
{
    int len;
    char* slash;

    if (endpoint [0] != '@' || (slash = strchr (endpoint, '/')) == NULL)
        return 0;

    endpoint++;
    len = (uintptr_t)slash - (uintptr_t)endpoint;
    if (len <= 0 || len > PURC_LEN_APP_NAME)
        return 0;

    strncpy (host_name, endpoint, len);
    host_name [len] = '\0';

    return len;
}

char* purc_extract_host_name_alloc (const char* endpoint)
{
    char* host_name;
    if ((host_name = malloc (PURC_LEN_HOST_NAME + 1)) == NULL)
        return NULL;

    if ( purc_extract_host_name (endpoint, host_name) > 0)
        return host_name;

    free (host_name);
    return NULL;
}

/* @<host_name>/<app_name>/<runner_name> */
int purc_extract_app_name (const char* endpoint, char* app_name)
{
    int len;
    char *first_slash, *second_slash;

    if (endpoint [0] != '@' || (first_slash = strchr (endpoint, '/')) == 0 ||
            (second_slash = strrchr (endpoint, '/')) == 0 ||
            first_slash == second_slash)
        return 0;

    first_slash++;
    len = (uintptr_t)second_slash - (uintptr_t)first_slash;
    if (len <= 0 || len > PURC_LEN_APP_NAME)
        return 0;

    strncpy (app_name, first_slash, len);
    app_name [len] = '\0';

    return len;
}

char* purc_extract_app_name_alloc (const char* endpoint)
{
    char* app_name;

    if ((app_name = malloc (PURC_LEN_APP_NAME + 1)) == NULL)
        return NULL;

    if ( purc_extract_app_name (endpoint, app_name) > 0)
        return app_name;

    free (app_name);
    return NULL;
}

int purc_extract_runner_name (const char* endpoint, char* runner_name)
{
    int len;
    char *second_slash;

    if (endpoint [0] != '@' ||
            (second_slash = strrchr (endpoint, '/')) == 0)
        return 0;

    second_slash++;
    len = strlen (second_slash);
    if (len > PURC_LEN_RUNNER_NAME)
        return 0;

    strcpy (runner_name, second_slash);

    return len;
}

char* purc_extract_runner_name_alloc (const char* endpoint)
{
    char* runner_name;

    if ((runner_name = malloc (PURC_LEN_RUNNER_NAME + 1)) == NULL)
        return NULL;

    if ( purc_extract_runner_name (endpoint, runner_name) > 0)
        return runner_name;

    free (runner_name);
    return NULL;
}

int purc_assemble_endpoint_name (const char* host_name, const char* app_name,
        const char* runner_name, char* buff)
{
    int host_len, app_len, runner_len;

    if ((host_len = strlen (host_name)) > PURC_LEN_HOST_NAME)
        return 0;

    if ((app_len = strlen (app_name)) > PURC_LEN_APP_NAME)
        return 0;

    if ((runner_len = strlen (runner_name)) > PURC_LEN_RUNNER_NAME)
        return 0;

    buff [0] = '@';
    buff [1] = '\0';
    strcat (buff, host_name);
    buff [host_len + 1] = '/';
    buff [host_len + 2] = '\0';

    strcat (buff, app_name);
    buff [host_len + app_len + 2] = '/';
    buff [host_len + app_len + 3] = '\0';

    strcat (buff, runner_name);

    return host_len + app_len + runner_len + 3;
}

char* purc_assemble_endpoint_name_alloc (const char* host_name,
        const char* app_name, const char* runner_name)
{
    char* endpoint;
    int host_len, app_len, runner_len;

    if ((host_len = strlen (host_name)) > PURC_LEN_HOST_NAME)
        return NULL;

    if ((app_len = strlen (app_name)) > PURC_LEN_APP_NAME)
        return NULL;

    if ((runner_len = strlen (runner_name)) > PURC_LEN_RUNNER_NAME)
        return NULL;

    if ((endpoint = malloc (host_len + app_len + runner_len + 4)) == NULL)
        return NULL;

    endpoint [0] = '@';
    endpoint [1] = '\0';
    strcat (endpoint, host_name);
    endpoint [host_len + 1] = '/';
    endpoint [host_len + 2] = '\0';

    strcat (endpoint, app_name);
    endpoint [host_len + app_len + 2] = '/';
    endpoint [host_len + app_len + 3] = '\0';

    strcat (endpoint, runner_name);

    return endpoint;
}

bool purc_is_valid_host_name (const char* host_name)
{
    // TODO
    (void)host_name;
    return true;
}

/* cn.fmsoft.hybridos.aaa */
bool purc_is_valid_app_name (const char* app_name)
{
    int len, max_len = PURC_LEN_APP_NAME;
    const char *start;
    char *end;

    start = app_name;
    while (*start) {
        char saved;
        end = strchr (start, '.');
        if (end == NULL) {
            saved = 0;
            end += strlen (start);
        }
        else {
            saved = '.';
            *end = 0;
        }

        if (end == start)
            return false;

        if ((len = purc_is_valid_token (start, max_len)) <= 0)
            return false;

        max_len -= len;
        if (saved) {
            start = end + 1;
            *end = saved;
            max_len--;
        }
        else {
            break;
        }
    }

    return true;
}

void purc_generate_unique_id (char* id_buff, const char* prefix)
{
    static unsigned long accumulator;
    struct timespec tp;
    int i, n = strlen (prefix);
    char my_prefix [9];

    for (i = 0; i < 8; i++) {
        if (i < n) {
            my_prefix [i] = purc_toupper (prefix [i]);
        }
        else
            my_prefix [i] = 'X';
    }
    my_prefix [8] = '\0';

    clock_gettime (CLOCK_REALTIME, &tp);
    snprintf (id_buff, PURC_LEN_UNIQUE_ID + 1,
            "%s-%016lX-%016lX-%016lX",
            my_prefix, tp.tv_sec, tp.tv_nsec, accumulator);
    accumulator++;
}

void purc_generate_md5_id (char* id_buff, const char* prefix)
{
    int n;
    char key [256];
    unsigned char md5_digest [MD5_DIGEST_SIZE];
    struct timespec tp;

    clock_gettime (CLOCK_REALTIME, &tp);
    n = snprintf (key, sizeof (key), "%s-%ld-%ld-%ld", prefix,
            tp.tv_sec, tp.tv_nsec, random ());

    if (n < 0) {
        PC_INFO ("Unexpected call to snprintf.\n");
    }
    else if ((size_t)n >= sizeof (key))
        PC_INFO ("The buffer is too small for resultId.\n");

    pcutils_md5digest (key, md5_digest);
    pcutils_bin2hex (md5_digest, MD5_DIGEST_SIZE, id_buff);
}

bool purc_is_valid_unique_id (const char* id)
{
    int n = 0;

    while (id [n]) {
        if (n > PURC_LEN_UNIQUE_ID)
            return false;

        if (!purc_isalnum (id [n]) && id [n] != '-')
            return false;

        n++;
    }

    return true;
}

bool purc_is_valid_md5_id (const char* id)
{
    int n = 0;

    while (id [n]) {
        if (n > (MD5_DIGEST_SIZE << 1))
            return false;

        if (!purc_isalnum (id [n]))
            return false;

        n++;
    }

    return true;
}

double purc_get_elapsed_seconds (const struct timespec *ts1,
        const struct timespec *ts2)
{
    struct timespec ts_curr;
    time_t ds;
    long dns;

    if (ts2 == NULL) {
        clock_gettime (CLOCK_MONOTONIC, &ts_curr);
        ts2 = &ts_curr;
    }

    ds = ts2->tv_sec - ts1->tv_sec;
    dns = ts2->tv_nsec - ts1->tv_nsec;
    return ds + dns * 1.0E-9;
}

