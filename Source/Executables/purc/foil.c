/*
 * @file foil.c
 * @author Vincent Wei
 * @date 2022/10/02
 * @brief The built-in text-mode renderer.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of purc, which is an HVML interpreter with
 * a command line interface (CLI).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#undef NDEBUG

#include "config.h"

#include <assert.h>
#include <errno.h>

#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */

#include "foil.h"
#include "endpoint.h"
#include "timer.h"

static int init_renderer(pcmcth_renderer *rdr)
{
    pcmcth_set_renderer_callbacks(rdr);

    rdr->master_rid = 0;
    rdr->nr_endpoints = 0;
    rdr->t_start = purc_get_monotoic_time();
    rdr->t_elapsed = rdr->t_elapsed_last = 0;

    kvlist_init(&rdr->endpoint_list, NULL);
    avl_init(&rdr->living_avl, comp_living_time, true, NULL);
    avl_init(&rdr->timer_avl, foil_timer_compare, true, NULL);

    return rdr->cbs.prepare(rdr);
}

static void deinit_renderer(pcmcth_renderer *rdr)
{
    const char* name;
    void *next, *data;
    pcmcth_endpoint *endpoint;

    rdr->cbs.cleanup(rdr);

    foil_timer_delete_all(rdr);
    remove_all_living_endpoints(&rdr->living_avl);

    kvlist_for_each_safe(&rdr->endpoint_list, name, next, data) {
        endpoint = *(pcmcth_endpoint **)data;

        purc_log_info("Deleting endpoint: %s (%p) in %s\n",
                name, endpoint, __func__);

        del_endpoint(rdr, endpoint, CDE_EXITING);
        kvlist_delete(&rdr->endpoint_list, name);
        rdr->nr_endpoints--;
    }

    kvlist_free(&rdr->endpoint_list);
}

#define FOIL_RENDERER   "renderer"

pcmcth_renderer *foil_get_renderer(void)
{
    uintptr_t v;
    if (purc_get_local_data(FOIL_RENDERER, &v, NULL) == 1)
        return (pcmcth_renderer *)(void *)v;
    return NULL;
}

static bool handle_instance_request(pcmcth_renderer *rdr, pcrdr_msg *msg)
{
    const char *operation =
        purc_variant_get_string_const(msg->operation);
    const char *origin_edpt = purc_atom_to_string(msg->__origin);

    if (UNLIKELY(operation == NULL || origin_edpt == NULL)) {
        purc_log_error("Bad operation or source URI in message: %s, %s\n",
                operation, origin_edpt);
        purc_set_error(PCRDR_ERROR_BAD_MESSAGE);
    }
    else {
        if (strcmp(operation, PCRDR_THREAD_OPERATION_HELLO) == 0) {
            pcmcth_endpoint *edpt = new_endpoint(rdr, origin_edpt);
            if (edpt) {
                send_initial_response(rdr, edpt);
                if (rdr->nr_endpoints == 0) {
                    goto no_any_endpoints;
                }
            }
            else {
                purc_log_warn("Cannot create endpoint for %s.\n", origin_edpt);
            }
        }
        else if (strcmp(operation, PCRDR_THREAD_OPERATION_BYE) == 0) {
            pcmcth_endpoint *edpt = retrieve_endpoint(rdr, origin_edpt);
            if (edpt) {
                del_endpoint(rdr, edpt, CDE_EXITING);
                if (rdr->nr_endpoints == 0) {
                    goto no_any_endpoints;
                }
            }
            else {
                purc_set_error(PCRDR_ERROR_PROTOCOL);
                purc_log_warn("Bye request from unknown endpoint: %s.\n",
                        origin_edpt);
            }
        }
        else {
            purc_set_error(PCRDR_ERROR_UNKNOWN_REQUEST);
        }
    }

    return true;

no_any_endpoints:
    return false;
}

#ifndef NDEBUG

#define IDT_REGULAR         0
#define IDT_ONCE            1
#define MAX_TIMES_FIRED     20

static unsigned nr_timer_fired;
static int on_regular_timer(foil_timer_t timer, int id, void *ctxt)
{
    assert(timer != NULL);
    assert(id == IDT_REGULAR);
    assert(ctxt == NULL);

    printf("regular timer fired: %d\n", nr_timer_fired);
    nr_timer_fired++;
    if (nr_timer_fired == MAX_TIMES_FIRED)
        return 100;
    return 0;
}

static int on_once_timer(foil_timer_t timer, int id, void *ctxt)
{
    assert(timer != NULL);
    assert(id == IDT_ONCE);
    assert(ctxt == NULL);

    printf("once timer fired\n");
    return -1;
}

static void test_timer(pcmcth_renderer *rdr)
{
    foil_timer_new(rdr, IDT_REGULAR, 10, on_regular_timer, NULL);
    foil_timer_new(rdr, IDT_ONCE, 100, on_once_timer, NULL);

    while (rdr->t_elapsed < 2) {
        if (rdr->cbs.handle_event(rdr, 10000))
            break;

        rdr->t_elapsed = purc_get_monotoic_time() - rdr->t_start;
        if (UNLIKELY(rdr->t_elapsed != rdr->t_elapsed_last)) {
            rdr->t_elapsed_last = rdr->t_elapsed;
        }

        foil_timer_check_expired(rdr);
    }

    unsigned n;
    n = foil_timer_delete_all(rdr);
    assert(n == 1);
}
#endif /* not defined NDEBUG */

static void event_loop(pcmcth_renderer *rdr)
{
    (void)rdr;
    size_t n;
    int ret;

    do {
        ret = purc_inst_holding_messages_count(&n);

        if (ret) {
            purc_log_error("purc_inst_holding_messages_count failed: %d\n", ret);
        }
        else if (n == 0) {
            if (rdr->cbs.handle_event(rdr, 10000))  // timeout value: 10ms
                break;

            rdr->t_elapsed = purc_get_monotoic_time() - rdr->t_start;
            if (UNLIKELY(rdr->t_elapsed != rdr->t_elapsed_last)) {
#if 0 // VW: no need to check dead endpoints for THREAD-based renderer
                if (rdr->t_elapsed % 10 == 0) {
                    check_no_responding_endpoints(rdr);
                }
#endif
                rdr->t_elapsed_last = rdr->t_elapsed;
            }

            foil_timer_check_expired(rdr);
            continue;
        }

        purc_clr_error();

        pcrdr_msg *msg = purc_inst_take_away_message(0);
        if (msg->type == PCRDR_MSG_TYPE_REQUEST &&
                msg->target == PCRDR_MSG_TARGET_INSTANCE) {
            if (!handle_instance_request(rdr, msg)) {
                pcrdr_release_message(msg);
                purc_log_warn("No any living endpoints, quiting...\n");
                break;
            }
        }
        else {
            const char *origin_edpt = purc_atom_to_string(msg->__origin);
            if (origin_edpt == NULL) {
                const char *operation =
                    purc_variant_get_string_const(msg->operation);
                purc_log_error("Bad endpoint in message: %d (%s)\n",
                        msg->type, operation);
                purc_set_error(PCRDR_ERROR_BAD_MESSAGE);
            }
            else {
                pcmcth_endpoint *edpt = retrieve_endpoint(rdr, origin_edpt);
                if (edpt) {
                    update_endpoint_living_time(rdr, edpt);
                    on_endpoint_message(rdr, edpt, msg);
                }
                else {
                    purc_set_error(PCRDR_ERROR_PROTOCOL);
                }
            }
        }

        pcrdr_release_message(msg);

        int last_error = purc_get_last_error();
        if (UNLIKELY(last_error)) {
            purc_log_warn("Encounter error when handle message: %s\n",
                    purc_get_error_message(last_error));
        }

    } while(true);
}

struct thread_arg {
    sem_t          *wait;
    const char     *app_name;
    const char     *run_name;
    purc_atom_t     rid;
};

static void* foil_thread_entry(void* arg)
{
    struct thread_arg *my_arg = (struct thread_arg *)arg;
    sem_t *sw = my_arg->wait;

    purc_atom_t rid = 0;
    // initial purc instance
    int ret = purc_init_ex(PURC_MODULE_EJSON | PURC_MODULE_HTML,
            my_arg->app_name, my_arg->run_name, NULL);
    if (ret == PURC_ERROR_OK) {
        rid = my_arg->rid = purc_inst_create_move_buffer(
                PCINST_MOVE_BUFFER_FLAG_NONE, 16);
    }

    purc_enable_log(true, false);

    sem_post(sw);

    if (rid) {
        pcmcth_renderer rdr;

        if (init_renderer(&rdr) == 0) {
            purc_set_local_data(FOIL_RENDERER, (uintptr_t)&rdr, NULL);
#ifndef NDEBUG
            test_timer(&rdr);
#endif
            event_loop(&rdr);
            purc_remove_local_data(FOIL_RENDERER);
            deinit_renderer(&rdr);
        }
        purc_inst_destroy_move_buffer();
    }

    if (ret == PURC_ERROR_OK) {
        LOG_INFO("Foil is going to be cleaned up and the thread is exiting.\n");
        purc_cleanup();
    }

    return NULL;
}

#define SEM_NAME_SYNC_START     "sync-foil-start"

static pthread_t foil_th;
purc_atom_t foil_start(const char *rdr_uri)
{
    int ret;
    struct thread_arg arg;

    char app_name[PURC_LEN_APP_NAME + 1];
    if (purc_extract_app_name(rdr_uri, app_name) == 0) {
        purc_log_error("bad renderer URI: %s\n", rdr_uri);
        return 0;
    }

    char run_name[PURC_LEN_RUNNER_NAME + 1];
    if (purc_extract_runner_name(rdr_uri, run_name) == 0) {
        purc_log_error("bad renderer URI: %s\n", rdr_uri);
        return 0;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

ALLOW_DEPRECATED_DECLARATIONS_BEGIN
    sem_unlink(SEM_NAME_SYNC_START);
    arg.wait = sem_open(SEM_NAME_SYNC_START, O_CREAT | O_EXCL, 0644, 0);
    if (arg.wait == SEM_FAILED) {
        purc_log_error("failed to create semaphore: %s\n", strerror(errno));
        goto failed;
    }

    arg.app_name = app_name;
    arg.run_name = run_name;
    ret = pthread_create(&foil_th, &attr, foil_thread_entry, &arg);
    if (ret) {
        purc_log_error("failed to create thread for built-in renderer: %s\n",
                strerror(errno));
        sem_close(arg.wait);
        goto failed;
    }
    pthread_attr_destroy(&attr);

    sem_wait(arg.wait);
    sem_close(arg.wait);
ALLOW_DEPRECATED_DECLARATIONS_END

    return arg.rid;

failed:
    pthread_attr_destroy(&attr);
    return 0;
}

void foil_sync_exit(void)
{
    pthread_join(foil_th, NULL);
}

