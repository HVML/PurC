/*
** @file endpoint.h
** @author Vincent Wei
** @date 2022/10/02
** @brief The interface of endpoint (copied from PurC Midnight Commander).
**
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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

#ifndef purc_foil_endpoint_h_
#define purc_foil_endpoint_h_

#include <time.h>
#include <stdbool.h>
#include <string.h>

#include "foil.h"

/* causes to delete endpoint */
enum {
    CDE_EXITING,
    CDE_NO_RESPONDING,
};

int comp_living_time(const void *k1, const void *k2, void *ptr);

struct avl_tree;
void remove_all_living_endpoints(struct avl_tree *avl);

purcth_endpoint* new_endpoint(purcth_renderer* rdr, const char *uri);
int del_endpoint(purcth_renderer* rdr, purcth_endpoint* endpoint, int cause);
purcth_endpoint* retrieve_endpoint(purcth_renderer* rdr, const char *uri);
purc_atom_t get_endpoint_rid(purcth_endpoint* endpoint);
const char *get_endpoint_uri(purcth_endpoint* endpoint);

void update_endpoint_living_time(purcth_renderer *rdr,
        purcth_endpoint* endpoint);

int check_no_responding_endpoints(purcth_renderer *rdr);

int send_message_to_endpoint(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg);
int send_initial_response(purcth_renderer* rdr, purcth_endpoint* endpoint);
int on_endpoint_message(purcth_renderer* rdr, purcth_endpoint* endpoint,
        const pcrdr_msg *msg);

#endif /* !purc_foil_endpoint_h_ */

