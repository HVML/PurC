/**
 * @file purc-helpers.h
 * @date 2021/03/08
 * @brief This file declares APIs of global helpers.
 *
 * Copyright (C) 2022 FMSoft (http://www.fmsoft.cn)
 *
 * Authors:
 *  Vincent Wei (<https://github.com/VincentWei>), 2022
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
 */

#ifndef PURC_PURC_HELPERS_H
#define PURC_PURC_HELPERS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>

#include "purc-macros.h"
#include "purc-variant.h"
#include "purc-utils.h"
#include "purc-features.h"

PCA_EXTERN_C_BEGIN

/**
 * @defgroup Helpers Global Helper Functions
 * @{
 */

struct pcutils_printbuf {
    char *buf;
    size_t bpos;
    size_t size;
};

PCA_EXPORT int
pcutils_printbuf_init(struct pcutils_printbuf *pb);

PCA_EXPORT struct
pcutils_printbuf *pcutils_printbuf_new(void);

/* As an optimization, printbuf_memappend_fast() is defined as a macro
 * that handles copying data if the buffer is large enough; otherwise
 * it invokes printbuf_memappend() which performs the heavy
 * lifting of realloc()ing the buffer and copying data.
 *
 * Your code should not use printbuf_memappend() directly unless it
 * checks the return code. Use printbuf_memappend_fast() instead.
 */
PCA_EXPORT int
pcutils_printbuf_memappend(struct pcutils_printbuf *p,
        const char *buf, size_t size);

#define pcutils_printbuf_memappend_fast(p, bufptr, bufsize)         \
    do {                                                            \
        if ((p->size - p->bpos) > bufsize)  {                       \
            memcpy(p->buf + p->bpos, (bufptr), bufsize);            \
            p->bpos += bufsize;                                     \
            p->buf[p->bpos] = '\0';                                 \
        }                                                           \
        else {                                                      \
            pcutils_printbuf_memappend(p, (bufptr), bufsize);       \
        }                                                           \
    } while (0)

#define pcutils_printbuf_length(p) ((p)->bpos)

#define _printbuf_check_literal(mystr) ("" mystr)

#define pcutils_printbuf_strappend(pb, str)                         \
    pcutils_printbuf_memappend((pb), _printbuf_check_literal(str),  \
            sizeof(str) - 1)

PCA_EXPORT int
pcutils_printbuf_memset(struct pcutils_printbuf *pb,
        ssize_t offset, int charvalue, size_t len);

PCA_EXPORT int
pcutils_printbuf_shrink(struct pcutils_printbuf *pb, size_t len);

PCA_EXPORT int
pcutils_printbuf_format(struct pcutils_printbuf *p, const char *msg, ...)
    PCA_ATTRIBUTE_PRINTF(2, 3);

PCA_EXPORT void
pcutils_printbuf_reset(struct pcutils_printbuf *p);

PCA_EXPORT char *
pcutils_printbuf_delete(struct pcutils_printbuf *p, bool keep_buf);

PCA_EXPORT bool
purc_is_valid_host_name(const char *host_name);

PCA_EXPORT bool
purc_is_valid_app_name(const char *app_name);

PCA_EXPORT bool
purc_is_valid_endpoint_name(const char *endpoint_name);

PCA_EXPORT int
purc_extract_host_name(const char *endpoint, char *buff);

PCA_EXPORT int
purc_extract_app_name(const char *endpoint, char *buff);

PCA_EXPORT int
purc_extract_runner_name(const char *endpoint, char *buff);

PCA_EXPORT char *
purc_extract_host_name_alloc(const char *endpoint);

PCA_EXPORT char *
purc_extract_app_name_alloc(const char *endpoint);

PCA_EXPORT char *
purc_extract_runner_name_alloc(const char *endpoint);

PCA_EXPORT int
purc_assemble_endpoint_name_ex(const char *host_name, const char *app_name,
        const char *runner_name, char *buff, size_t sz);

static inline int
purc_assemble_endpoint_name(const char *host_name, const char *app_name,
        const char *runner_name, char *buff) {
    return purc_assemble_endpoint_name_ex (host_name,
        app_name, runner_name, buff, PURC_LEN_ENDPOINT_NAME + 1);
}

PCA_EXPORT char *
purc_assemble_endpoint_name_alloc(const char *host_name,
        const char *app_name, const char *runner_name);

PCA_EXPORT char *
purc_assemble_hvml_uri_alloc(const char *host_name,
        const char *app_name, const char *runner_name, const char *page_name);

/**
 * Assemble an HVML URI from components to the specified buffer:
 *
 *      hvml://<host>/<app>/<runner>/[<page>]
 *
 * Note the buffer should be large enough to hold the URI.
 *
 * Since: 0.1.0
 */
PCA_EXPORT size_t
purc_hvml_uri_assemble(char *uri, const char *host, const char *app,
        const char *runner, const char* group, const char *page);

/**
 * Assemble an HVML URI from components to an newly-allocated buffer:
 *
 *      hvml://<host>/<app>/<runner>/[<page>]
 *
 * Since: 0.1.0
 */
PCA_EXPORT char *
purc_hvml_uri_assemble_alloc(const char *host, const char *app,
        const char *runner, const char* group, const char *page);

/**
 * Break down an HVML URI in the following pattern to component buffers:
 *
 *  hvml://<host>/<app>/<runner>/<group>/<page>[?key1=value1&key2=value2]
 *
 * Note that we use `-` for `<group>` when there is no group name.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
purc_hvml_uri_split(const char *uri,
        char *host, char *app, char *runner, char *group, char *page);

/**
 * Break down an HVML URI in the following pattern:
 *
 *  hvml://<host>/<app>/<runner>/<group>/<page>[?key1=value1&key2=value2]
 *
 * Note that we use `-` for `<group>` when there is no group name.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
purc_hvml_uri_split_alloc(const char *uri,
        char **host, char **app, char **runner, char **group, char **page);

/**
 * Copy the value of the specified key in a HVML URI to the bufffer if found.
 * Since: 0.1.0
 */
PCA_EXPORT bool
purc_hvml_uri_get_query_value(const char *uri, const char *key,
        char *value_buff);

/**
 * Copy the value of the specified key in a HVML URI to the newly allocated
 * buffer if found.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
purc_hvml_uri_get_query_value_alloc(const char *uri, const char *key,
        char **value_buff);

/**
 * Check whether a string is a valid token.
 *
 * @param token: the pointer to the token string.
 * @param max_len: The maximal possible length of the token string.
 *
 * Checks whether a token string is valid. According to PurCMC protocal,
 * the runner name should be a valid token.
 *
 * Note that a string with a length longer than \a max_len will
 * be considered as an invalid token.
 *
 * Returns: true for a valid token, otherwise false.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
purc_is_valid_token(const char *token, int max_len);

/**
 * Check whether a string is a valid loose token.
 *
 * @param token: the pointer to the token string.
 * @param max_len: The maximal possible length of the token string.
 *
 * Checks whether a loose token string is valid. According to PurCMC protocal,
 * the identifier should be a valid loose token. A loose token can contain
 * one or more `-` ASCII characters.
 *
 * Note that a string with a length longer than \a max_len will
 * be considered as an invalid loose token.
 *
 * Returns: true for a valid loose token, otherwise false.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
purc_is_valid_loose_token(const char *token, int max_len);


/**
 * Check whether a string is a valid html attribute.
 *
 * @param attrname: the pointer to the attribute's name string.
 *
 * Checks whether a html attribute name string is valid. According to PurCMC protocal,
 * the identifier should be a html attribute name. can contain
 * one or more `-` ':' ASCII characters.
 *
 *
 * Returns: true for a valid html token, otherwise false.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool 
purc_is_valid_html_attribute(const char* attrname);

/**
 * Generate a global unique unsigned long long integer.
 *
 * Returns: the unique integer.
 *
 * Since: 0.9.22
 */
PCA_EXPORT unsigned long long
purc_generate_unique_ulongint(void);

/**
 * Generate an unique identifier.
 *
 * @param id_buff: the buffer to save the identifier.
 * @param prefix: the prefix used for the identifier.
 *
 * Generates a unique id; the size of \a id_buff should be at least 64 long.
 *
 * Returns: none.
 *
 * Since: 0.1.0
 */
PCA_EXPORT void
purc_generate_unique_id(char *id_buff, const char *prefix);

/**
 * Generate an unique MD5 identifier.
 *
 * @param id_buff: the buffer to save the identifier.
 * @param prefix: the prefix used for the identifier.
 *
 * Generates a unique id by using MD5 digest algorithm.
 * The size of \a id_buff should be at least 33 bytes long.
 *
 * Returns: none.
 *
 * Since: 0.1.0
 */
PCA_EXPORT void
purc_generate_md5_id(char *id_buff, const char *prefix);

/**
 * Check whether a string is a valid unique identifier.
 *
 * @param id: the unique identifier.
 *
 * Checks whether a unique id is valid.
 *
 * Returns: none.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
purc_is_valid_unique_id(const char *id);

/**
 * Check whether a string is a valid MD5 identifier.
 *
 * @param id: the unique identifier.
 *
 * Checks whether a unique identifier is valid.
 *
 * Returns: none.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
purc_is_valid_md5_id(const char *id);

/**
 * Get monotomic time after the specific seconds.
 *
 * @param seconds: the number of seconds.
 *
 * Calculates monotomic time after the specific seconds.
 *
 * Returns: the monotomic time after the specific seconds.
 *
 * Since: 0.2.0
 */
PCA_EXPORT time_t
purc_monotonic_time_after(time_t seconds);

/**
 * Get the elapsed seconds.
 *
 * @param ts1: the earlier time.
 * @param ts2 (nullable): the later time.
 *
 * Calculates the elapsed seconds between two times.
 * If \a ts2 is NULL, the function uses the current time.
 *
 * Returns: the elapsed time in seconds (a double).
 *
 * Since: 0.1.0
 */
PCA_EXPORT double
purc_get_elapsed_seconds(const struct timespec *ts1,
        const struct timespec *ts2);

/**
 * Gets the elapsed milliseconds.
 *
 * @param ts1: the earlier time.
 * @param ts2 (nullable): the later time.
 *
 * Calculates the elapsed milliseconds between two times.
 * If \a ts2 is NULL, the function uses the current time.
 *
 * Returns: the elapsed time in milliseconds (an int64_t value).
 *
 * Since: 0.9.6
 */
PCA_EXPORT int64_t
purc_get_elapsed_milliseconds(const struct timespec *ts1,
        const struct timespec *ts2);

/**
 * Gets the elapsed milliseconds alternatively.
 *
 * @param t1: the earlier time in seconds.
 * @param ts2 (nullable): the later time.
 *
 * Calculates the elapsed milliseconds between two times.
 * If \a ts2 is NULL, the function uses the current time.
 *
 * Returns: the elapsed time in milliseconds (an int64_t value).
 *
 * Since: 0.9.6
 */
PCA_EXPORT int64_t
purc_get_elapsed_milliseconds_alt(time_t t1,
        const struct timespec *ts2);

/**
 * Loads the whole contents from a file.
 *
 * @param file: the file name.
 * @param ts2 (nullable): the length of the contents in bytes.
 *
 * Loads the whole contents from the specific file.
 *
 * Returns: The pointer to the loaded contents, @NULL for error.
 *
 * Since: 0.2.0
 */
char *purc_load_file_contents(const char *file, size_t *length);

#define PURC_ENVV_LOG_ENABLE        "PURC_LOG_ENABLE"
#define PURC_ENVV_LOG_SYSLOG        "PURC_LOG_SYSLOG"

#define PURC_LOG_FILE_PATH_FORMAT   "/var/tmp/purc-%s-%s.log"

// TODO for Windows:
// #define LOG_FILE_PATH_FORMAT    "C:\\tmp\\purc-%s\\%s.log"

typedef enum {
    PURC_LOG_FACILITY_FILE = 0,
    PURC_LOG_FACILITY_STDOUT,
    PURC_LOG_FACILITY_STDERR,
    PURC_LOG_FACILITY_SYSLOG,
} purc_log_facility_k;

typedef enum {
    PURC_LOG_first = 0,

#define PURC_LOG_LEVEL_EMERG    "EMRG"
    PURC_LOG_EMERG = PURC_LOG_first,    /* system is unusable */
#define PURC_LOG_LEVEL_ALERT    "ALRT"
    PURC_LOG_ALERT,                     /* action must be taken immediately */
#define PURC_LOG_LEVEL_CRIT     "CRIT"
    PURC_LOG_CRIT,                      /* critical conditions */
#define PURC_LOG_LEVEL_ERR      "ERRO"
    PURC_LOG_ERR,                       /* error conditions */
#define PURC_LOG_LEVEL_WARNING  "WARN"
    PURC_LOG_WARNING,                   /* warning conditions */
#define PURC_LOG_LEVEL_NOTICE   "NOTI"
    PURC_LOG_NOTICE,                    /* normal, but significant, condition */
#define PURC_LOG_LEVEL_INFO     "INFO"
    PURC_LOG_INFO,                      /* informational message */
#define PURC_LOG_LEVEL_DEBUG    "DEBG"
    PURC_LOG_DEBUG,                     /* debug-level message */

    /* XXX: change this if you append a new type. */
    PURC_LOG_last = PURC_LOG_DEBUG,
} purc_log_level_k;

#define PURC_LOG_LEVEL_nr   (PURC_LOG_last - PURC_LOG_first + 1)

#define PURC_LOG_MASK_EMERG     (0x01 << PURC_LOG_EMERG)
#define PURC_LOG_MASK_ALERT     (0x01 << PURC_LOG_ALERT)
#define PURC_LOG_MASK_CRIT      (0x01 << PURC_LOG_CRIT)
#define PURC_LOG_MASK_ERR       (0x01 << PURC_LOG_ERR)
#define PURC_LOG_MASK_WARNING   (0x01 << PURC_LOG_WARNING)
#define PURC_LOG_MASK_NOTICE    (0x01 << PURC_LOG_NOTICE)
#define PURC_LOG_MASK_INFO      (0x01 << PURC_LOG_INFO)
#define PURC_LOG_MASK_DEBUG     (0x01 << PURC_LOG_DEBUG)

#define PURC_LOG_MASK_DEFAULT   (                               \
                                PURC_LOG_MASK_EMERG     |       \
                                PURC_LOG_MASK_ALERT     |       \
                                PURC_LOG_MASK_ERR       |       \
                                PURC_LOG_MASK_WARNING   |       \
                                PURC_LOG_MASK_NOTICE    |       \
                                0)

#define PURC_LOG_MASK_ALL       ((unsigned)-1)

/**
 * Sets the log levels and the log facility for the current PurC instance.
 *
 * @param levels: The log levels enabled for this instance.
 * @param facility: The log facility.
 *
 * Returns: @true for success, otherwise @false.
 *
 * Since: 0.9.12
 */
PCA_EXPORT bool
purc_enable_log_ex(unsigned levels, purc_log_facility_k facility);

/**
 * Gets the log levels of the current PurC instance.
 *
 * Returns: The log levels.
 *
 * Since: 0.9.12
 */
PCA_EXPORT unsigned
purc_get_log_levels(void);

/**
 * Enable or disable the log facility for the current PurC instance.
 *
 * @param the_most: The most log level to enable.
 * @param use_syslog: @true to use syslog, @false to use log file.
 *
 * Returns: @true for success, otherwise @false.
 *
 * Since: 0.1.0 (changed in 0.9.12)
 */
static inline bool
purc_enable_log(bool enable, bool use_syslog) {
    return purc_enable_log_ex(enable ? PURC_LOG_MASK_DEFAULT : 0,
            use_syslog ? PURC_LOG_FACILITY_SYSLOG : PURC_LOG_FACILITY_FILE);
}

/**
 * Log a message with level.
 *
 * @param level: the log level of the message.
 * @param msg: the message or the format string.
 *
 * Returns: none.
 *
 * Since: 0.9.12
 */
PCA_EXPORT void
purc_log_with_level(purc_log_level_k level, const char *msg, va_list ap)
    PCA_ATTRIBUTE_PRINTF(2, 0);

/**
 * Log a message with a specified tag.
 *
 * @param tag: the tag of the message.
 * @param msg: the message or the format string.
 *
 * Returns: none.
 *
 * Since: 0.1.0 (changed in 0.9.12).
 */
PCA_EXPORT void
purc_log_with_tag(purc_log_level_k level, const char* tag,
        const char *msg, va_list ap)
    PCA_ATTRIBUTE_PRINTF(3, 0);

/**
 * Log a message with a specified tag.
 *
 * @param tag: the tag of the message.
 * @param msg: the message or the format string.
 *
 * Returns: none.
 *
 * Since: 0.1.0 (changed in 0.9.12).
 */
PCA_ATTRIBUTE_PRINTF(3, 4)
static inline void
purc_log_with_tag_f(purc_log_level_k level, const char* tag,
        const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    purc_log_with_tag(level, tag, msg, ap);
    va_end(ap);
}

/**
 * Log a debugging message.
 *
 * @param msg: the message or the format string.
 *
 * Returns: none.
 *
 * Since: 0.1.0
 */
PCA_ATTRIBUTE_PRINTF(1, 2)
static inline void
purc_log_debug(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    purc_log_with_level(PURC_LOG_DEBUG, msg, ap);
    va_end(ap);
}

/**
 * Log an information message.
 *
 * @param msg: the message or the format string.
 *
 * Returns: none.
 *
 * Since: 0.1.0
 */
PCA_ATTRIBUTE_PRINTF(1, 2)
static inline void
purc_log_info(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    purc_log_with_level(PURC_LOG_INFO, msg, ap);
    va_end(ap);
}

/**
 * Log a notice message.
 *
 * @param msg: the message or the format string.
 *
 * Returns: none.
 *
 * Since: 0.9.12
 */
PCA_ATTRIBUTE_PRINTF(1, 2)
static inline void
purc_log_notice(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    purc_log_with_level(PURC_LOG_NOTICE, msg, ap);
    va_end(ap);
}

/**
 * Log a warning message.
 *
 * @param msg: the message or the format string.
 *
 * Returns: none.
 *
 * Since: 0.1.0
 */
PCA_ATTRIBUTE_PRINTF(1, 2)
static inline void
purc_log_warn(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    purc_log_with_level(PURC_LOG_WARNING, msg, ap);
    va_end(ap);
}

/**
 * Log an error message.
 *
 * @param msg: the message or the format string.
 *
 * Returns: none.
 *
 * Since: 0.1.0
 */
PCA_ATTRIBUTE_PRINTF(1, 2)
static inline void
purc_log_error(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    purc_log_with_level(PURC_LOG_ERR, msg, ap);
    va_end(ap);
}

/**
 * Make an object from a URL query string which conforms to RFC 1738
 * or RFC 3986.
 *
 * @param query: The pointer to a null-terminated query string.
 * @param rfc1738: Use RFC 1738 ('+' for space) or RFC 3986;
 *       %true for RFC 1738.
 *
 * Returns: An object variant on success, %PURC_VARIANT_INVALID on failure.
 *
 * Since: 0.9.2
 */
PCA_EXPORT purc_variant_t
purc_make_object_from_query_string(const char *query, bool rfc1738);

/**
 * Encode a string for URL query which conforms to RFC 1738
 * or RFC 3986.
 *
 * @param query: The pointer to a null-terminated string.
 * @param rfc1738: Use RFC 1738 ('+' for space) or RFC 3986;
 *       %true for RFC 1738.
 *
 * Returns: A pointer to a new encoded string. Note that the caller should
 *  free the string by calling `free()`.
 *
 * Since: 0.9.22
 */
PCA_EXPORT char *
purc_url_encode_alloc(const char *string, bool rfc1738);

/**
 * Decode a URL-encoded string which conforms to RFC 1738
 * or RFC 3986.
 *
 * @param query: The pointer to a null-terminated URL-encoded string.
 * @param rfc1738: Use RFC 1738 ('+' for space) or RFC 3986;
 *       %true for RFC 1738.
 *
 * Returns: A pointer to a new encoded string. Note that the caller should
 *  free the string by calling `free()`.
 *
 * Since: 0.9.22
 */
PCA_EXPORT char *
purc_url_decode_alloc(const char *string, bool rfc1738);

struct purc_page_ostack;
typedef struct purc_page_ostack *purc_page_ostack_t;

/** The page owner information to be maintained in the page owner stack. */
struct purc_page_owner {
    /* The pointer to the session */
    void    *sess;
    /* The handle to the coroutine */
    uint64_t corh;
};

/**
 * Creates a new page owner stack for the specified page identifier.
 *
 * @param page_map: The map from the page identifier to the new owner stack.
 * @param id: The page identifier.
 * @param page: The pointer to the page.
 *
 * Returns: The pointer to a new struct @purc_page_ostack; NULL for failure.
 *
 * Since: 0.9.10
 */
PCA_EXPORT purc_page_ostack_t
purc_page_ostack_new(pcutils_kvlist_t page_map, const char *id, void *page);

/**
 * Destroys a page owner stack.
 *
 * @param page_map: The map from the page identifier to the new owner stack.
 * @param ostack: The page owner stack to destroy.
 *
 * Returns: None.
 *
 * Since: 0.9.10
 */
PCA_EXPORT void
purc_page_ostack_delete(pcutils_kvlist_t page_map, purc_page_ostack_t ostack);

/**
 * Registers a new page owner.
 *
 * @param ostack: The pointer to a page owner stack.
 * @param owner: A struct containing the owner to register.
 *      See @purc_page_owner.
 *
 * Returns: The owner should be suppressed; it's valid only the handle to
 *      the coroutine is not zero.
 *
 * Since: 0.9.10
 */
PCA_EXPORT struct purc_page_owner
purc_page_ostack_register(purc_page_ostack_t ostack,
        struct purc_page_owner owner);

/**
 * Revokes a page owner.
 *
 * @param ostack: The pointer to a page owner stack.
 * @param owner: A struct containing the owner to revoke.
 *      See @purc_page_owner.
 *
 * Returns: The owner should be reloaded; it's valid only the handle to
 *      the coroutine is not zero.
 *
 * Since: 0.9.10
 */
PCA_EXPORT struct purc_page_owner
purc_page_ostack_revoke(purc_page_ostack_t ostack,
        struct purc_page_owner owner);

/**
 * Revokes all page owners belonging to the specific session.
 *
 * @param ostack: The pointer to a page owner stack.
 *
 * Returns: The owner should be reloaded; it's valid only the handle to
 *      the coroutine is not zero.
 *
 * Since: 0.9.10
 */
PCA_EXPORT struct purc_page_owner
purc_page_ostack_revoke_session(purc_page_ostack_t ostack, void *sess);

/**
 * Retrieves the page identifier from a page owner stack.
 *
 * @param ostack: The pointer to a page owner stack.
 *
 * Returns: The pointer to the page identifier.
 *
 * Since: 0.9.10
 */
PCA_EXPORT const char *
purc_page_ostack_get_id(purc_page_ostack_t ostack);

/**
 * Retrieves the pointer to the page from a page owner stack.
 *
 * @param ostack: The pointer to a page owner stack.
 *
 * Returns: The pointer to the page.
 *
 * Since: 0.9.10
 */
PCA_EXPORT void *
purc_page_ostack_get_page(purc_page_ostack_t ostack);

/**
 * Retrieves the birth time of the page.
 *
 * @param ostack: The pointer to a page owner stack.
 *
 * Returns: A struct timespec representing the birth time of the page.
 *
 * Since: 0.9.10
 */
PCA_EXPORT struct timespec
purc_page_ostack_get_birth(purc_page_ostack_t ostack);

#define PURC_PREFIX_PLAINWIN         "plainwin:"
#define PURC_PREFIX_WIDGET           "widget:"
#define PURC_SEP_PAGE_TYPE           ':'
#define PURC_SEP_GROUP_NAME          '@'
#define PURC_SEP_WORKSPACE_NAME      '/'

#define PURC_MAX_PLAINWIN_ID     \
    (sizeof(PURC_PREFIX_PLAINWIN) + PURC_LEN_IDENTIFIER * 2 + 2)
#define PURC_MAX_WIDGET_ID     \
    (sizeof(PURC_PREFIX_WIDGET) + PURC_LEN_IDENTIFIER * 2 + 2)

/**
 * Checks and makes page identifier for a plain window specified by
 * the pattern 'name[@group]'.
 *
 * @param id_buf: The pointer to a buffer to store the page identifier.
 * @param name_buf: The pointer to a buffer to store the page name.
 * @param name_group: The pointer to a string contains the name and group
 *  in the pattern 'name[@group]'.
 *
 * Returns: The pointer to the group part; NULL for no group part, and
 *  @PURC_INVPTR for bad name or group.
 *
 * Since: 0.9.10
 */
const char *purc_check_and_make_plainwin_id(char *id_buf, char *name_buf,
        const char *name_group);

/**
 * Checks and makes page identifier for a plain window specified by
 * the pattern 'name[@group]'.
 *
 * @param id_buf: The pointer to a buffer to store the page identifier.
 * @param name_buf: The pointer to a buffer to store the page name.
 * @param name_group: The pointer to a string contains the name and group
 *  in the pattern 'name[@group]'.
 *
 * Returns: The pointer to the group part; NULL for no group part, and
 *  @PURC_INVPTR for bad name or group.
 *
 * Since: 0.9.10
 */
const char *purc_check_and_make_plainwin_id(char *id_buf, char *name_buf,
        const char *name_group);

/**
 * Checks and makes page identifier for a widget specified by
 * the pattern 'name@group'.
 *
 * @param id_buf: The pointer to a buffer to store the page identifier.
 * @param name_buf: The pointer to a buffer to store the page name.
 * @param name_group: The pointer to a string contains the name and group
 *  in the pattern 'name[@group]'.
 *
 * Returns: The pointer to the group part; NULL for bad name or group.
 *
 * Since: 0.9.10
 */
const char *purc_check_and_make_widget_id(char *id_buf, char *name_buf,
        const char *name_group);

/**
 * Checks whether a given identifier is a valid CSS identifier.
 *
 * @param id: The pointer the given identifier.
 *
 * Returns: `true` for a valid CSS identifier, otherwise `false`.
 *
 * Since: 0.9.10
 */
PCA_EXPORT bool
purc_is_valid_css_identifier(const char *id);

/**
 * Splits a page identifier in pattern
 *      '<type>:[<name>[@[<workspace>/]<group>]]'
 * to name, workspace, and page group identifier, and returns the page type.
 *
 * @param page_id: The pointer to a string which contains the page identifier.
 * @param type_buf: The pointer to a buffer to store the page type; nullable.
 * @param name_buf: The pointer to a buffer to store the page name; nullable.
 * @param workspace_buf: The pointer to a buffer to store the workspace;
 *      nullable.
 * @param group_buf: The pointer to a buffer to store the page group
 *      identifier; nullable.
 *
 * Returns: a value (the page type identifier) greater than or equal to 0
 *      for success; a value less than 0 for bad page identifier.
 *
 * Since: 0.9.18
 */
int
purc_split_page_identifier(const char *page_id, char *type_buf,
        char *name_buf, char *workspace_buf, char *group_buf);

struct purc_screen_info {
    /** The number of horinzontal physical pixels (dots) of screen. */
    int width;

    /** The number of vertial physical pixels (dots) of screen. */
    int height;

    /** The number of dots per inch. */
    float dpi;

    /** The ratio of logical pixels to physical pixels. */
    float density;
};

struct purc_window_geometry {
    /* the x and y coordinates of the window in dots. */
    int x, y;

    /* the width and height of the window in dots. */
    int width, height;
};

/**
 * Evaluates the geometry of a standalone window from the styles.
 *
 * @param styles: The styles like `window-size:50%;window-position:center;`.
 * @param screen_info: The screen parameters (width, height, and density).
 * @param geometry: The pointer to a buffer to store the evaluted window
 *  geometry.
 *
 * Returns: 0 for success; a value less than 0 for bad styles.
 *
 * Since: 0.9.18
 */
PCA_EXPORT int
purc_evaluate_standalone_window_geometry_from_styles(const char *styles,
        const struct purc_screen_info *screen_info,
        struct purc_window_geometry *geometry);

typedef enum {
    PURC_WINDOW_TRANSITION_FUNCTION_first = 0,

#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_NONE            "None"
    PURC_WINDOW_TRANSITION_FUNCTION_NONE = PURC_WINDOW_TRANSITION_FUNCTION_first,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_LINEAR          "Linear"
    PURC_WINDOW_TRANSITION_FUNCTION_LINEAR,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INQUAD          "InQuad"
    PURC_WINDOW_TRANSITION_FUNCTION_INQUAD,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTQUAD         "OutQuad"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTQUAD,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTQUAD       "InOutQuad"
    PURC_WINDOW_TRANSITION_FUNCTION_INOUTQUAD,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINQUAD       "OutInQuad"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTINQUAD,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INCUBIC         "InCubic"
    PURC_WINDOW_TRANSITION_FUNCTION_INCUBIC,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTCUBIC        "OutCubic"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTCUBIC,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTCUBIC      "InOutCubic"
    PURC_WINDOW_TRANSITION_FUNCTION_INOUTCUBIC,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINCUBIC      "OutInCubic"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTINCUBIC,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INQUART         "InQuart"
    PURC_WINDOW_TRANSITION_FUNCTION_INQUART,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTQUART        "OutQuart"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTQUART,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTQUART      "InOutQuart"
    PURC_WINDOW_TRANSITION_FUNCTION_INOUTQUART,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINQUART      "OutInQuart"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTINQUART,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INQUINT         "InQuint"
    PURC_WINDOW_TRANSITION_FUNCTION_INQUINT,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTQUINT        "OutQuint"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTQUINT,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTQUINT      "InOutQuint"
    PURC_WINDOW_TRANSITION_FUNCTION_INOUTQUINT,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINQUINT      "OutInQuint"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTINQUINT,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INSINE          "InSine"
    PURC_WINDOW_TRANSITION_FUNCTION_INSINE,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTSINE         "OutSine"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTSINE,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTSINE       "InOutSine"
    PURC_WINDOW_TRANSITION_FUNCTION_INOUTSINE,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINSINE       "OutInSine"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTINSINE,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INEXPO          "InExpo"
    PURC_WINDOW_TRANSITION_FUNCTION_INEXPO,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTEXPO         "OutExpo"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTEXPO,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTEXPO       "InOutExpo"
    PURC_WINDOW_TRANSITION_FUNCTION_INOUTEXPO,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINEXPO       "OutInExpo"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTINEXPO,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INCIRC          "InCirc"
    PURC_WINDOW_TRANSITION_FUNCTION_INCIRC,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTCIRC         "OutCirc"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTCIRC,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTCIRC       "InOutCirc"
    PURC_WINDOW_TRANSITION_FUNCTION_INOUTCIRC,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINCIRC       "OutInCirc"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTINCIRC,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INELASTIC       "InElastic"
    PURC_WINDOW_TRANSITION_FUNCTION_INELASTIC,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTELASTIC      "OutElastic"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTELASTIC,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTELASTIC    "InOutElastic"
    PURC_WINDOW_TRANSITION_FUNCTION_INOUTELASTIC,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINELASTIC    "OutInElastic"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTINELASTIC,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INBACK          "InBack"
    PURC_WINDOW_TRANSITION_FUNCTION_INBACK,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTBACK         "OutBack"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTBACK,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTBACK       "InOutBack"
    PURC_WINDOW_TRANSITION_FUNCTION_INOUTBACK,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINBACK       "OutInBack"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTINBACK,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INBOUNCE        "InBounce"
    PURC_WINDOW_TRANSITION_FUNCTION_INBOUNCE,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTBOUNCE       "OutBounce"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTBOUNCE,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INOUTBOUNCE     "InOutBounce"
    PURC_WINDOW_TRANSITION_FUNCTION_INOUTBOUNCE,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTINBOUNCE     "OutInBounce"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTINBOUNCE,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_INCURVE         "InCurve"
    PURC_WINDOW_TRANSITION_FUNCTION_INCURVE,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_OUTCURVE        "OutCurve"
    PURC_WINDOW_TRANSITION_FUNCTION_OUTCURVE,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_SINECURVE       "SineCurve"
    PURC_WINDOW_TRANSITION_FUNCTION_SINECURVE,
#define PURC_WINDOW_TRANSITION_FUNCTION_NAME_COSINECURVE     "CosineCurve"
    PURC_WINDOW_TRANSITION_FUNCTION_COSINECURVE,

    /* XXX: change this if you append a new function. */
    PURC_WINDOW_TRANSITION_FUNCTION_last = PURC_WINDOW_TRANSITION_FUNCTION_COSINECURVE,
} purc_window_transition_function;

#define PURC_NR_WINDOW_TRANSITION_FUNCTION \
    (PURC_WINDOW_TRANSITION_FUNCTION_last - PURC_WINDOW_TRANSITION_FUNCTION_first + 1)

struct purc_window_transition {
    purc_window_transition_function move_func;
    uint32_t move_duration;
};

/**
 * Evaluates the transition of a standalone window from the styles.
 *
 * @param styles: The styles like `window-transition-move: linear 200`.
 * @param transition: The pointer to a buffer to store the evaluted window
 *  transition.
 *
 * Returns: 0 for success; a value less than 0 for bad styles.
 *
 * Since: 0.9.18
 */
PCA_EXPORT int
purc_evaluate_standalone_window_transition_from_styles(const char *styles,
      struct purc_window_transition *transition);


/**
 * Checks if a UNIX domain socket alive.
 *
 * @param path: The path to the UNIX domain socket.
 *
 * Returns: 0 for success and -1 for failure.
 *
 * Notes: Only available on UNIX-like systems.
 *
 * Since: 0.9.18
 */
int purc_check_unix_socket(const char *path);

#define PURC_MAX_LEN_HOSTNAME       1023

/**
 * Gets the hostname of the local machine.
 *
 * @param name_buf: The buffer to return the hostname. Note that
 *  the size of the buffer should be at least (PURC_MAX_LEN_HOSTNAME + 1).
 *
 * Returns: The pointer to the buffer.
 *
 * Since: 0.9.18
 */
PCA_EXPORT char *
purc_get_local_hostname(char *name_buf);

/**
 * Gets the hostname of the local machine and returns it in a newly
 * allocated string.
 *
 * Returns: The pointer to the new buffer which contains the hostname;
 *  NULL for failure.
 *  Note that the caller should free the buffer by calling `free()`.
 *
 * Since: 0.9.18
 */
PCA_EXPORT char *
purc_get_local_hostname_alloc(void);

#if PCA_ENABLE_DNSSD

struct purc_dnssd_conn;

typedef void (*dnssd_on_register_reply)(struct purc_dnssd_conn *dnssd,
        void *reg_handle, unsigned int flags, int err_code,
        const char *name, const char *reg_type, const char *domain,
        void *ctxt);

typedef void (*dnssd_on_service_discovered)(struct purc_dnssd_conn *dnssd,
        void *service_handle,
        unsigned int flags, uint32_t if_index, int error_code,
        const char *service_name, const char *reg_type, const char *hostname,
        uint16_t port, uint16_t len_txt_record, const char *txt_record,
        void *ctxt);

/**
 * Connects to the mDNS responder daemon.
 *
 * Returns: A pointer to `struct purc_dnssd_conn` representing the connection
 * to the mDNS Responder daemon; NULL for failure.
 *
 * Since: 0.9.18
 */
struct purc_dnssd_conn *
purc_dnssd_connect(dnssd_on_register_reply register_reply_cb,
        dnssd_on_service_discovered service_discovered_cb, void *context);

/**
 * Disconnects from the mDNS responder daemon.
 *
 * Since: 0.9.18
 */
void
purc_dnssd_disconnect(struct purc_dnssd_conn *dnssd);

/**
 * Returns the file descriptor of the connection to the mDNS responder daemon.
 *
 * Since: 0.9.18
 */
int purc_dnssd_fd(struct purc_dnssd_conn *dnssd);

/**
 * Registers a service.
 *
 * @param dnssd: The connection to the mDNS responder daemon.
 * @param service_name: The name of the service (nullable).
 *  It it is NULL, this function will use hostname of the machine
 *  as the service name.
 * @param reg_type: The type of the service to register, like `_http._tcp`.
 * @param domain: If non-NULL, specifies the domain on which to advertise
 *  the service. Most applications will not specify a domain, instead
 *  automatically registering in the default domain(s).
 * @param host: If non-NULL, specifies the SRV target host name.
 *  Most applications will not specify a host, instead automatically using
 * the machine's default host name(s). Note that specifying a non-NULL host
 * does NOT create an address record for that host - the application is
 * responsible for ensuring that the appropriate address record exists,
 * or creating it via purc_dnssd_serivce_register_record().
 * @param port: The port on which the service accepts connections.
 * @param txt_record_values: A string array contains the TXT record values
 *  of the service to register. NULL for none.
 * @nr_txt_records: The number of valid TXT record values to register;
 *  zero for none.
 *
 * Returns: The handle of the registered service; NULL for error.
 *
 * Since: 0.9.18
 */
void *purc_dnssd_register_service(struct purc_dnssd_conn *dnssd,
        const char *service_name, const char *reg_type,
        const char *domain, const char *hostname, uint16_t port,
        const char *txt_record_values[], size_t nr_txt_record_values);

/**
 * Revokes a service registered previously.
 *
 * @param dnssd: The connection to the mDNS responder daemon.
 * @param service_handle: The handle of a registered service returned by
 *  purc_dnssd_register_local_service().
 *
 * Returns: 0 for success and -1 for failure.
 *
 * Since: 0.9.18
 */
void purc_dnssd_revoke_service(struct purc_dnssd_conn *dnssd,
        void *service_handle);

/**
 * Starts browsing services on network.
 *
 * @param dnssd: The connection to the mDNS responder daemon.
 * @param reg_type: The service type to browser.
 * @param domain: The domain to browser.
 *
 * Returns: A handle of the browsing; NULL for failure.
 *
 * Since: 0.9.18
 */
void *purc_dnssd_start_browsing(struct purc_dnssd_conn *dnssd,
        const char *reg_type, const char *domain);

/**
 * Stops browsing services on network.
 *
 * @param dnssd: The connection to the mDNS responder daemon.
 * @param browsing_handle: A browsing handle returned by
 *  purc_dnssd_start_browsing().
 *
 * Since: 0.9.18
 */
void purc_dnssd_stop_browsing(struct purc_dnssd_conn *dnssd,
        void *browsing_handle);

/**
 * Gets a reply from mDNS responder daemon and processes the browsing result.
 * When there is a reply, this function will call the callback to handle
 * a newly found service.
 *
 * @param dnssd: The connection to the mDNS responder daemon.
 *
 * Note: This call will block until a reply from the daemon is received.
 * Use purc_dnssd_fd() in conjunction with a run loop or select()
 * to determine the presence of a reply from the daemon before calling
 * this function to process the reply without blocking.
 *
 * Returns: 0 for success and -1 for failure.
 *
 * Since: 0.9.18
 */
int purc_dnssd_process_result(struct purc_dnssd_conn *dnssd);

#endif /* PCA_ENABLE_DNSSD */

/**@}*/

PCA_EXTERN_C_END

/**
 * @addtogroup Helpers
 *  @{
 */

/**
 * Convert a string to lowercases in place.
 *
 * @param name: the pointer to a name string(not nullable).
 *
 * Converts a name string lowercase in place.
 *
 * Returns: the length of the name string.
 *
 * Since: 0.1.0
 */
static inline int
purc_name_tolower(char *name)
{
    int i = 0;

    while (name [i]) {
        name [i] = tolower(name[i]);
        i++;
    }

    return i;
}

/**
 * Convert a string to uppercases in place.
 *
 * @param name: the pointer to a name string (not nullable).
 *
 * Converts a name string uppercase in place.
 *
 * Returns: the length of the name string.
 *
 * Since: 0.1.0
 */
static inline int
purc_name_toupper(char *name)
{
    int i = 0;

    while (name [i]) {
        name [i] = purc_toupper(name[i]);
        i++;
    }

    return i;
}

/**
 * Convert a string to lowercases and copy to another buffer.
 *
 * @param name: the pointer to a name string (not nullable).
 * @param buff: the buffer used to return the converted name string (not nullable).
 * @param max_len: The maximal length of the name string to convert.
 *
 * Converts a name string lowercase and copies the letters to
 * the specified buffer.
 *
 * Note that if \a max_len <= 0, the argument will be ignored.
 *
 * Returns: the total number of letters converted.
 *
 * Since: 0.1.0
 */
static inline int
purc_name_tolower_copy(const char *name, char *buff, int max_len)
{
    int n = 0;

    while (*name) {
        buff [n] = purc_tolower(*name);
        name++;
        n++;

        if (max_len > 0 && n == max_len)
            break;
    }

    buff [n] = '\0';
    return n;
}

/**
 * Convert a string to uppercases and copy to another buffer.
 *
 * @param name: the pointer to a name string (not nullable).
 * @param buff: the buffer used to return the converted name string (not nullable).
 * @param max_len: The maximal length of the name string to convert.
 *
 * Converts a name string uppercase and copies the letters to
 * the specified buffer.
 *
 * Note that if \a max_len <= 0, the argument will be ignored.
 *
 * Returns: the total number of letters converted.
 *
 * Since: 0.1.0
 */
static inline int
purc_name_toupper_copy(const char *name, char *buff, int max_len)
{
    int n = 0;

    while (*name) {
        buff [n] = purc_toupper(*name);
        name++;
        n++;

        if (max_len > 0 && n == max_len)
            break;
    }

    buff [n] = '\0';
    return n;
}

/**
 * Get monotonic time in seconds
 *
 * Gets the monotoic time in seconds.
 *
 * Returns: the the monotoic time in seconds.
 *
 * Since: 0.1.0
 */
static inline time_t purc_get_monotoic_time(void)
{
    struct timespec tp;

    clock_gettime(CLOCK_MONOTONIC, &tp);
    return tp.tv_sec;
}

static inline bool
purc_is_valid_runner_name(const char *runner_name)
{
    return purc_is_valid_token(runner_name, PURC_LEN_RUNNER_NAME);
}

static inline bool
purc_is_valid_identifier(const char *id)
{
    return purc_is_valid_loose_token(id, PURC_LEN_IDENTIFIER);
}

/**@}*/

#endif /* !PURC_PURC_HELPERS_H */

