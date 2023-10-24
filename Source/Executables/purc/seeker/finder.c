/*
** @file workspace.c
** @author Vincent Wei
** @date 2023/10/20
** @brief The implementation of workspace of Seeker.
**
** Copyright (C) 2023 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of purc, which is an HVML interpreter with
** a command line interface (CLI).
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "seeker.h"
#include "finder.h"
#include "purcmc-thread.h"
#include "endpoint.h"

#include <assert.h>

int seeker_look_for_local_renderer(const char *name, void *ctxt)
{
    pcmcth_renderer *rdr = ctxt;
    LOG_WARN("It is time to find a new local renderer: %s for rdr: %p\n",
            name, rdr);
    return 0;
}

#if PCA_ENABLE_DNSSD
void seeker_dnssd_on_service_discovered(struct purc_dnssd_conn *dnssd,
        void *browsing_handle,
        unsigned int flags, uint32_t if_index, int error_code,
        const char *service_name, const char *reg_type, const char *hostname,
        uint16_t port, uint16_t len_txt_record, const char *txt_record,
        void *ctxt)
{
    pcmcth_renderer *rdr = ctxt;
    (void)dnssd;
    (void)rdr;
    (void)browsing_handle;
    (void)flags;

    if (error_code == 0) {
        LOG_WARN("Find a service `%s` with type `%s` on `%s` at port (%u)\n",
                service_name, reg_type, hostname, (unsigned)port);
        LOG_WARN("    The interface index: %u\n", if_index);
        if (len_txt_record) {
            LOG_WARN("    The TXT record: %s\n", txt_record);
        }

        // TODO: emit a `newRenderer` event here
    }
    else {
        LOG_WARN("Error occurred when browsing service: %d.\n", error_code);
    }
}
#endif

