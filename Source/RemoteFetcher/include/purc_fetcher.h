/**
 * @file purc_fetcher.h
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/09/18
 * @brief The main header file of PurCFetcher.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurCFetcher, which contains the examples of my course:
 * _the Best Practices of C Language_.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef PURCFETCHER_PURCFETCHER_H
#define PURCFETCHER_PURCFETCHER_H

#include "purc_fetcher_macros.h"
#include "purc_fetcher_version.h"
#include "purc_fetcher_features.h"

struct purc_fetcher_instance;
typedef struct purc_fetcher_instance *purc_fetcher_instance_t;

typedef struct purc_fetcher_instance_extra_info {
} purc_fetcher_instance_extra_info;

PURCFETCHER_EXTERN_C_BEGIN

/**
 * purc_fetcher_init:
 *
 * @app_name: a pointer to the string contains the app name.
 *      If this argument is null, the executable program name of the command
 *      line will be used for the app name.
 * @runner_name: a pointer to the string contains the runner name.
 *      If this argument is null, `unknown` will be used for the runner name.
 * @extra_info: a pointer (nullable) to the extra information for
 *      the new PurCFetcher instance.
 *
 * Initializes a new PurCFetcher instance for the current thread.
 *
 * Returns: A new PurCFetcher instance; NULL on error.
 *
 * Since 0.0.1
 */
PURCFETCHER_EXPORT purc_fetcher_instance_t purc_fetcher_init(const char* app_name,
        const char* runner_name,
        const purc_fetcher_instance_extra_info* extra_info);

/**
 * purc_fetcher_term:
 *
 * Cleans up the PurCFetcher instance attached to the current thread.
 *
 * Returns: @true for success; @false for bad PurCFetcher instance.
 *
 * Since 0.0.1
 */
PURCFETCHER_EXPORT bool purc_fetcher_term(purc_fetcher_instance_t inst);

PURCFETCHER_EXTERN_C_END

#endif /* not defined PURCFETCHER_PURCFETCHER_H */
