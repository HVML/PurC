/**
 * @file purc-pcrdr.h
 * @date 2021/01/12
 * @brief This file declares API to handle PCRDR protocol.
 *
 * Copyright (c) 2022 FMSoft (http://www.fmsoft.cn)
 *
 * Authors:
 *  Vincent Wei (<https://github.com/VincentWei>), 2021, 2022
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

#ifndef PURC_PURC_PCRDR_H
#define PURC_PURC_PCRDR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>

#include "purc-macros.h"
#include "purc-rwstream.h"
#include "purc-variant.h"
#include "purc-errors.h"
#include "purc-utils.h"

/* Constants */
#define PCRDR_PURCMC_PROTOCOL_NAME              "PURCMC"
#define PCRDR_PURCMC_PROTOCOL_VERSION_STRING    "100"
#define PCRDR_PURCMC_PROTOCOL_VERSION           100
#define PCRDR_PURCMC_MINIMAL_PROTOCOL_VERSION   100

#define PCRDR_PURCMC_US_PATH                    "/var/tmp/purcmc.sock"
#define PCRDR_PURCMC_WS_PORT                    "7702"
#define PCRDR_PURCMC_WS_PORT_RESERVED           "7703"

#define PCRDR_LOCALHOST                 "localhost"
#define PCRDR_NOT_AVAILABLE             "<N/A>"

/* Status Codes */
#define PCRDR_SC_IOERR                  1
#define PCRDR_SC_OK                     200
#define PCRDR_SC_CREATED                201
#define PCRDR_SC_ACCEPTED               202
#define PCRDR_SC_NO_CONTENT             204
#define PCRDR_SC_RESET_CONTENT          205
#define PCRDR_SC_PARTIAL_CONTENT        206
#define PCRDR_SC_BAD_REQUEST            400
#define PCRDR_SC_UNAUTHORIZED           401
#define PCRDR_SC_FORBIDDEN              403
#define PCRDR_SC_NOT_FOUND              404
#define PCRDR_SC_METHOD_NOT_ALLOWED     405
#define PCRDR_SC_NOT_ACCEPTABLE         406
#define PCRDR_SC_CONFLICT               409
#define PCRDR_SC_GONE                   410
#define PCRDR_SC_PRECONDITION_FAILED    412
#define PCRDR_SC_PACKET_TOO_LARGE       413
#define PCRDR_SC_EXPECTATION_FAILED     417
#define PCRDR_SC_IM_A_TEAPOT            418
#define PCRDR_SC_UNPROCESSABLE_PACKET   422
#define PCRDR_SC_LOCKED                 423
#define PCRDR_SC_FAILED_DEPENDENCY      424
#define PCRDR_SC_TOO_EARLY              425
#define PCRDR_SC_UPGRADE_REQUIRED       426
#define PCRDR_SC_RETRY_WITH             449
#define PCRDR_SC_UNAVAILABLE_FOR_LEGAL_REASONS             451
#define PCRDR_SC_INTERNAL_SERVER_ERROR  500
#define PCRDR_SC_NOT_IMPLEMENTED        501
#define PCRDR_SC_BAD_CALLEE             502
#define PCRDR_SC_SERVICE_UNAVAILABLE    503
#define PCRDR_SC_CALLEE_TIMEOUT         504
#define PCRDR_SC_INSUFFICIENT_STORAGE   507

#define PCRDR_LEN_HOST_NAME             127
#define PCRDR_LEN_APP_NAME              127
#define PCRDR_LEN_RUNNER_NAME           63
#define PCRDR_LEN_METHOD_NAME           63
#define PCRDR_LEN_BUBBLE_NAME           63
#define PCRDR_LEN_ENDPOINT_NAME         \
    (PCRDR_LEN_HOST_NAME + PCRDR_LEN_APP_NAME + PCRDR_LEN_RUNNER_NAME + 3)
#define PCRDR_LEN_UNIQUE_ID             63

#define PCRDR_MIN_PACKET_BUFF_SIZE      512
#define PCRDR_DEF_PACKET_BUFF_SIZE      1024
#define PCRDR_DEF_TIME_EXPECTED         5   /* 5 seconds */

/* the maximal size of a payload in a frame (4KiB) */
#define PCRDR_MAX_FRAME_PAYLOAD_SIZE    4096

/* the maximal size of a payload which will be held in memory (40KiB) */
#define PCRDR_MAX_INMEM_PAYLOAD_SIZE    40960

/* the maximal time to ping client (60 seconds) */
#define PCRDR_MAX_PING_TIME             60

/* the maximal no responding time (90 seconds) */
#define PCRDR_MAX_NO_RESPONDING_TIME    90

/* Protocol types */
typedef enum {
    PURC_RDRPROT_PURCMC  = 0,
    PURC_RDRPROT_HIBUS,
} purc_rdrprot_t;

/* Connection types */
enum {
    CT_UNIX_SOCKET = 1,
    CT_WEB_SOCKET,
};

/* The frame operation codes for UnixSocket */
typedef enum USOpcode_ {
    US_OPCODE_CONTINUATION = 0x00,
    US_OPCODE_TEXT = 0x01,
    US_OPCODE_BIN = 0x02,
    US_OPCODE_END = 0x03,
    US_OPCODE_CLOSE = 0x08,
    US_OPCODE_PING = 0x09,
    US_OPCODE_PONG = 0x0A,
} USOpcode;

/* The frame header for UnixSocket */
typedef struct USFrameHeader_ {
    int op;
    unsigned int fragmented;
    unsigned int sz_payload;
    unsigned char payload[0];
} USFrameHeader;

/* packet body types */
enum {
    PT_TEXT = 0,
    PT_BINARY,
};

struct pcrdr_msg;
typedef struct pcrdr_msg pcrdr_msg;

struct pcrdr_conn;
typedef struct pcrdr_conn pcrdr_conn;

PCA_EXTERN_C_BEGIN

/**
 * @defgroup PCRDRHelpers PCRDR Helper functions
 * @{
 */

PCA_EXPORT bool
pcrdr_is_valid_host_name(const char *host_name);

PCA_EXPORT bool
pcrdr_is_valid_app_name(const char *app_name);

PCA_EXPORT bool
pcrdr_is_valid_endpoint_name(const char *endpoint_name);

PCA_EXPORT int
pcrdr_extract_host_name(const char *endpoint, char *buff);

PCA_EXPORT int
pcrdr_extract_app_name(const char *endpoint, char *buff);

PCA_EXPORT int
pcrdr_extract_runner_name(const char *endpoint, char *buff);

PCA_EXPORT char *
pcrdr_extract_host_name_alloc(const char *endpoint);

PCA_EXPORT char *
pcrdr_extract_app_name_alloc(const char *endpoint);

PCA_EXPORT char *
pcrdr_extract_runner_name_alloc(const char *endpoint);

PCA_EXPORT int
pcrdr_assemble_endpoint_name(const char *host_name, const char *app_name,
        const char *runner_name, char *buff);

PCA_EXPORT char *
pcrdr_assemble_endpoint_name_alloc(const char *host_name,
        const char *app_name, const char *runner_name);

/**
 * Get the return message of a return code.
 *
 * @param ret_code: the return code.
 *
 * Returns the pointer to the message string of the specific return code.
 *
 * Returns: a pointer to the message string.
 *
 * Since: 0.1.0
 */
PCA_EXPORT const char *
pcrdr_get_ret_message(int ret_code);

/**
 * Convert an error code to a return code.
 *
 * @param err_code: the internal error code.
 *
 * Returns the return code of the PurCMC protocol according to
 * the internal error code.
 *
 * Returns: the return code of PurCMC protocol.
 *
 * Since: 0.1.0
 */
PCA_EXPORT int
pcrdr_errcode_to_retcode(int err_code);

/**
 * Check whether a string is a valid token.
 *
 * @param token: the pointer to the token string.
 * @param max_len: The maximal possible length of the token string.
 *
 * Checks whether a token string is valid. According to PurCMC protocal,
 * the runner name, method name, bubble name should be a valid token.
 *
 * Note that a string with a length longer than \a max_len will
 * be considered as an invalid token.
 *
 * Returns: true for a valid token, otherwise false.
 *
 * Since: 0.1.0
 */
PCA_EXPORT bool
pcrdr_is_valid_token(const char *token, int max_len);

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
pcrdr_generate_unique_id(char *id_buff, const char *prefix);

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
pcrdr_generate_md5_id(char *id_buff, const char *prefix);

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
pcrdr_is_valid_unique_id(const char *id);

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
pcrdr_is_valid_md5_id(const char *id);

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
pcrdr_get_elapsed_seconds(const struct timespec *ts1,
        const struct timespec *ts2);

/**@}*/

/**
 * @defgroup PCRDRConnection Connection functions
 *
 * The connection functions are implemented in libhibus.c, only for clients.
 * @{
 */

/**
 * Disconnect from the renderer.
 *
 * @param conn: the pointer to the renderer connection.
 *
 * Disconnects the renderer connection.
 *
 * Returns: -1 for error; zero means everything is ok.
 *
 * Since: 0.1.0
 */
PCA_EXPORT int
pcrdr_disconnect(pcrdr_conn* conn);

/**
 * Free a connection.
 *
 * @param conn: the pointer to the renderer connection.
 *
 * Frees the space used by the connection, including the connection itself.
 *
 * Returns: -1 for error; zero means everything is ok.
 *
 * Since: 0.1.0
 */
PCA_EXPORT int
pcrdr_free_connection(pcrdr_conn* conn);

/**
 * The prototype of a request handler.
 *
 * @param conn: the pointer to the renderer connection.
 * @param msg: the request message object.
 *
 * Since: 0.1.0
 */
typedef void (*pcrdr_request_handler)(pcrdr_conn* conn, const pcrdr_msg *msg);

/**
 * pcrdr_conn_get_request_handler:
 * @param conn: the pointer to the renderer connection.
 *
 * Returns the current request handler of the renderer connection.
 *
 * Since: 0.1.0
 */
PCA_EXPORT pcrdr_request_handler
pcrdr_conn_get_request_handler(pcrdr_conn* conn);

/**
 * Set the request handler of the connection.
 *
 * @param conn: the pointer to the renderer connection.
 * @param request_handler: the new request handler.
 *
 * Sets the request handler of the renderer connection, and returns the old one.
 *
 * Since: 0.1.0
 */
PCA_EXPORT pcrdr_request_handler
pcrdr_conn_set_request_handler(pcrdr_conn* conn,
        pcrdr_request_handler request_handler);

/**
 * The prototype of an event handler.
 *
 * @param conn: the pointer to the renderer connection.
 * @param msg: the event message object.
 *
 * Since: 0.1.0
 */
typedef void (*pcrdr_event_handler)(pcrdr_conn* conn, const pcrdr_msg *msg);

/**
 * pcrdr_conn_get_event_handler:
 * @param conn: the pointer to the renderer connection.
 *
 * Returns the current event handler of the renderer connection.
 *
 * Since: 0.1.0
 */
PCA_EXPORT pcrdr_event_handler
pcrdr_conn_get_event_handler(pcrdr_conn* conn);

/**
 * Set the event handler of the connection.
 *
 * @param conn: the pointer to the renderer connection.
 * @param event_handler: the new event handler.
 *
 * Sets the event handler of the renderer connection, and returns the old one.
 *
 * Since: 0.1.0
 */
PCA_EXPORT pcrdr_event_handler
pcrdr_conn_set_event_handler(pcrdr_conn* conn,
        pcrdr_event_handler event_handler);

/**
 * Get the user data associated with the connection.
 *
 * @param conn: the pointer to the renderer connection.
 *
 * Returns the current user data (a pointer) bound with the renderer connection.
 *
 * Since: 0.1.0
 */
PCA_EXPORT void *
pcrdr_conn_get_user_data(pcrdr_conn* conn);

/**
 * Set the user data associated with the connection.
 *
 * @param conn: the pointer to the renderer connection.
 * @param user_data: the new user data (a pointer).
 *
 * Sets the user data of the renderer connection, and returns the old one.
 *
 * Since: 0.1.0
 */
PCA_EXPORT void *
pcrdr_conn_set_user_data(pcrdr_conn* conn, void* user_data);

/**
 * Get the last return code from the renderer.
 *
 * @param conn: the pointer to the renderer connection.
 *
 * Returns the last return code.
 *
 * Since: 0.1.0
 */
PCA_EXPORT int 
pcrdr_conn_get_last_ret_code(pcrdr_conn* conn);

/**
 * Get the server host name of a connection.
 *
 * @param conn: the pointer to the renderer connection.
 *
 * Returns the host name of the PurCMC server.
 *
 * Since: 0.1.0
 */
PCA_EXPORT const char *
pcrdr_conn_srv_host_name(pcrdr_conn* conn);

/**
 * Get the own host name of a connection.
 *
 * @param conn: the pointer to the renderer connection.
 *
 * Returns the host name of the current PurCMC client.
 *
 * Since: 0.1.0
 */
PCA_EXPORT const char *
pcrdr_conn_own_host_name(pcrdr_conn* conn);

/**
 * Get the app name of a connection.
 *
 * @param conn: the pointer to the renderer connection.
 *
 * Returns the app name of the current PurCMC client.
 *
 * Since: 0.1.0
 */
PCA_EXPORT const char *
pcrdr_conn_app_name(pcrdr_conn* conn);

/**
 * Get the runner name of a connection.
 *
 * @param conn: the pointer to the renderer connection.
 *
 * Returns the runner name of the current PurCMC client.
 *
 * Since: 0.1.0
 */
PCA_EXPORT const char *
pcrdr_conn_runner_name(pcrdr_conn* conn);

/**
 * Get the file descriptor of the connection.
 *
 * @param conn: the pointer to the renderer connection.
 *
 * Returns the file descriptor of the renderer connection socket.
 *
 * Returns: the file descriptor.
 *
 * Since: 0.1.0
 */
PCA_EXPORT int
pcrdr_conn_socket_fd(pcrdr_conn* conn);

/**
 * Get the connnection socket type.
 *
 * @param conn: the pointer to the renderer connection.
 *
 * Returns the socket type of the renderer connection.
 *
 * Returns: \a CT_UNIX_SOCKET for UnixSocket, and \a CT_WEB_SOCKET for WebSocket.
 *
 * Since: 0.1.0
 */
PCA_EXPORT int
pcrdr_conn_socket_type(pcrdr_conn* conn);

/**
 * Get the connnection protocol.
 *
 * @param conn: the pointer to the renderer connection.
 *
 * Returns the protocol of the renderer connection.
 *
 * Returns: \a PURC_RDRPROT_PURCMC for PurCMC,
 *      and \a PURC_RDRPROT_HIBUS for hiBus.
 *
 * Since: 0.1.0
 */
PCA_EXPORT purc_rdrprot_t
pcrdr_conn_protocol(pcrdr_conn* conn);

typedef enum {
    PCRDR_MSG_TYPE_VOID = 0,
    PCRDR_MSG_TYPE_REQUEST,
    PCRDR_MSG_TYPE_RESPONSE,
    PCRDR_MSG_TYPE_EVENT,
} pcrdr_msg_type;

typedef enum {
    PCRDR_MSG_TARGET_SESSION = 0,
    PCRDR_MSG_TARGET_WINDOW,
    PCRDR_MSG_TARGET_TAB,
    PCRDR_MSG_TARGET_DOM,
} pcrdr_msg_target;

typedef enum {
    PCRDR_MSG_ELEMENT_TYPE_VOID = 0,
    PCRDR_MSG_ELEMENT_TYPE_CSS,
    PCRDR_MSG_ELEMENT_TYPE_XPATH,
    PCRDR_MSG_ELEMENT_TYPE_HANDLE,
    PCRDR_MSG_ELEMENT_TYPE_HANDLES,
} pcrdr_msg_element_type;

PCA_EXPORT pcrdr_msg_type
pcrdr_message_get_type(const pcrdr_msg *msg);

typedef enum {
    PCRDR_MSG_DATA_TYPE_VOID = 0,
    PCRDR_MSG_DATA_TYPE_EJSON,
    PCRDR_MSG_DATA_TYPE_TEXT,
} pcrdr_msg_data_type;

struct pcrdr_msg {
    pcrdr_msg_type          type;
    pcrdr_msg_target        target;
    pcrdr_msg_element_type  elementType;
    pcrdr_msg_data_type     dataType;
    unsigned int            retCode;

    uintptr_t       targetValue;
    uintptr_t       resultValue;

    purc_variant_t  operation;
    purc_variant_t  element;
    purc_variant_t  property;
    purc_variant_t  event;

    purc_variant_t  requestId;

    purc_variant_t  data;

    /* internal use only */
    size_t          _data_len;
};

/**
 * Make a void message.
 *
 * Returns: The pointer to message object; NULL on error.
 *
 * Since: 0.1.0
 */
PCA_EXPORT pcrdr_msg *
pcrdr_make_void_message(void);

/**
 * Make a request message.
 *
 * @param target: the target of the message.
 * @param target_value: the value of the target object
 * @param operation: the request operation.
 * @param element_type: the element type of the request
 * @param element: the pointer to the element(s) (nullable).
 * @param property: the property (nullable).
 * @param data_type: the data type of the request.
 * @param data: the pointer to the data (nullable)
 *
 * Returns: The pointer to message object; NULL on error.
 *
 * Since: 0.1.0
 */
PCA_EXPORT pcrdr_msg *
pcrdr_make_request_message(
        pcrdr_msg_target target, uintptr_t target_value,
        const char *operation,
        const char *request_id,
        pcrdr_msg_element_type element_type, const char *element,
        const char *property,
        pcrdr_msg_data_type data_type, const char* data, size_t data_len);

/**
 * Make a response message for a request message.
 *
 * @param request_msg: the request message.
 * @param ret_code: the return code.
 * @param result_value: the result value.
 * @param extra_info: the extra information (nullable).
 *
 * Returns: The pointer to the response message object; NULL on error.
 *
 * Since: 0.1.0
 */
PCA_EXPORT pcrdr_msg *
pcrdr_make_response_message(
        const char *request_id,
        unsigned int ret_code, uintptr_t result_value,
        pcrdr_msg_data_type data_type, const char* data, size_t data_len);

/**
 * Make an event message.
 *
 * @param target: the target of the message.
 * @param target_value: the value of the target object
 * @param event: the event name.
 * @param element_type: the element type.
 * @param element: the pointer to the element(s) (nullable).
 * @param property: the property (nullable)
 * @param data_type: the data type of the event.
 * @param data: the pointer to the data (nullable)
 *
 * Returns: The pointer to the event message object; NULL on error.
 *
 * Since: 0.1.0
 */
PCA_EXPORT pcrdr_msg *
pcrdr_make_event_message(
        pcrdr_msg_target target, uintptr_t target_value,
        const char *event,
        pcrdr_msg_element_type element_type, const char *element,
        const char *property,
        pcrdr_msg_data_type data_type, const char* data, size_t data_len);

/**
 * Clone a message.
 *
 * @param msg: The pointer to a message.
 *
 * Returns: The pointer to the cloned new message; NULL for error.
 *
 * Since: 0.1.0
 */
PCA_EXPORT pcrdr_msg *
pcrdr_clone_message(const pcrdr_msg* msg);

/**
 * Parse a packet and make a corresponding message.
 *
 * @param packet_text: the pointer to the packet text buffer.
 * @param sz_packet: the size of the packet.
 * @param msg: The pointer to a pointer to return the parsed message object.
 *
 * Returns: -1 for error; zero means everything is ok.
 *
 * Note that this function may change the content in \a packet.
 *
 * Since: 0.1.0
 */
PCA_EXPORT int
pcrdr_parse_packet(char *packet, size_t sz_packet, pcrdr_msg **msg);

typedef ssize_t (*cb_write)(void *ctxt, const void *buf, size_t count);

/**
 * Serialize a message.
 *
 * @param msg: the poiter to the message to serialize.
 * @param fn: the callback to write characters.
 * @param ctxt: the context will be passed to fn.
 *
 * Returns: -1 for error; zero means everything is ok.
 *
 * Since: 0.1.0
 */
PCA_EXPORT int
pcrdr_serialize_message(const pcrdr_msg *msg, cb_write fn, void *ctxt);

/**
 * Serialize a message to buffer.
 *
 * @param msg: the poiter to the message to serialize.
 * @param buff: the pointer to the buffer.
 * @param sz: the size of the buffer.
 *
 * Returns: the number of characters should be written to the buffer.
 * A return value more than @sz means that the output was truncated.
 *
 * Since: 0.1.0
 */
PCA_EXPORT size_t
pcrdr_serialize_message_to_buffer(const pcrdr_msg *msg,
        void *buff, size_t sz);

/**
 * Compare two messages.
 *
 * @param msga: the poiter to the first message.
 * @param msga: the poiter to the second message.
 *
 * Returns: zero when the messages are identical.
 *
 * Since: 0.1.0
 */
PCA_EXPORT int
pcrdr_compare_messages(const pcrdr_msg *msg_a, const pcrdr_msg *msg_b);

/**
 * Release a message.
 *
 * @param msg: the poiter to the message to release.
 *
 * Returns: None.
 *
 * Since: 0.1.0
 */
PCA_EXPORT void
pcrdr_release_message(pcrdr_msg *msg);

enum {
    PCRDR_RESPONSE_RESULT = 0,
    PCRDR_RESPONSE_TIMEOUT,
    PCRDR_RESPONSE_CANCELLED,
};

/**
 * The prototype of a response handler.
 *
 * @param conn: the pointer to the renderer connection.
 * @param request_id: the original request identifier.
 * @param state: the state of the response, can be one of the following values:
 *      - PCRDR_RESPONSE_RESULT: got a valid response message.
 *      - PCRDR_RESPONSE_TIMEOUT: time out.
 *      - PRCRD_RESPONSE_CANCELLED: the request is cancelled due to
 *          the connection was losted.
 * @param context: the context of the response handler.
 * @param response_msg: the response message, may be NULL.
 *
 * Returns: 0 for finished the handle of the response; otherwise -1.
 *
 * Note that after calling the response handler, the response message object
 * will be released.
 *
 * Since: 0.1.0
 */
typedef int (*pcrdr_response_handler)(pcrdr_conn* conn,
        const char *request_id, int state,
        void *context, const pcrdr_msg *response_msg);

/**
 * Send a request and handle the response in a callback.
 *
 * @param conn: the pointer to the renderer connection.
 * @param request_msg: the pointer to the request message.
 * @param seconds_expected: the expected return time in seconds.
 * @param context: the context will be passed to the response handler.
 * @param response_handler: the response handler.
 *
 * This function emits a request to the renderer and
 * returns immediately. The response handler will be called
 * in subsequent calls of \a pcrdr_read_and_dispatch_message().
 *
 * Returns: -1 for error; zero means everything is ok.
 *
 * Since: 0.1.0
 */
PCA_EXPORT int
pcrdr_send_request(pcrdr_conn* conn, pcrdr_msg *request_msg,
        int seconds_expected, void *context,
        pcrdr_response_handler response_handler);

/**
 * Read and dispatch the message from the renderer connection.
 *
 * @param conn: the pointer to the renderer connection.
 *
 * This function read a packet and translate it to a message
 * and dispatches the message to an event handler or a response handler.
 *
 * Returns: -1 for error; zero means everything is ok.
 *
 * Since: 0.1.0
 */
PCA_EXPORT int
pcrdr_read_and_dispatch_message(pcrdr_conn* conn);

/**
 * Wait and dispatch the message from the renderer connection.
 *
 * @param conn: the pointer to the renderer connection.
 *
 * This function wait for the new message within a timeout time,
 * if there is a new message, it dispatches the message to the handlers.
 *
 * Returns: -1 for error or timeout; zero means everything is ok.
 *
 * Since: 0.1.0
 */
PCA_EXPORT int
pcrdr_wait_and_dispatch_message(pcrdr_conn* conn, int timeout_ms);

/**
 * Send a request to the renderer and wait the response.
 *
 * @param conn: the pointer to the renderer connection.
 * @param request_msg: the pointer to the request message.
 * @param seconds_expected: the expected return time in seconds.
 * @param response_msg: the pointer to a pointer to return the response message.
 *
 * This function send a request to the renderer and wait for the response.
 *
 * Returns: -1 for error; zero means everything is ok.
 *
 * Since: 0.1.0
 */
PCA_EXPORT int
pcrdr_send_request_and_wait_response(pcrdr_conn* conn, const pcrdr_msg *request_msg,
        int seconds_expected, pcrdr_msg **response_msg);

/**
 * Ping the renderer.
 *
 * @param conn: the pointer to the renderer connection.
 *
 * Pings the renderer. The client should ping the renderer
 * about every 30 seconds to tell the renderer "I am alive".
 * According to the PurCMC protocol, the server may consider
 * a client died if there was no any data from the client
 * for 90 seconds.
 *
 * Returns: -1 for error; zero means everything is ok.
 *
 * Since: 0.1.0
 */
PCA_EXPORT int
pcrdr_ping_renderer(pcrdr_conn* conn);

/**
 * Connect to the PurCMC server via UNIX domain socket.
 *
 * @param path_to_socket: the path to the unix socket.
 * @param app_name: the app name.
 * @param runner_name: the runner name.
 * @param conn: the pointer to a pcrdr_conn* to return the renderer connection.
 *
 * Connects to a PurCMC server via UNIX domain socket.
 *
 * Returns: -1 for error; zero means everything is ok.
 *
 * Since: 0.1.0
 */
PCA_EXPORT int
pcrdr_purcmc_connect_via_unix_socket(const char *path_to_socket,
        const char *app_name, const char *runner_name, pcrdr_conn** conn);

/**
 * Connect to the PurCMC server via WebSocket.
 *
 * @param srv_host_name: the host name of the PurCMC server.
 * @param port: the port.
 * @param app_name: the app name.
 * @param runner_name: the runner name.
 * @param conn: the pointer to a pcrdr_conn* to return the renderer connection.
 *
 * Connects to a PurCMC server via WebSocket.
 *
 * Returns: -1 for error; zero means everything is ok.
 *
 * Note that this function is not implemented so far.
 */
PCA_EXPORT int
pcrdr_purcmc_connect_via_web_socket(const char *srv_host_name, int port,
        const char *app_name, const char *runner_name, pcrdr_conn** conn);

/**
 * Read a packet (pre-allocation version).
 *
 * @param conn: the pointer to the renderer connection.
 * @param packet_buf: the pointer to a buffer for saving the contents
 *      of the packet.
 * @param sz_packet: the pointer to a unsigned integer for returning
 *      the length of the packet.
 *
 * Reads a packet and saves the contents of the packet and returns
 * the length of the packet.
 *
 * Returns: -1 for error; zero means everything is ok.
 *
 * Note that use this function only if you know the length of
 * the next packet, and have a long enough buffer to save the
 * contents of the packet.
 *
 * Also note that if the length of the packet is 0, there is no data in
 * the packet. You should ignore the packet in this case.
 *
 * Since: 0.1.0
 */
PCA_EXPORT int
pcrdr_purcmc_read_packet(pcrdr_conn* conn,
        char* packet_buf, size_t *sz_packet);

/**
 * Read a packet (allocation version).
 *
 * @param conn: the pointer to the renderer connection.
 * @param packet: the pointer to a pointer to a buffer for returning
 *      the contents of the packet.
 * @param sz_packet: the pointer to a unsigned integer for returning
 *      the length of the packet.
 *
 * Reads a packet and allocates a buffer for the contents of the packet
 * and returns the contents and the length.
 *
 * Returns: -1 for error; zero means everything is ok.
 *
 * Note that the caller is responsible for releasing the buffer.
 *
 * Also note that if the length of the packet is 0, there is no data in the packet.
 * You should ignore the packet in this case.
 *
 * Since: 0.1.0
 */
PCA_EXPORT int
pcrdr_purcmc_read_packet_alloc(pcrdr_conn* conn,
        void **packet, size_t *sz_packet);

/**
 * Send a text packet to the PurCMC server.
 *
 * @param conn: the pointer to the renderer connection.
 * @param text: the pointer to the text to send.
 * @param txt_len: the length to send.
 *
 * Sends a text packet to the PurCMC server.
 *
 * Returns: -1 for error; zero means everything is ok.
 *
 * Since: 0.1.0
 */
PCA_EXPORT int
pcrdr_purcmc_send_text_packet(pcrdr_conn* conn,
        const char *text, size_t txt_len);

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
pcrdr_name_toupper(char *name)
{
    int i = 0;

    while (name [i]) {
        name [i] = toupper(name[i]);
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
pcrdr_name_tolower_copy(const char *name, char *buff, int max_len)
{
    int n = 0;

    while (*name) {
        buff [n] = tolower(*name);
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
pcrdr_name_toupper_copy(const char *name, char *buff, int max_len)
{
    int n = 0;

    while (*name) {
        buff [n] = toupper(*name);
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
static inline time_t pcrdr_get_monotoic_time(void)
{
    struct timespec tp;

    clock_gettime(CLOCK_MONOTONIC, &tp);
    return tp.tv_sec;
}

static inline bool
pcrdr_is_valid_runner_name(const char *runner_name)
{
    return pcrdr_is_valid_token(runner_name, PCRDR_LEN_RUNNER_NAME);
}

/**@}*/

#endif /* !PURC_PURC_PCRDR_H */

