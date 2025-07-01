/*
 * openssl-shared-context.c
 *
 * Copyright (C) 2011 EXCELIANCE
 * Copyright (C) 2025 FMSoft
 *
 * Authors:
 *  - Emeric Brun - emeric@exceliance.fr
 *  - Vincent Wei - <https://github.com/VincentWei>
 *
 * Copyright 2015-2016 Varnish Software
 * Copyright 2012 Bump Technologies, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in  the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY BUMP TECHNOLOGIES, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL BUMP TECHNOLOGIES, INC. OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of Bump Technologies, Inc.
 */

#include "private/openssl-shared-context.h"
#include "private/debug.h"
#include "ebtree/ebmbtree.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>

#if OS(LINUX) && HAVE(STDATOMIC_H)
#   include <stdatomic.h>
#   define USE_SYSCALL_FUTEX   1
#endif

#ifdef USE_SYSCALL_FUTEX
#  include <unistd.h>
#  include <linux/futex.h>
#  include <sys/syscall.h>
#else
#  include <pthread.h>
#endif

#define SHSESS_NAME_PATTERN "/hvml-openssl-shsess-%s"

#ifndef SHSESS_MAX_FOOTER_LEN
#  define SHSESS_MAX_FOOTER_LEN sizeof(uint32_t) + EVP_MAX_MD_SIZE
#endif

#ifndef SHSESS_MAX_DATA_LEN
#  define SHSESS_MAX_DATA_LEN 512
#endif

#define SHSESS_MAX_ENCODED_LEN \
    SSL_MAX_SSL_SESSION_ID_LENGTH + \
    SHSESS_MAX_DATA_LEN + \
    SHSESS_MAX_FOOTER_LEN

#define AN(foo)        do { (void)foo; } while (0)

struct shared_session {
    struct ebmb_node    key;
    unsigned char       key_data[SSL_MAX_SSL_SESSION_ID_LENGTH];
    time_t              c_date;
    int                 data_len;
    unsigned char       data[SHSESS_MAX_DATA_LEN];
    struct shared_session    *p;
    struct shared_session    *n;
};

struct _openssl_shared_context {
#ifdef USE_SYSCALL_FUTEX
    atomic_uint             waiters;
#else
    pthread_mutex_t         mutex;
#endif
    struct shared_session   active;
    struct shared_session   free;
};

/* Lock functions */
#ifdef USE_SYSCALL_FUTEX
static inline unsigned
xchg(atomic_uint *ptr, unsigned x)
{
    return atomic_exchange(ptr, x);
}

static inline unsigned
cmpxchg(atomic_uint *ptr, unsigned old, unsigned new)
{
    atomic_compare_exchange_strong(ptr, &old, new);
    return old;
}

static inline unsigned
atomic_dec(atomic_uint *ptr)
{
    return atomic_fetch_sub(ptr, 1);
}

static inline void
shared_context_lock(struct openssl_shctx_wrapper* wrapper)
{
    unsigned x;

    x = cmpxchg(&wrapper->shctx->waiters, 0, 1);
    if (x) {
        if (x != 2)
            x = xchg(&wrapper->shctx->waiters, 2);

        while (x) {
            syscall(SYS_futex, &wrapper->shctx->waiters, FUTEX_WAIT,
                    2, NULL, 0, 0);
            x = xchg(&wrapper->shctx->waiters, 2);
        }
    }
}

static inline void
shared_context_unlock(struct openssl_shctx_wrapper* wrapper)
{
    if (atomic_dec(&wrapper->shctx->waiters)) {
        wrapper->shctx->waiters = 0;
        syscall(SYS_futex, &wrapper->shctx->waiters, FUTEX_WAKE,
                1, NULL, 0, 0);
    }
}

#else /* USE_SYSCALL_FUTEX */
#  define shared_context_lock(v)    pthread_mutex_lock(&v->shctx->mutex)
#  define shared_context_unlock(v)  pthread_mutex_unlock(&v->shctx->mutex)
#endif

/* List Macros */

#define shsess_unset(s)                                     \
    do {                                                    \
        (s)->n->p = (s)->p;                                 \
        (s)->p->n = (s)->n;                                 \
    } while (0)

#define shsess_set_free(s)                                  \
    do {                                                    \
        shsess_unset(s);                                    \
        (s)->p = &wrapper->shctx->free;                     \
        (s)->n = wrapper->shctx->free.n;                    \
        wrapper->shctx->free.n->p = s;                      \
        wrapper->shctx->free.n = s;                         \
    } while (0)


#define shsess_set_active(s)                                \
    do {                                                    \
        shsess_unset(s);                                    \
        (s)->p = &wrapper->shctx->active;                   \
        (s)->n = wrapper->shctx->active.n;                  \
        wrapper->shctx->active.n->p = s;                    \
        wrapper->shctx->active.n = s;                       \
    } while (0)


#define shsess_get_next()                                   \
    (wrapper->shctx->free.p == wrapper->shctx->free.n ?     \
     wrapper->shctx->active.p : wrapper->shctx->free.p)

/* Tree Macros */

#define shsess_tree_delete(s) ebmb_delete(&(s)->key)

#define shsess_tree_insert(s)                               \
    (struct shared_session *)ebmb_insert(&wrapper->shctx->active.key.node.branches, \
        &(s)->key, SSL_MAX_SSL_SESSION_ID_LENGTH);

#define shsess_tree_lookup(k) \
    (struct shared_session *)ebmb_lookup(&wrapper->shctx->active.key.node.branches, \
        (k), SSL_MAX_SSL_SESSION_ID_LENGTH);

/* Copy-with-padding Macros */

#define shsess_memcpypad(dst, dlen, src, slen)              \
    do {                                                    \
        assert((slen) <= (dlen));                           \
        memcpy((dst), (src), (slen));                       \
        if ((slen) < (dlen))                                \
            memset((char *)(dst) + (slen), 0,               \
                (dlen) - (slen));                           \
    } while (0)

#define shsess_set_key(s, k, l)                             \
    do {                                                    \
        shsess_memcpypad((s)->key_data,                     \
            SSL_MAX_SSL_SESSION_ID_LENGTH, (k), (l));       \
    } while (0)

/* SSL context callbacks */

/* SSL callback used on new session creation */
static int
shctx_new_cb(SSL *ssl, SSL_SESSION *sess)
{
    SSL_CTX *ssl_ctx;
    struct openssl_shctx_wrapper *wrapper;
    struct shared_session *shsess;
    unsigned char *data,*p;
    const unsigned char *key;
    unsigned keylen;
    unsigned data_len;
    unsigned char encsess[SHSESS_MAX_ENCODED_LEN];

    AN(ssl);

    ssl_ctx = SSL_get_SSL_CTX(ssl);
    wrapper = SSL_CTX_get_app_data(ssl_ctx);
    assert(wrapper);

    data_len = i2d_SSL_SESSION(sess, NULL);
    if (data_len > SHSESS_MAX_DATA_LEN)
        return (1);

    /* process ASN1 session encoding before the lock: lower cost */
    p = data = encsess+SSL_MAX_SSL_SESSION_ID_LENGTH;
    i2d_SSL_SESSION(sess, &p);

    shared_context_lock(wrapper);

    shsess = shsess_get_next();

    shsess_tree_delete(shsess);

    key = SSL_SESSION_get_id(sess, &keylen);
    shsess_set_key(shsess, key, keylen);

    shsess = shsess_tree_insert(shsess);
    AN(shsess);

    /* store ASN1 encoded session into cache */
    shsess->data_len = data_len;
    memcpy(shsess->data, data, data_len);

    /* store creation date */
#if OPENSSL_VERSION_NUMBER >= 0x30300000L
    shsess->c_date = SSL_SESSION_get_time_ex(sess);
#else
    shsess->c_date = SSL_SESSION_get_time(sess);
#endif

    shsess_set_active(shsess);

    shared_context_unlock(wrapper);

    if (wrapper->shared_session_new_cbk) { /* if user level callback is set */
        shsess_memcpypad(encsess, SSL_MAX_SSL_SESSION_ID_LENGTH,
            key, keylen);

        wrapper->shared_session_new_cbk(encsess,
            SSL_MAX_SSL_SESSION_ID_LENGTH + data_len,
#if OPENSSL_VERSION_NUMBER >= 0x30300000L
            SSL_SESSION_get_time_ex(sess)
#else
            SSL_SESSION_get_time(sess)
#endif
            );
    }

    return (0); /* do not increment session reference count */
}

/* SSL callback used on lookup an existing session cause none found
   in internal cache */
#if OPENSSL_VERSION_NUMBER < 0x10100000L
static SSL_SESSION *
shctx_get_cb(SSL *ssl, unsigned char *key, int key_len, int *do_copy)
#else
static SSL_SESSION *
shctx_get_cb(SSL *ssl, const unsigned char *key, int key_len, int *do_copy)
#endif
{
    SSL_CTX *ssl_ctx;
    struct openssl_shctx_wrapper *wrapper;
    struct shared_session *shsess;
    unsigned char data[SHSESS_MAX_DATA_LEN], *p;
    unsigned char padded_key[SSL_MAX_SSL_SESSION_ID_LENGTH];
    unsigned data_len;
    time_t cdate;
    SSL_SESSION *sess;

    AN(ssl);
    ssl_ctx = SSL_get_SSL_CTX(ssl);
    wrapper = SSL_CTX_get_app_data(ssl_ctx);
    assert(wrapper);

    /* allow the session to be freed automatically by openssl */
    *do_copy = 0;

    shsess_memcpypad(padded_key, sizeof padded_key, key, (size_t)key_len);

    shared_context_lock(wrapper);

    shsess = shsess_tree_lookup(padded_key);
    if(shsess == NULL) {
        shared_context_unlock(wrapper);
        return (NULL);
    }

    /* backup creation date to reset in session after ASN1 decode */
    cdate = shsess->c_date;

    /* copy ASN1 session data to decode outside the lock */
    data_len = shsess->data_len;
    memcpy(data, shsess->data, shsess->data_len);

    shsess_set_active(shsess);

    shared_context_unlock(wrapper);

    /* decode ASN1 session */
        p = data;
    sess = d2i_SSL_SESSION(NULL, (const unsigned char **)&p, data_len);

    /* reset creation date */
    if (sess) {
#if OPENSSL_VERSION_NUMBER >= 0x30300000L
        SSL_SESSION_set_time_ex(sess, cdate);
#else
        SSL_SESSION_set_time(sess, cdate);
#endif
    }

    return (sess);
}

/* SSL callback used to signal session is no more used in internal cache */
static void
shctx_remove_cb(SSL_CTX *ctx, SSL_SESSION *sess)
{
    struct openssl_shctx_wrapper *wrapper;
    struct shared_session *shsess;
    unsigned char padded_key[SSL_MAX_SSL_SESSION_ID_LENGTH];
    const unsigned char *key;
    unsigned keylen;

    AN(ctx);
    wrapper = SSL_CTX_get_app_data(ctx);
    assert(wrapper);

    key = SSL_SESSION_get_id(sess, &keylen);
    shsess_memcpypad(padded_key, sizeof padded_key, key, (size_t)keylen);

    shared_context_lock(wrapper);

    shsess = shsess_tree_lookup(padded_key);
    if (shsess != NULL)
        shsess_set_free(shsess);

    /* unlock cache */
    shared_context_unlock(wrapper);
}

/* User level function called to add a session to the cache (remote updates) */
void
openssl_shctx_sess_add(struct openssl_shctx_wrapper *wrapper,
        const unsigned char *encsess, unsigned len, time_t cdate)
{
    struct shared_session *shsess;

    /* check buffer is at least padded key long + 1 byte
        and data_len not too long */
    if (len <= SSL_MAX_SSL_SESSION_ID_LENGTH ||
        len > SHSESS_MAX_DATA_LEN + SSL_MAX_SSL_SESSION_ID_LENGTH)
        return;

    shared_context_lock(wrapper);

    shsess = shsess_get_next();
    shsess_tree_delete(shsess);
    shsess_set_key(shsess, encsess, SSL_MAX_SSL_SESSION_ID_LENGTH);

    shsess = shsess_tree_insert(shsess);
    AN(shsess);

    /* store into cache and update earlier on session get events */
    if (cdate)
        shsess->c_date = (long)cdate;

    /* copy ASN1 session data into cache */
    shsess->data_len = len - SSL_MAX_SSL_SESSION_ID_LENGTH;
    memcpy(shsess->data, encsess + SSL_MAX_SSL_SESSION_ID_LENGTH,
            shsess->data_len);

    shsess_set_active(shsess);

    shared_context_unlock(wrapper);
}

/* Function used to set a callback on new session creation */
void
openssl_shsess_set_new_cbk(struct openssl_shctx_wrapper *wrapper,
        shsess_new_f *func)
{
    AN(func);
    wrapper->shared_session_new_cbk = func;
}

/* Allocate the shared memory context.
 *
 */
static int
shared_context_alloc(struct openssl_shctx_wrapper *wrapper, size_t size)
{
    struct shared_session *prev, *cur;
#ifndef USE_SYSCALL_FUTEX
    pthread_mutexattr_t attr;
#endif

    wrapper->sz_shm =
        sizeof(*wrapper->shctx) + (sizeof(struct shared_session) * size);

    if (ftruncate(wrapper->fd, wrapper->sz_shm) == -1) {
        return -1;
    }

    wrapper->shctx = mmap(NULL, wrapper->sz_shm,
        PROT_READ | PROT_WRITE, MAP_SHARED, wrapper->fd, 0);
    if (wrapper->shctx == MAP_FAILED)
        return -1;

#ifdef USE_SYSCALL_FUTEX
    wrapper->shctx->waiters = 0;
#else
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&wrapper->shctx->mutex, &attr);
#endif
    memset(&wrapper->shctx->active.key, 0, sizeof(struct ebmb_node));
    memset(&wrapper->shctx->free.key, 0, sizeof(struct ebmb_node));

    /* No duplicate authorized in tree: */
    wrapper->shctx->active.key.node.branches.b[1] = (void *)1;

    cur = &wrapper->shctx->active;
    cur->n = cur->p = cur;

    cur = &wrapper->shctx->free;
    for (size_t i = 0 ; i < size ; i++) {
        prev = cur;
        cur++;
        prev->n = cur;
        cur->p = prev;
    }
    cur->n = &wrapper->shctx->free;
    wrapper->shctx->free.p = cur;

    return 0;
}

int openssl_shctx_create(struct openssl_shctx_wrapper *wrapper,
        const char *shctxid, mode_t mode, SSL_CTX *ctx, size_t size)
{
    char name[NAME_MAX + 1];
    int ret;

    if (strlen(shctxid) > OPENSSL_SHCTX_ID_LEN)
        return HELPER_RETV_BAD_ARGS;

    ret = snprintf(name, sizeof(name), SHSESS_NAME_PATTERN, shctxid);
    assert(ret > 0 && (size_t)ret < sizeof(name));

    if (shm_unlink(name) == -1 && errno != ENOENT) {
        return HELPER_RETV_BAD_SYSCALL;
    }

    strcpy(wrapper->shctxid, shctxid);
    int fd = shm_open(name, O_CREAT | O_RDWR | O_EXCL, mode);
    if (fd < 0)
        return HELPER_RETV_BAD_SYSCALL;

    wrapper->fd = fd;
    ret = shared_context_alloc(wrapper, size);
    if (ret < 0) {
        return HELPER_RETV_BAD_SYSCALL;
    }

    SSL_CTX_set_app_data(ctx, wrapper);

    /* set SSL internal cache size to external cache / 8  + 123 */
    SSL_CTX_sess_set_cache_size(ctx, size >> 3 | 0x3ff);

    /* Set callbacks */
    SSL_CTX_sess_set_new_cb(ctx, shctx_new_cb);
    SSL_CTX_sess_set_get_cb(ctx, shctx_get_cb);
    SSL_CTX_sess_set_remove_cb(ctx, shctx_remove_cb);

    return 0;
}

int openssl_shctx_destroy(struct openssl_shctx_wrapper *wrapper)
{
    char name[NAME_MAX + 1];
    int ret = snprintf(name, sizeof(name),
            SHSESS_NAME_PATTERN, wrapper->shctxid);
    assert(ret > 0 && (size_t)ret < sizeof(name));
    (void)ret;

    if (munmap(wrapper->shctx, wrapper->sz_shm) == -1)
        return HELPER_RETV_BAD_SYSCALL;

    close(wrapper->fd);
    memset(wrapper, 0, sizeof(*wrapper));
    wrapper->fd = -1;
    return shm_unlink(name);
}

int openssl_shctx_attach(struct openssl_shctx_wrapper *wrapper,
        const char *shctxid, SSL_CTX *ctx)
{
    char name[NAME_MAX + 1];

    if (strlen(shctxid) > OPENSSL_SHCTX_ID_LEN)
        return HELPER_RETV_BAD_ARGS;

    int ret = snprintf(name, sizeof(name), SHSESS_NAME_PATTERN, shctxid);
    assert(ret > 0 && (size_t)ret < sizeof(name));
    (void)ret;

    strcpy(wrapper->shctxid, shctxid);
    int fd = shm_open(name, O_RDWR, 0);
    if (fd < 0)
        return HELPER_RETV_BAD_SYSCALL;

    struct stat st_buf;
    if (fstat(fd, &st_buf) == -1)
        return HELPER_RETV_BAD_SYSCALL;

    wrapper->fd = fd;
    wrapper->sz_shm = st_buf.st_size;
    wrapper->shctx = mmap(NULL, wrapper->sz_shm,
        PROT_READ | PROT_WRITE, MAP_SHARED, wrapper->fd, 0);
    if (wrapper->shctx == MAP_FAILED)
        return HELPER_RETV_BAD_SYSCALL;

    SSL_CTX_set_app_data(ctx, wrapper);

    size_t size = (wrapper->sz_shm - sizeof(*wrapper->shctx)) /
        sizeof(struct shared_session);

    /* set SSL internal cache size to external cache / 8  + 123 */
    SSL_CTX_sess_set_cache_size(ctx, size >> 3 | 0x3ff);

    /* Set callbacks */
    SSL_CTX_sess_set_new_cb(ctx, shctx_new_cb);
    SSL_CTX_sess_set_get_cb(ctx, shctx_get_cb);
    SSL_CTX_sess_set_remove_cb(ctx, shctx_remove_cb);
    return 0;
}

int openssl_shctx_detach(struct openssl_shctx_wrapper *wrapper)
{
    if (munmap(wrapper->shctx, wrapper->sz_shm) == -1)
        return HELPER_RETV_BAD_SYSCALL;

    close(wrapper->fd);
    memset(wrapper, 0, sizeof(*wrapper));
    wrapper->fd = -1;
    return 0;
}

