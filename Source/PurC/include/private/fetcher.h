/*
 * @file fetcher.h
 * @author XueShuming
 * @date 2021/11/16
 * @brief The interfaces for fetcher.
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

#ifndef PURC_FETCHER_H
#define PURC_FETCHER_H

#include "purc.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <time.h>

enum pcfetcher_request_method {
    PCFETCHER_REQUEST_METHOD_GET = 0,
    PCFETCHER_REQUEST_METHOD_POST,
    PCFETCHER_REQUEST_METHOD_DELETE,
};


struct pcfetcher_resp_header {
    int ret_code;
    char* mime_type;
    size_t sz_resp;
};

typedef void (*response_handler)(
        purc_variant_t request_id, void* ctxt,
        const struct pcfetcher_resp_header *resp_header,
        purc_rwstream_t resp);


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

int pcfetcher_init(size_t max_conns, size_t cache_quota,
        bool enable_remote_fetcher);

int pcfetcher_term(void);

const char* pcfetcher_set_base_url(const char* base_url);

void pcfetcher_cookie_set(const char* domain,
        const char* path, const char* name, const char* content,
        time_t expire_time, bool secure);

const char* pcfetcher_cookie_get(const char* domain,
        const char* path, const char* name, time_t *expire, bool *secure);

const char* pcfetcher_cookie_remove(const char* domain,
        const char* path, const char* name);


purc_variant_t pcfetcher_request_async(
        const char* url,
        enum pcfetcher_request_method method,
        purc_variant_t params,
        uint32_t timeout,
        response_handler handler,
        void* ctxt);

purc_rwstream_t pcfetcher_request_sync(
        const char* url,
        enum pcfetcher_request_method method,
        purc_variant_t params,
        uint32_t timeout,
        struct pcfetcher_resp_header *resp_header);

int pcfetcher_check_response(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_FETCHER_H */


