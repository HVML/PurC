/*
** @file sign_verify_dummy.c
** @author Vincent Wei
** @date 2023/05/28 (Move here from HBDBus).
** @brief The dummy implementation for signing the challenge code
**  and verifying the signature.
**  This file will be used when WITH_APP_AUTH is not defined.
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

#if !ENABLE(APP_AUTH)

#include "purc-errors.h"

#define DUMMY_SIGNATURE     "DUMB"
#define LEN_DUMMY_SIGNATURE 4

int pcutils_sign_data(const char *app_name,
        const unsigned char* data, unsigned int data_len,
        unsigned char **sig, unsigned int *sig_len)
{
    (void)app_name;
    (void)data;
    (void)data_len;
    *sig = NULL;
    *sig_len = 0;

    if ((*sig = calloc(1, LEN_DUMMY_SIGNATURE)) == NULL) {
        return PURC_ERROR_OUT_OF_MEMORY;
    }

    *sig_len = LEN_DUMMY_SIGNATURE;
    memcpy(*sig, DUMMY_SIGNATURE, LEN_DUMMY_SIGNATURE);
    return 0;
}

int pcutils_verify_signature(const char* app_name,
        const unsigned char* data, unsigned int data_len,
        const unsigned char* sig, unsigned int sig_len)
{
    (void)app_name;
    (void)data;
    (void)data_len;
    if (memcmp(sig, DUMMY_SIGNATURE, sig_len) == 0)
        return 1;

    return 0;
}

#endif /* !ENABLE(APP_AUTH) */

