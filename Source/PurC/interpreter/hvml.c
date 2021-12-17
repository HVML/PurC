/**
 * @file hvml.c
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

#include "private/debug.h"
#include "private/executor.h"
#include "private/interpreter.h"

#include "ops.h"


// struct ctxt_for_hvml {
//     struct purc_exec_ops     ops;
// 
//     // the instance of the current executor.
//     purc_exec_inst_t exec_inst;
// 
//     // the iterator if the current element is `hvml`.
//     purc_exec_iter_t it;
// 
//     struct pcvdom_element       *curr;
// 
//     unsigned int                uniquely:1;
//     unsigned int                case_insensitive:1;
// };
// 
// static inline void
// ctxt_release(struct ctxt_for_hvml *ctxt)
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
// }
// 
// static inline void
// ctxt_destroy(struct ctxt_for_hvml *ctxt)
// {
//     if (ctxt) {
//         ctxt_release(ctxt);
//         free(ctxt);
//     }
// }
// 
// static inline int
// hvml_after_pushed_as_via(pcintr_stack_t stack, pcvdom_element_t pos,
//         struct ctxt_for_hvml *ctxt,
//         purc_variant_t as, purc_variant_t via)
// {
//     PC_ASSERT(as && purc_variant_is_type(as, PURC_VARIANT_TYPE_STRING));
//     PC_ASSERT(via && purc_variant_is_type(via, PURC_VARIANT_TYPE_STRING));
// 
//     const char *s_as  = purc_variant_get_string_const(as);
//     const char *s_via = purc_variant_get_string_const(via);
// 
//     struct pcvdom_node *vdom_node = &pos->node;
//     struct pctree_node *tree_node = &vdom_node->node;
//     size_t nr_children;
//     nr_children = pctree_node_children_number(tree_node);
//     PC_ASSERT(nr_children == 1);
// 
//     vdom_node = pcvdom_node_first_child(vdom_node);
//     PC_ASSERT(PCVDOM_NODE_IS_CONTENT(vdom_node));
// 
//     struct pcvdom_content *content;
//     content = PCVDOM_CONTENT_FROM_NODE(vdom_node);
// 
//     PC_ASSERT(content);
//     UNUSED_PARAM(s_as);
//     UNUSED_PARAM(s_via);
//     UNUSED_PARAM(stack);
//     UNUSED_PARAM(ctxt);
//     return -1;
// }
// 
// static inline int
// hvml_after_pushed(pcintr_stack_t stack, pcvdom_element_t pos,
//         struct ctxt_for_hvml *ctxt)
// {
//     struct pcvdom_attr *from = pcvdom_element_find_attr(pos, "from");
//     PC_ASSERT(from == NULL); // Not implemented yet
//     struct pcvdom_attr *with = pcvdom_element_find_attr(pos, "with");
//     PC_ASSERT(with == NULL); // Not implemented yet
// 
//     if (pcvdom_element_find_attr(pos, "uniquely")) {
//         ctxt->uniquely = 1;
//     }
//     if (pcvdom_element_find_attr(pos, "caseinsensitively")) {
//         ctxt->case_insensitive = 1;
//     }
// 
//     purc_variant_t as  = pcvdom_element_eval_attr_val(pos, "as");
//     purc_variant_t via = pcvdom_element_eval_attr_val(pos, "via");
// 
//     int r = hvml_after_pushed_as_via(stack, pos, ctxt, as, via);
// 
//     purc_variant_unref(as);
//     purc_variant_unref(via);
// 
//     return r ? -1 : 0;
// }
// 
// // called after pushed
// static inline void *
// after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
// {
//     struct ctxt_for_hvml *ctxt;
//     ctxt = (struct ctxt_for_hvml*)calloc(1, sizeof(*ctxt));
//     if (!ctxt)
//         return NULL;
// 
//     int r = hvml_after_pushed(stack, pos, ctxt);
//     if (r) {
//         ctxt_destroy(ctxt);
//         return NULL;
//     }
// 
//     return ctxt;
// }
// 
// // called on popping
// static inline bool
// on_popping(pcintr_stack_t stack, void* ctxt)
// {
//     struct pcintr_stack_frame *frame;
//     frame = pcintr_stack_get_bottom_frame(stack);
//     PC_ASSERT(frame->ctxt == ctxt);
// 
//     struct ctxt_for_hvml *hvml_ctxt;
//     hvml_ctxt = (struct ctxt_for_hvml*)ctxt;
// 
//     ctxt_destroy(hvml_ctxt);
//     frame->ctxt = NULL;
// 
//     return false;
// }

static struct pcintr_element_ops
ops = {
//     .after_pushed       = after_pushed,
//     .on_popping         = on_popping,
//     .rerun              = NULL,
//     .select_child       = NULL,
};

struct pcintr_element_ops pcintr_hvml_get_ops(void)
{
    return ops;
}


