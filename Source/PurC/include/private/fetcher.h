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

#define FETCHER_PARAM_RAW_HEADER    "__fetcher_param_raw_header"
#define FETCHER_PARAM_DATA          "__fetcher_param_data"

#define RESP_CODE_USER_STOP         -1
#define RESP_CODE_USER_CANCEL       -2

struct pcfetcher_session {
};

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

bool pcfetcher_is_init(void);

const char* pcfetcher_set_base_url(const char* base_url);

void pcfetcher_cancel_async(purc_variant_t request);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_FETCHER_H */


