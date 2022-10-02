/*
** @file endpoint.h
** @author Vincent Wei
** @date 2022/10/02
** @brief The interface of Endpoint (copied from PurC Midnight Commander).
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

Endpoint* new_endpoint(Renderer* rdr, const char *uri);
int del_endpoint(Renderer* rdr, Endpoint* endpoint, int cause);
int check_no_responding_endpoints(Renderer *rdr);

int send_message_to_endpoint(Renderer* rdr, Endpoint* endpoint,
        const pcrdr_msg *msg);
int send_initial_response(Renderer* rdr, Endpoint* endpoint);
int on_got_message(Renderer* rdr, Endpoint* endpoint,
        const pcrdr_msg *msg);

#endif /* !purc_foil_endpoint_h_ */

