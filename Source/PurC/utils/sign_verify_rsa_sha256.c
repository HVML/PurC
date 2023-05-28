/*
** @file sign_verify_rsa_sha256.c
** @author Vincent Wei
** @date 2023/05/28 (Move here from HBDBus).
** @brief The utilitis for signing the challenge code
**  and verifying the signature with RSA and SHA256 algorithms.
**
** Copyright (C) 2020 ~ 2023 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if ENABLE(APP_AUTH)
#if HAVE(OPENSSL)

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>

#include "purc-helpers.h"
#include "purc-errors.h"
#include "private/debug.h"

static int my_error_printer(const char *str, size_t len, void *u)
{
    (void)len;
    (void)u;
    PC_ERROR("%s\n", str);
    return 0;
}

static RSA* read_private_key_for_app(const char* app_name)
{
    int size;
    char buff [512];
    FILE *fp = NULL;
    RSA *pri_key = NULL;

    size = snprintf(buff, sizeof (buff), PURC_PRIVATE_PEM_KEY_FILE,
            app_name, app_name);
    if (size < 0 || (size_t)size >= sizeof(buff)) {
        PC_ERROR("Too long app name in read_private_key_for_app: %s\n",
                app_name);
        return NULL;
    }

    if ((fp = fopen (buff, "r")) == NULL) {
        PC_ERROR("Failed to open the private key file (%s) for app (%s): %s\n",
                buff, app_name, strerror (errno));
        return NULL;
    }

    pri_key = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL);
    if (pri_key == NULL) {
        PC_ERROR("Failed to read RSA private key for app (%s):\n",
                app_name);
        ERR_print_errors_cb(my_error_printer, NULL);
    }

    fclose(fp);
    return pri_key;
}

int pcutils_sign_data(const char *app_name,
        const unsigned char* data, unsigned int data_len,
        unsigned char **sig, unsigned int *sig_len)
{
    int err_code = 0;
    RSA *priv_key = NULL;
    unsigned char md[SHA256_DIGEST_LENGTH];
    int retv = 0;

    *sig = NULL;
    *sig_len = 0;

    priv_key = read_private_key_for_app(app_name);
    if (!priv_key) {
        return PURC_ERROR_IO_FAILURE;
    }

    if ((*sig = calloc(1, 128)) == NULL) {
        err_code = PURC_ERROR_OUT_OF_MEMORY;
        goto failed;
    }

    SHA256 (data, data_len, md);
    retv = RSA_sign(NID_sha256, md, SHA256_DIGEST_LENGTH,
            *sig, sig_len, priv_key);
    if (retv != 1) {
        free(*sig);
        *sig = NULL;
        *sig_len = 0;
        err_code = PURC_ERROR_NOT_ACCEPTABLE;
    }

failed:
    RSA_free(priv_key);
    return err_code;
}

static RSA* read_public_key_for_app(const char* app_name)
{
    int size;
    char buff[512];
    FILE *fp;
    RSA *pub_key = NULL;

    size = snprintf(buff, sizeof(buff), PURC_PUBLIC_PEM_KEY_FILE, app_name);
    if (size < 0 || (size_t)size >= sizeof(buff)) {
        PC_ERROR("Too long app name: %s\n", app_name);
        return NULL;
    }

    if ((fp = fopen(buff, "r")) == NULL) {
        PC_ERROR("Failed to open public key file for app (%s): %s\n",
                app_name, strerror(errno));
        return NULL;
    }

    if ((pub_key = PEM_read_RSA_PUBKEY(fp, NULL, NULL, NULL)) == NULL) {
        PC_ERROR("Failed to read RSA public key for app (%s):\n", app_name);
        ERR_print_errors_cb(my_error_printer, NULL);
        goto failed;
    }

failed:
    fclose(fp);
    return pub_key;
}

int pcutils_verify_signature(const char* app_name,
        const unsigned char* data, unsigned int data_len,
        const unsigned char* sig, unsigned int sig_len)
{
    unsigned char md[SHA256_DIGEST_LENGTH];
    RSA *pub_key = NULL;
    int retv = 0;

    pub_key = read_public_key_for_app(app_name);
    if (!pub_key) {
        return PURC_ERROR_IO_FAILURE;
    }

    SHA256(data, data_len, md);

    retv = RSA_verify(NID_sha256, md, SHA256_DIGEST_LENGTH, sig, sig_len, pub_key);
    RSA_free(pub_key);
    return retv ? 1 : 0;
}

#endif /* HAVE_OPENSSL */
#endif /* ENABLE(APP_AUTH) */

