/**
 * @file purc-fetcher.h
 * @author Xue Shuming
 * @date 2024/03/11
 * @brief The API for fetcher.
 *
 * Copyright (C) 2024 FMSoft <https://www.fmsoft.cn>
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


#ifndef PURC_PURC_FETCHER_H
#define PURC_PURC_FETCHER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <time.h>

#include "purc-macros.h"
#include "purc-variant.h"

enum pcfetcher_method {
    PCFETCHER_METHOD_GET = 0,
    PCFETCHER_METHOD_POST,
    PCFETCHER_METHOD_DELETE,
};

struct pcfetcher_resp_header {
    int ret_code;
    char* mime_type;
    size_t sz_resp;
};

PCA_EXTERN_C_BEGIN

struct pcfetcher_session;

/**
 * pcfetcher_init:
 *
 * @max_conns: The maximum number of connections.
 * @cache_quota: The cache limit.
 *
 * Init data fetcher of the current PurC instance.
 *
 * Returns: 0 for success, otherwise failure.
 *
 * Since: 0.9.20
 */
int pcfetcher_init(size_t max_conns, size_t cache_quota);

/**
 * pcfetcher_term:
 *
 * Terminate the data fetcher of the current PurC instance.
 *
 * Returns: 0 for success, otherwise failure.
 *
 * Since: 0.9.20
 */
int pcfetcher_term(void);

/**
 * pcfetcher_get_conn_fd:
 *
 * Get the fd of the current connection.
 *
 * Returns: The fd fo current connection, or -1 on failure.
 *
 * Since: 0.9.20
 */
int pcfetcher_get_conn_fd(void);

/**
 * pcfetcher_session_create:
 *
 * @user_data: The user data attach on the session.
 *
 * Create a fetcher session.
 *
 * Returns: A fetcher session, or NULL on failure.
 *
 * Since: 0.9.20
 */
struct pcfetcher_session *pcfetcher_session_create(void *user_data);

/**
 * pcfetcher_session_destroy:
 *
 * @session: The session to destroy.
 *
 * destroy the fetcher session.
 *
 * Since: 0.9.20
 */
void pcfetcher_session_destroy(struct pcfetcher_session *session);

/**
 * pcfetcher_session_set_user_data:
 *
 * @session: The fetcher session.
 * @user_data: The user data to set.
 *
 * Set the user data of the session.
 *
 * Returns: 0 for success, otherwise failure.
 *
 * Since: 0.9.20
 */
int pcfetcher_session_set_user_data(struct pcfetcher_session *session,
        void *user_data);

/**
 * pcfetcher_session_get_user_data:
 *
 * @session: The fetcher session.
 *
 * Get the user data of the session.
 *
 * Returns: The pointer to the user data , or NULL if not set.
 *
 * Since: 0.9.20
 */
void *pcfetcher_session_get_user_data(struct pcfetcher_session *session);

/**
 * pcfetcher_session_set_base_url:
 *
 * @session: The fetcher session.
 * @base_url: The base url to set.
 *
 * Set the base url of the session.
 *
 * Returns: 0 for success, otherwise failure.
 *
 * Since: 0.9.20
 */
int pcfetcher_session_set_base_url(struct pcfetcher_session *session,
    const char *base_url);

/**
 * pcfetcher_session_get_base_url:
 *
 * @session: The fetcher session.
 *
 * Get the base url of the session.
 *
 * Returns: The base url, or NULL if not set.
 *
 * Since: 0.9.20
 */
const char *pcfetcher_session_get_base_url(struct pcfetcher_session *session);

/**
 * pcfetcher_cookie_set:
 *
 * @session: The fetcher session.
 * @domain: The domain of the cookie.
 * @path: The path of the cookie.
 * @name: The name of the cookie.
 * @content: The content of the cookie.
 * @expire_time: The expire time of the cookie.
 * @secure: Secure or not.
 *
 * Set cookie of the session.
 *
 * Returns: 0 for success, otherwise failure.
 *
 * Since: 0.9.20
 */
int pcfetcher_cookie_set(struct pcfetcher_session *session,
        const char *domain, const char *path, const char *name,
        const char *content, time_t expire_time, bool secure);

/**
 * pcfetcher_cookie_get:
 *
 * @session: The fetcher session.
 * @domain: The domain of the cookie.
 * @path: The path of the cookie.
 * @name: The name of cookie to get.
 * @expire: Return the expire time of the cookie.
 * @secure: Return if secure or not.
 *
 * Get cookie of the session.
 *
 * Returns: the cookie content, or NULL if not set.
 *
 * Since: 0.9.20
 */
const char *pcfetcher_cookie_get(struct pcfetcher_session *session,
        const char *domain, const char *path, const char *name,
        time_t *expire, bool *secure);

/**
 * pcfetcher_cookie_remove:
 *
 * @session: The fetcher session.
 * @domain: The domain of the cookie.
 * @path: The path of the cookie.
 * @name: The name of cookie to remove.
 *
 * Remove the cookie.
 *
 * Returns: True on succes, or false on failure.
 *
 * Since: 0.9.20
 */
int pcfetcher_cookie_remove(struct pcfetcher_session *session,
        const char *domain, const char *path, const char *name);

enum pcfetcher_resp_type {
    PCFETCHER_RESP_TYPE_HEADER,
    PCFETCHER_RESP_TYPE_DATA,
    PCFETCHER_RESP_TYPE_ERROR,
    PCFETCHER_RESP_TYPE_FINISH
};

typedef void (*pcfetcher_response_handler)(
        struct pcfetcher_session *session,
        purc_variant_t request_id,
        void *ctxt,
        enum pcfetcher_resp_type type,
        const char *data, size_t sz_data);

typedef void (*pcfetcher_progress_tracker)(
        struct pcfetcher_session *session,
        purc_variant_t request_id,
        void *ctxt, double progress);

/**
 * pcfetcher_request_async:
 *
 * @session: The fetcher session.
 * @url: The request url.
 * @methd: The request method.
 * @params: The requst params.
 * @timeout: The timeout of request.
 * @handler: The request callback.
 * @ctxt: The pointer to the context data which will be passed to the callback.
 *
 * Send an asynchronous request.
 *
 * Returns: The request id of the request, or PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.9.20
 */
purc_variant_t pcfetcher_request_async(
        struct pcfetcher_session *session,
        const char *url,
        enum pcfetcher_method method,
        purc_variant_t params,
        unsigned int timeout,
        pcfetcher_response_handler handler,
        void *ctxt,
        pcfetcher_progress_tracker tracker,
        void* tracker_ctxt);

/**
 * pcfetcher_request_sync:
 *
 * @session: The fetcher session.
 * @url: The request url.
 * @methd: The request method.
 * @params: The requst params.
 * @resp_header: The header of response.
 *
 * Send an synchronization request.
 *
 * Returns: The rwstream of the response on success, or NULL on failure.
 *
 * Since: 0.9.20
 */
purc_rwstream_t pcfetcher_request_sync(
        struct pcfetcher_session *session,
        const char* url,
        enum pcfetcher_method method,
        purc_variant_t params,
        unsigned int timeout,
        struct pcfetcher_resp_header *resp_header);


/**
 * pcfetcher_check_response:
 *
 * Check the network response. Used to poll network task status,
 * if there is response data, the callback function will be called.
 *
 * Returns: 0 for success, otherwise failure.
 *
 * Since: 0.9.20
 */
int pcfetcher_check_response(void);

PCA_EXTERN_C_END

#endif /* not defined PURC_PURC_FETCHER_H */
