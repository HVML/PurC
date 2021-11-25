%code top {
/*
 * @file exe_formula.y
 * @author
 * @date
 * @brief The implementation of public part for formula.
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
}

%code top {
    // here to include header files required for generated exe_formula.tab.c
    #define _GNU_SOURCE
    #include <stddef.h>
    #include <stdio.h>
    #include <string.h>

    #include "purc-errors.h"

    #include "pcexe-helper.h"
    #include "exe_formula.h"
}

%code requires {
    // related struct/function decls
    // especially, for struct exe_formula_param
    // and parse function for example:
    // int exe_formula_parse(const char *input,
    //        struct exe_formula_param *param);
    // #include "exe_formula.h"
    // here we define them locally
    struct exe_formula_token {
        const char      *text;
        size_t           leng;
    };

    #define YYSTYPE       EXE_FORMULA_YYSTYPE
    #define YYLTYPE       EXE_FORMULA_YYLTYPE
    #ifndef YY_TYPEDEF_YY_SCANNER_T
    #define YY_TYPEDEF_YY_SCANNER_T
    typedef void* yyscan_t;
    #endif
}

%code provides {
}

%code {
    // generated header from flex
    // introduce yylex decl for later use
    #include "exe_formula.lex.h"

    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct exe_formula_param *param,           // match %parse-param
        const char *errsg
    );

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

    #define TOKEN_DUP_STR(_l, _r) do {              \
        char *s = (char*)malloc(_r.leng + 1);       \
        if (!s) {                                   \
            YYABORT;                                \
        }                                           \
        snprintf(s, _r.leng + 1, "%s", _r.text);    \
        _l = s;                                     \
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


    #define LOGICAL_EXP_INIT_NCC(_logic, _ncc) do {           \
        _logic = logical_expression_create();                 \
        if (!_logic) {                                        \
            YYABORT;                                          \
        }                                                     \
        _logic->type = LOGICAL_EXPRESSION_NUM;                \
        _logic->ncc  = _ncc;                                  \
    } while (0)

    #define LOGICAL_EXP_INIT_STR(_logic, _mexp) do {          \
        _logic = logical_expression_create();                 \
        if (!_logic) {                                        \
            string_matching_expression_reset(&_mexp);         \
            YYABORT;                                          \
        }                                                     \
        _logic->type = LOGICAL_EXPRESSION_STR;                \
        _logic->mexp = _mexp;                                 \
    } while (0)

    #define LOGICAL_EXP_AND(_logic, _l, _r) do {                        \
        _logic = logical_expression_create();                           \
        if (!_logic) {                                                  \
            logical_expression_destroy(_l);                             \
            logical_expression_destroy(_r);                             \
            YYABORT;                                                    \
        }                                                               \
        _logic->type = LOGICAL_EXPRESSION_OP;                           \
        _logic->op = logical_and;                                       \
        bool ok = pctree_node_append_child(&_logic->node, &_l->node);   \
        if (ok) {                                                       \
            ok = pctree_node_append_child(&_logic->node, &_r->node);    \
            if (ok)                                                     \
                break;                                                  \
        } else {                                                        \
            logical_expression_destroy(_l);                             \
        }                                                               \
        logical_expression_destroy(_r);                                 \
        logical_expression_destroy(_logic);                             \
        YYABORT;                                                        \
    } while (0)

    #define LOGICAL_EXP_OR(_logic, _l, _r) do {                         \
        _logic = logical_expression_create();                           \
        if (!_logic) {                                                  \
            logical_expression_destroy(_l);                             \
            logical_expression_destroy(_r);                             \
            YYABORT;                                                    \
        }                                                               \
        _logic->type = LOGICAL_EXPRESSION_OP;                           \
        _logic->op = logical_or;                                        \
        bool ok = pctree_node_append_child(&_logic->node, &_l->node);   \
        if (ok) {                                                       \
            ok = pctree_node_append_child(&_logic->node, &_r->node);    \
            if (ok)                                                     \
                break;                                                  \
        } else {                                                        \
            logical_expression_destroy(_l);                             \
        }                                                               \
        logical_expression_destroy(_r);                                 \
        logical_expression_destroy(_logic);                             \
        YYABORT;                                                        \
    } while (0)

    #define LOGICAL_EXP_XOR(_logic, _l, _r) do {                        \
        _logic = logical_expression_create();                           \
        if (!_logic) {                                                  \
            logical_expression_destroy(_l);                             \
            logical_expression_destroy(_r);                             \
            YYABORT;                                                    \
        }                                                               \
        _logic->type = LOGICAL_EXPRESSION_OP;                           \
        _logic->op = logical_xor;                                       \
        bool ok = pctree_node_append_child(&_logic->node, &_l->node);   \
        if (ok) {                                                       \
            ok = pctree_node_append_child(&_logic->node, &_r->node);    \
            if (ok)                                                     \
                break;                                                  \
        } else {                                                        \
            logical_expression_destroy(_l);                             \
        }                                                               \
        logical_expression_destroy(_r);                                 \
        logical_expression_destroy(_logic);                             \
        YYABORT;                                                        \
    } while (0)

    #define LOGICAL_EXP_NOT(_logic, _l) do {                            \
        _logic = logical_expression_create();                           \
        if (!_logic) {                                                  \
            logical_expression_destroy(_l);                             \
            YYABORT;                                                    \
        }                                                               \
        _logic->type = LOGICAL_EXPRESSION_OP;                           \
        _logic->op = logical_not;                                       \
        bool ok = pctree_node_append_child(&_logic->node, &_l->node);   \
        if (ok) {                                                       \
            break;                                                      \
        }                                                               \
        logical_expression_destroy(_l);                                 \
        logical_expression_destroy(_logic);                             \
        YYABORT;                                                        \
    } while (0)

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

    #define SET_RULE(_rule) do {                            \
        if (param) {                                        \
            param->rule = _rule;                            \
        } else {                                            \
            formula_rule_release(&_rule);                   \
        }                                                   \
    } while (0)
}

/* Bison declarations. */
%require "3.0.4"
%define api.prefix {exe_formula_yy}
%define api.pure full
%define api.token.prefix {TOK_EXE_FORMULA_}
%define locations
%define parse.error verbose
%define parse.lac full
%define parse.trace true
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct exe_formula_param *param }

// union members
%union { struct exe_formula_token token; }
%union { char *str; }
%union { char c; }
%union { struct logical_expression *logic; }
%union { struct iterative_formula_expression *ife; }
%union { struct number_comparing_condition ncc; }
%union { struct formula_rule rule; }
%union { double nexp; }

%destructor { logical_expression_destroy($$); } <logic>
%destructor { iterative_formula_expression_destroy($$); } <ife>
%destructor { formula_rule_release(&$$); } <rule>

%token FORMULA BY
%token LT GT LE GE NE EQ NOT
%token <token> INTEGER NUMBER ID

%left '-' '+'
%left '*' '/'
%precedence UMINUS

%left AND OR XOR
%precedence NEG

%nterm <rule>  formula_rule;
%nterm <logic> logical_expression;
%nterm <ife>   iterative_formula_expression;
%nterm <ncc>   number_comparing_condition;
%nterm <nexp>  exp;

%% /* The grammar follows. */

input:
  formula_rule       { SET_RULE($1); }
;

formula_rule:
  FORMULA ':' logical_expression BY iterative_formula_expression     { $$.lexp = $3; $$.ife = $5; }
;

logical_expression:
  number_comparing_condition   { LOGICAL_EXP_INIT_NCC($$, $1); }
| logical_expression AND logical_expression { LOGICAL_EXP_AND($$, $1, $3); }
| logical_expression OR logical_expression  { LOGICAL_EXP_OR($$, $1, $3); }
| logical_expression XOR logical_expression { LOGICAL_EXP_XOR($$, $1, $3); }
| NOT logical_expression %prec NEG  { LOGICAL_EXP_NOT($$, $2); }
| '(' logical_expression ')'   { $$ = $2; }
;

number_comparing_condition:
  LT exp           { $$.op_type = NUMBER_COMPARING_LT; $$.nexp = $2; }
| GT exp           { $$.op_type = NUMBER_COMPARING_GT; $$.nexp = $2; }
| LE exp           { $$.op_type = NUMBER_COMPARING_LE; $$.nexp = $2; }
| GE exp           { $$.op_type = NUMBER_COMPARING_GE; $$.nexp = $2; }
| NE exp           { $$.op_type = NUMBER_COMPARING_NE; $$.nexp = $2; }
| EQ exp           { $$.op_type = NUMBER_COMPARING_EQ; $$.nexp = $2; }
;

exp:
  INTEGER               { NUMERIC_EXP_INIT_I64($$, $1); }
| NUMBER                { NUMERIC_EXP_INIT_LD($$, $1); }
| exp '+' exp           { NUMERIC_EXP_ADD($$, $1, $3); }
| exp '-' exp           { NUMERIC_EXP_SUB($$, $1, $3); }
| exp '*' exp           { NUMERIC_EXP_MUL($$, $1, $3); }
| exp '/' exp           { NUMERIC_EXP_DIV($$, $1, $3); }
| '-' exp %prec UMINUS  { NUMERIC_EXP_UMINUS($$, $2); }
| '(' exp ')'           { $$ = $2; }
;

iterative_formula_expression:
  INTEGER           { IFE_INIT_INTEGER($$, $1); }
| NUMBER            { IFE_INIT_NUMBER($$, $1); }
| ID                { IFE_INIT_ID($$, $1); }
| iterative_formula_expression '+' iterative_formula_expression     { IFE_ADD($$, $1, $3); }
| iterative_formula_expression '-' iterative_formula_expression     { IFE_SUB($$, $1, $3); }
| iterative_formula_expression '*' iterative_formula_expression     { IFE_MUL($$, $1, $3); }
| iterative_formula_expression '/' iterative_formula_expression     { IFE_DIV($$, $1, $3); }
| '-' iterative_formula_expression %prec UMINUS    { IFE_NEG($$, $2); }
| '(' iterative_formula_expression ')'             { $$ = $2; }
;

%%

/* Called by yyparse on error. */
static void
yyerror(
    YYLTYPE *yylloc,                   // match %define locations
    yyscan_t arg,                      // match %param
    struct exe_formula_param *param,           // match %parse-param
    const char *errsg
)
{
    // to implement it here
    (void)yylloc;
    (void)arg;
    (void)param;
    if (!param)
        return;
    asprintf(&param->err_msg, "(%d,%d)->(%d,%d): %s",
        yylloc->first_line, yylloc->first_column,
        yylloc->last_line, yylloc->last_column - 1,
        errsg);
}

int exe_formula_parse(const char *input, size_t len,
        struct exe_formula_param *param)
{
    yyscan_t arg = {0};
    exe_formula_yylex_init(&arg);
    // exe_formula_yyset_in(in, arg);
    int debug_flex = param ? param->debug_flex : 0;
    int debug_bison = param ? param->debug_bison: 0;
    exe_formula_yyset_debug(debug_flex, arg);
    yydebug = debug_bison;
    // exe_formula_yyset_extra(param, arg);
    exe_formula_yy_scan_bytes(input ? input : "", input ? len : 0, arg);
    int ret =exe_formula_yyparse(arg, param);
    exe_formula_yylex_destroy(arg);
    return ret ? -1 : 0;
}

