/*
 * @file fetcher.c
 * @author XueShuming
 * @date 2021/11/16
 * @brief The impl for fetcher api.
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

#include "config.h"

#include "fetcher.h"
#include "fetcher-internal.h"

static struct pcfetcher* s_fetcher;

int pcfetcher_init(size_t max_conns, size_t cache_quota)
{
    if (s_fetcher) {
        return 0;
    }

#if ENABLE(LINK_PURC_FETCHER)
    s_fetcher = pcfetcher_remote_init(max_conns, cache_quota);
#else
    s_fetcher = pcfetcher_local_init(max_conns, cache_quota);
#endif

    return 0;
}

int pcfetcher_term(void)
{
    if (!s_fetcher) {
        return 0;
    }

    int ret = s_fetcher->term(s_fetcher);
    s_fetcher = NULL;
    return ret;
}

const char* pcfetcher_set_base_url(const char* base_url)
{
    return s_fetcher ? s_fetcher->set_base_url(s_fetcher, base_url) : NULL;
}

void pcfetcher_cookie_set(const char* domain,
        const char* path, const char* name, const char* content,
        time_t expire_time, bool secure)
{
    if (s_fetcher) {
        s_fetcher->cookie_set(s_fetcher, domain, path, name, content,
                expire_time, secure);
    }
}

const char* pcfetcher_cookie_get(const char* domain,
        const char* path, const char* name, time_t *expire, bool *secure)
{
    return s_fetcher ? s_fetcher->cookie_get(s_fetcher, domain, path,
            name, expire, secure) : NULL;
}

const char* pcfetcher_cookie_remove(const char* domain,
        const char* path, const char* name)
{
    if (s_fetcher) {
        return s_fetcher->cookie_remove(s_fetcher, domain, path, name);
    }
    return NULL;
}

purc_variant_t pcfetcher_request_async(
        const char* url,
        enum pcfetcher_request_method method,
        purc_variant_t params,
        uint32_t timeout,
        response_handler handler,
        void* ctxt)
{
    return s_fetcher ? s_fetcher->request_async(s_fetcher, url, method,
            params, timeout, handler, ctxt) : PURC_VARIANT_INVALID;
}

purc_rwstream_t pcfetcher_request_sync(
        const char* url,
        enum pcfetcher_request_method method,
        purc_variant_t params,
        uint32_t timeout,
        struct pcfetcher_resp_header *resp_header)
{
    return s_fetcher ? s_fetcher->request_sync(s_fetcher, url, method,
            params, timeout, resp_header) : NULL;
}


int pcfetcher_check_response(uint32_t timeout_ms)
{
    return s_fetcher ? s_fetcher->check_response(s_fetcher,
            timeout_ms) : 0;
}



