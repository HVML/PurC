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

#define TEST_APP_NAME       "cn.fmsoft.hvml.test"
#define TEST_RUN_NAME       "instmgr"

#define INSTMGR_APP_NAME        "cn.fmsoft.hvml"
#define INSTMGR_RUN_NAME        "manager"

struct instance_arg {
    sem_t      *sync;
    const char *app;
    const char *run;
    purc_atom_t atom;
};

struct inst_info {
    unsigned nr_cors;
    struct sorted_array *sa_cors;
};

static const char *request_handler0(struct inst_info *info,
        const pcrdr_msg *request)
{
    (void)info;
    (void)request;
    return "from handler0";
}

static const char *request_handler1(struct inst_info *info,
        const pcrdr_msg *request)
{
    (void)info;
    (void)request;
    return "from handler1";
}

static const char *request_handler2(struct inst_info *info,
        const pcrdr_msg *request)
{
    (void)info;
    (void)request;
    return "from handler2";
}

static const char *request_handler3(struct inst_info *info,
        const pcrdr_msg *request)
{
    (void)info;
    (void)request;
    return "from handler3";
}

typedef const char *(*fn_request_handler)(struct inst_info *info,
        const pcrdr_msg *request);

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
    assert(endpoint_name);  /* must be valid */

    response->type = PCRDR_MSG_TYPE_RESPONSE;
    response->requestId = purc_variant_ref(request->requestId);
    response->sourceURI = purc_variant_make_string(endpoint_name, false);
    response->dataType = PCRDR_MSG_DATA_TYPE_VOID;
    response->data = PURC_VARIANT_INVALID;

    purc_atom_t atom = (purc_atom_t)request->targetValue;
    fn_request_handler handler;
    if (!pcutils_sorted_array_find(info->sa_cors, (void *)(uintptr_t)atom,
            (void **)&handler)) {
        response->retCode = PCRDR_SC_NOT_FOUND;
        response->resultValue = 0;
    }
    else {
        const char *result = handler(info, request);

        response->dataType = PCRDR_MSG_DATA_TYPE_TEXT;
        response->data = purc_variant_make_string(result, false);
        response->retCode = PCRDR_SC_OK;
        response->resultValue = 0;
    }
}

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
    purc_atom_t self = purc_atom_from_string_ex2(PURC_ATOM_BUCKET_USER,
            full_cor_id, &newly_created);
    assert(newly_created);  /* must be a newly created atom */

    pcutils_sorted_array_add(info->sa_cors, (void *)(uintptr_t)self,
            (void *)handlers[self % sizeof(handlers)/sizeof(handlers[0])]);

    response->type = PCRDR_MSG_TYPE_RESPONSE;
    response->requestId = purc_variant_ref(request->requestId);
    response->sourceURI = purc_variant_make_string(endpoint_name, false);
    response->retCode = PCRDR_SC_OK;
    response->resultValue = (uint64_t)self;
    response->dataType = PCRDR_MSG_DATA_TYPE_VOID;
    response->data = PURC_VARIANT_INVALID;

    info->nr_cors++;
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

    void *handler;
    if (!pcutils_sorted_array_find(info->sa_cors, (void *)(uintptr_t)atom,
            &handler)) {
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

    purc_enable_log(true, false);

    inst_arg->atom =
        purc_inst_create_move_buffer(PCINST_MOVE_BUFFER_BROADCAST, 16);
    sem_post(inst_arg->sync);

    purc_log_info("purc_inst_create_move_buffer returns: %x\n",
            inst_arg->atom);

    struct inst_info info;
    info.nr_cors = 0;
    info.sa_cors = pcutils_sorted_array_create(SAFLAG_DEFAULT, 0,
            NULL, NULL);
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
                        (requester = purc_atom_try_string(source_uri)) == 0) {
                    purc_log_info("No sourceURI or the requester disappeared\n");
                    pcrdr_release_message(msg);
                    continue;
                }

                pcrdr_msg *response = pcrdr_make_void_message();

                const char *op;
                op = purc_variant_get_string_const(msg->operation);

                if (msg->target == PCRDR_MSG_TARGET_INSTANCE) {
                    if (strcmp(op, "createCoroutine") == 0) {
                        create_coroutine(&info, msg, response);
                    }
                    else if (strcmp(op, "killCoroutine") == 0) {
                        /* elementType is HANDLE
                           elementValue is the atom of a coroutine */
                        kill_coroutine(&info, msg, response);
                    }
                    else if (strcmp(op, "pauseCoroutine") == 0) {
                    }
                    else if (strcmp(op, "resumeCoroutine") == 0) {
                    }
                    else if (strcmp(op, "shutdown") == 0) {
                        shutdown_instance(&info, msg, response);
                        break;
                    }
                }
                else if (msg->target == PCRDR_MSG_TARGET_COROUTINE) {
                /* When the target of a request is a coroutine, the target
                   value is the atom value of the coroutine identifier.

                   Generally, a `callMethod` request sent to a coroutine
                   should be handled by a document variable. The manner is
                   similiar as the `callMethod` operation sent to the renderer.

                   For this purpose,

                   1. the `elementValue` of the message contains the
                      variable name; the `elementType` should be
                      `ELEMENT_TYPE_ID`.

                   2. the `data` of the message should be an object variant,
                      which contains the method name and the argument for
                      calling the method.

                   When we got such a request message, we should dispatch
                   the message to the target coroutine.
                   */
                    if (strcmp(op, "callMethod") == 0) {
                        call_method(&info, msg, response);
                    }
                }

                if (response->type == PCRDR_MSG_TYPE_VOID) {
                    /* must be a bad request */
                    response->type = PCRDR_MSG_TYPE_RESPONSE;
                    response->requestId = purc_variant_ref(msg->requestId);
                    response->sourceURI = purc_variant_make_string_static(
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

static void get_instance(const pcrdr_msg *request, pcrdr_msg *response)
{
    char endpoint_name[PURC_LEN_ENDPOINT_NAME + 1];

    const char *runner_name;
    runner_name = purc_variant_get_string_const(request->elementValue);

    if (!purc_is_valid_runner_name(runner_name)) {
        return;
    }

    int n;
    n = purc_assemble_endpoint_name_ex(PCRDR_LOCALHOST,
            TEST_APP_NAME, runner_name,
            endpoint_name, sizeof(endpoint_name) - 1);
    assert(n > 0 && n < (int)sizeof(endpoint_name));

    purc_atom_t atom = purc_atom_try_string(endpoint_name);
    if (atom == 0) {
        pthread_t th;
        atom = start_instance(&th, TEST_APP_NAME, runner_name);
    }

    if (atom) {
        response->type = PCRDR_MSG_TYPE_RESPONSE;
        response->requestId = purc_variant_ref(request->requestId);
        response->sourceURI = purc_variant_make_string(purc_get_endpoint(NULL),
                false);
        response->retCode = PCRDR_SC_OK;
        response->resultValue = (uint64_t)atom;
        response->dataType = PCRDR_MSG_DATA_TYPE_VOID;
        response->data = PURC_VARIANT_INVALID;
    }
}

static void cancel_instance(const pcrdr_msg *request, pcrdr_msg *response)
{
    char endpoint_name[PURC_LEN_ENDPOINT_NAME + 1];

    const char *runner_name;
    runner_name = purc_variant_get_string_const(request->elementValue);

    if (!purc_is_valid_runner_name(runner_name)) {
        return; /* invalid runner name */
    }

    int n;
    n = purc_assemble_endpoint_name_ex(PCRDR_LOCALHOST,
            TEST_APP_NAME, runner_name,
            endpoint_name, sizeof(endpoint_name) - 1);
    assert(n > 0 && n < (int)sizeof(endpoint_name));

    purc_atom_t atom = purc_atom_try_string(endpoint_name);
    if (atom == 0) {
        return; /* not instance for the runner name */
    }

    response->type = PCRDR_MSG_TYPE_RESPONSE;
    response->requestId = purc_variant_ref(request->requestId);
    response->sourceURI = purc_variant_make_string(purc_get_endpoint(NULL),
            false);
    response->retCode = PCRDR_SC_NOT_IMPLEMENTED;
    response->resultValue = (uint64_t)atom;
    response->dataType = PCRDR_MSG_DATA_TYPE_VOID;
    response->data = PURC_VARIANT_INVALID;
}

static void kill_instance(const pcrdr_msg *request, pcrdr_msg *response)
{
    char endpoint_name[PURC_LEN_ENDPOINT_NAME + 1];

    const char *runner_name;
    runner_name = purc_variant_get_string_const(request->elementValue);

    if (!purc_is_valid_runner_name(runner_name)) {
        return; /* invalid runner name */
    }

    int n;
    n = purc_assemble_endpoint_name_ex(PCRDR_LOCALHOST,
            TEST_APP_NAME, runner_name,
            endpoint_name, sizeof(endpoint_name) - 1);
    assert(n > 0 && n < (int)sizeof(endpoint_name));

    purc_atom_t atom = purc_atom_try_string(endpoint_name);
    if (atom == 0) {
        return; /* not instance for the runner name */
    }

    response->type = PCRDR_MSG_TYPE_RESPONSE;
    response->requestId = purc_variant_ref(request->requestId);
    response->sourceURI = purc_variant_make_string(purc_get_endpoint(NULL),
            false);
    response->retCode = PCRDR_SC_NOT_IMPLEMENTED;
    response->resultValue = (uint64_t)atom;
    response->dataType = PCRDR_MSG_DATA_TYPE_VOID;
    response->data = PURC_VARIANT_INVALID;
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

    purc_enable_log(true, false);
    my_arg->atom = purc_inst_create_move_buffer(PCINST_MOVE_BUFFER_BROADCAST, 16);
    purc_log_info("purc_inst_create_move_buffer returns: %x\n", my_arg->atom);
    sem_post(my_arg->sync);

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
                        (requester = purc_atom_try_string(source_uri)) == 0) {
                    purc_log_info("No sourceURI or the requester disappeared\n");
                    pcrdr_release_message(msg);
                    continue;
                }

                const char *op;
                op = purc_variant_get_string_const(msg->operation);
                assert(op);
                purc_log_info("got a %s request from %s\n",
                        op, source_uri);

                pcrdr_msg *response = pcrdr_make_void_message();

                /* we ignore target and targetValue */

                /* we use elementValue to pass the runner name */
                if (msg->elementType == PCRDR_MSG_ELEMENT_TYPE_ID) {
                    if (strcmp(op, "getInstance") == 0) {
                        get_instance(msg, response);
                    }
                    else if (strcmp(op, "cancelInstance") == 0) {
                        cancel_instance(msg, response);
                    }
                    else if (strcmp(op, "KillInstance") == 0) {
                        kill_instance(msg, response);
                    }
                    else if (strcmp(op, "quit") == 0) {
                        break;
                    }
                    else {
                        // unkown request.
                    }
                }

                if (response->type == PCRDR_MSG_TYPE_VOID) {
                    /* must be a bad request */
                    response->type = PCRDR_MSG_TYPE_RESPONSE;
                    response->requestId = purc_variant_ref(msg->requestId);
                    response->sourceURI = purc_variant_make_string_static(
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

/*
 * Not that in a HVML interpreter instance, we can reuse the connection
 * to the renderer to maintain the pending requests.
 */
static LIST_HEAD(pending_requests);

static int new_pending_request(purc_variant_t request_id,
        pcrdr_response_handler response_handler, void *context)
{
    struct pending_request *pr;
    if ((pr = (struct pending_request *)malloc(sizeof(*pr))) == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    pr->request_id = purc_variant_ref(request_id);
    pr->response_handler = response_handler;
    pr->context = context;
    pr->time_expected = purc_get_monotoic_time() + 1;
    list_add_tail(&pr->list, &pending_requests);
    return 0;
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
handle_response_message(const pcrdr_msg *msg)
{
    int retval = 0;

    if (!list_empty(&pending_requests)) {
        struct pending_request *pr;
        pr = list_first_entry(&pending_requests,
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

static int nr_runners;
static purc_atom_t atom_instmgr;

int on_instance_ready(pcrdr_conn* conn,
        const char *request_id, int state,
        void *context, const pcrdr_msg *response_msg)
{
    (void)conn;
    (void)request_id;
    (void)state;
    (void)context;
    (void)response_msg;

    return 0;
}

void fire(void)
{
    const char *endpoint_name = purc_get_endpoint(NULL);
    assert(endpoint_name);
    char runner_name[16];
    sprintf(runner_name, "worker%d", nr_runners);

    pcrdr_msg *request = pcrdr_make_request_message(
        PCRDR_MSG_TARGET_INSTANCE, atom_instmgr,
        "getInstance", NULL, endpoint_name,
        PCRDR_MSG_ELEMENT_TYPE_ID, runner_name,
        NULL,
        PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);

    new_pending_request(request->requestId,
        on_instance_ready, NULL);

    purc_inst_move_message(atom_instmgr, request);
    pcrdr_release_message(request);
}

#define NR_MAX_MESSAGES     100

TEST(instance, requester)
{
    int ret;

    ret = purc_init_ex(PURC_MODULE_VARIANT, TEST_APP_NAME, TEST_RUN_NAME, NULL);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    purc_enable_log(true, false);

    purc_atom_t atom_requester =
            purc_inst_create_move_buffer(PCINST_MOVE_BUFFER_BROADCAST, 16);
    ASSERT_NE(atom_requester, 0);

    pthread_t th_instmgr;
    atom_instmgr = start_instance_manager(&th_instmgr);

    fire();

    size_t n;
    int nr_got = 0;
    do {
        ret = purc_inst_holding_messages_count(&n);
        if (ret) {
            purc_log_error("purc_inst_holding_messages_count failed\n");
            break;
        }
        else if (n > 0) {
            pcrdr_msg *msg = purc_inst_take_away_message(0);

            const char *source_uri;
            source_uri = purc_variant_get_string_const(msg->sourceURI);
            ASSERT_NE(source_uri, nullptr);

            if (msg->type == PCRDR_MSG_TYPE_RESPONSE) {
                purc_log_info("got a response from %s\n", source_uri);
                /* call handler according to the array of pending requests */
                handle_response_message(msg);
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

            pcrdr_release_message(msg);

            nr_got++;
            if (nr_got == NR_MAX_MESSAGES) {
                const char *endpoint_name = purc_get_endpoint(NULL);

                pcrdr_msg *request;
                request = pcrdr_make_request_message(
                        PCRDR_MSG_TARGET_INSTANCE, atom_instmgr,
                        "quit", PCRDR_REQUESTID_NORETURN, endpoint_name,
                        PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
                        PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);

                if (purc_inst_move_message(atom_instmgr, request) == 0) {
                    purc_log_error("purc_inst_move_message: no recipient\n");
                }
                pcrdr_release_message(request);
                break;
            }
        }
        else {
            usleep(10000);  // 10m
        }

    } while (true);

    n = purc_inst_destroy_move_buffer();
    purc_log_info("move buffer destroyed, %d messages discarded\n", (int)n);

    pthread_join(th_instmgr, NULL);

    purc_cleanup();
}


