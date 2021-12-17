/**
 * @file iterate.c
 * @author Xu Xiaohong
 * @date 2021/12/06
 * @brief
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
 */

#include "config.h"

#include "internal.h"

#include "private/debug.h"
#include "private/executor.h"

#include "ops.h"

// struct ctxt_for_iterate {
//     struct purc_exec_ops     ops;
// 
//     // the instance of the current executor.
//     purc_exec_inst_t exec_inst;
// 
//     // the iterator if the current element is `iterate`.
//     purc_exec_iter_t it;
// 
//     purc_variant_t               in;
// 
//     struct pcvdom_element       *curr;
// };
// 
// static inline void
// ctxt_release(struct ctxt_for_iterate *ctxt)
// {
//     if (!ctxt)
//         return;
// 
//     if (ctxt->exec_inst) {
//         struct purc_exec_ops *ops = &ctxt->ops;
// 
//         if (ops->destroy) {
//             ops->destroy(ctxt->exec_inst);
//         }
// 
//         ctxt->exec_inst = NULL;
//     }
// 
//     PURC_VARIANT_SAFE_CLEAR(ctxt->in);
// }
// 
// static inline void
// ctxt_destroy(struct ctxt_for_iterate *ctxt)
// {
//     if (ctxt) {
//         ctxt_release(ctxt);
//         free(ctxt);
//     }
// }
// 
// static inline int
// iterate_eval(pcintr_stack_t stack, struct ctxt_for_iterate *ctxt)
// {
//     struct purc_exec_ops *ops = &ctxt->ops;
// 
//     purc_variant_t result = PURC_VARIANT_INVALID;
//     result = ops->it_value(ctxt->exec_inst, ctxt->it);
//     if (result == PURC_VARIANT_INVALID) {
//         return -1;
//     }
// 
//     struct pcintr_stack_frame *frame;
//     frame = pcintr_stack_get_bottom_frame(stack);
// 
//     PC_ASSERT(frame->symbol_vars[PURC_SYMBOL_VAR_QUESTION_MARK]
//             == PURC_VARIANT_INVALID);
//     frame->symbol_vars[PURC_SYMBOL_VAR_QUESTION_MARK] = result;
// 
//     return 0;
// }
// 
// static inline int
// iterate_after_pushed_on_in_by(pcintr_stack_t stack,
//         struct ctxt_for_iterate *ctxt,
//         purc_variant_t on, purc_variant_t in, purc_variant_t by)
// {
//     const char *rule = purc_variant_get_string_const(by);
//     PC_ASSERT(rule);
// 
//     struct purc_exec_ops *ops = &ctxt->ops;
//     bool ok = purc_get_executor(rule, ops);
//     if (!ok)
//         return -1;
// 
//     if (!ops->create)
//         return -1;
// 
//     ctxt->exec_inst = ops->create(PURC_EXEC_TYPE_ITERATE, on, true);
//     if (!ctxt->exec_inst)
//         return -1;
// 
//     PC_ASSERT(ops->it_begin);
//     PC_ASSERT(ops->it_next);
//     PC_ASSERT(ops->it_value);
// 
//     ctxt->it = ops->it_begin(ctxt->exec_inst, rule);
// 
//     if (!ctxt->it) {
//         return -1;
//     }
// 
//     ctxt->in = in;
//     purc_variant_ref(in);
// 
//     int r = iterate_eval(stack, ctxt);
// 
//     return r ? -1 : 0;
// }
// 
// 
// static inline int
// iterate_after_pushed(pcintr_stack_t stack, pcvdom_element_t pos,
//         struct ctxt_for_iterate *ctxt)
// {
//     purc_variant_t on = pcvdom_element_eval_attr_val(pos, "on");
//     purc_variant_t in = pcvdom_element_eval_attr_val(pos, "in");
//     purc_variant_t by = pcvdom_element_eval_attr_val(pos, "by");
// 
//     int r = iterate_after_pushed_on_in_by(stack, ctxt, on, in, by);
//     purc_variant_unref(on);
//     purc_variant_unref(in);
//     purc_variant_unref(by);
// 
//     return r ? -1 : 0;
// }
// 
// // called after pushed
// static inline void *
// after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
// {
//     struct ctxt_for_iterate *ctxt;
//     ctxt = (struct ctxt_for_iterate*)calloc(1, sizeof(*ctxt));
//     if (!ctxt)
//         return NULL;
// 
//     int r = iterate_after_pushed(stack, pos, ctxt);
//     if (r) {
//         ctxt_destroy(ctxt);
//         return NULL;
//     }
// 
//     return ctxt;
// }
// 
// static inline int
// iterate_on_popping(pcintr_stack_t stack, struct ctxt_for_iterate *ctxt,
//         int *popped)
// {
//     struct pcintr_stack_frame *frame;
//     frame = pcintr_stack_get_bottom_frame(stack);
//     PC_ASSERT(frame->ctxt == ctxt);
// 
//     struct purc_exec_ops *ops = &ctxt->ops;
// 
//     if (ctxt->it) {
//         // TODO: re-evaluate rule
//         const char *rule = NULL;
//         PC_ASSERT(0);
//         ctxt->it = ops->it_next(ctxt->exec_inst, ctxt->it, rule);
//         if (ctxt->it) {
//             *popped = 0;
//             return 0;
//         }
//     }
// 
//     *popped = 1;
// 
//     ctxt_destroy(ctxt);
//     frame->ctxt = NULL;
// 
//     return 0;
// }
// 
// // called on popping
// static inline bool
// on_popping(pcintr_stack_t stack, void* ctxt)
// {
//     struct ctxt_for_iterate *iterate_ctxt;
//     iterate_ctxt = (struct ctxt_for_iterate*)ctxt;
// 
//     int popped;
//     int r = iterate_on_popping(stack, iterate_ctxt, &popped);
// 
//     PC_ASSERT(r == 0);
//     return popped ? true : false;
// }
// 
// static inline bool
// iterate_rerun(pcintr_stack_t stack, struct ctxt_for_iterate *ctxt)
// {
//     struct pcintr_stack_frame *frame;
//     frame = pcintr_stack_get_bottom_frame(stack);
//     PC_ASSERT(frame->ctxt == ctxt);
// 
//     int r = iterate_eval(stack, ctxt);
//     if (r) {
//         // FIXME: how to react????
//         PC_ASSERT(0);
//         return false;
//     }
// 
//     PURC_VARIANT_SAFE_CLEAR(frame->mid_vars);
// 
//     return true;
// }
// 
// // called to rerun
// static inline bool
// rerun(pcintr_stack_t stack, void* ctxt)
// {
//     struct ctxt_for_iterate *iterate_ctxt;
//     iterate_ctxt = (struct ctxt_for_iterate*)ctxt;
// 
//     return iterate_rerun(stack, iterate_ctxt);
// }
// 
// static inline pcvdom_element_t
// iterate_select_child(pcintr_stack_t stack, struct ctxt_for_iterate *ctxt)
// {
//     if (ctxt->curr) {
//         struct pcvdom_element *next;
//         next = pcvdom_element_next_sibling_element(ctxt->curr);
//         if (next) {
//             ctxt->curr = next;
//         }
//         return next;
//     }
// 
//     struct pcintr_stack_frame *frame;
//     frame = pcintr_stack_get_bottom_frame(stack);
//     pcvdom_element_t element = frame->pos;
// 
//     ctxt->curr = pcvdom_element_first_child_element(element);
// 
//     return ctxt->curr;
// }
// 
// 
// // called after executed
// static inline pcvdom_element_t
// select_child(pcintr_stack_t stack, void* ctxt)
// {
//     struct ctxt_for_iterate *iterate_ctxt;
//     iterate_ctxt = (struct ctxt_for_iterate*)ctxt;
// 
//     return iterate_select_child(stack, iterate_ctxt);
// }

static struct pcintr_element_ops
ops = {
//     .after_pushed       = after_pushed,
//     .on_popping         = on_popping,
//     .rerun              = rerun,
//     .select_child       = select_child,
};

struct pcintr_element_ops* pcintr_iterate_get_ops(void)
{
    return &ops;
}

