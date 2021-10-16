%code top {
/*
 * @file filter.y
 * @author
 * @date
 * @brief The implementation of public part for filter.
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
    // here to include header files required for generated filter.tab.c
}

%code requires {
    #ifdef _GNU_SOURCE
    #undef _GNU_SOURCE
    #endif
    #define _GNU_SOURCE
    #include <stdio.h>
    #include <stddef.h>
    // related struct/function decls
    // especially, for struct filter_param
    // and parse function for example:
    // int filter_parse(const char *input,
    //        struct filter_param *param);
    // #include "filter.h"
    // here we define them locally
    struct filter_param {
        char *err_msg;
        int debug_flex;
        int debug_bison;
    };

    struct filter_token {
        const char      *text;
        size_t           leng;
    };

    #define YYSTYPE       FILTER_YYSTYPE
    #define YYLTYPE       FILTER_YYLTYPE
    typedef void *yyscan_t;

    int filter_parse(const char *input, size_t len,
            struct filter_param *param);
}

%code provides {
}

%code {
    // generated header from flex
    // introduce yylex decl for later use
    #include "filter.lex.h"

    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct filter_param *param,           // match %parse-param
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
%define api.prefix {filter_yy}
%define api.pure full
%define api.token.prefix {TOK_FILTER_}
%define locations
%define parse.error verbose
%define parse.lac full
%define parse.trace true
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct filter_param *param }

// union members
%union { struct filter_token token; }
%union { char *str; }
%union { char c; }

    /* %destructor { free($$); } <str> */ // destructor for `str`

%token FILTER ALL LIKE KV KEY VALUE FOR
%token LT GT LE GE NE EQ
%token SP
%token SQ "'"
%token STR CHR
%token <c>             MATCHING_FLAG REGEXP_FLAG
%token <token>  INTEGER

%left '-' '+'
%left '*' '/'
%precedence NEG /* negation--unary minus */

 /* %nterm <str>   args */ // non-terminal `input` use `str` to store
                           // token value as well

%expect 21

%% /* The grammar follows. */

input:
  rule
| rule_ln
;

rule_ln:
  rule '\n'
| rule_ln ws
;

ws:
  SP
| '\n'
| ws SP
| ws '\n'
;

ows:
  %empty
| ws
;

sp:
  SP
| sp SP
;

colon:
  ':'
| colon ws
;

comma:
  ','
| comma ws
;

all:
  ALL
| all ws
;

for:
  FOR
| for SP
;

filter:
  FILTER
| filter SP
;

rule:
  filter colon all for_clause
| filter colon subrules for_clause
;

subrules:
  subrule
| subrules comma subrule
;

subrule:
  pred_exp
| LIKE sp pattern_expression
| literal_str_exp
| subrule SP
;

for_clause:
  %empty
| comma for for_param
;

for_param:
  KV
| KEY
| VALUE
| for_param SP
;

pred_exp:
  pred sp int_eval
;

pred:
  LT
| GT
| LE
| GE
| NE
| EQ
;

pattern_expression:
  literal_str_exp
| '/' regular_str '/'
| '/' regular_str '/' regexp_flags
;

literal_str:
  SQ sq_str SQ
;

literal_str_exp:
  literal_str
| literal_str matching_flags
| literal_str matching_flags max_matching_length
| literal_str max_matching_length
;

sq_str:
  STR
| CHR
| sq_str STR
| sq_str CHR
;

regular_str:
  STR
| CHR
| regular_str STR
| regular_str CHR
;

regexp_flags:
  REGEXP_FLAG
| regexp_flags REGEXP_FLAG
;

matching_flags:
  MATCHING_FLAG
| matching_flags MATCHING_FLAG
;

max_matching_length:
  INTEGER
;

int_eval:
  exp
;

exp:
  INTEGER
| exp '+' ows exp
| exp '-' ows exp
| exp '*' ows exp
| exp '/' ows exp
| '(' ows exp ')'
| exp ws
;

%%

/* Called by yyparse on error. */
static void
yyerror(
    YYLTYPE *yylloc,                   // match %define locations
    yyscan_t arg,                      // match %param
    struct filter_param *param,           // match %parse-param
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

int filter_parse(const char *input, size_t len,
        struct filter_param *param)
{
    yyscan_t arg = {0};
    filter_yylex_init(&arg);
    // filter_yyset_in(in, arg);
    int debug_flex = param ? param->debug_flex : 0;
    int debug_bison = param ? param->debug_bison: 0;
    filter_yyset_debug(debug_flex, arg);
    yydebug = debug_bison;
    // filter_yyset_extra(param, arg);
    filter_yy_scan_bytes(input ? input : "", input ? len : 0, arg);
    int ret =filter_yyparse(arg, param);
    filter_yylex_destroy(arg);
    return ret ? 1 : 0;
}

