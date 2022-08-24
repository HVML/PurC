/*
 * @file channel.h
 * @author Vincent Wei
 * @date 2022/08/23
 * @brief The private interface for channel management.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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

#ifndef PURC_PRIVATE_CHANNEL_H
#define PURC_PRIVATE_CHANNEL_H

#include <stdbool.h>

#include "private/list.h"
#include "private/utils.h"

#define PCCHAN_MAX_LEN_NAME     63

struct pcchan {
    /* size of the circular queue */
    unsigned int    qsize;
    /* total variants in the queue */
    unsigned int    qcount;

    /* indices to send and receive */
    unsigned int    sendx;
    unsigned int    recvx;

    /* list of coroutines waiting to send */
    struct list_head send_crtns;

    /* list of coroutines waiting to receive */
    struct list_head recv_crtns;

    /* the buffer for variants. */
    purc_variant_t  data[0];
};

typedef struct pcchan *pcchan_t;

PCA_EXTERN_C_BEGIN

pcchan_t
pcchan_open(const char *chan_name, unsigned int cap) WTF_INTERNAL;

bool
pcchan_ctrl(const char *chan_name, unsigned int new_cap) WTF_INTERNAL;

pcchan_t
pcchan_retrieve(const char *chan_name) WTF_INTERNAL;

void
pcchan_destroy(pcchan_t chan) WTF_INTERNAL;

purc_variant_t
pcchan_make_entity(pcchan_t chan) WTF_INTERNAL;

static inline unsigned int
pcchan_capability(pcchan_t chan) {
    return chan->qsize;
}

static inline unsigned int
pcchan_length(pcchan_t chan) {
    return chan->qcount;
}

PCA_EXTERN_C_END

#endif /* not defined PURC_PRIVATE_CHANNEL_H */

