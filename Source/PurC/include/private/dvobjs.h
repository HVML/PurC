/**
 * @file dvojbs.h
 * @author 
 * @date 2021/07/02
 * @brief The interface for dynamic variant objects.
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

#ifndef PURC_PRIVATE_DVOBJS_H
#define PURC_PRIVATE_DVOBJS_H

#include "config.h"
#include "purc-rwstream.h"
#include "purc-variant.h"

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define VARIANT_STRING_NULL         "null"
#define VARIANT_STRING_UNDEFINED    "undefined"
#define VARIANT_STRING_BOOLEAN      "boolean"
#define VARIANT_STRING_NUMBER       "number"
#define VARIANT_STRING_LONGINT      "longint"
#define VARIANT_STRING_ULONGINT     "ulongint"
#define VARIANT_STRING_LONGDOUBLE   "longdouble"
#define VARIANT_STRING_ATOMSTRING   "atomstring"
#define VARIANT_STRING_STRING       "string"
#define VARIANT_STRING_BYTESEQUENCE "bsequence"
#define VARIANT_STRING_DYNAMIC      "dynamic"
#define VARIANT_STRING_NATIVE       "native"
#define VARIANT_STRING_OBJECT       "object"
#define VARIANT_STRING_ARRAY        "array"
#define VARIANT_STRING_SET          "set"

#define STRING_COMP_MODE_CASELESS   "caseless"
#define STRING_COMP_MODE_CASE       "case"
#define STRING_COMP_MODE_REG        "reg"
#define STRING_COMP_MODE_WILDCARD   "wildcard"

#define UNAME_SYSTEM                "operating-system"
#define UNAME_KERNAME               "kernel-name"
#define UNAME_NODE_NAME             "nodename"
#define UNAME_KERRELEASE            "kernel-release"
#define UNAME_KERVERSION            "kernel-version"
#define UNAME_HARDWARE              "hardware-platform"
#define UNAME_PROCESSOR             "processor"
#define UNAME_MACHINE               "machine"
#define UNAME_DEFAULT               "default"
#define UNAME_ALL                   "all"

#define LOCALE_ALL                  "all"
#define LOCALE_CTYPE                "ctype"
#define LOCALE_ADDRESS              "address"
#define LOCALE_COLLATE              "collate"
#define LOCALE_NUMERIC              "numeric"
#define LOCALE_NAME                 "name"
#define LOCALE_TIME                 "time"
#define LOCALE_TELEPHONE            "telephone"
#define LOCALE_MONETARY             "monetary"
#define LOCALE_PAPER                "paper"
#define LOCALE_MESSAGE              "messages"
#define LOCALE_MEASUREMENT          "measurement"
#define LOCALE_IDENTIFICATION       "identification"


// initialize dvobjs module (once)
void pcdvobjs_init_once(void) WTF_INTERNAL;

struct pcinst;

// initialize the dvobjs module for a PurC instance.
void pcdvobjs_init_instance(struct pcinst* inst) WTF_INTERNAL;

// clean up the dvobjs module for a PurC instance.
void pcdvobjs_cleanup_instance(struct pcinst* inst) WTF_INTERNAL;

purc_variant_t pcdvojbs_get_system(void);
purc_variant_t pcdvojbs_get_string(void);
purc_variant_t pcdvojbs_get_logical(void);
purc_variant_t pcdvojbs_get_ejson(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PURC_PRIVATE_DVOBJS_H */

