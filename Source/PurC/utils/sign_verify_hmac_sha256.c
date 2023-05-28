/*
** @file sign_verify_hmac_sha256.c
** @author Vincent Wei
** @date 2023/05/28 (Move here from HBDBus).
** @brief The utilitis for sign the challenge code
**  and verify the signature with HMAC SHA256 algorithms.
**  This file will be used when OpenSSL is not available.
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
#include <assert.h>

#if ENABLE(APP_AUTH)
#if !HAVE(OPENSSL)

#include "purc-helpers.h"
#include "purc-errors.h"
#include "private/debug.h"

static int read_private_key_for_app(const char* app_name,
        unsigned char* key, unsigned int key_len)
{
    int size;
    char buff[512];
    FILE *fp = NULL;

    size = snprintf(buff, sizeof(buff), PURC_PRIVATE_HMAC_KEY_FILE,
            app_name, app_name);
    if (size < 0 || (size_t)size >= sizeof (buff)) {
        PC_ERROR("Too long app name: %s\n", app_name);
        return -1;
    }

    if ((fp = fopen(buff, "r")) == NULL) {
        PC_ERROR("Failed to open the private key file for app (%s): %s\n",
                app_name, strerror (errno));
        return -2;
    }

    size = fread(key, 1, key_len, fp);
    if (size < PURC_LEN_PRIVATE_HMAC_KEY) {
        fclose(fp);
        return -3;
    }

    fclose(fp);
    return 0;
}

int pcutils_sign_data(const char *app_name,
        const unsigned char* data, unsigned int data_len,
        unsigned char **sig, unsigned int *sig_len)
{
    unsigned char key[PURC_LEN_PRIVATE_HMAC_KEY];

    *sig = NULL;
    *sig_len = 0;

    if (read_private_key_for_app(app_name, key, PURC_LEN_PRIVATE_HMAC_KEY)) {
        return PURC_ERROR_IO_FAILURE;
    }

    if ((*sig = calloc(1, PCUTILS_SHA256_DIGEST_SIZE)) == NULL) {
        return PURC_ERROR_OUT_OF_MEMORY;
    }

    *sig_len = PCUTILS_SHA256_DIGEST_SIZE;
    pcutils_hmac_sha256(*sig, data, data_len, key, PURC_LEN_PRIVATE_HMAC_KEY);

    return 0;
}

int pcutils_verify_signature(const char* app_name,
        const unsigned char* data, unsigned int data_len,
        const unsigned char* sig, unsigned int sig_len)
{
    unsigned char key[PURC_LEN_PRIVATE_HMAC_KEY];
    unsigned char my_sig[PCUTILS_SHA256_DIGEST_SIZE];

    if (read_private_key_for_app(app_name, key, PURC_LEN_PRIVATE_HMAC_KEY)) {
        return PURC_ERROR_IO_FAILURE;
    }

    pcutils_hmac_sha256(my_sig, data, data_len, key, PURC_LEN_PRIVATE_HMAC_KEY);
    if (memcmp(my_sig, sig, sig_len) == 0)
        return 1;

    return 0;
}

#endif /* !HAVE(OPENSSL) */
#endif /* ENABLE(APP_AUTH) */
