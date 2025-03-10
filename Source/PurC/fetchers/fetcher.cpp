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

#include "private/fetcher.h"
#include "private/instance.h"

#include "fetcher-internal.h"

#include <wtf/Lock.h>
#include <wtf/URL.h>

#include <unistd.h>

#define FETCHER_MAX_CONNS        100
#define FETCHER_CACHE_QUOTA      10240

static struct pcfetcher* s_remote_fetcher = NULL;
static struct pcfetcher* s_local_fetcher = NULL;

static struct pcfetcher* get_fetcher(void)
{
    struct pcinst *inst = pcinst_current();
    if (inst->enable_remote_fetcher && s_remote_fetcher) {
        return s_remote_fetcher;
    }
    return s_local_fetcher;
}

bool pcfetcher_is_init(void)
{
    return s_remote_fetcher || s_local_fetcher;
}

static struct pcfetcher_cookie *cookie_create(
        const char *domain, const char *path, const char *name,
        const char *content, time_t expire_time, bool secure)
{
    struct pcfetcher_cookie *cookie =
        (struct pcfetcher_cookie*) calloc(1, sizeof(*cookie));
    cookie->domain = domain ? strdup(domain) : NULL;
    cookie->path = path ? strdup(path) : NULL;
    cookie->name = name ? strdup(name) : NULL;
    cookie->content = content ? strdup(content) : NULL;
    cookie->expire_time = expire_time;
    cookie->secure = secure;
    return cookie;
}

static void cookie_destroy(struct pcfetcher_cookie *cookie)
{
    if (cookie->domain) {
        free(cookie->domain);
    }
    if (cookie->path) {
        free(cookie->path);
    }
    if (cookie->name) {
        free(cookie->name);
    }
    if (cookie->content) {
        free(cookie->content);
    }
    free(cookie);
}

struct pcfetcher_session *pcfetcher_session_create(void *user_data)
{
    struct pcfetcher_session *session =
        (struct pcfetcher_session*) calloc(1, sizeof (*session));
    session->user_data = user_data;
    list_head_init(&session->cookies);
    return session;
}

void pcfetcher_session_destroy(struct pcfetcher_session *session)
{
    if (session->base_url) {
        free(session->base_url);
    }

    struct list_head *cookies = &session->cookies;
    struct pcfetcher_cookie *p;
    struct pcfetcher_cookie *n;
    list_for_each_entry_safe(p, n, cookies, node) {
        cookie_destroy(p);
    }
    free(session);
}

int pcfetcher_session_set_user_data(struct pcfetcher_session *session,
    void *user_data)
{
    if (session) {
        session->user_data = user_data;
        return 0;
    }
    return -1;
}

void *pcfetcher_session_get_user_data(struct pcfetcher_session *session)
{
    return session ? session->user_data : NULL;
}

int pcfetcher_session_set_base_url(struct pcfetcher_session *session,
    const char *base_url)
{
    if (!session) {
        return -1;
    }

    if (!base_url) {
        if (session->base_url) {
            free(session->base_url);
            session->base_url = NULL;
        }
        goto out;
    }

    if (session->base_url) {
        if (strcmp(base_url, session->base_url) != 0) {
            free(session->base_url);
            session->base_url = strdup(base_url);
        }
    }
    else {
        session->base_url = strdup(base_url);
    }

out:
    return 0;
}

const char *pcfetcher_session_get_base_url(struct pcfetcher_session *session)
{
    return session ? session->base_url : NULL;
}

static bool cookie_match(struct pcfetcher_cookie *cookie,
        const char *domain, const char *path, const char *name)
{
    if (!(cookie->domain && domain && strcmp(cookie->domain, domain) == 0)) {
        return false;
    }

    if (!(cookie->path && path && strcmp(cookie->path, path) == 0)) {
        return false;
    }

    if (!(cookie->name && name && strcmp(cookie->name, name) == 0)) {
        return false;
    }
    return true;
}

struct pcfetcher_cookie *find_cookie(struct pcfetcher_session *session,
        const char *domain, const char *path, const char *name)
{
    struct pcfetcher_cookie *result = NULL;
    struct list_head *cookies = &session->cookies;
    struct pcfetcher_cookie *p;
    struct pcfetcher_cookie *n;
    list_for_each_entry_safe(p, n, cookies, node) {
        if (cookie_match(p, domain, path, name)) {
            result = p;
            break;
        }
    }
    return result;
}

int pcfetcher_cookie_set(struct pcfetcher_session *session,
        const char *domain, const char *path, const char *name,
        const char *content, time_t expire_time, bool secure)
{
    int ret = -1;
    struct pcfetcher_cookie *cookie;

    if (!domain || !path || !name || !content) {
        goto out;
    }

    cookie = find_cookie(session, domain, path, name);
    if (cookie) {
        if (strcmp(cookie->content, content) != 0) {
            free(cookie->content);
            cookie->content = strdup(content);
        }
        cookie->expire_time = expire_time;
        cookie->secure = secure;
    }
    else {
        cookie = cookie_create(domain, path, name, content, expire_time, secure);
        if (!cookie) {
            goto out;
        }

        list_add_tail(&cookie->node, &session->cookies);
    }
    ret = 0;

out:
    return ret;
}

const char *pcfetcher_cookie_get(struct pcfetcher_session *session,
        const char *domain, const char *path, const char *name,
        time_t *expire, bool *secure)
{
    struct pcfetcher_cookie *cookie = NULL;
    const char *content = NULL;
    if (!domain || !path || !name) {
        goto out;
    }
    cookie = find_cookie(session, domain, path, name);
    if (!cookie) {
        goto out;
    }

    content = cookie->content;
    if (expire) {
        *expire = cookie->expire_time;
    }

    if (secure) {
        *secure = cookie->secure;
    }

out:
    return content;
}

int pcfetcher_cookie_remove(struct pcfetcher_session *session,
        const char *domain, const char *path, const char *name)
{
    struct list_head *cookies = &session->cookies;
    struct pcfetcher_cookie *p;
    struct pcfetcher_cookie *n;
    int ret = -1;
    if (!domain || !path || !name) {
        goto out;
    }

    list_for_each_entry_safe(p, n, cookies, node) {
        if (cookie_match(p, domain, path, name)) {
            list_del(&p->node);
            cookie_destroy(p);
            break;
        }
    }

    ret = 0;
out:
    return ret;
}

purc_variant_t pcfetcher_request_async(
        struct pcfetcher_session *session,
        const char* url,
        enum pcfetcher_method method,
        purc_variant_t params,
        uint32_t timeout,
        pcfetcher_response_handler handler,
        void* ctxt,
        pcfetcher_progress_tracker tracker,
        void* tracker_ctxt)
{
    struct pcfetcher* fetcher = get_fetcher();
    return fetcher ? fetcher->request_async(session, fetcher, url, method,
            params, timeout, handler, ctxt, tracker, tracker_ctxt) : PURC_VARIANT_INVALID;
}

purc_rwstream_t pcfetcher_request_sync(
        struct pcfetcher_session *session,
        const char* url,
        enum pcfetcher_method method,
        purc_variant_t params,
        uint32_t timeout,
        struct pcfetcher_resp_header *resp_header)
{
    struct pcfetcher* fetcher = get_fetcher();
    return fetcher ? fetcher->request_sync(session, fetcher, url, method,
            params, timeout, resp_header) : NULL;
}


int pcfetcher_check_response(uint32_t timeout_ms)
{
    struct pcfetcher* fetcher = get_fetcher();
    return fetcher ? fetcher->check_response(fetcher,
            timeout_ms) : 0;
}

void pcfetcher_cancel_async(purc_variant_t request)
{
    struct pcfetcher* fetcher = get_fetcher();
    if (fetcher) {
        fetcher->cancel_async(fetcher, request);
    }
}

struct pcfetcher_callback_info *pcfetcher_create_callback_info()
{
    return (struct pcfetcher_callback_info*) calloc(1,
            sizeof(struct pcfetcher_callback_info));
}

void pcfetcher_destroy_callback_info(struct pcfetcher_callback_info *info)
{
    if (!info) {
        return;
    }
    if (info->header.mime_type) {
        free(info->header.mime_type);
    }
    if (info->rws) {
        purc_rwstream_destroy(info->rws);
    }
    free(info);
}

String pcfetcher_build_uri(const char *base_url,  const char *url)
{
    PurCWTF::URL u(URL(), url);
    if (u.isValid()) {
        return url;
    }

    size_t nr = strlen(url);
    char buf[PATH_MAX + nr + 2];

    PurCWTF::URL uri(URL(), base_url);
    if (uri.isLocalFile() && uri.host().isEmpty() && (uri.path() == "/") &&
            u.protocol().isEmpty() && url[0] != '/') {
        if (getcwd(buf, sizeof(buf)) != NULL) {
            strcat(buf, "/");
            strcat(buf, url);
            url = buf;
        }
    }

    String result;
    if (nr > 1 && url[0] == '/' && url[1] == '/') {
        result.append(uri.protocol().toString());
        result.append(":");
        result.append(url);
    }
    else if (url[0] == '/') {
        uri.setPath(url);
        result = uri.string();
    }
    else {
        result.append(base_url);
        size_t nr = strlen(base_url);
        if (base_url[nr - 1] != '/') {
            result.append('/');
        }
        result.append(url);
    }

    return result;
}

static void _local_cleanup_once(void)
{
    if (s_local_fetcher) {
        s_local_fetcher->term(s_local_fetcher);
        s_local_fetcher = NULL;
    }
}

static int _local_init_once(void)
{
    if (!s_local_fetcher) {
        s_local_fetcher = pcfetcher_local_init(FETCHER_MAX_CONNS,
                FETCHER_CACHE_QUOTA);

        atexit(_local_cleanup_once);
    }
    return 0;
}

static int _local_init_instance(struct pcinst* curr_inst,
        const purc_instance_extra_info* extra_info)
{
    UNUSED_PARAM(curr_inst);
    UNUSED_PARAM(extra_info);
    return 0;
}

static void _local_cleanup_instance(struct pcinst* curr_inst)
{
    UNUSED_PARAM(curr_inst);
}

struct pcmodule _module_fetcher_local = {
    .id              = PURC_HAVE_FETCHER,
    .module_inited   = 0,

    .init_once              = _local_init_once,
    .init_instance          = _local_init_instance,
    .cleanup_instance       = _local_cleanup_instance,
};

#if ENABLE(REMOTE_FETCHER)                /* { */
static void _remote_cleanup_once(void)
{
    if (s_remote_fetcher) {
        s_remote_fetcher->term(s_remote_fetcher);
        s_remote_fetcher = NULL;
    }
}
#endif                                    /* } */

static int _remote_init_once(void)
{
#if ENABLE(REMOTE_FETCHER)                /* { */
    if (!s_remote_fetcher) {
        s_remote_fetcher = pcfetcher_remote_init(FETCHER_MAX_CONNS,
                FETCHER_CACHE_QUOTA);
        if (!s_remote_fetcher) {
            return PURC_ERROR_OUT_OF_MEMORY;
        }

        atexit(_remote_cleanup_once);
    }
#endif
    return 0;
}

static int _remote_init_instance(struct pcinst* curr_inst,
        const purc_instance_extra_info* extra_info)
{
    UNUSED_PARAM(curr_inst);
    UNUSED_PARAM(extra_info);
    return 0;
}

static void _remote_cleanup_instance(struct pcinst* curr_inst)
{
    UNUSED_PARAM(curr_inst);
}

struct pcmodule _module_fetcher_remote = {
    .id              = PURC_HAVE_FETCHER_R,
    .module_inited   = 0,

    .init_once              = _remote_init_once,
    .init_instance          = _remote_init_instance,
    .cleanup_instance       = _remote_cleanup_instance,
};

