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
#include "purc-utils.h"

#define PURC_LEN_HOST_NAME              127
#define PURC_LEN_APP_NAME               127
#define PURC_LEN_RUNNER_NAME            63
#define PURC_LEN_IDENTIFIER             63

#define PURC_EDPT_SCHEMA                "edpt://"
#define PURC_LEN_EDPT_SCHEMA            7

#define PURC_LEN_ENDPOINT_NAME         \
    (PURC_LEN_EDPT_SCHEMA + PURC_LEN_HOST_NAME + PURC_LEN_APP_NAME + \
     PURC_LEN_RUNNER_NAME + 2)
#define PURC_LEN_UNIQUE_ID             63
#define PURC_LEN_PROPERTY_NAME         255

PCA_EXTERN_C_BEGIN

/**
 * @defgroup Helpers Global Helper Functions
 * @{
 */

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

/**
 * Enable or disable the log facility for the current PurC instance.
 *
 * @param enable: @true to enable, @false to disable.
 * @param use_syslog: @true to use syslog, @false to use log file.
 *
 * Returns: @true for success, otherwise @false.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
purc_enable_log(bool enable, bool use_syslog);

/**
 * Log a message with tag.
 *
 * @param tag: the tag of the message.
 * @param msg: the message or the format string.
 *
 * Returns: none.
 *
 * Since: 0.1.0
 */
PCA_EXPORT void
purc_log_with_tag(const char* tag, const char *msg, va_list ap)
    PCA_ATTRIBUTE_PRINTF(2, 0);

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
    purc_log_with_tag("INFO", msg, ap);
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
    purc_log_with_tag("DEBUG", msg, ap);
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
    purc_log_with_tag("WARN", msg, ap);
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
    purc_log_with_tag("ERROR", msg, ap);
    va_end(ap);
}

/**@}*/

PCA_EXTERN_C_END

/**
 * @addtogroup Helpers
 *  @{
 */

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

