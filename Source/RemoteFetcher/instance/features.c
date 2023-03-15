/*
 * @file features.c
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/09/20
 * @brief The functions for testing features of PurCFetcher.
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

#include "purc_fetcher_features.h"

bool
purc_fetcher_is_feature_enabled(enum purc_fetcher_feature feature)
{
    switch (feature) {
        case PURCFETCHER_FEATURE_HTML:
            if (PURCFETCHER_ENABLE_DOCTYPE_HTML)
                return true;
            break;
        case PURCFETCHER_FEATURE_XGML:
            if (PURCFETCHER_ENABLE_DOCTYPE_XGML)
                return true;
            break;
        case PURCFETCHER_FEATURE_XML:
            if (PURCFETCHER_ENABLE_DOCTYPE_XML)
                return true;
            break;
        case PURCFETCHER_FEATURE_LCMD:
            if (PURCFETCHER_ENABLE_SCHEMA_LCMD)
                return true;
            break;
        case PURCFETCHER_FEATURE_LSQL:
            if (PURCFETCHER_ENABLE_SCHEMA_LSQL)
                return true;
            break;
        case PURCFETCHER_FEATURE_RSQL:
            if (PURCFETCHER_ENABLE_SCHEMA_RSQL)
                return true;
            break;
        case PURCFETCHER_FEATURE_HTTP:
            if (PURCFETCHER_ENABLE_SCHEMA_HTTP)
                return true;
            break;
    }

    return false;
}

