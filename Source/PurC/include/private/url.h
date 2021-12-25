/*
 * @file url.h
 * @author gengyue
 * @date 2021/12/24
 * @brief the header for url utility.
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
 *
 * Note that the code is copied from GPL'd MiniGUI developed by FMSoft.
 */
#ifndef PURC_PRIVATE_URL_H
#define PURC_PRIVATE_URL_H

#include <string.h>       /* for strcmp */
#include <glib.h>


#define MSPIDER_URL_HTTP_PORT        80
#define MSPIDER_URL_HTTPS_PORT       443
#define MSPIDER_URL_FTP_PORT         21
#define MSPIDER_URL_MAILTO_PORT      25
#define MSPIDER_URL_NEWS_PORT        119
#define MSPIDER_URL_TELNET_PORT      23
#define MSPIDER_URL_GOPHER_PORT      70


/*
 * Values for pcutils_url->flags.
 * Specifies which which action to perform with an URL.
 */
#define URL_Get                 (1 << 0)
#define URL_Post                (1 << 1)
#define URL_ISindex             (1 << 2)
#define URL_Ismap               (1 << 3)
#define URL_RealmAccess         (1 << 4)

#define URL_E2EReload           (1 << 5)
#define URL_ReloadImages        (1 << 6)
#define URL_ReloadPage          (1 << 7)
#define URL_ReloadFromCache     (1 << 8)

#define URL_ReloadIncomplete    (1 << 9)
#define URL_SpamSafe            (1 << 10)

#define URL_MustCache           (1 << 11)
#define URL_IsImage             (1 << 12)
#define URL_OnOtherHost         (1 << 13)


/*
 * Access methods to fields inside mSpiderURL.
 * (non '_'-ended macros MUST use these for initialization sake)
 */
/* these MAY return NULL: */
#define URL_SCHEME_(u)         u->scheme
#define URL_AUTHORITY_(u)      u->authority
#define URL_PATH_(u)           u->path
#define URL_QUERY_(u)          u->query
#define URL_FRAGMENT_(u)       u->fragment
#define URL_HOST_(u)           pcutils_url_hostname(u)
#define URL_DATA_(u)           u->data
#define URL_ALT_(u)            u->alt
#define URL_STR_(u)            pcutils_url_str(u)
#define URL_REFERER_(u)    u->referer
#define URL_TARGET_(u)     u->target
/* these return an integer */
#define URL_PORT_(u)           (URL_HOST(u) ? u->port : u->port)
#define URL_FLAGS_(u)          u->flags
#define URL_ILLEGAL_CHARS_(u)  url->illegal_chars

/*
 * Access methods that always return a string:
 * When the "empty" and "undefined" concepts of RFC-2396 are irrelevant to
 * the caller, and a string is required, use these methods instead:
 */
#define NPTR2STR(p)           ((p) ? (p) : "")
#define URL_SCHEME(u)         NPTR2STR(URL_SCHEME_(u))
#define URL_AUTHORITY(u)      NPTR2STR(URL_AUTHORITY_(u))
#define URL_PATH(u)           NPTR2STR(URL_PATH_(u))
#define URL_QUERY(u)          NPTR2STR(URL_QUERY_(u))
#define URL_FRAGMENT(u)       NPTR2STR(URL_FRAGMENT_(u))
#define URL_HOST(u)           NPTR2STR(URL_HOST_(u))
#define URL_DATA(u)           NPTR2STR(URL_DATA_(u))
#define URL_ALT(u)            NPTR2STR(URL_ALT_(u))
#define URL_STR(u)            NPTR2STR(URL_STR_(u))
#define URL_PORT(u)           URL_PORT_(u)
#define URL_FLAGS(u)          URL_FLAGS_(u)
#define URL_POSX(u)           URL_POSX_(u)
#define URL_POSY(u)           URL_POSY_(u)
#define URL_ILLEGAL_CHARS(u)  URL_ILLEGAL_CHARS_(u)
#define URL_REFERER(u)     NPTR2STR(URL_REFERER_(u))
#define URL_TARGET(u)      NPTR2STR(URL_TARGET_(u))



/* URL-camp compare methods */
#define URL_STRCAMP_EQ(s1,s2) \
   ((!(s1) && !(s2)) || ((s1) && (s2) && !strcmp(s1,s2)))
#define URL_STRCAMP_I_EQ(s1,s2) \
   ((!(s1) && !(s2)) || ((s1) && (s2) && !strcasecmp(s1,s2)))

typedef struct __tag_pcutils_url pcutils_url;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct __tag_pcutils_url {
   GString *url_string;
   const char *buffer;
   const char *scheme;            //
   const char *hostname;          //
   const char *authority;         //
   const char *path;              // These are references only
   const char *query;             // (no need to free them)
   const char *fragment;          //
   unsigned int port;
   unsigned int flags;
   const char *data;              /* POST */
   const char *alt;               /* "alt" text (used by image maps) */
   const char *target;            /* target frame/window */
   unsigned int ismap_url_len;             /* Used by server side image maps */
                                   /* remember position of visited urls */
   unsigned int illegal_chars;             /* number of illegal chars */
   const char *referer;           /* The URL that refered to this one */
};


pcutils_url* pcutils_url_new(const char *url_str, const char *base_url,
                    unsigned int flags);
void pcutils_url_free(pcutils_url *u);
char *pcutils_url_str(const pcutils_url *url);
const char *pcutils_url_hostname(const pcutils_url *u);
pcutils_url* pcutils_url_dup(const pcutils_url *u);
int pcutils_url_cmp(const pcutils_url *A, const pcutils_url *B);
void pcutils_url_set_flags(pcutils_url *u, unsigned int flags);
void pcutils_url_set_data(pcutils_url *u, char *data);
void pcutils_url_set_alt(pcutils_url *u, const char *alt);
void pcutils_url_set_target(pcutils_url *u, const char *target);
void pcutils_url_set_referer(pcutils_url *u, pcutils_url *ref);
void pcutils_url_set_ismap_coords(pcutils_url *u, char *coord_str);
char *pcutils_url_decode_hex_str(const char *str);
char *pcutils_url_encode_hex_str(const char *str);
char *pcutils_url_string_strip_delimiters(char *str);

char *pcutils_url_parse_hex_path(const pcutils_url *u);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PURC_PRIVATE_URL_H */
