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

bool
purc_is_feature_enabled(enum purc_feature feature)
{
    switch (feature) {
        case PURC_FEATURE_SOCKET_STREAM:
            if (PCA_ENABLE_SOCKET_STREAM)
                return true;
            break;
        case PURC_FEATURE_DOCTYPE_HTML:
            if (PCA_ENABLE_DOCTYPE_HTML)
                return true;
            break;
        case PURC_FEATURE_DOCTYPE_XML:
            if (PCA_ENABLE_DOCTYPE_XML)
                return true;
            break;
        case PURC_FEATURE_DOCTYPE_XGML:
            if (PCA_ENABLE_DOCTYPE_XGML)
                return true;
            break;
        case PURC_FEATURE_REMOTE_FETCHER:
            if (PCA_ENABLE_REMOTE_FETCHER)
                return true;
            break;
        case PURC_FEATURE_RDRCM_THREAD:
            if (PCA_ENABLE_RDRCM_THREAD)
                return true;
            break;
        case PURC_FEATURE_RDRCM_SOCKET:
            if (PCA_ENABLE_RDRCM_SOCKET)
                return true;
            break;
        case PURC_FEATURE_RDRCM_HBDBUS:
            if (PCA_ENABLE_RDRCM_HBDBUS)
                return true;
            break;
        case PURC_FEATURE_STREAM_HBDBUS:
            if (PCA_ENABLE_STREAM_HBDBUS)
                return true;
            break;
        case PURC_FEATURE_STREAM_MQTT:
            if (PCA_ENABLE_STREAM_MQTT)
                return true;
            break;
        case PURC_FEATURE_STREAM_WEB_SOCKET:
            if (PCA_ENABLE_STREAM_WEB_SOCKET)
                return true;
            break;
        case PURC_FEATURE_SSL:
            if (PCA_ENABLE_SSL)
                return true;
            break;
    }

    return false;
}

