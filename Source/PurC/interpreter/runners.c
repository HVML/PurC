/*
 * @file runners.c
 * @author Vincent Wei
 * @date 2022/07/05
 * @brief The implementation of purc_inst_xxx APIs.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
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

//#undef NDEBUG

#include "config.h"

#include "purc.h"
#include "private/runners.h"
#include "private/instance.h"
#include "private/sorted-array.h"
#include "private/ports.h"
#include "internal.h"

#include <assert.h>
#include <errno.h>

static void create_coroutine(const pcrdr_msg *msg, pcrdr_msg *response)
{
    if (msg->dataType != PCRDR_MSG_DATA_TYPE_JSON) {
        purc_log_warn("Bad request data type: %d\n", msg->dataType);
        return;
    }

    assert(msg->data);

    purc_variant_t tmp;

    purc_vdom_t vdom = NULL;
    tmp = purc_variant_object_get_by_ckey(msg->data, "vdom");
    if (tmp && purc_variant_is_ulongint(tmp)) {
        uint64_t u64;
        purc_variant_cast_to_ulongint(tmp, &u64, false);
        vdom = (purc_vdom_t)(uintptr_t)u64;
    }

    if (vdom == NULL) {
        purc_log_warn("Bad vDOM (%p)\n", vdom);
        return;
    }

    purc_atom_t curator = 0;
    tmp = purc_variant_object_get_by_ckey(msg->data, "curator");
    if (tmp && purc_variant_is_ulongint(tmp)) {
        uint64_t u64;
        purc_variant_cast_to_ulongint(tmp, &u64, false);
        curator = (purc_atom_t)u64;
    }

    pcrdr_page_type_k page_type = PCRDR_PAGE_TYPE_NULL;
    tmp = purc_variant_object_get_by_ckey(msg->data, "pageType");
    if (tmp && purc_variant_is_ulongint(tmp)) {
        uint64_t u64;
        purc_variant_cast_to_ulongint(tmp, &u64, false);
        page_type = (pcrdr_page_type_k)u64;
    }

    purc_variant_t request;
    request = purc_variant_object_get_by_ckey(msg->data, "request");
    if (request)
        purc_variant_ref(request);

    const char *target_workspace = NULL;
    tmp = purc_variant_object_get_by_ckey(msg->data, "targetWorkspace");
    if (tmp) {
        target_workspace = purc_variant_get_string_const(tmp);
    }

    const char *target_group = NULL;
    tmp = purc_variant_object_get_by_ckey(msg->data, "targetGroup");
    if (tmp) {
        target_group = purc_variant_get_string_const(tmp);
    }

    const char *page_name = NULL;
    tmp = purc_variant_object_get_by_ckey(msg->data, "pageName");
    if (tmp) {
        page_name = purc_variant_get_string_const(tmp);
    }

    purc_renderer_extra_info extra_rdr_info = {};

    tmp = purc_variant_object_get_by_ckey(msg->data, "class");
    if (tmp) {
        extra_rdr_info.klass = purc_variant_get_string_const(tmp);
    }

    tmp = purc_variant_object_get_by_ckey(msg->data, "title");
    if (tmp) {
        extra_rdr_info.title = purc_variant_get_string_const(tmp);
    }

    tmp = purc_variant_object_get_by_ckey(msg->data, "layoutStyle");
    if (tmp) {
        extra_rdr_info.layout_style = purc_variant_get_string_const(tmp);
    }

    extra_rdr_info.toolkit_style =
        purc_variant_object_get_by_ckey(msg->data, "toolkitStyle");
    if (extra_rdr_info.toolkit_style)
        purc_variant_ref(extra_rdr_info.toolkit_style);

    tmp = purc_variant_object_get_by_ckey(msg->data, "transitionStyle");
    if (tmp) {
        extra_rdr_info.transition_style = purc_variant_get_string_const(tmp);
    }

    tmp = purc_variant_object_get_by_ckey(msg->data, "pageGroups");
    if (tmp) {
        extra_rdr_info.page_groups = purc_variant_get_string_const(tmp);
    }

    const char *body_id = NULL;
    tmp = purc_variant_object_get_by_ckey(msg->data, "bodyId");
    if (tmp) {
        body_id = purc_variant_get_string_const(tmp);
    }

    purc_coroutine_t cor = purc_schedule_vdom(vdom, curator,
            request, page_type, target_workspace,
            target_group, page_name, &extra_rdr_info, body_id, NULL);
    if (request)
        purc_variant_unref(request);
    if (extra_rdr_info.toolkit_style)
        purc_variant_unref(extra_rdr_info.toolkit_style);

    if (cor) {
        purc_atom_t cor_atom = purc_coroutine_identifier(cor);

        const char *endpoint_name = purc_get_endpoint(NULL);
        response->type = PCRDR_MSG_TYPE_RESPONSE;
        response->requestId = purc_variant_ref(msg->requestId);
        response->sourceURI = purc_variant_make_string(endpoint_name, false);
        response->retCode = PCRDR_SC_OK;
        response->resultValue = (uint64_t)cor_atom;
        response->dataType = PCRDR_MSG_DATA_TYPE_VOID;
        response->data = PURC_VARIANT_INVALID;
    }
}

static int shutdown_instance(purc_atom_t requester,
        const pcrdr_msg *msg, pcrdr_msg *response)
{
    purc_atom_t self_atom;
    const char *self_ept = purc_get_endpoint(&self_atom);
    if (self_atom != requester) {
        const char *rqst_ept = purc_variant_get_string_const(msg->sourceURI);

        char self_host_name[PURC_LEN_HOST_NAME + 1];
        purc_extract_host_name(self_ept, self_host_name);

        char self_app_name[PURC_LEN_APP_NAME + 1];
        purc_extract_app_name(self_ept, self_app_name);

        char rqst_host_name[PURC_LEN_HOST_NAME + 1];
        purc_extract_host_name(rqst_ept, rqst_host_name);

        char rqst_app_name[PURC_LEN_APP_NAME + 1];
        purc_extract_app_name(rqst_ept, rqst_app_name);

        if (strcmp(self_host_name, rqst_host_name) ||
                (strcmp(rqst_app_name, PCRUN_INSTMGR_APP_NAME) &&
                strcmp(self_app_name, rqst_app_name))) {
            // not allowed
            return -1;
        }
    }

    struct pcinst *inst = pcinst_current();
    assert(inst && inst->intr_heap);
    if (inst->intr_heap->cond_handler) {
        if (inst->intr_heap->cond_handler(PURC_COND_SHUTDOWN_ASKED,
                (void*)msg, NULL) == 0) {
            inst->keep_alive = 0;
        }
        else {
            inst->keep_alive = 1;
        }
    }
    else {
        inst->keep_alive = 0;
    }

    response->type = PCRDR_MSG_TYPE_RESPONSE;
    response->requestId = purc_variant_ref(msg->requestId);
    response->sourceURI = purc_variant_make_string(self_ept, false);
    response->retCode = inst->keep_alive ? PCRDR_SC_FORBIDDEN : PCRDR_SC_OK;
    response->resultValue = 0;
    response->dataType = PCRDR_MSG_DATA_TYPE_VOID;
    response->data = PURC_VARIANT_INVALID;
    return 0;
}

/*
   A request message sent to the instance can be used to manage
   the coroutines, for example, create or kill a coroutine. This type
   of request can also be used to implement the debugger. The debugger
   can send the operations like `pauseCoroutine` or `resumeCoroutine`
   to control the execution of a coroutine.

   When controlling an existing coroutine, we use `elementValue` to
   pass the atom value of the target coroutine. In this situation,
   the `elementType` should be `PCRDR_MSG_ELEMENT_HANDLE`.

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
   to the target coroutine. And the coroutine should prepare a virtual
   stack frame to call the operation group in the scope of the specified
   element. The result of the operation group should be sent back to the caller
   as a response message.

   In this way, the coroutine can act as a service provider for others.
 */
void pcrun_request_handler(pcrdr_conn* conn, const pcrdr_msg *msg)
{
    UNUSED_PARAM(conn);

    const char* source_uri;
    purc_atom_t requester;

    source_uri = purc_variant_get_string_const(msg->sourceURI);
    if (source_uri == NULL || (requester =
                purc_atom_try_string_ex(PURC_ATOM_BUCKET_DEF,
                    source_uri)) == 0) {
        purc_log_warn("No sourceURI or the requester disappeared\n");
        return;
    }

    pcrdr_msg *response = pcrdr_make_void_message();

    const char *op;
    op = purc_variant_get_string_const(msg->operation);
    assert(op);

    PC_DEBUG("%s got `%s` request from %s\n",
            purc_get_endpoint(NULL), op, source_uri);

    int ok_to_shutdown = -1;
    if (msg->target == PCRDR_MSG_TARGET_INSTANCE) {
        if (strcmp(op, PCRUN_OPERATION_createCoroutine) == 0) {
            create_coroutine(msg, response);
        }
        else if (strcmp(op, PCRUN_OPERATION_killCoroutine) == 0) {
            purc_log_warn("Not implemented operation: %s\n", op);
        }
        else if (strcmp(op, PCRUN_OPERATION_pauseCoroutine) == 0) {
            purc_log_warn("Not implemented operation: %s\n", op);
        }
        else if (strcmp(op, PCRUN_OPERATION_resumeCoroutine) == 0) {
            purc_log_warn("Not implemented operation: %s\n", op);
        }
        else if (strcmp(op, PCRUN_OPERATION_shutdownInstance) == 0) {
            ok_to_shutdown = shutdown_instance(requester, msg, response);
        }
        else {
            struct pcinst *inst = pcinst_current();
            assert(inst && inst->intr_heap);
            if (inst->intr_heap->cond_handler) {
                inst->intr_heap->cond_handler(PURC_COND_UNK_REQUEST,
                        (void*)msg, response);
            }
            else {
                purc_log_warn("Unknown operation: %s\n", op);
            }
        }
    }
    else if (msg->target == PCRDR_MSG_TARGET_COROUTINE) {
        struct pcinst *inst = pcinst_current();
        if (strcmp(op, PCRDR_OP2INTR_SUPPRESSPAGE) == 0) {
            pcintr_suppress_crtn_doc(inst, NULL, msg->targetValue);
        }
        else if (strcmp(op, PCRDR_OP2INTR_RELOADPAGE) == 0) {
            pcintr_reload_crtn_doc(inst, conn, NULL, msg->targetValue);
        }
        else if (strcmp(op, PCRDR_OPERATION_CALLMETHOD) == 0) {
            purc_log_warn("Not implemented operation: %s\n", op);
        }
        else {
            purc_log_warn("Unknown operation: %s\n", op);
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

    struct pcinst *inst = pcinst_current();
    if (ok_to_shutdown == 0 && inst->keep_alive == 0 &&
            list_empty(&inst->intr_heap->crtns) &&
            list_empty(&inst->intr_heap->stopped_crtns)) {
        purc_runloop_stop(inst->running_loop);
        PC_DEBUG("Called purc_runloop_stop()\n");
    }
}

pcrdr_msg *pcrun_extra_message_source(pcrdr_conn* conn, void *ctxt)
{
    UNUSED_PARAM(conn);
    UNUSED_PARAM(ctxt);

    size_t n;

    int ret = purc_inst_holding_messages_count(&n);
    if (ret) {
        purc_log_error("Failed purc_inst_holding_messages_count(): %s(%d)\n",
                purc_get_error_message(ret), ret);
    }
    else if (n > 0) {
        return purc_inst_take_away_message(0);
    }

    return NULL;
}

void
pcrun_notify_instmgr(const char* event_name, purc_atom_t inst_crtn_id)
{
    purc_atom_t instmgr = purc_get_instmgr_rid();
    assert(instmgr != 0);

    pcrdr_msg *event;
    event = pcrdr_make_event_message(
            PCRDR_MSG_TARGET_INSTANCE, instmgr,
            event_name, purc_get_endpoint(NULL),
            PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
            PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);
    assert(event);
    event->elementType = PCRDR_MSG_ELEMENT_TYPE_VARIANT;
    event->elementValue = purc_variant_make_ulongint(inst_crtn_id);

    // move the event message to instance manager
    if (purc_inst_move_message(instmgr, event) == 0) {
        purc_log_error("no instance manager\n");
    }

    pcrdr_release_message(event);
}

static void create_instance(struct instmgr_info *mgr_info,
        const pcrdr_msg *request, pcrdr_msg *response)
{
    char endpoint_name[PURC_LEN_ENDPOINT_NAME + 1];

    if (!purc_variant_is_object(request->data)) {
        return;
    }

    purc_variant_t tmp;

    const char *app_name = NULL;
    tmp = purc_variant_object_get_by_ckey(request->data, "appName");
    if (tmp) {
        app_name = purc_variant_get_string_const(tmp);
    }

    const char *runner_name = NULL;
    tmp = purc_variant_object_get_by_ckey(request->data, "runnerName");
    if (tmp) {
        runner_name = purc_variant_get_string_const(tmp);
    }

    if (app_name == NULL || runner_name == NULL ||
            !purc_is_valid_app_name(app_name) ||
            !purc_is_valid_runner_name(runner_name)) {
        return;
    }

    purc_assemble_endpoint_name_ex(PCRDR_LOCALHOST,
            app_name, runner_name,
            endpoint_name, sizeof(endpoint_name) - 1);
    purc_atom_t atom = purc_atom_try_string_ex(PURC_ATOM_BUCKET_DEF,
            endpoint_name);
    if (atom) {
        goto done;
    }

    purc_cond_handler cond_handler = NULL;
    tmp = purc_variant_object_get_by_ckey(request->data, "condHandler");
    if (tmp) {
        uint64_t u64;
        purc_variant_cast_to_ulongint(tmp, &u64, false);
        cond_handler = (purc_cond_handler)(uintptr_t)u64;
    }

    struct purc_instance_extra_info info = {};

    tmp = purc_variant_object_get_by_ckey(request->data, "rendererProt");
    if (tmp && purc_variant_is_ulongint(tmp)) {
        uint64_t u64;
        purc_variant_cast_to_ulongint(tmp, &u64, false);
        info.renderer_comm = (purc_rdrcomm_k)u64;
    }

    tmp = purc_variant_object_get_by_ckey(request->data, "keepAlive");
    if (tmp && purc_variant_is_boolean(tmp)) {
        info.keep_alive = purc_variant_booleanize(tmp);
    }

    tmp = purc_variant_object_get_by_ckey(request->data, "allowSwitchingRdr");
    if (tmp && purc_variant_is_boolean(tmp)) {
        info.allow_switching_rdr = purc_variant_booleanize(tmp);
    }

    tmp = purc_variant_object_get_by_ckey(request->data, "allowScalingByDensity");
    if (tmp && purc_variant_is_boolean(tmp)) {
        info.allow_scaling_by_density = purc_variant_booleanize(tmp);
    }

    tmp = purc_variant_object_get_by_ckey(request->data, "rendererURI");
    if (tmp) {
        info.renderer_uri = purc_variant_get_string_const(tmp);
    }

    tmp = purc_variant_object_get_by_ckey(request->data, "sslCert");
    if (tmp) {
        info.ssl_cert = purc_variant_get_string_const(tmp);
    }

    tmp = purc_variant_object_get_by_ckey(request->data, "sslKey");
    if (tmp) {
        info.ssl_key = purc_variant_get_string_const(tmp);
    }

    tmp = purc_variant_object_get_by_ckey(request->data, "workspaceName");
    if (tmp) {
        info.workspace_name = purc_variant_get_string_const(tmp);
    }

    tmp = purc_variant_object_get_by_ckey(request->data, "workspaceTitle");
    if (tmp) {
        info.workspace_title = purc_variant_get_string_const(tmp);
    }

    tmp = purc_variant_object_get_by_ckey(request->data, "workspaceLayout");
    if (tmp) {
        info.workspace_layout = purc_variant_get_string_const(tmp);
    }

    void *th = NULL;
    atom = pcrun_create_inst_thread(app_name, runner_name, cond_handler,
            &info, &th);
    if (atom) {
        pcutils_sorted_array_add(mgr_info->sa_insts,
                (void *)(uintptr_t)atom, th, NULL);
        mgr_info->nr_insts++;
    }

done:
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

    if (!purc_variant_is_object(request->data)) {
        return;
    }

    purc_variant_t tmp;

    const char *app_name = NULL;
    tmp = purc_variant_object_get_by_ckey(request->data, "appName");
    if (tmp) {
        app_name = purc_variant_get_string_const(tmp);
    }

    const char *runner_name = NULL;
    tmp = purc_variant_object_get_by_ckey(request->data, "runnerName");
    if (tmp) {
        runner_name = purc_variant_get_string_const(tmp);
    }

    if (app_name == NULL || runner_name == NULL ||
            !purc_is_valid_app_name(app_name) ||
            !purc_is_valid_runner_name(runner_name)) {
        return;
    }

    response->type = PCRDR_MSG_TYPE_RESPONSE;
    response->requestId = purc_variant_ref(request->requestId);
    response->sourceURI = purc_variant_make_string(purc_get_endpoint(NULL),
            false);
    response->dataType = PCRDR_MSG_DATA_TYPE_VOID;
    response->data = PURC_VARIANT_INVALID;

    purc_assemble_endpoint_name_ex(PCRDR_LOCALHOST,
            app_name, runner_name,
            endpoint_name, sizeof(endpoint_name) - 1);

    purc_atom_t atom = purc_atom_try_string_ex(PURC_ATOM_BUCKET_DEF,
            endpoint_name);
    if (atom == 0) {
        response->retCode = PCRDR_SC_NOT_FOUND;
        response->resultValue = (uint64_t)atom;
        return; /* not instance for the runner name */
    }

    void *th;
    if (!pcutils_sorted_array_find(info->sa_insts, (void *)(uintptr_t)atom,
            (void **)&th, NULL)) {
        response->retCode = PCRDR_SC_GONE;
        response->resultValue = (uint64_t)atom;
    }
    else {
#if 0 // TODO
        if (pthread_cancel(*th)) {
            pcutils_sorted_array_remove(info->sa_insts, (void *)(uintptr_t)atom);
            info->nr_insts--;
        }
        response->retCode = PCRDR_SC_OK;
        response->resultValue = (uint64_t)atom;
#else
        response->retCode = PCRDR_SC_NOT_IMPLEMENTED;
        response->resultValue = (uint64_t)atom;
#endif
    }
}

static void kill_instance(struct instmgr_info *info,
        const pcrdr_msg *request, pcrdr_msg *response)
{
    char endpoint_name[PURC_LEN_ENDPOINT_NAME + 1];

    if (!purc_variant_is_object(request->data)) {
        return;
    }

    purc_variant_t tmp;

    const char *app_name = NULL;
    tmp = purc_variant_object_get_by_ckey(request->data, "appName");
    if (tmp) {
        app_name = purc_variant_get_string_const(tmp);
    }

    const char *runner_name = NULL;
    tmp = purc_variant_object_get_by_ckey(request->data, "runnerName");
    if (tmp) {
        runner_name = purc_variant_get_string_const(tmp);
    }

    if (app_name == NULL || runner_name == NULL ||
            !purc_is_valid_app_name(app_name) ||
            !purc_is_valid_runner_name(runner_name)) {
        return;
    }

    response->type = PCRDR_MSG_TYPE_RESPONSE;
    response->requestId = purc_variant_ref(request->requestId);
    response->sourceURI = purc_variant_make_string(purc_get_endpoint(NULL),
            false);
    response->dataType = PCRDR_MSG_DATA_TYPE_VOID;
    response->data = PURC_VARIANT_INVALID;

    purc_assemble_endpoint_name_ex(PCRDR_LOCALHOST,
            app_name, runner_name,
            endpoint_name, sizeof(endpoint_name) - 1);

    purc_atom_t atom = purc_atom_try_string_ex(PURC_ATOM_BUCKET_DEF,
            endpoint_name);
    if (atom == 0) {
        response->retCode = PCRDR_SC_NOT_FOUND;
        response->resultValue = (uint64_t)atom;
        return; /* not instance for the runner name */
    }

    void *th;
    if (!pcutils_sorted_array_find(info->sa_insts, (void *)(uintptr_t)atom,
            (void **)&th, NULL)) {
        response->retCode = PCRDR_SC_GONE;
        response->resultValue = (uint64_t)atom;
    }
    else {
#if 0 // TODO
        pthread_kill(*th, SIGKILL);
        pcutils_sorted_array_remove(info->sa_insts, (void *)(uintptr_t)atom);
        info->nr_insts--;

        response->retCode = PCRDR_SC_OK;
        response->resultValue = (uint64_t)atom;
#else
        response->retCode = PCRDR_SC_NOT_IMPLEMENTED;
        response->resultValue = (uint64_t)atom;
#endif
    }
}

void pcrun_instmgr_handle_message(void *ctxt)
{
    struct instmgr_info *info = ctxt;

    size_t n;
    int ret = purc_inst_holding_messages_count(&n);
    if (ret) {
        purc_log_error("Failed to check messages in move buffer: %d\n", ret);
        return;
    }
    else if (n == 0) {
        // sleep 1ms to take a breath
        pcutils_usleep(1000);
        return;
    }

    /* there is a new message */
    pcrdr_msg *msg = purc_inst_take_away_message(0);
    if (msg->type == PCRDR_MSG_TYPE_REQUEST) {
        const char* source_uri;
        purc_atom_t requester;

        source_uri = purc_variant_get_string_const(msg->sourceURI);
        if (source_uri == NULL || (requester =
                    purc_atom_try_string_ex( PURC_ATOM_BUCKET_DEF,
                        source_uri)) == 0) {
            purc_log_info("No sourceURI (%s) or the requester disappeared\n",
                    source_uri);
            pcrdr_release_message(msg);
            return;
        }

        const char *op;
        op = purc_variant_get_string_const(msg->operation);
        assert(op);
        PC_DEBUG("InstMgr got `%s` request from %s\n", op, source_uri);

        pcrdr_msg *response = pcrdr_make_void_message();

        if (strcmp(op, PCRUN_OPERATION_createInstance) == 0) {
            create_instance(info, msg, response);
        }
        else if (strcmp(op, PCRUN_OPERATION_cancelInstance) == 0) {
            cancel_instance(info, msg, response);
        }
        else if (strcmp(op, PCRUN_OPERATION_killInstance) == 0) {
            kill_instance(info, msg, response);
        }
        else {
            purc_log_warn("InstMgr got an unknown `%s` request from %s\n",
                    op, source_uri);
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

        PC_DEBUG("InstMgr got an event message: %s\n", event_name);
        if (strcmp(event_name, PCRUN_EVENT_inst_stopped) == 0) {
            assert(msg->elementType == PCRDR_MSG_ELEMENT_TYPE_VARIANT &&
                    purc_variant_is_type(msg->elementValue,
                        PURC_VARIANT_TYPE_ULONGINT));

            uint64_t sid;
            purc_variant_cast_to_ulongint(msg->elementValue, &sid, false);

            if (pcutils_sorted_array_find(info->sa_insts,
                        (void *)(uintptr_t)sid, NULL, NULL)) {
                pcutils_sorted_array_remove(info->sa_insts,
                        (void *)(uintptr_t)sid);
                info->nr_insts--;

                PC_DEBUG("InstMgr removes record of instance %u/%u\n",
                        (unsigned)sid, (unsigned)info->nr_insts);

                if (info->nr_insts == 0) {
                    PC_DEBUG("InstMgr askes the main runner (%u) to shutdown...\n",
                            info->rid_main);

                    pcrdr_msg *request_msg = pcrdr_make_request_message(
                            PCRDR_MSG_TARGET_INSTANCE, info->rid_main,
                            PCRUN_OPERATION_shutdownInstance,
                            PCRDR_REQUESTID_NORETURN,
                            purc_get_endpoint(NULL),
                            PCRDR_MSG_ELEMENT_TYPE_VOID, NULL,
                            NULL,
                            PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);

                    purc_inst_move_message(info->rid_main, request_msg);
                    pcrdr_release_message(request_msg);
                }
            }
        }
        else {
            PC_DEBUG("InstMgr got an event message not interested in:\n");
            PC_DEBUG("    type:        %d\n", msg->type);
            PC_DEBUG("    target:      %d\n", msg->target);
            PC_DEBUG("    targetValue: %u\n", (unsigned)msg->targetValue);
            PC_DEBUG("    eventName:   %s\n", event_name);
            PC_DEBUG("    sourceURI:   %s\n",
                    purc_variant_get_string_const(msg->sourceURI));
        }
    }
    else if (msg->type == PCRDR_MSG_TYPE_RESPONSE) {
        PC_DEBUG("InstMgr got a response for request: %s from %s\n",
                purc_variant_get_string_const(msg->requestId),
                purc_variant_get_string_const(msg->sourceURI));
    }

    pcrdr_release_message(msg);
}


purc_atom_t
purc_inst_create_or_get(const char *app_name, const char *runner_name,
        purc_cond_handler cond_handler,
        const purc_instance_extra_info* extra_info)
{
    char endpoint_name[PURC_LEN_ENDPOINT_NAME + 1];

    if (!purc_is_valid_app_name(app_name) ||
            !purc_is_valid_runner_name(runner_name)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return 0;
    }

    purc_assemble_endpoint_name_ex(PCRDR_LOCALHOST,
            app_name, runner_name,
            endpoint_name, sizeof(endpoint_name) - 1);
    purc_atom_t atom = purc_atom_try_string_ex(PURC_ATOM_BUCKET_DEF,
            endpoint_name);
    if (atom != 0) {
        /* TODO: change the condition handler for an exisiting runner? */
        return atom;
    }

    atom = purc_get_instmgr_rid();
    if (atom == 0) {
        purc_set_error(PURC_ERROR_NO_INSTANCE);
        return 0;
    }

    pcrdr_msg *request;
    request = pcrdr_make_request_message(
            PCRDR_MSG_TARGET_INSTANCE, atom,
            PCRUN_OPERATION_createInstance, NULL, purc_get_endpoint(NULL),
            PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
            PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);

    purc_variant_t data, tmp;
    data = purc_variant_make_object_0();

    /* here we make static string variant because this is a sync request */
    tmp = purc_variant_make_string_static(app_name, false);
    purc_variant_object_set_by_static_ckey(data, "appName", tmp);
    purc_variant_unref(tmp);

    tmp = purc_variant_make_string_static(runner_name, false);
    purc_variant_object_set_by_static_ckey(data, "runnerName", tmp);
    purc_variant_unref(tmp);

    if (cond_handler) {
        tmp = purc_variant_make_ulongint((uint64_t)(uintptr_t)cond_handler);
        purc_variant_object_set_by_static_ckey(data, "condHandler", tmp);
        purc_variant_unref(tmp);
    }

    if (extra_info) {
        tmp = purc_variant_make_ulongint((uint64_t)extra_info->renderer_comm);
        purc_variant_object_set_by_static_ckey(data, "rendererProt", tmp);
        purc_variant_unref(tmp);

        tmp = purc_variant_make_boolean(extra_info->keep_alive);
        purc_variant_object_set_by_static_ckey(data, "keepAlive", tmp);
        purc_variant_unref(tmp);

        tmp = purc_variant_make_boolean(extra_info->allow_switching_rdr);
        purc_variant_object_set_by_static_ckey(data, "allowSwitchingRdr", tmp);
        purc_variant_unref(tmp);

        tmp = purc_variant_make_boolean(extra_info->allow_scaling_by_density);
        purc_variant_object_set_by_static_ckey(data, "allowScalingByDensity", tmp);
        purc_variant_unref(tmp);

        if (extra_info->renderer_uri) {
            tmp = purc_variant_make_string_static(extra_info->renderer_uri,
                    false);
            purc_variant_object_set_by_static_ckey(data, "rendererURI", tmp);
            purc_variant_unref(tmp);
        }

        if (extra_info->ssl_cert) {
            tmp = purc_variant_make_string_static(extra_info->ssl_cert,
                    false);
            purc_variant_object_set_by_static_ckey(data, "sslCert", tmp);
            purc_variant_unref(tmp);
        }

        if (extra_info->ssl_key) {
            tmp = purc_variant_make_string_static(extra_info->ssl_key,
                    false);
            purc_variant_object_set_by_static_ckey(data, "sslKey", tmp);
            purc_variant_unref(tmp);
        }

        if (extra_info->workspace_name) {
            tmp = purc_variant_make_string_static(extra_info->workspace_name,
                    false);
            purc_variant_object_set_by_static_ckey(data, "workspaceName", tmp);
            purc_variant_unref(tmp);
        }

        if (extra_info->workspace_title) {
            tmp = purc_variant_make_string_static(extra_info->workspace_title,
                    false);
            purc_variant_object_set_by_static_ckey(data, "workspaceTitle", tmp);
            purc_variant_unref(tmp);
        }

        if (extra_info->workspace_layout) {
            tmp = purc_variant_make_string_static(extra_info->workspace_layout,
                    false);
            purc_variant_object_set_by_static_ckey(data, "workspaceLayout", tmp);
            purc_variant_unref(tmp);
        }
    }

    purc_variant_t request_id = purc_variant_ref(request->requestId);

    request->dataType = PCRDR_MSG_DATA_TYPE_JSON;
    request->data = data;
    size_t n = purc_inst_move_message(atom, request);
    pcrdr_release_message(request);
    if (n == 0) {
        purc_log_warn("Failed to send request message\n");
        return 0;
    }

    struct pcrdr_conn *conn = purc_get_conn_to_renderer();
    assert(conn);

    pcrdr_msg *response = NULL;
    int ret = pcrdr_wait_response_for_specific_request(conn,
            request_id, PCRUN_TIMEOUT_DEF, &response); // Wait forever
    purc_variant_unref(request_id);

    if (ret) {
        purc_log_error("Failed to wait response: %s\n",
               purc_get_error_message(purc_get_last_error()));
    }
    else if (response->retCode != PCRDR_SC_OK &&
                response->retCode != PCRDR_SC_CONFLICT) {
        purc_log_error("Failed to create a new instance: %d\n",
                response->retCode);
    }
    else {
        atom = (purc_atom_t)response->resultValue;
    }

    if (response)
        pcrdr_release_message(response);
    return atom;
}

purc_atom_t
purc_inst_schedule_vdom(purc_atom_t inst, purc_vdom_t vdom,
        purc_atom_t curator, purc_variant_t request,
        pcrdr_page_type_k page_type, const char *target_workspace,
        const char *target_group, const char *page_name,
        purc_renderer_extra_info *extra_rdr_info,
        const char *body_id)
{
    const char *inst_endpoint = purc_atom_to_string(inst);
    struct pcinst *curr_inst = pcinst_current();
    if (inst_endpoint == NULL || curr_inst == NULL ||
            curr_inst->intr_heap == NULL ||
            curr_inst->intr_heap->move_buff == inst) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return 0;
    }

    purc_atom_t atom = 0;
    pcrdr_msg *request_msg = pcrdr_make_request_message(
            PCRDR_MSG_TARGET_INSTANCE, inst,
            PCRUN_OPERATION_createCoroutine, NULL,
            purc_get_endpoint(NULL),
            PCRDR_MSG_ELEMENT_TYPE_VOID, NULL,
            NULL,
            PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);

    purc_variant_t data, tmp;
    data = purc_variant_make_object_0();

    tmp = purc_variant_make_ulongint((uint64_t)(uintptr_t)vdom);
    purc_variant_object_set_by_static_ckey(data, "vdom", tmp);
    purc_variant_unref(tmp);

    tmp = purc_variant_make_ulongint((uint64_t)curator);
    purc_variant_object_set_by_static_ckey(data, "curator", tmp);
    purc_variant_unref(tmp);

    purc_variant_object_set_by_static_ckey(data, "request", request);

    tmp = purc_variant_make_ulongint((uint64_t)page_type);
    purc_variant_object_set_by_static_ckey(data, "pageType", tmp);
    purc_variant_unref(tmp);

    if (target_workspace) {
        tmp = purc_variant_make_string_static(target_workspace, false);
        purc_variant_object_set_by_static_ckey(data, "targetWorkspace", tmp);
        purc_variant_unref(tmp);
    }

    if (target_group) {
        tmp = purc_variant_make_string_static(target_group, false);
        purc_variant_object_set_by_static_ckey(data, "targetGroup", tmp);
        purc_variant_unref(tmp);
    }

    if (page_name) {
        tmp = purc_variant_make_string_static(page_name, false);
        purc_variant_object_set_by_static_ckey(data, "pageName", tmp);
        purc_variant_unref(tmp);
    }

    if (extra_rdr_info) {
        if (extra_rdr_info->klass) {
            tmp = purc_variant_make_string_static(extra_rdr_info->klass, false);
            purc_variant_object_set_by_static_ckey(data, "class", tmp);
            purc_variant_unref(tmp);
        }

        if (extra_rdr_info->title) {
            tmp = purc_variant_make_string_static(extra_rdr_info->title, false);
            purc_variant_object_set_by_static_ckey(data, "title", tmp);
            purc_variant_unref(tmp);
        }

        if (extra_rdr_info->layout_style) {
            tmp = purc_variant_make_string_static(extra_rdr_info->layout_style,
                    false);
            purc_variant_object_set_by_static_ckey(data, "layoutStyle",
                    tmp);
            purc_variant_unref(tmp);
        }

        if (extra_rdr_info->toolkit_style) {
            purc_variant_object_set_by_static_ckey(data, "toolkitStyle",
                    extra_rdr_info->toolkit_style);
        }

        if (extra_rdr_info->transition_style) {
            tmp = purc_variant_make_string_static(extra_rdr_info->transition_style,
                    false);
            purc_variant_object_set_by_static_ckey(data, "transitionStyle",
                    tmp);
            purc_variant_unref(tmp);
        }

        if (extra_rdr_info->page_groups) {
            tmp = purc_variant_make_string_static(extra_rdr_info->page_groups,
                    false);
            purc_variant_object_set_by_static_ckey(data, "pageGroups",
                    tmp);
            purc_variant_unref(tmp);
        }
    }

    if (body_id) {
        tmp = purc_variant_make_string_static(body_id, false);
        purc_variant_object_set_by_static_ckey(data, "bodyId", tmp);
        purc_variant_unref(tmp);
    }

    request_msg->dataType = PCRDR_MSG_DATA_TYPE_JSON;
    request_msg->data = data;

    purc_variant_t request_id = purc_variant_ref(request_msg->requestId);
    size_t n = purc_inst_move_message(inst, request_msg);
    pcrdr_release_message(request_msg);
    if (n == 0) {
        purc_log_warn("Failed to send request message\n");
        return 0;
    }

    struct pcrdr_conn *conn = purc_get_conn_to_renderer();
    assert(conn);

    pcrdr_msg *response = NULL;
    int ret = pcrdr_wait_response_for_specific_request(conn,
            request_id, PCRUN_TIMEOUT_DEF, &response);  // wait forever
    purc_variant_unref(request_id);

    if (ret) {
        purc_log_error("Failed to wait response: %s\n",
               purc_get_error_message(purc_get_last_error()));
    }
    else if (response->retCode != PCRDR_SC_OK) {
        purc_log_error("Failed to schedule vDOM in another instance: %d\n",
                response->retCode);
    }
    else {
        atom = (purc_atom_t)response->resultValue;
    }

    if (response)
        pcrdr_release_message(response);
    return atom;
}

int
purc_inst_ask_to_shutdown(purc_atom_t inst)
{
    const char *inst_endpoint = purc_atom_to_string(inst);
    if (inst_endpoint == NULL) {
        return PCRDR_SC_OK;
    }

    struct pcinst *curr_inst = pcinst_current();
    if (curr_inst == NULL || curr_inst->intr_heap == NULL ||
            curr_inst->intr_heap->move_buff == inst) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    pcrdr_msg *request_msg = pcrdr_make_request_message(
            PCRDR_MSG_TARGET_INSTANCE, inst,
            PCRUN_OPERATION_shutdownInstance, PCRDR_REQUESTID_NORETURN,
            purc_get_endpoint(NULL),
            PCRDR_MSG_ELEMENT_TYPE_VOID, NULL,
            NULL,
            PCRDR_MSG_DATA_TYPE_VOID, NULL, 0);

    size_t n = purc_inst_move_message(inst, request_msg);
    pcrdr_release_message(request_msg);
    if (n == 0) {
        purc_log_warn("Failed to send request message\n");
        return -1;
    }

#if 1
    return PCRDR_SC_OK;
#else
    purc_variant_t request_id = purc_variant_ref(request_msg->requestId);
    struct pcrdr_conn *conn = purc_get_conn_to_renderer();
    assert(conn);

    pcrdr_msg *response = NULL;
    int ret = pcrdr_wait_response_for_specific_request(conn,
            request_id, PCRUN_TIMEOUT_DEF, &response);  // wait forever
    purc_variant_unref(request_id);

    int retv = 0;
    if (ret || response == NULL) {
        purc_log_error("Failed to ask the instance to shutdown\n");
        retv = -1;
    }
    else {
        retv = response->retCode;
        pcrdr_release_message(response);
    }

    return retv;
#endif
}

purc_atom_t
purc_get_rid_by_cid(purc_atom_t cid)
{
    const char *cor_uri = purc_atom_to_string(cid);
    if (cor_uri == NULL) {
        purc_set_error(PURC_ERROR_ENTITY_NOT_FOUND);
        return 0;
    }

    char endpoint_name[PURC_LEN_ENDPOINT_NAME + 1];
    char *last_slash = strrchr(cor_uri, '/');
    assert(last_slash);

    size_t len = last_slash - cor_uri;
    assert(len < PURC_LEN_ENDPOINT_NAME + 1);
    strncpy(endpoint_name, cor_uri, len);
    endpoint_name[len] = 0;

    purc_atom_t sid =
        purc_atom_try_string_ex(PURC_ATOM_BUCKET_DEF, endpoint_name);

    return sid;
}

purc_atom_t
purc_get_instmgr_rid(void)
{
    char endpoint_name[PURC_LEN_ENDPOINT_NAME + 1];

    purc_assemble_endpoint_name_ex(PCRDR_LOCALHOST,
            PCRUN_INSTMGR_APP_NAME, PCRUN_INSTMGR_RUN_NAME,
            endpoint_name, sizeof(endpoint_name) - 1);

    purc_atom_t atom;
    atom = purc_atom_try_string_ex(PURC_ATOM_BUCKET_DEF, endpoint_name);
    if (atom == 0) {
        purc_log_warn("No instance manager: %s\n", endpoint_name);
    }

    return atom;
}

