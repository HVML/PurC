/*
 * @file fetcher-local.cpp
 * @author XueShuming
 * @date 2021/11/16
 * @brief The impl for fetcher local.
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

#include "fetcher-internal.h"

#include <wtf/URL.h>
#include <wtf/RunLoop.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdlib.h>

struct pcfetcher_local {
    struct pcfetcher base;
};

struct mime_type {
    const char* ext;
    const char* mime;
};

struct mime_type  mime_types[] = {
    { "",       "unknown" },
    { ".hvml",   "text/hvml" },
    { ".html",   "text/html" },
    { ".json",   "application/json" },
    { ".xml",    "application/xml" },
    { ".tar",    "application/x-tar" },
    { ".sh",     "application/x-sh" },
    { ".txt",    "text/plain" },
    { ".jpg",    "image/jpeg" },
    { ".jpeg",   "image/jpeg" },
    { ".png",    "image/png" },
    { ".mp3",    "audio/mpeg" },
    { ".mp4",    "video/mp4" },
};

static const char* get_mime(const char* name)
{
    const char* ext = strrchr(name, '.');
    if (!ext) {
        return mime_types[0].mime;
    }

    size_t sz = sizeof(mime_types) / sizeof(struct mime_type);
    for (size_t i = 1; i < sz; i++) {
        if (strcmp(ext, mime_types[i].ext) == 0) {
            return mime_types[i].mime;
        }
    }
    return mime_types[0].mime;
}

struct pcfetcher* pcfetcher_local_init(size_t max_conns, size_t cache_quota)
{
    struct pcfetcher_local* local = (struct pcfetcher_local*)malloc(
            sizeof(struct pcfetcher_local));
    if (local == NULL) {
        return NULL;
    }

    struct pcfetcher* fetcher = (struct pcfetcher*) local;
    fetcher->max_conns = max_conns;
    fetcher->cache_quota = cache_quota;
    fetcher->init = pcfetcher_local_init;
    fetcher->term = pcfetcher_local_term;
    fetcher->request_async = pcfetcher_local_request_async;
    fetcher->request_sync = pcfetcher_local_request_sync;
    fetcher->cancel_async = pcfetcher_local_cancel_async;
    fetcher->check_response = pcfetcher_local_check_response;

    return fetcher;
}

int pcfetcher_local_term(struct pcfetcher* fetcher)
{
    if (!fetcher) {
        return 0;
    }

    struct pcfetcher_local* local = (struct pcfetcher_local*)fetcher;
    free(local);
    return 0;
}

purc_variant_t pcfetcher_local_request_async(
        struct pcfetcher_session *session,
        struct pcfetcher* fetcher,
        const char* url,
        enum pcfetcher_method method,
        purc_variant_t params,
        uint32_t timeout,
        pcfetcher_response_handler handler,
        void* ctxt,
        pcfetcher_progress_tracker tracker,
        void* tracker_ctxt)
{
    UNUSED_PARAM(fetcher);
    UNUSED_PARAM(url);
    UNUSED_PARAM(method);
    UNUSED_PARAM(params);
    UNUSED_PARAM(timeout);
    UNUSED_PARAM(handler);
    UNUSED_PARAM(ctxt);

    if (!fetcher || !url || !handler) {
        return PURC_VARIANT_INVALID;
    }

    struct pcfetcher_callback_info *info = pcfetcher_create_callback_info();
    info->rws = pcfetcher_local_request_sync(session, fetcher, url, method,
            params, timeout, &info->header);
    info->handler = handler;
    info->session = session;
    info->ctxt = ctxt;
    info->tracker = tracker;
    info->tracker_ctxt = tracker_ctxt;
    info->req_id = purc_variant_make_native(info, NULL);

    if (!info->rws) {
        info->header.ret_code = 404;
    }

    RunLoop *runloop = &RunLoop::current();
    if (info->tracker) {
#ifdef NDEBUG
        runloop->dispatch([info] {
#else
        double tm = 0.1;
        runloop->dispatchAfter(Seconds(tm), [info] {
#endif
                info->tracker(info->session, info->req_id, info->tracker_ctxt,
                        PCFETCHER_INITIAL_PROGRESS);
            }
        );
    }

#ifdef NDEBUG
    runloop->dispatch([info] {
#else
    double tm = 0.2;
    runloop->dispatchAfter(Seconds(tm), [info] {
#endif
                if (info->tracker) {
                    info->tracker(info->session, info->req_id,
                            info->tracker_ctxt, 1.0);
                }
                if (!info->cancelled) {
                    info->handler(info->session, info->req_id, info->ctxt,
                            PCFETCHER_RESP_TYPE_HEADER,
                            (const char *)&info->header, 0);

                    purc_rwstream_t stream = purc_rwstream_new_buffer(1024, 0);
                    purc_rwstream_dump_to_another(info->rws, stream, -1);
                    size_t sz_content = 0;
                    char *content = (char *)purc_rwstream_get_mem_buffer(stream,
                            &sz_content);

                    info->handler(info->session, info->req_id, info->ctxt,
                            PCFETCHER_RESP_TYPE_DATA,
                            content, sz_content);

                    info->handler(info->session, info->req_id, info->ctxt,
                            PCFETCHER_RESP_TYPE_FINISH,
                            NULL, 0);

                    purc_rwstream_destroy(stream);
                    purc_rwstream_destroy(info->rws);
                    info->rws = NULL;
                }
                pcfetcher_destroy_callback_info(info);
            });

    return info->req_id;
}

off_t filesize(const char* filename)
{
    struct stat statbuf;
    stat(filename,&statbuf);
    return statbuf.st_size;
}

String pcfetcher_build_uri(const char *base_url,  const char *url);

purc_rwstream_t pcfetcher_local_request_sync(
        struct pcfetcher_session *session,
        struct pcfetcher* fetcher,
        const char* url,
        enum pcfetcher_method method,
        purc_variant_t params,
        uint32_t timeout,
        struct pcfetcher_resp_header *resp_header)
{
    UNUSED_PARAM(fetcher);
    UNUSED_PARAM(url);
    UNUSED_PARAM(method);
    UNUSED_PARAM(params);
    UNUSED_PARAM(timeout);
    UNUSED_PARAM(resp_header);

    if (!fetcher || !url) {
        return NULL;
    }

    String uri;
    if (session->base_url) {
        uri = pcfetcher_build_uri(session->base_url, url);
    }
    else {
        uri.append(url);
    }

    PurCWTF::URL wurl(URL(), uri);
    if (!wurl.isLocalFile()) {
        resp_header->ret_code = 404;
        resp_header->sz_resp = 0;
        resp_header->mime_type = NULL;
        return NULL;
    }

    const StringView path = wurl.path();
    const CString& cpath = path.utf8();

    const char* file = cpath.data();

    purc_rwstream_t rws = purc_rwstream_new_from_file(file, "r");
    if (rws && resp_header) {
        resp_header->ret_code = 200;
        resp_header->sz_resp = filesize(file);
        resp_header->mime_type = strdup(get_mime(file));
        return rws;
    }

    return NULL;
}

void pcfetcher_local_cancel_async(struct pcfetcher* fetcher,
        purc_variant_t request)
{
    UNUSED_PARAM(fetcher);
    UNUSED_PARAM(request);

#ifndef NDEBUG
    struct pcfetcher_callback_info *info = (struct pcfetcher_callback_info *)
        purc_variant_native_get_entity(request);
    info->cancelled = true;
    info->header.ret_code = RESP_CODE_USER_CANCEL;
    info->handler(info->session, info->req_id, info->ctxt,
            PCFETCHER_RESP_TYPE_ERROR,
            (const char *)&info->header, 0);
#endif
}

int pcfetcher_local_check_response(struct pcfetcher* fetcher,
        uint32_t timeout_ms)
{
    UNUSED_PARAM(fetcher);
    UNUSED_PARAM(timeout_ms);
    return 0;
}


