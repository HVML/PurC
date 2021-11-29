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

#define STRTOL(_v, _s) do {                     \
    long int v;                                 \
    char *s = (char*)malloc(_s.leng+1);         \
    if (!s) {                                   \
        YYABORT;                                \
    }                                           \
    memcpy(s, _s.text, _s.leng);                \
    s[_s.leng] = '\0';                          \
    char *end;                                  \
    v = strtol(s, &end, 0);                     \
    if (end && *end) {                          \
        free(s);                                \
        YYABORT;                                \
    }                                           \
    free(s);                                    \
    _v = v;                                     \
} while (0)

#define STRTOD(_v, _s) do {                     \
    double v;                                   \
    char *s = (char*)malloc(_s.leng+1);         \
    if (!s) {                                   \
        YYABORT;                                \
    }                                           \
    memcpy(s, _s.text, _s.leng);                \
    s[_s.leng] = '\0';                          \
    char *end;                                  \
    v = strtod(s, &end);                        \
    if (end && *end) {                          \
        free(s);                                \
        YYABORT;                                \
    }                                           \
    free(s);                                    \
    _v = v;                                     \
} while (0)

#define STRTOLL(_v, _s) do {                    \
    long long int v;                            \
    char *s = (char*)malloc(_s.leng+1);         \
    if (!s) {                                   \
        YYABORT;                                \
    }                                           \
    memcpy(s, _s.text, _s.leng);                \
    s[_s.leng] = '\0';                          \
    char *end;                                  \
    v = strtoll(s, &end, 0);                    \
    if (end && *end) {                          \
        free(s);                                \
        YYABORT;                                \
    }                                           \
    free(s);                                    \
    _v = v;                                     \
} while (0)

#define STRTOLD(_v, _s) do {                    \
    long double v;                              \
    char *s = (char*)malloc(_s.leng+1);         \
    if (!s) {                                   \
        YYABORT;                                \
    }                                           \
    memcpy(s, _s.text, _s.leng);                \
    s[_s.leng] = '\0';                          \
    char *end;                                  \
    v = strtold(s, &end);                       \
    if (end && *end) {                          \
        free(s);                                \
        YYABORT;                                \
    }                                           \
    free(s);                                    \
    _v = v;                                     \
} while (0)

#define TOKEN_DUP_STR(_l, _r) do {              \
    char *s = (char*)malloc(_r.leng + 1);       \
    if (!s) {                                   \
        YYABORT;                                \
    }                                           \
    snprintf(s, _r.leng + 1, "%s", _r.text);    \
    _l = s;                                     \
} while (0)

#define STRLIST_INIT_STR(_list, _s) do {                          \
    pcexe_strlist_init(&_list);                                   \
    if (pcexe_strlist_append_buf(&_list, _s.text, _s.leng)) {     \
        pcexe_strlist_reset(&_list);                              \
        YYABORT;                                                  \
    }                                                             \
} while (0)

#define STRLIST_INIT_CHR(_list, _c) do {                          \
    pcexe_strlist_init(&_list);                                   \
    if (pcexe_strlist_append_chr(&_list, _c)) {                   \
        pcexe_strlist_reset(&_list);                              \
        YYABORT;                                                  \
    }                                                             \
} while (0)

#define STRLIST_INIT_UNI(_list, _u) do {                          \
    pcexe_strlist_init(&_list);                                   \
    if (pcexe_strlist_append_uni(&_list, _u.text, _u.leng)) {     \
        pcexe_strlist_reset(&_list);                              \
        YYABORT;                                                  \
    }                                                             \
} while (0)

#define STRLIST_APPEND_STR(_list, _s) do {                        \
    if (pcexe_strlist_append_buf(&_list, _s.text, _s.leng)) {     \
        pcexe_strlist_reset(&_list);                              \
        YYABORT;                                                  \
    }                                                             \
} while (0)

#define STRLIST_APPEND_CHR(_list, _c) do {                        \
    if (pcexe_strlist_append_chr(&_list, _c)) {                   \
        pcexe_strlist_reset(&_list);                              \
        YYABORT;                                                  \
    }                                                             \
} while (0)

#define STRLIST_APPEND_UNI(_list, _u) do {                        \
    if (pcexe_strlist_append_uni(&_list, _u.text, _u.leng)) {     \
        pcexe_strlist_reset(&_list);                              \
        YYABORT;                                                  \
    }                                                             \
} while (0)

#define STRLIST_TO_STR(_str, _list) do {                          \
    _str = pcexe_strlist_to_str(&_list);                          \
    pcexe_strlist_reset(&_list);                                  \
    if (!_str) {                                                  \
        YYABORT;                                                  \
    }                                                             \
} while (0)

#define STR_LITERAL_RESET(_l) do {              \
    literal_expression_reset(&_l);              \
} while (0)

#define STR_LITERAL_SET(_l, _slist, _sfx) do {     \
    _l.literal = pcexe_strlist_to_str(&_slist);    \
    pcexe_strlist_reset(&_slist);                  \
    _l.suffix = _sfx;                              \
    if (!_l.literal) {                             \
        YYABORT;                                   \
    }                                              \
} while (0)

#define STR_PATTERN_RESET(_sp) do {             \
    string_pattern_expression_reset(&_sp);      \
} while (0)

#define STR_PATTERN_SET_WILDCARD(_sp, _s, _sfx) do {     \
    memset(&_sp, 0, sizeof(_sp));                        \
    _sp.type = STRING_PATTERN_WILDCARD;                  \
    _sp.wildcard.wildcard = _s;                          \
    _sp.wildcard.suffix = _sfx;                          \
} while (0)

#define STR_PATTERN_SET_REGEXP(_sp, _slist, _flags) do { \
    memset(&_sp, 0, sizeof(_sp));                        \
    _sp.type = STRING_PATTERN_REGEXP;                    \
    _sp.regexp.regexp = pcexe_strlist_to_str(&_slist);   \
    pcexe_strlist_reset(&_slist);                        \
    _sp.regexp.flags = _flags;                           \
    if (!_sp.regexp.regexp) {                            \
        YYABORT;                                         \
    }                                                    \
} while (0)

#define STR_LITERAL_DUP(_dst, _src) do {                           \
    _dst = (struct literal_expression*)calloc(1, sizeof(*_dst));   \
    if (!_dst)                                                     \
        break;                                                     \
    memcpy(_dst, _src, sizeof(*_src));                             \
} while (0)

#define STR_LITERAL_LIST_INIT(_literals, _l) do {        \
    _literals = string_literal_list_create();            \
    if (!_literals) {                                    \
        literal_expression_reset(&_l);                   \
        YYABORT;                                         \
    }                                                    \
    struct literal_expression *smle;                     \
    STR_LITERAL_DUP(smle, &_l);                          \
    if (!smle) {                                         \
        literal_expression_reset(&_l);                   \
        string_literal_list_destroy(_literals);          \
        YYABORT;                                         \
    }                                                    \
    list_add(&smle->node, &_literals->list);             \
} while (0)

#define STR_LITERAL_LIST_APPEND(_literals, _l) do {      \
    struct literal_expression *smle;                     \
    STR_LITERAL_DUP(smle, &_l);                          \
    if (!smle) {                                         \
        literal_expression_reset(&_l);                   \
        string_literal_list_destroy(_literals);          \
        YYABORT;                                         \
    }                                                    \
    list_add(&smle->node, &_literals->list);             \
} while (0)

#define STR_PATTERN_DUP(_dst, _src) do {                                  \
    _dst = (struct string_pattern_expression*)calloc(1, sizeof(*_dst));   \
    if (!_dst)                                                            \
        break;                                                            \
    memcpy(_dst, _src, sizeof(*_src));                                    \
} while (0)

#define STR_PATTERN_LIST_INIT(_patterns, _l) do {        \
    _patterns = string_pattern_list_create();            \
    if (!_patterns) {                                    \
        string_pattern_expression_reset(&_l);            \
        YYABORT;                                         \
    }                                                    \
    struct string_pattern_expression *smle;              \
    STR_PATTERN_DUP(smle, &_l);                          \
    if (!smle) {                                         \
        string_pattern_expression_reset(&_l);            \
        string_pattern_list_destroy(_patterns);          \
        YYABORT;                                         \
    }                                                    \
    list_add(&smle->node, &_patterns->list);             \
} while (0)

#define STR_PATTERN_LIST_APPEND(_patterns, _l) do {      \
    struct string_pattern_expression *smle;              \
    STR_PATTERN_DUP(smle, &_l);                          \
    if (!smle) {                                         \
        string_pattern_expression_reset(&_l);            \
        string_pattern_list_destroy(_patterns);          \
        YYABORT;                                         \
    }                                                    \
    list_add(&smle->node, &_patterns->list);             \
} while (0)

#define NUMERIC_EXP_INIT_I64(_nexp, _i64) do {               \
    int64_t i64;                                             \
    STRTOLL(i64, _i64);                                      \
    _nexp = i64;                                             \
} while (0)

#define NUMERIC_EXP_INIT_LD(_nexp, _ld) do {                 \
    long double ld;                                          \
    STRTOLD(ld, _ld);                                        \
    _nexp = ld;                                              \
} while (0)

#define NUMERIC_EXP_VAL(_n)                                  \
    (_n.type == NUMERIC_EXPRESSION_NUMERIC ? _n.ld : _n.i64)

#define NUMERIC_EXP_ADD(_nexp, _l, _r) do {                  \
    _nexp = _l + _r;                                         \
} while (0)

#define NUMERIC_EXP_SUB(_nexp, _l, _r) do {                  \
    _nexp = _l - _r;                                         \
} while (0)

#define NUMERIC_EXP_MUL(_nexp, _l, _r) do {                  \
    _nexp = _l * _r;                                         \
} while (0)

#define NUMERIC_EXP_DIV(_nexp, _l, _r) do {                  \
    _nexp = _l / _r;                                         \
} while (0)

#define NUMERIC_EXP_UMINUS(_nexp, _l) do {                   \
    _nexp = -_l;                                             \
} while (0)

#define IFE_INIT_INTEGER(_l, _r) do {                     \
    _l = iterative_formula_expression_create();           \
    if (!_l) {                                            \
        YYABORT;                                          \
    }                                                     \
    _l->type = ITERATIVE_FORMULA_EXPRESSION_NUM;          \
    STRTOD(_l->d, _r);                                    \
} while (0)

#define IFE_INIT_NUMBER(_l, _r) do {                      \
    _l = iterative_formula_expression_create();           \
    if (!_l) {                                            \
        YYABORT;                                          \
    }                                                     \
    _l->type = ITERATIVE_FORMULA_EXPRESSION_NUM;          \
    STRTOD(_l->d, _r);                                    \
} while (0)

#define IFE_INIT_ID(_l, _r) do {                          \
    _l = iterative_formula_expression_create();           \
    if (!_l) {                                            \
        YYABORT;                                          \
    }                                                     \
    _l->type = ITERATIVE_FORMULA_EXPRESSION_ID;           \
    TOKEN_DUP_STR(_l->id, _r);                            \
} while (0)

#define IFE_OP(_l, _ra, _rb, _op) do {                          \
    _l = iterative_formula_expression_create();                 \
    if (!_l) {                                                  \
        iterative_formula_expression_destroy(_ra);              \
        iterative_formula_expression_destroy(_rb);              \
        YYABORT;                                                \
    }                                                           \
    _l->type = ITERATIVE_FORMULA_EXPRESSION_OP;                 \
    _l->op = _op;                                               \
    bool ok = false;                                            \
    do {                                                        \
        if (!pctree_node_append_child(&_l->node, &_ra->node))   \
            break;                                              \
        _ra = NULL;                                             \
        if (!pctree_node_append_child(&_l->node, &_rb->node))   \
            break;                                              \
        _rb = NULL;                                             \
        ok = true;                                              \
    } while (0);                                                \
    if (ok)                                                     \
        break;                                                  \
    if (_ra)                                                    \
        iterative_formula_expression_destroy(_ra);              \
    if (_rb)                                                    \
        iterative_formula_expression_destroy(_rb);              \
    iterative_formula_expression_destroy(_l);                   \
    YYABORT;                                                    \
} while (0)

#define IFE_ADD(_l, _ra, _rb) IFE_OP(_l, _ra, _rb, iterative_formula_add)
#define IFE_SUB(_l, _ra, _rb) IFE_OP(_l, _ra, _rb, iterative_formula_sub)
#define IFE_MUL(_l, _ra, _rb) IFE_OP(_l, _ra, _rb, iterative_formula_mul)
#define IFE_DIV(_l, _ra, _rb) IFE_OP(_l, _ra, _rb, iterative_formula_div)
#define IFE_NEG(_l, _r) do {                                    \
    _l = iterative_formula_expression_create();                 \
    if (!_l) {                                                  \
        iterative_formula_expression_destroy(_r);               \
        YYABORT;                                                \
    }                                                           \
    _l->type = ITERATIVE_FORMULA_EXPRESSION_OP;                 \
    _l->op = iterative_formula_neg;                             \
    bool ok = false;                                            \
    do {                                                        \
        if (!pctree_node_append_child(&_l->node, &_r->node))    \
            break;                                              \
        _r = NULL;                                              \
        ok = true;                                              \
    } while (0);                                                \
    if (ok)                                                     \
        break;                                                  \
    if (_r)                                                     \
        iterative_formula_expression_destroy(_r);               \
    iterative_formula_expression_destroy(_l);                   \
    YYABORT;                                                    \
} while (0)

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
        number_comparing_logical_expression_destroy(_ncle);        \
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
        number_comparing_logical_expression_destroy(_ncle);        \
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

#define VNCLE_INIT(_vncle, _r) do {                             \
    _vncle = vncle_create();                                    \
    if (!_vncle) {                                              \
        YYABORT;                                                \
    }                                                           \
    _vncle->type = NUMBER_COMPARING_LOGICAL_EXPRESSION_NUM;     \
    _vncle->vncc  = _r;                                         \
} while (0)

#define VNCLE_OP(_vncle, _l, _r, _op) do {                         \
    _vncle = vncle_create();                                       \
    bool ok = false;                                               \
    do {                                                           \
        if (!_vncle)                                               \
            break;                                                 \
        _vncle->type = _op;                                        \
        if (!pctree_node_append_child(&_vncle->node, &_l->node))   \
            break;                                                 \
        _l = NULL;                                                 \
        if (!pctree_node_append_child(&_vncle->node, &_r->node))   \
            break;                                                 \
        _r = NULL;                                                 \
        ok = true;                                                 \
    } while (0);                                                   \
    if (ok)                                                        \
        break;                                                     \
    if (_l)                                                        \
        vncle_destroy(_l);                                         \
    if (_r)                                                        \
        vncle_destroy(_r);                                         \
    if (_vncle)                                                    \
        vncle_destroy(_vncle);                                     \
    YYABORT;                                                       \
} while (0)

#define VNCLE_AND(_vncle, _l, _r)                          \
    VNCLE_OP(_vncle, _l, _r, NUMBER_COMPARING_LOGICAL_EXPRESSION_AND)
#define VNCLE_OR(_vncle, _l, _r)                           \
    VNCLE_OP(_vncle, _l, _r, NUMBER_COMPARING_LOGICAL_EXPRESSION_OR)
#define VNCLE_XOR(_vncle, _l, _r)                          \
    VNCLE_OP(_vncle, _l, _r, NUMBER_COMPARING_LOGICAL_EXPRESSION_XOR)

#define VNCLE_NOT(_vncle, _l) do {                                 \
    _vncle = vncle_create();                                       \
    bool ok = false;                                               \
    do {                                                           \
        if (!_vncle)                                               \
            break;                                                 \
        _vncle->type = NUMBER_COMPARING_LOGICAL_EXPRESSION_NOT;    \
        if (!pctree_node_append_child(&_vncle->node, &_l->node))   \
            break;                                                 \
        _l = NULL;                                                 \
        ok = true;                                                 \
    } while (0);                                                   \
    if (ok)                                                        \
        break;                                                     \
    if (_l)                                                        \
        vncle_destroy(_l);                                         \
    if (_vncle)                                                    \
        vncle_destroy(_vncle);                                     \
    YYABORT;                                                       \
} while (0)

#define VNCC_INIT(_vncc, _l, _r) do {            \
    char *id = strndup(_l.text, _l.leng);        \
    if (!id)                                     \
        YYABORT;                                 \
    _vncc.key_name = id;                         \
    _vncc.ncc      = _r;                         \
} while (0)

#define IAL_INIT(_ial, _l) do {                  \
    _ial = ial_create();                         \
    if (!_ial)                                   \
        YYABORT;                                 \
    list_add(&_l->node, &_ial->list);            \
} while (0)

#define IAL_APPEND(_ial, _l, _r) do {          \
    list_add(&_r->node, &_l->list);            \
    _ial = _l;                                 \
} while (0)

#define IAE_INIT(_iae, _l, _r) do {                 \
    _iae = iae_create();                            \
    if (!_iae)                                      \
        YYABORT;                                    \
    _iae->key_name = strndup(_l.text, _l.leng);     \
    if (!_iae->key_name) {                          \
        iae_destroy(_iae);                          \
        YYABORT;                                    \
    }                                               \
    _iae->ife = _r;                                 \
} while (0)

#endif // PURC_EXECUTOR_TAB_H

