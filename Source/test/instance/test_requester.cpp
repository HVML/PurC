/*
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "purc.h"
#include "private/list.h"
#include "private/sorted-array.h"

#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>           /* For O_* constants */
#include <gtest/gtest.h>
#include <wtf/Compiler.h>

#define TEST_APP_NAME           "cn.fmsoft.hvml.test"
#define TEST_RUN_NAME           "requester"

#define INSTMGR_APP_NAME        "cn.fmsoft.hvml"
#define INSTMGR_RUN_NAME        "instmgr"

struct instance_arg {
    sem_t      *sync;
    const char *app;
    const char *run;
    purc_atom_t atom;
};

typedef purc_variant_t (*fn_request_handler)(struct inst_info *info,
        const pcrdr_msg *request);

struct cort_info {
    char                id[PURC_LEN_UNIQUE_ID + 1];
    fn_request_handler  method_handler;
};

struct inst_info {
    unsigned nr_cors;
    struct sorted_array *sa_cors;
};

static purc_variant_t request_handler0(struct inst_info *info,
        const pcrdr_msg *request)
{
    uint32_t arg = 0;
    (void)info;

    if (request->data) {
        if (purc_variant_cast_to_uint32(request->data, &arg, false)) {
            purc_log_warn("failed to cast call argument to u32\n");
        }
    }

    char buff[64];
    sprintf(buff, "From handler0 for the Call #%u", arg);

    return purc_variant_make_string(buff, false);
}

static purc_variant_t request_handler1(struct inst_info *info,
        const pcrdr_msg *request)
{
    uint32_t arg = 0;
    (void)info;

    if (request->data) {
        if (purc_variant_cast_to_uint32(request->data, &arg, false)) {
            purc_log_warn("failed to cast call argument to u32\n");
        }
    }

    char buff[64];
    sprintf(buff, "From handler1 for the Call #%u", arg);

    return purc_variant_make_string(buff, false);
}

static purc_variant_t request_handler2(struct inst_info *info,
        const pcrdr_msg *request)
{
    uint32_t arg = 0;
    (void)info;

    if (request->data) {
        if (purc_variant_cast_to_uint32(request->data, &arg, false)) {
            purc_log_warn("failed to cast call argument to u32\n");
        }
    }

    char buff[64];
    sprintf(buff, "From handler2 for the Call #%u", arg);

    return purc_variant_make_string(buff, false);
}

static purc_variant_t request_handler3(struct inst_info *info,
        const pcrdr_msg *request)
{
    uint32_t arg = 0;
    (void)info;

    if (request->data) {
        if (purc_variant_cast_to_uint32(request->data, &arg, false)) {
            purc_log_warn("failed to cast call argument to u32\n");
        }
    }

    char buff[64];
    sprintf(buff, "From handler3 for the Call #%u", arg);

    return purc_variant_make_string(buff, false);
}

static const fn_request_handler handlers[] = {
    request_handler0,
    request_handler1,
    request_handler2,
    request_handler3,
};

static void call_method(struct inst_info *info,
        const pcrdr_msg *request, pcrdr_msg *response)
{
    const char *endpoint_name = purc_get_endpoint(NULL);

    response->type = PCRDR_MSG_TYPE_RESPONSE;
    response->requestId = purc_variant_ref(request->requestId);
    response->dataType = PCRDR_MSG_DATA_TYPE_VOID;
    response->data = PURC_VARIANT_INVALID;

    purc_atom_t cort_atom = (purc_atom_t)request->targetValue;
    struct cort_info *cort_info;

    if (!pcutils_sorted_array_find(info->sa_cors, (void *)(uintptr_t)cort_atom,
            (void **)&cort_info)) {
        response->sourceURI = purc_variant_make_string(endpoint_name, false);
        response->retCode = PCRDR_SC_NOT_FOUND;
        response->resultValue = (uint64_t)cort_atom;
    }
    else {
        char full_cor_id[PURC_LEN_ENDPOINT_NAME + PURC_LEN_UNIQUE_ID + 4];
        sprintf(full_cor_id, "%s/%s", endpoint_name, cort_info->id);

        purc_atom_t atom = purc_atom_try_string_ex(PURC_ATOM_BUCKET_DEF,
                full_cor_id);
        if (cort_atom != atom) {
            purc_log_error("atom for coroutine do not matched (%u vs %u).\n",
                    cort_atom, atom);
        }

        response->sourceURI = purc_variant_make_string(full_cor_id, false);
        response->dataType = PCRDR_MSG_DATA_TYPE_TEXT;
        response->data = cort_info->method_handler(info, request);
        response->retCode = PCRDR_SC_OK;
        response->resultValue = (uint64_t)cort_atom;
    }
}

#define NR_HANDLERS (sizeof(handlers)/sizeof(handlers[0]))

static void create_coroutine(struct inst_info *info,
        const pcrdr_msg *request, pcrdr_msg *response)
{
    char id_buf[PURC_LEN_UNIQUE_ID + 1];
    purc_generate_unique_id(id_buf, "CORONTINE");

    const char *endpoint_name = purc_get_endpoint(NULL);
    assert(endpoint_name);  /* must be valid */

    char full_cor_id[PURC_LEN_ENDPOINT_NAME + PURC_LEN_UNIQUE_ID + 4];
    sprintf(full_cor_id, "%s/%s", endpoint_name, id_buf);

    bool newly_created;
    purc_atom_t cort_atom = purc_atom_from_string_ex2(PURC_ATOM_BUCKET_DEF,
            full_cor_id, &newly_created);
    assert(newly_created);  /* must be a newly created atom */

    struct cort_info *cort_info;
    cort_info = (struct cort_info *)malloc(sizeof(*cort_info));
    strcpy(cort_info->id, id_buf);
    cort_info->method_handler = handlers[cort_atom % NR_HANDLERS];

    pcutils_sorted_array_add(info->sa_cors, (void *)(uintptr_t)cort_atom,
            (void *)cort_info);
    info->nr_cors++;

    response->type = PCRDR_MSG_TYPE_RESPONSE;
    response->requestId = purc_variant_ref(request->requestId);
    response->sourceURI = purc_variant_make_string(endpoint_name, false);
    response->retCode = PCRDR_SC_OK;
    response->resultValue = (uint64_t)cort_atom;
    response->dataType = PCRDR_MSG_DATA_TYPE_VOID;
    response->data = PURC_VARIANT_INVALID;
}

static void kill_coroutine(struct inst_info *info,
        const pcrdr_msg *request, pcrdr_msg *response)
{
    const char *endpoint_name = purc_get_endpoint(NULL);
    assert(endpoint_name);  /* must be valid */

    if (request->elementType != PCRDR_MSG_ELEMENT_TYPE_HANDLE)
        return;

    const char *element_value = purc_variant_get_string_const(
            request->elementValue);
    if (element_value == NULL)
        return;

    purc_atom_t atom;
    atom = strtoul(element_value, NULL, 16);
    if (atom == 0)
        return;

    response->type = PCRDR_MSG_TYPE_RESPONSE;
    response->requestId = purc_variant_ref(request->requestId);
    response->sourceURI = purc_variant_make_string(endpoint_name, false);
    response->dataType = PCRDR_MSG_DATA_TYPE_VOID;
    response->data = PURC_VARIANT_INVALID;

    struct cort_info *cort_info;
    if (!pcutils_sorted_array_find(info->sa_cors, (void *)(uintptr_t)atom,
            (void **)&cort_info)) {
        response->retCode = PCRDR_SC_NOT_FOUND;
        response->resultValue = atom;
    }
    else {
        pcutils_sorted_array_remove(info->sa_cors, (void *)(uintptr_t)atom);
        response->retCode = (uint64_t)PCRDR_SC_OK;
        response->resultValue = (uint64_t)atom;
        info->nr_cors--;
    }
}

static void shutdown_instance(struct inst_info *info,
        const pcrdr_msg *request, pcrdr_msg *response)
{
    purc_atom_t endpoint_atom;
    const char *endpoint_name = purc_get_endpoint(&endpoint_atom);
    assert(endpoint_name);  /* must be valid */

    pcutils_sorted_array_destroy(info->sa_cors);
    info->nr_cors = 0;

    response->type = PCRDR_MSG_TYPE_RESPONSE;
    response->requestId = purc_variant_ref(request->requestId);
    response->sourceURI = purc_variant_make_string(endpoint_name, false);
    response->retCode = PCRDR_SC_OK;
    response->resultValue = (uint64_t)endpoint_atom;
    response->dataType = PCRDR_MSG_DATA_TYPE_VOID;
    response->data = PURC_VARIANT_INVALID;
}

static void my_sa_free(void *sortv, void *data)
{
    (void)sortv;
    free(data);
}

static void* general_instance_entry(void* arg)
{
    struct instance_arg *inst_arg = (struct instance_arg *)arg;

    int ret = purc_init_ex(PURC_MODULE_VARIANT,
            inst_arg->app, inst_arg->run, NULL);

    if (ret != PURC_ERROR_OK) {
        inst_arg->atom = 0;
        sem_post(inst_arg->sync);
        return NULL;
    }

    purc_enable_log(false, false);

    inst_arg->atom =
        purc_inst_create_move_buffer(PCINST_MOVE_BUFFER_BROADCAST, 16);
    sem_post(inst_arg->sync);

    purc_log_info("purc_inst_create_move_buffer returns: %x\n",
            inst_arg->atom);

    struct inst_info info;
    info.nr_cors = 0;
    info.sa_cors = pcutils_sorted_array_create(SAFLAG_DEFAULT, 0,
            my_sa_free, NULL);
    assert(info.sa_cors);

    size_t n;
    do {
        ret = purc_inst_holding_messages_count(&n);

        if (ret) {
            purc_log_error("purc_inst_holding_messages_count failed: %d\n", ret);
        }
        else if (n > 0) {
            purc_log_info("purc_inst_holding_messages_count returns: %d\n", (int)n);

            pcrdr_msg *msg = purc_inst_take_away_message(0);
            if (msg->type == PCRDR_MSG_TYPE_REQUEST) {
                const char* source_uri;
                purc_atom_t requester;

                source_uri = purc_variant_get_string_const(msg->sourceURI);
                if (source_uri == NULL ||
                        (requester = purc_atom_try_string_ex(
                            PURC_ATOM_BUCKET_DEF, source_uri)) == 0) {
                    purc_log_info("No sourceURI or the requester disappeared\n");
                    pcrdr_release_message(msg);
                    continue;
                }

                pcrdr_msg *response = pcrdr_make_void_message();

                const char *op;
                op = purc_variant_get_string_const(msg->operation);
                purc_log_info("Got `%s` operation from %s\n", op, source_uri);

                if (msg->target == PCRDR_MSG_TARGET_INSTANCE) {
/*
   A request message sent to the instance can be used to manage
   the coroutines, for example, create or kill a coroutine. This type
   of request can also be used to implement the debugger. The debugger
   can send the operations like `pauseCoroutine` or `resumeCoroutine`
   to control the execution of a coroutine.

   When controlling an existing coroutine, we use `elementValue` to
   pass the atom value of the target coroutine. In this situation,
   the `elementType` should be `PCRDR_MSG_ELEMENT_HANDLE`.
*/
                    if (strcmp(op, "createCoroutine") == 0) {
                        create_coroutine(&info, msg, response);
                    }
                    else if (strcmp(op, "killCoroutine") == 0) {
                        kill_coroutine(&info, msg, response);
                    }
                    else if (strcmp(op, "pauseCoroutine") == 0) {
                    }
                    else if (strcmp(op, "resumeCoroutine") == 0) {
                    }
                    else if (strcmp(op, "shutdown") == 0) {
                        shutdown_instance(&info, msg, response);
                        pcrdr_release_message(msg);
                        pcrdr_release_message(response);
                        break;
                    }
                }
                else if (msg->target == PCRDR_MSG_TARGET_COROUTINE) {
/*
   When the target of a request is a coroutine, the target value should be
   the atom value of the coroutine identifier.

   Generally, a `callMethod` request sent to a coroutine should be handled by
   an operation group which is scoped at the specified element of the document.

   For this purpose,

   1. the `elementValue` of the message can contain the identifier of
   the element in vDOM; the `elementType` should be `PCRDR_MSG_ELEMENT_TYPE_ID`.

   2. the `data` of the message should be an object variant, which contains
   the variable name of the operation group and the argument for calling
   the operation group.

   When the instance got such a request message, it should dispatch the message
   to the target coroutine. And the coroutine should prepare a virtual stack frame
   to call the operation group in the scope of the specified element. The result
   of the operation group should be sent back to the caller as a response message.

   In this way, the coroutine can act as a service provider for others.
*/
                    if (strcmp(op, "callMethod") == 0) {
                        call_method(&info, msg, response);
                    }
                }

                if (response->type == PCRDR_MSG_TYPE_VOID) {
                    /* must be a bad request */
                    response->type = PCRDR_MSG_TYPE_RESPONSE;
                    response->requestId = purc_variant_ref(msg->requestId);
                    response->sourceURI = purc_variant_make_string(
                            purc_get_endpoint(NULL), false);
                    response->retCode = PCRDR_SC_BAD_REQUEST;
                    response->resultValue = 0;
                    response->dataType = PCRDR_MSG_DATA_TYPE_VOID;
                    response->data = PURC_VARIANT_INVALID;

                }

                const char *request_id;
                request_id = purc_variant_get_string_const(msg->requestId);
                if (strcmp(request_id, PCRDR_REQUESTID_NORETURN)) {
                    purc_inst_move_message(requester, response);
                }
                pcrdr_release_message(response);
            }
            else if (msg->type == PCRDR_MSG_TYPE_EVENT) {
                const char *event_name;
                event_name = purc_variant_get_string_const(msg->eventName);

                if (strcmp(event_name, "quit") == 0 &&
                        msg->target == PCRDR_MSG_TARGET_INSTANCE &&
                        msg->targetValue == 0) {
                    purc_log_info("got the quit from %s\n",
                            purc_variant_get_string_const(msg->sourceURI));
                    pcrdr_release_message(msg);
                    break;
                }
                else {
                    purc_log_info("got an event message not interested in:\n");
                    purc_log_info("    type:        %d\n", msg->type);
                    purc_log_info("    target:      %d\n", msg->target);
                    purc_log_info("    targetValue: %d\n", (int)msg->targetValue);
                    purc_log_info("    eventName:   %s\n", event_name);
                    purc_log_info("    sourceURI: %s\n",
                            purc_variant_get_string_const(msg->sourceURI));
                }
            }
            else if (msg->type == PCRDR_MSG_TYPE_RESPONSE) {
                purc_log_info("got a response message for request: %s from %s\n",
                        purc_variant_get_string_const(msg->requestId),
                        purc_variant_get_string_const(msg->sourceURI));
            }

            pcrdr_release_message(msg);
        }
        else {
            usleep(10000);  // 10m

#if 0
            pcrdr_msg *request = pcrdr_make_request_message(
                PCRDR_MSG_TARGET_INSTANCE, inst_responser,
                "ping", runner_name, runner_name,
                PCRDR_MSG_ELEMENT_TYPE_VOID, NULL,
                NULL,
                PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);

            request->dataType = PCRDR_MSG_DATA_TYPE_JSON;
            request->data = purc_variant_make_longint((int64_t)nr);
            purc_inst_move_message(inst_responser, request);
            pcrdr_release_message(request);
#endif
        }

    } while (true);

    n = purc_inst_destroy_move_buffer();
    purc_log_info("move buffer destroyed, %d messages discarded\n", (int)n);

    purc_cleanup();
    return NULL;
}

static purc_atom_t start_instance(pthread_t *th,
        const char *app, const char *run)
{
    struct instance_arg arg;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

ALLOW_DEPRECATED_DECLARATIONS_BEGIN
    sem_unlink("sync");
    arg.sync = sem_open("sync", O_CREAT | O_EXCL, 0644, 0);
    if (arg.sync == SEM_FAILED) {
        purc_log_error("failed to create semaphore: %s\n", strerror(errno));
        return 0;
    }
    arg.app = app;
    arg.run = run;
    int ret = pthread_create(th, NULL, general_instance_entry, &arg);
    if (ret) {
        sem_close(arg.sync);
        purc_log_error("failed to create thread for instance: %s/%s\n",
                app, run);
        return 0;
    }
    pthread_attr_destroy(&attr);

    sem_wait(arg.sync);
    sem_close(arg.sync);
ALLOW_DEPRECATED_DECLARATIONS_END

    return arg.atom;
}

struct instmgr_info {
    unsigned nr_insts;
    struct sorted_array *sa_insts;
};

static void get_instance(struct instmgr_info *info,
        const pcrdr_msg *request, pcrdr_msg *response)
{
    char endpoint_name[PURC_LEN_ENDPOINT_NAME + 1];

    const char *runner_name;
    runner_name = purc_variant_get_string_const(request->elementValue);

    if (!purc_is_valid_runner_name(runner_name)) {
        return;
    }

    purc_assemble_endpoint_name_ex(PCRDR_LOCALHOST,
            TEST_APP_NAME, runner_name,
            endpoint_name, sizeof(endpoint_name) - 1);

    purc_atom_t atom = purc_atom_try_string_ex(PURC_ATOM_BUCKET_DEF,
            endpoint_name);
    if (atom == 0) {
        pthread_t *th = (pthread_t *)malloc(sizeof(pthread_t));

        atom = start_instance(th, TEST_APP_NAME, runner_name);
        if (atom) {
            pcutils_sorted_array_add(info->sa_insts,
                    (void *)(uintptr_t)atom, (void *)th);
            info->nr_insts++;
        }
    }

    response->type = PCRDR_MSG_TYPE_RESPONSE;
    response->requestId = purc_variant_ref(request->requestId);
    response->sourceURI = purc_variant_make_string(purc_get_endpoint(NULL),
            false);
    response->retCode = PCRDR_SC_OK;
    response->resultValue = (uint64_t)atom;
    response->dataType = PCRDR_MSG_DATA_TYPE_VOID;
    response->data = PURC_VARIANT_INVALID;

    if (atom) {
        response->retCode = PCRDR_SC_OK;
        response->resultValue = (uint64_t)atom;
    }
    else {
        response->retCode = PCRDR_SC_CONFLICT;
        response->resultValue = 0;
    }
}

static void cancel_instance(struct instmgr_info *info,
        const pcrdr_msg *request, pcrdr_msg *response)
{
    char endpoint_name[PURC_LEN_ENDPOINT_NAME + 1];

    const char *runner_name;
    runner_name = purc_variant_get_string_const(request->elementValue);

    if (!purc_is_valid_runner_name(runner_name)) {
        return; /* invalid runner name */
    }

    response->type = PCRDR_MSG_TYPE_RESPONSE;
    response->requestId = purc_variant_ref(request->requestId);
    response->sourceURI = purc_variant_make_string(purc_get_endpoint(NULL),
            false);
    response->dataType = PCRDR_MSG_DATA_TYPE_VOID;
    response->data = PURC_VARIANT_INVALID;

    purc_assemble_endpoint_name_ex(PCRDR_LOCALHOST,
            TEST_APP_NAME, runner_name,
            endpoint_name, sizeof(endpoint_name) - 1);

    purc_atom_t atom = purc_atom_try_string_ex(PURC_ATOM_BUCKET_DEF,
            endpoint_name);
    if (atom == 0) {
        response->retCode = PCRDR_SC_NOT_FOUND;
        response->resultValue = (uint64_t)atom;
        return; /* not instance for the runner name */
    }

    pthread_t *th;
    if (!pcutils_sorted_array_find(info->sa_insts, (void *)(uintptr_t)atom,
            (void **)&th)) {
        response->retCode = PCRDR_SC_GONE;
        response->resultValue = (uint64_t)atom;
    }
    else {
        if (pthread_cancel(*th)) {
            pcutils_sorted_array_remove(info->sa_insts, (void *)(uintptr_t)atom);
            info->nr_insts--;
        }

        response->retCode = (uint64_t)PCRDR_SC_OK;
        response->resultValue = (uint64_t)atom;
    }

}

static void kill_instance(struct instmgr_info *info,
        const pcrdr_msg *request, pcrdr_msg *response)
{
    char endpoint_name[PURC_LEN_ENDPOINT_NAME + 1];

    const char *runner_name;
    runner_name = purc_variant_get_string_const(request->elementValue);

    if (!purc_is_valid_runner_name(runner_name)) {
        return; /* invalid runner name */
    }

    response->type = PCRDR_MSG_TYPE_RESPONSE;
    response->requestId = purc_variant_ref(request->requestId);
    response->sourceURI = purc_variant_make_string(purc_get_endpoint(NULL),
            false);
    response->dataType = PCRDR_MSG_DATA_TYPE_VOID;
    response->data = PURC_VARIANT_INVALID;

    purc_assemble_endpoint_name_ex(PCRDR_LOCALHOST,
            TEST_APP_NAME, runner_name,
            endpoint_name, sizeof(endpoint_name) - 1);

    purc_atom_t atom = purc_atom_try_string_ex(PURC_ATOM_BUCKET_DEF,
            endpoint_name);
    if (atom == 0) {
        response->retCode = PCRDR_SC_NOT_FOUND;
        response->resultValue = (uint64_t)atom;
        return; /* not instance for the runner name */
    }

    pthread_t *th;
    if (!pcutils_sorted_array_find(info->sa_insts, (void *)(uintptr_t)atom,
            (void **)&th)) {
        response->retCode = PCRDR_SC_GONE;
        response->resultValue = (uint64_t)atom;
    }
    else {
        pthread_kill(*th, SIGKILL);
        pcutils_sorted_array_remove(info->sa_insts, (void *)(uintptr_t)atom);
        info->nr_insts--;

        response->retCode = (uint64_t)PCRDR_SC_OK;
        response->resultValue = (uint64_t)atom;
    }
}

struct thread_arg {
    sem_t      *sync;
    purc_atom_t atom;
};

static void* instance_manager_entry(void* arg)
{
    struct thread_arg *my_arg = (struct thread_arg *)arg;

    // initial purc instance
    int ret = purc_init_ex(PURC_MODULE_EJSON,
            INSTMGR_APP_NAME, INSTMGR_RUN_NAME, NULL);
    assert(ret == PURC_ERROR_OK);

    purc_enable_log(false, false);

    my_arg->atom = purc_inst_create_move_buffer(PCINST_MOVE_BUFFER_BROADCAST, 16);
    purc_log_info("purc_inst_create_move_buffer returns: %x\n", my_arg->atom);
    sem_post(my_arg->sync);

    struct instmgr_info info = { 0, NULL };
    info.sa_insts = pcutils_sorted_array_create(SAFLAG_DEFAULT, 0,
            my_sa_free, NULL);
    assert(info.sa_insts);

    size_t n;
    do {
        ret = purc_inst_holding_messages_count(&n);

        if (ret) {
            purc_log_error("purc_inst_holding_messages_count failed: %d\n", ret);
        }
        else if (n > 0) {
            purc_log_info("purc_inst_holding_messages_count returns: %d\n", (int)n);

            pcrdr_msg *msg = purc_inst_take_away_message(0);
            if (msg->type == PCRDR_MSG_TYPE_REQUEST) {
                const char* source_uri;
                purc_atom_t requester;

                source_uri = purc_variant_get_string_const(msg->sourceURI);
                if (source_uri == NULL ||
                        (requester = purc_atom_try_string_ex(
                            PURC_ATOM_BUCKET_DEF, source_uri)) == 0) {
                    purc_log_info("No sourceURI (%s) or the requester disappeared\n",
                            source_uri);
                    pcrdr_release_message(msg);
                    continue;
                }

                const char *op;
                op = purc_variant_get_string_const(msg->operation);
                assert(op);
                purc_log_info("Got `%s` request from %s\n",
                        op, source_uri);

                pcrdr_msg *response = pcrdr_make_void_message();

                /* we ignore target and targetValue */
                if (strcmp(op, "quit") == 0) {
                    pcrdr_release_message(msg);
                    pcrdr_release_message(response);
                    break;
                }

                /* we use elementValue to pass the runner name */
                if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_ID) {
                    if (strcmp(op, "getInstance") == 0) {
                        get_instance(&info, msg, response);
                    }
                    else if (strcmp(op, "cancelInstance") == 0) {
                        cancel_instance(&info, msg, response);
                    }
                    else if (strcmp(op, "KillInstance") == 0) {
                        kill_instance(&info, msg, response);
                    }
                    else {
                        // unkown request.
                    }
                }

                if (response->type == PCRDR_MSG_TYPE_VOID) {
                    /* must be a bad request */
                    response->type = PCRDR_MSG_TYPE_RESPONSE;
                    response->requestId = purc_variant_ref(msg->requestId);
                    response->sourceURI = purc_variant_make_string(
                            purc_get_endpoint(NULL), false);
                    response->retCode = PCRDR_SC_BAD_REQUEST;
                    response->resultValue = 0;
                    response->dataType = PCRDR_MSG_DATA_TYPE_VOID;
                    response->data = PURC_VARIANT_INVALID;
                }

                const char *request_id;
                request_id = purc_variant_get_string_const(msg->requestId);
                if (strcmp(request_id, PCRDR_REQUESTID_NORETURN)) {
                    purc_inst_move_message(requester, response);
                }
                pcrdr_release_message(response);
            }
            else if (msg->type == PCRDR_MSG_TYPE_EVENT) {
                const char *event_name;
                event_name = purc_variant_get_string_const(msg->eventName);

                if (strcmp(event_name, "quit") == 0 &&
                        msg->target == PCRDR_MSG_TARGET_INSTANCE &&
                        msg->targetValue == 0) {
                    purc_log_info("got the quit from %s\n",
                            purc_variant_get_string_const(msg->sourceURI));
                    pcrdr_release_message(msg);
                    break;
                }
                else {
                    purc_log_info("got an event message not interested in:\n");
                    purc_log_info("    type:        %d\n", msg->type);
                    purc_log_info("    target:      %d\n", msg->target);
                    purc_log_info("    targetValue: %d\n", (int)msg->targetValue);
                    purc_log_info("    eventName:   %s\n", event_name);
                    purc_log_info("    sourceURI: %s\n",
                            purc_variant_get_string_const(msg->sourceURI));
                }
            }
            else if (msg->type == PCRDR_MSG_TYPE_RESPONSE) {
                purc_log_info("got a response message for request: %s from %s\n",
                        purc_variant_get_string_const(msg->requestId),
                        purc_variant_get_string_const(msg->sourceURI));
            }

            pcrdr_release_message(msg);
        }
        else {
            usleep(10000);  // 10m
        }

    } while (true);

    n = pcutils_sorted_array_count(info.sa_insts);
    for (size_t i = 0; i < n; i++) {
        pthread_t *th;
        purc_atom_t inst_atom = (purc_atom_t)(uintptr_t)pcutils_sorted_array_get(
                info.sa_insts, i, (void **)&th);

        const char *inst_endpoint = purc_atom_to_string(inst_atom);
        if (inst_endpoint) {
            purc_log_info("ask instance %s to shutdown\n", inst_endpoint);
            pcrdr_msg *request = pcrdr_make_request_message(
                    PCRDR_MSG_TARGET_INSTANCE, inst_atom,
                    "shutdown", PCRDR_REQUESTID_NORETURN, purc_get_endpoint(NULL),
                    PCRDR_MSG_ELEMENT_TYPE_VOID, NULL,
                    NULL,
                    PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);
            purc_inst_move_message(inst_atom, request);
            pcrdr_release_message(request);
        }
        else {
            purc_log_info("wrong instance atom: %u (%u)\n",
                    inst_atom, (unsigned)n);
        }

        pthread_join(*th, NULL);
    }

    pcutils_sorted_array_destroy(info.sa_insts);

    n = purc_inst_destroy_move_buffer();
    purc_log_info("move buffer destroyed, %d messages discarded\n", (int)n);

    purc_cleanup();
    return NULL;
}

static purc_atom_t start_instance_manager(pthread_t *th)
{
    int ret;
    struct thread_arg arg = { NULL, 0 };

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

ALLOW_DEPRECATED_DECLARATIONS_BEGIN
    sem_unlink("sync");
    arg.sync = sem_open("sync", O_CREAT | O_EXCL, 0644, 0);
    if (arg.sync == SEM_FAILED) {
        purc_log_error("failed to create semaphore: %s\n", strerror(errno));
        return -1;
    }
    ret = pthread_create(th, NULL, instance_manager_entry, &arg);
    if (ret) {
        sem_close(arg.sync);
        purc_log_error("failed to create thread for instance manager\n");
        return -1;
    }
    pthread_attr_destroy(&attr);

    sem_wait(arg.sync);
    sem_close(arg.sync);
ALLOW_DEPRECATED_DECLARATIONS_END

    assert(arg.atom);

    return arg.atom;
}

struct pending_request {
    struct list_head list;

    purc_variant_t      request_id;
    pcrdr_response_handler response_handler;
    void *context;

    time_t time_expected;
};

struct main_inst_info {
    unsigned nr_runners;
    unsigned nr_coroutines;
    unsigned nr_calls;
    purc_atom_t atom_instmgr;
    pthread_t th_instmgr;
    struct list_head pending_requests;
};

/*
 * Not that in a HVML interpreter instance, we can reuse the connection
 * to the renderer to maintain the pending requests.
 */
static int new_pending_request(struct main_inst_info *info,
        purc_variant_t request_id,
        pcrdr_response_handler response_handler, void *context)
{
    const char *tmp = purc_variant_get_string_const(request_id);
    assert(tmp);
    if (strcmp(tmp, PCRDR_REQUESTID_NORETURN) == 0) {
        return 0;
    }

    struct pending_request *pr;
    if ((pr = (struct pending_request *)malloc(sizeof(*pr))) == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    pr->request_id = purc_variant_ref(request_id);
    pr->response_handler = response_handler;
    pr->context = context;

    /* may the request should be returned in one second */
    pr->time_expected = purc_get_monotoic_time() + 1;
    list_add_tail(&pr->list, &info->pending_requests);
    return 0;
}

static size_t remove_timeout_pending_requests(struct main_inst_info *info)
{
    size_t nr = 0;
    struct pending_request *pr, *n;
    time_t curr_time = purc_get_monotoic_time();

    list_for_each_entry_safe(pr, n, &info->pending_requests, list) {
        if (curr_time > pr->time_expected) {
            pr->response_handler(NULL,
                    purc_variant_get_string_const(pr->request_id),
                    PCRDR_RESPONSE_TIMEOUT, pr->context, NULL);
        }
        list_del(&pr->list);
        purc_variant_unref(pr->request_id);
        free(pr);
        nr++;
    }

    return nr;
}

static size_t cleanup_all_pending_requests(struct main_inst_info *info)
{
    size_t nr = 0;
    struct pending_request *pr, *n;
    list_for_each_entry_safe(pr, n, &info->pending_requests, list) {
        if (pr->response_handler) {
            pr->response_handler(NULL,
                    purc_variant_get_string_const(pr->request_id),
                    PCRDR_RESPONSE_CANCELLED, pr->context, NULL);
        }
        list_del(&pr->list);
        purc_variant_unref(pr->request_id);
        free(pr);
        nr++;
    }

    return nr;
}

static inline int
variant_strcmp(purc_variant_t a, purc_variant_t b)
{
    if (a == b)
        return 0;

    return strcmp(
            purc_variant_get_string_const(a),
            purc_variant_get_string_const(b));
}

static int
handle_response_message(struct main_inst_info *info, const pcrdr_msg *msg)
{
    int retval = 0;

    if (!list_empty(&info->pending_requests)) {
        struct pending_request *pr;
        pr = list_first_entry(&info->pending_requests,
                struct pending_request, list);

        if (variant_strcmp(msg->requestId, pr->request_id) == 0) {
            if (pr->response_handler && pr->response_handler(NULL,
                        purc_variant_get_string_const(msg->requestId),
                        PCRDR_RESPONSE_RESULT, pr->context, msg) < 0) {
                retval = -1;
            }

            list_del(&pr->list);
            purc_variant_unref(pr->request_id);
            free(pr);
        }
        else {
            purc_set_error(PCRDR_ERROR_UNEXPECTED);
            retval = -1;
        }
    }
    else {
        purc_set_error(PCRDR_ERROR_UNEXPECTED);
        retval = -1;
    }

    return retval;
}

static pcrdr_msg *get_message(struct main_inst_info *info,
        unsigned timeout_seconds)
{
    size_t n;
    int ret;
    pcrdr_msg *msg = NULL;
    unsigned timeout_ms = timeout_seconds * 1000;
    unsigned wait_ms = 0;

    (void)info;

    do {
        ret = purc_inst_holding_messages_count(&n);
        if (ret) {
            purc_log_error("purc_inst_holding_messages_count failed\n");
            break;
        }
        else if (n > 0) {
            msg = purc_inst_take_away_message(0);
            break;
        }
        else {
            usleep(10000);  /* let me catch my breath */
            wait_ms += 10;
        }

    } while (wait_ms < timeout_ms);

    return msg;
}

static void wait_response_for_specific_request(struct main_inst_info *info,
        purc_variant_t request_id, pcrdr_response_handler response_handler,
        void *context, unsigned expected_seconds)
{
    pcrdr_msg *msg;

    do {
        msg = get_message(info, expected_seconds);
        if (msg == NULL) {
            response_handler(NULL,
                    purc_variant_get_string_const(request_id),
                    PCRDR_RESPONSE_TIMEOUT, context, NULL);
            break;
        }

        if (msg->type == PCRDR_MSG_TYPE_RESPONSE) {
            if (variant_strcmp(msg->requestId, request_id) == 0) {
                response_handler(NULL,
                        purc_variant_get_string_const(request_id),
                        PCRDR_RESPONSE_RESULT, context, msg);
                pcrdr_release_message(msg);
                break;
            }
            else {
                /* call handler according to the array of pending requests */
                handle_response_message(info, msg);
            }
        }
        else {
            /* handle other messages here */
            const char *source_uri;
            source_uri = purc_variant_get_string_const(msg->sourceURI);

            if (msg->type == PCRDR_MSG_TYPE_RESPONSE) {
                purc_log_info("got a response from %s\n", source_uri);
            }
            else if (msg->type == PCRDR_MSG_TYPE_REQUEST) {
                purc_log_info("got a request from %s\n", source_uri);
            }
            else if (msg->type == PCRDR_MSG_TYPE_EVENT) {
                purc_log_info("got an event from %s\n", source_uri);
            }
            else {
                purc_log_info("bad message from %s\n", source_uri);
            }
        }

        pcrdr_release_message(msg);

    } while (true);
}

static int on_call_method_handler(pcrdr_conn* conn,
        const char *request_id, int state,
        void *context, const pcrdr_msg *response)
{
    struct main_inst_info *info = (struct main_inst_info *)context;

    (void)conn;
    (void)request_id;

    if (state == PCRDR_RESPONSE_RESULT) {
        const char *source_uri;
        source_uri = purc_variant_get_string_const(response->sourceURI);
        purc_log_info("got response from %s for request %s: result: %d/%llu\n",
                source_uri, request_id,
                response->retCode, (unsigned long long)response->resultValue);

        if (response->retCode == PCRDR_SC_OK) {
            purc_atom_t cor_atom = (purc_atom_t)response->resultValue;
            const char *cor_id = purc_atom_to_string(cor_atom);

            char inst_endpoint[PURC_LEN_ENDPOINT_NAME + 1];
            const char *last_slash = strrchr(source_uri, '/');
            // strncpy(inst_endpoint, source_uri, last_slash - source_uri);
            snprintf(inst_endpoint, sizeof(inst_endpoint),
                    "%.*s", (int)(last_slash - source_uri), source_uri);
            purc_log_info("The instance URI: %s\n", inst_endpoint);

            purc_atom_t inst_atom = purc_atom_try_string_ex(
                    PURC_ATOM_BUCKET_DEF, inst_endpoint);
            if (inst_atom) {
                if (response->data) {
                    purc_log_info("Result returned from: %s:\n %s\n\n", cor_id,
                            purc_variant_get_string_const(response->data));
                }

                pcrdr_msg *request = pcrdr_make_request_message(
                        PCRDR_MSG_TARGET_COROUTINE, cor_atom,
                        "callMethod", NULL, purc_get_endpoint(NULL),
                        PCRDR_MSG_ELEMENT_TYPE_VOID, NULL,
                        NULL,
                        PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);
                request->dataType = PCRDR_MSG_DATA_TYPE_JSON;
                request->data = purc_variant_make_number((double)info->nr_calls);
                info->nr_calls++;

                purc_variant_t request_id = purc_variant_ref(request->requestId);
                purc_inst_move_message(inst_atom, request);
                pcrdr_release_message(request);

                new_pending_request(info, request_id,
                        on_call_method_handler, info);
                purc_variant_unref(request_id);
            }
            else {
                purc_log_error("Bad instance atom for source: %s\n",
                        source_uri);
            }
        }
    }
    else if (state == PCRDR_RESPONSE_TIMEOUT) {
        purc_log_warn("request %s timeout!\n", request_id);
    }
    else {
        purc_log_warn("request %s cancelled!\n", request_id);
    }

    return 0;
}

static int on_coroutine_ready(pcrdr_conn* conn,
        const char *request_id, int state,
        void *context, const pcrdr_msg *response)
{
    struct main_inst_info *info = (struct main_inst_info *)context;

    (void)conn;
    (void)request_id;

    if (state == PCRDR_RESPONSE_RESULT) {
        const char *source_uri;
        source_uri = purc_variant_get_string_const(response->sourceURI);
        purc_log_info("got response from %s for request %s: result: %d/%llu\n",
                source_uri, request_id,
                response->retCode, (unsigned long long)response->resultValue);

        if (response->retCode == PCRDR_SC_OK) {
            purc_atom_t cor_atom = (purc_atom_t)response->resultValue;
            const char *cor_id = purc_atom_to_string(cor_atom);

            purc_log_info("new coroutine in instance is ready now: %s\n",
                    cor_id);

            purc_atom_t inst_atom = purc_atom_try_string_ex(
                    PURC_ATOM_BUCKET_DEF, source_uri);
            if (inst_atom) {
                pcrdr_msg *request = pcrdr_make_request_message(
                        PCRDR_MSG_TARGET_COROUTINE, cor_atom,
                        "callMethod", NULL, purc_get_endpoint(NULL),
                        PCRDR_MSG_ELEMENT_TYPE_VOID, NULL,
                        NULL,
                        PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);

                request->dataType = PCRDR_MSG_DATA_TYPE_JSON;
                request->data = purc_variant_make_number((double)info->nr_calls);
                info->nr_calls++;

                purc_variant_t request_id = purc_variant_ref(request->requestId);
                purc_inst_move_message(inst_atom, request);
                pcrdr_release_message(request);

                new_pending_request(info, request_id,
                        on_call_method_handler, info);
                purc_variant_unref(request_id);
            }
        }
    }
    else if (state == PCRDR_RESPONSE_TIMEOUT) {
        purc_log_warn("request %s timeout!\n", request_id);
    }
    else {
        purc_log_warn("request %s cancelled!\n", request_id);
    }

    return 0;
}

static int on_instance_ready(pcrdr_conn* conn,
        const char *request_id, int state,
        void *context, const pcrdr_msg *response)
{
    struct main_inst_info *info = (struct main_inst_info *)context;

    (void)conn;
    (void)request_id;

    if (state == PCRDR_RESPONSE_RESULT) {
        char coroutine_name[32];
        sprintf(coroutine_name, "coroutine%u", info->nr_coroutines++);

        const char *source_uri;
        source_uri = purc_variant_get_string_const(response->sourceURI);
        purc_log_info("got response from %s for request %s: result: %d/%llu\n",
                source_uri, request_id,
                response->retCode, (unsigned long long)response->resultValue);

        if (response->retCode == PCRDR_SC_OK) {
            purc_atom_t inst_atom = (purc_atom_t)response->resultValue;
            const char *inst_endpoint = purc_atom_to_string(inst_atom);

            purc_log_info("new instance is ready now: %s\n", inst_endpoint);

            pcrdr_msg *request = pcrdr_make_request_message(
                    PCRDR_MSG_TARGET_INSTANCE, inst_atom,
                    "createCoroutine", NULL, purc_get_endpoint(NULL),
                    PCRDR_MSG_ELEMENT_TYPE_ID, coroutine_name,
                    NULL,
                    PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);

            purc_variant_t request_id = purc_variant_ref(request->requestId);
            purc_inst_move_message(inst_atom, request);
            pcrdr_release_message(request);

            wait_response_for_specific_request(info,
                    request_id, on_coroutine_ready, info, 1);
            purc_variant_unref(request_id);
        }
    }
    else if (state == PCRDR_RESPONSE_TIMEOUT) {
        purc_log_warn("request %s timeout!\n", request_id);
    }
    else {
        purc_log_warn("request %s cancelled!\n", request_id);
    }

    return 0;
}

static void fire(struct main_inst_info *info)
{
    const char *endpoint_name = purc_get_endpoint(NULL);
    assert(endpoint_name);
    char runner_name[16];
    sprintf(runner_name, "worker%u", info->nr_runners++);

    pcrdr_msg *request = pcrdr_make_request_message(
        PCRDR_MSG_TARGET_INSTANCE, info->atom_instmgr,
        "getInstance", NULL, endpoint_name,
        PCRDR_MSG_ELEMENT_TYPE_ID, runner_name,
        NULL,
        PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);

    purc_variant_t request_id = purc_variant_ref(request->requestId);
    purc_inst_move_message(info->atom_instmgr, request);
    pcrdr_release_message(request);

    wait_response_for_specific_request(info,
        request_id, on_instance_ready, info, 1);
    purc_variant_unref(request_id);
}

#define NR_MAX_CALLS     10

TEST(instance, requester)
{
    int ret;

    ret = purc_init_ex(PURC_MODULE_VARIANT, TEST_APP_NAME, TEST_RUN_NAME, NULL);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    purc_enable_log(false, false);

    purc_atom_t atom_requester =
            purc_inst_create_move_buffer(PCINST_MOVE_BUFFER_BROADCAST, 16);
    ASSERT_NE(atom_requester, 0);

    struct main_inst_info info;
    info.nr_runners = 0;
    info.nr_coroutines = 0;
    info.nr_calls = 0;
    info.atom_instmgr = start_instance_manager(&info.th_instmgr);
    list_head_init(&info.pending_requests);

    fire(&info);

    size_t n;
    do {
        pcrdr_msg *msg = get_message(&info, 1);
        if (msg == NULL) {
            n = remove_timeout_pending_requests(&info);
            purc_log_info("%u timeout pending requests discarded\n",
                    (unsigned)n);
            continue;
        }

        if (msg->type == PCRDR_MSG_TYPE_RESPONSE) {
            /* call handler according to the array of pending requests */
            handle_response_message(&info, msg);
        }
        else {
            /* handle other messages here */
            const char *source_uri;
            source_uri = purc_variant_get_string_const(msg->sourceURI);

            if (msg->type == PCRDR_MSG_TYPE_RESPONSE) {
                purc_log_info("got a response from %s\n", source_uri);
            }
            else if (msg->type == PCRDR_MSG_TYPE_REQUEST) {
                purc_log_info("got a request from %s\n", source_uri);
            }
            else if (msg->type == PCRDR_MSG_TYPE_EVENT) {
                purc_log_info("got an event from %s\n", source_uri);
            }
            else {
                purc_log_info("bad message from %s\n", source_uri);
            }
        }

        pcrdr_release_message(msg);

        if (info.nr_calls == NR_MAX_CALLS) {
            const char *endpoint_name = purc_get_endpoint(NULL);

            pcrdr_msg *request;
            request = pcrdr_make_request_message(
                    PCRDR_MSG_TARGET_INSTANCE, info.atom_instmgr,
                    "quit", PCRDR_REQUESTID_NORETURN, endpoint_name,
                    PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
                    PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);

            if (purc_inst_move_message(info.atom_instmgr, request) == 0) {
                purc_log_error("purc_inst_move_message: no recipient\n");
            }
            pcrdr_release_message(request);
            break;
        }

    } while (true);

    n = cleanup_all_pending_requests(&info);
    purc_log_info("%u pending requests discarded\n", (unsigned)n);

    n = purc_inst_destroy_move_buffer();
    purc_log_info("move buffer destroyed, %u messages discarded\n",
            (unsigned)n);

    pthread_join(info.th_instmgr, NULL);

    purc_cleanup();
}


