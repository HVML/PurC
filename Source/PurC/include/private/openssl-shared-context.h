/*
 * openssl-shared-context.h
 *
 * Copyright (C) 2011 EXCELIANCE
 * Copyright (C) 2025 FMSoft
 *
 * Authors:
 *  - Emeric Brun - emeric@exceliance.fr
 *  - Vincent Wei - <https://github.com/VincentWei>
 *
 */

#ifndef SHCTX_H
#define SHCTX_H

#include "config.h"

#include <openssl/ssl.h>
#include <stdint.h>
#include <fcntl.h>

typedef void (shsess_new_f)(unsigned char *sess, unsigned len, time_t cdate);

#define OPENSSL_SHCTX_ID_LEN         7
#define OPENSSL_SHCTX_CACHESZ_MIN    4
#define OPENSSL_SHCTX_CACHESZ_DEF    256

struct _openssl_shared_context;
struct openssl_shctx_wrapper {
    char shctxid[OPENSSL_SHCTX_ID_LEN + 1];
    int fd;
    size_t sz_shm;
    struct _openssl_shared_context *shctx;
    shsess_new_f *shared_session_new_cbk;
};

/* Callback called on a new session event:
 *
 * sess contains the sessionid zeros padded to SSL_MAX_SSL_SESSION_ID_LENGTH
 * followed by ASN1 session encoding.
 *
 * len is set to SSL_MAX_SSL_SESSION_ID_LENGTH + ASN1 session length
 * len is always less than SSL_MAX_SSL_SESSION_ID_LENGTH + SHSESS_MAX_DATA_LEN.
 * Remaining Bytes from len to SHSESS_MAX_ENCODED_LEN can be used to add a footer.
 *
 * cdate is the creation date timestamp.
 */
void openssl_shsess_set_new_cbk(struct openssl_shctx_wrapper *wrapper,
        shsess_new_f *cb)
    WTF_INTERNAL;

/* Add a session into the cache, 
 * sess contains the sessionid zeros padded to SSL_MAX_SSL_SESSION_ID_LENGTH
 * followed by ASN1 session encoding.
 *
 * len is set to SSL_MAX_SSL_SESSION_ID_LENGTH + ASN1 data length.
 *            if len greater than SHSESS_MAX_ENCODED_LEN, session is not added.
 *
 * if cdate not 0, on get events session creation date will be reset to cdate */
void openssl_shctx_sess_add(struct openssl_shctx_wrapper *wrapper,
        const unsigned char *sess, unsigned len, time_t cdate)
    WTF_INTERNAL;

enum {
    HELPER_RETV_OK = 0,
    HELPER_RETV_BAD_SYSCALL = -1,
    HELPER_RETV_BAD_LIBCALL = -2,
    HELPER_RETV_BAD_ARGS    = -3,
};

/* Create a new OpenSSL shared context object: for dispatcher process.
 *
 * @wrapper is the pointer to the wrapper of the shared context.
 * @shctxid is the session-id-context.
 * @mode is the file mode for the shared memory according to shm_open().
 * @ctx is the pointer to SSL_CTX.
 * @size is the max number of stored session.
 *
 * Returns: -1 on system call failure, -2 on openssl call failure,
 * -3 for bad argument, 0 if success.
 */
int openssl_shctx_create(struct openssl_shctx_wrapper *wrapper,
        const char *shctxid, mode_t mode, SSL_CTX *ctx, size_t size)
    WTF_INTERNAL;

/* Destroy an OpenSSL shared context object: for dispatcher process.
 *
 * @wrapper is the pointer to the wrapper of the shared context.
 *
 * Returns: -1 on failure, 0 if success.
 */
int openssl_shctx_destroy(struct openssl_shctx_wrapper *wrapper)
    WTF_INTERNAL;

/* Attach to an OpenSSL shared context object: for worker processes.
 *
 * @wrapper is the pointer to the wrapper of the shared context.
 * @shctxid is the session-id-context to locate the shared memory file.
 *
 * Returns: -1 on failure, 0 if success.
 */
int openssl_shctx_attach(struct openssl_shctx_wrapper *wrapper,
        const char *shctxid, SSL_CTX *ctx)
    WTF_INTERNAL;

/* Detach from an OpenSSL shared context object: for worker processes.
 *
 * @wrapper is the pointer to the wrapper of the shared context.
 */
int openssl_shctx_detach(struct openssl_shctx_wrapper *wrapper)
    WTF_INTERNAL;

#endif /* SHCTX_H */

