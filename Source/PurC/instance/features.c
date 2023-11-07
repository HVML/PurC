/*
 * @file features.c
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2021/07/04
 * @brief The functions for testing features of PurC.
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

#include "purc-features.h"
#include "purc-macros.h"

static bool features[] = {
    PCA_ENABLE_SOCKET_STREAM,
    PCA_ENABLE_DOCTYPE_HTML,
    PCA_ENABLE_DOCTYPE_XML,
    PCA_ENABLE_DOCTYPE_XGML,
    PCA_ENABLE_REMOTE_FETCHER,
    PCA_ENABLE_RDRCM_THREAD,
    PCA_ENABLE_RDRCM_SOCKET,
    PCA_ENABLE_RDRCM_HBDBUS,
    PCA_ENABLE_STREAM_HBDBUS,
    PCA_ENABLE_STREAM_MQTT,
    PCA_ENABLE_STREAM_WEB_SOCKET,
    PCA_ENABLE_SSL,
    PCA_ENABLE_APP_AUTH,
    PCA_ENABLE_DNSSD,
};

/* Make sure the number of error messages matches the number of error codes */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(features,
        PCA_TABLESIZE(features) == PURC_FEATURE_NR);

#undef _COMPILE_TIME_ASSERT

bool
purc_is_feature_enabled(enum purc_feature feature)
{
    return features[feature];
}

