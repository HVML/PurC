%code top {
/*
 * @file range.y
 * @author
 * @date
 * @brief The implementation of public part for range.
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
    // here to include header files required for generated range.tab.c
}

%code requires {
    #ifdef _GNU_SOURCE
    #undef _GNU_SOURCE
    #endif
    #define _GNU_SOURCE
    #include <stdio.h>
    #include <stddef.h>
    // related struct/function decls
    // especially, for struct range_param
    // and parse function for example:
    // int range_parse(const char *input,
    //        struct range_param *param);
    // #include "range.h"
    // here we define them locally
    struct range_param {
        char *err_msg;
        int debug_flex;
        int debug_bison;
    };

    struct range_token {
        const char      *text;
        size_t           leng;
    };

    #define YYSTYPE       RANGE_YYSTYPE
    #define YYLTYPE       RANGE_YYLTYPE
    typedef void *yyscan_t;

    int range_parse(const char *input, size_t len,
            struct range_param *param);
}

%code provides {
}

%code {
    // generated header from flex
    // introduce yylex decl for later use
    #include "range.lex.h"

    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct range_param *param,           // match %parse-param
        const char *errsg
    );

    #define SET_ARGS(_r, _a) do {              \
        _r = strndup(_a.text, _a.leng);        \
        if (!_r)                               \
            YYABORT;                           \
    } while (0)

    #define APPEND_ARGS(_r, _a, _b) do {       \
        size_t len = strlen(_a);               \
        size_t n   = len + _b.leng;            \
        char *s = (char*)realloc(_a, n+1);     \
        if (!s) {                              \
            free(_r);                          \
            YYABORT;                           \
        }                                      \
        memcpy(s+len, _b.text, _b.leng);       \
        _r = s;                                \
    } while (0)
}

/* Bison declarations. */
%require "3.0.4"
%define api.prefix {range_yy}
%define api.pure full
%define api.token.prefix {TOK_RANGE_}
%define locations
%define parse.error verbose
%define parse.lac full
%define parse.trace true
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct range_param *param }

// union members
%union { struct range_token token; }
%union { char *str; }
%union { char c; }

    /* %destructor { free($$); } <str> */ // destructor for `str`

%token RANGE FROM TO ADVANCE
%token SP
%token <token>  INTEGER

%left '-' '+'
%left '*' '/'
%precedence NEG /* negation--unary minus */

 /* %nterm <str>   args */ // non-terminal `input` use `str` to store
                           // token value as well

%expect 11

%% /* The grammar follows. */

input:
  rule
;

rule:
  range_rule
| range_rule_ln
;

range_rule_ln:
  range_rule '\n'
| range_rule_ln ws
;

range_rule:
  range colon from_clause to_clause advance_clause
;

ws:
  SP
| '\n'
;

colon:
  ':'
| colon ws
;

comma:
  ','
| comma ws
;

range:
  RANGE
| range SP
;

from_clause:
  from int_eval
;

from:
  FROM
| from ws
;

to_clause:
  to int_eval
;

to:
  TO
| to ws
;

advance_clause:
  %empty
| ws comma advance int_eval
;

advance:
  ADVANCE
| advance ws
;

int_eval:
  exp
;

exp:
  INTEGER
| exp '+' exp
| exp '-' exp
| exp '*' exp
| exp '/' exp
| exp ws
;

%%

/* Called by yyparse on error. */
static void
yyerror(
    YYLTYPE *yylloc,                   // match %define locations
    yyscan_t arg,                      // match %param
    struct range_param *param,           // match %parse-param
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

int range_parse(const char *input, size_t len,
        struct range_param *param)
{
    yyscan_t arg = {0};
    range_yylex_init(&arg);
    // range_yyset_in(in, arg);
    int debug_flex = param ? param->debug_flex : 0;
    int debug_bison = param ? param->debug_bison: 0;
    range_yyset_debug(debug_flex, arg);
    yydebug = debug_bison;
    // range_yyset_extra(param, arg);
    range_yy_scan_bytes(input ? input : "", input ? len : 0, arg);
    int ret =range_yyparse(arg, param);
    range_yylex_destroy(arg);
    return ret ? 1 : 0;
}

