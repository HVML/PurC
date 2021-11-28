/*
 * @file tab.h
 * @author Xu Xiaohong
 * @date 2021/11/28
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
 */


#ifndef PURC_EXECUTOR_TAB_H
#define PURC_EXECUTOR_TAB_H

#define NCLE_INIT(_ncle, _r) do {                             \
    _ncle = number_comparing_logical_expression_create();     \
    if (!_ncle) {                                             \
        YYABORT;                                              \
    }                                                         \
    _ncle->type = NUMBER_COMPARING_LOGICAL_EXPRESSION_NUM;    \
    _ncle->ncc  = _r;                                         \
} while (0)

#define NCLE_OP(_ncle, _l, _r, _op) do {                           \
    _ncle = number_comparing_logical_expression_create();          \
    bool ok = false;                                               \
    do {                                                           \
        if (!_ncle)                                                \
            break;                                                 \
        _ncle->type = _op;                                         \
        if (!pctree_node_append_child(&_ncle->node, &_l->node))    \
            break;                                                 \
        _l = NULL;                                                 \
        if (!pctree_node_append_child(&_ncle->node, &_r->node))    \
            break;                                                 \
        _r = NULL;                                                 \
        ok = true;                                                 \
    } while (0);                                                   \
    if (_l)                                                        \
        number_comparing_logical_expression_destroy(_l);           \
    if (_r)                                                        \
        number_comparing_logical_expression_destroy(_r);           \
    if (ok)                                                        \
        break;                                                     \
    if (_ncle)                                                     \
        number_comparing_logical_expression_destroy(_l);           \
    YYABORT;                                                       \
} while (0)

#define NCLE_AND(_ncle, _l, _r)                          \
    NCLE_OP(_ncle, _l, _r, NUMBER_COMPARING_LOGICAL_EXPRESSION_AND)
#define NCLE_OR(_ncle, _l, _r)                           \
    NCLE_OP(_ncle, _l, _r, NUMBER_COMPARING_LOGICAL_EXPRESSION_OR)
#define NCLE_XOR(_ncle, _l, _r)                          \
    NCLE_OP(_ncle, _l, _r, NUMBER_COMPARING_LOGICAL_EXPRESSION_XOR)

#define NCLE_NOT(_ncle, _l) do {                                   \
    _ncle = number_comparing_logical_expression_create();          \
    bool ok = false;                                               \
    do {                                                           \
        if (!_ncle)                                                \
            break;                                                 \
        _ncle->type = NUMBER_COMPARING_LOGICAL_EXPRESSION_NOT;     \
        if (!pctree_node_append_child(&_ncle->node, &_l->node))    \
            break;                                                 \
        _l = NULL;                                                 \
        ok = true;                                                 \
    } while (0);                                                   \
    if (_l)                                                        \
        number_comparing_logical_expression_destroy(_l);           \
    if (ok)                                                        \
        break;                                                     \
    if (_ncle)                                                     \
        number_comparing_logical_expression_destroy(_l);           \
    YYABORT;                                                       \
} while (0)

#define SMLE_INIT(_smle, _r) do {                            \
    _smle = string_matching_logical_expression_create();     \
    if (!_smle) {                                            \
        string_matching_condition_reset(&_r);                \
        YYABORT;                                             \
    }                                                        \
    _smle->type = STRING_MATCHING_LOGICAL_EXPRESSION_STR;    \
    _smle->smc = _r;                                         \
} while (0)

#define SMLE_OP(_smle, _l, _r, _op) do {                           \
    _smle = string_matching_logical_expression_create();           \
    bool ok = false;                                               \
    do {                                                           \
        if (!_smle)                                                \
            break;                                                 \
        _smle->type = _op;                                         \
        if (!pctree_node_append_child(&_smle->node, &_l->node))    \
            break;                                                 \
        _l = NULL;                                                 \
        if (!pctree_node_append_child(&_smle->node, &_r->node))    \
            break;                                                 \
        _r = NULL;                                                 \
        ok = true;                                                 \
    } while (0);                                                   \
    if (_l)                                                        \
        string_matching_logical_expression_destroy(_l);            \
    if (_r)                                                        \
        string_matching_logical_expression_destroy(_r);            \
    if (ok)                                                        \
        break;                                                     \
    if (_smle)                                                     \
        string_matching_logical_expression_destroy(_l);            \
    YYABORT;                                                       \
} while (0)

#define SMLE_AND(_smle, _l, _r)                          \
    SMLE_OP(_smle, _l, _r, STRING_MATCHING_LOGICAL_EXPRESSION_AND)
#define SMLE_OR(_smle, _l, _r)                           \
    SMLE_OP(_smle, _l, _r, STRING_MATCHING_LOGICAL_EXPRESSION_OR)
#define SMLE_XOR(_smle, _l, _r)                          \
    SMLE_OP(_smle, _l, _r, STRING_MATCHING_LOGICAL_EXPRESSION_XOR)

#define SMLE_NOT(_smle, _l) do {                                   \
    _smle = string_matching_logical_expression_create();           \
    bool ok = false;                                               \
    do {                                                           \
        if (!_smle)                                                \
            break;                                                 \
        _smle->type = STRING_MATCHING_LOGICAL_EXPRESSION_NOT;      \
        if (!pctree_node_append_child(&_smle->node, &_l->node))    \
            break;                                                 \
        _l = NULL;                                                 \
        ok = true;                                                 \
    } while (0);                                                   \
    if (_l)                                                        \
        string_matching_logical_expression_destroy(_l);            \
    if (ok)                                                        \
        break;                                                     \
    if (_smle)                                                     \
        string_matching_logical_expression_destroy(_l);            \
    YYABORT;                                                       \
} while (0)


#endif // PURC_EXECUTOR_TAB_H

