%code top {
/*
 * @file exe_filter.y
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
    // here to include header files required for generated exe_filter.tab.c
}

%code requires {
    #ifdef _GNU_SOURCE
    #undef _GNU_SOURCE
    #endif
    #define _GNU_SOURCE
    #include <stdio.h>
    #include <stddef.h>
    // related struct/function decls
    // especially, for struct exe_filter_param
    // and parse function for example:
    // int exe_filter_parse(const char *input,
    //        struct exe_filter_param *param);
    // #include "exe_filter.h"
    // here we define them locally
    struct exe_filter_param {
        char *err_msg;
        int debug_flex;
        int debug_bison;
    };

    struct exe_filter_token {
        const char      *text;
        size_t           leng;
    };

    #define YYSTYPE       EXE_FILTER_YYSTYPE
    #define YYLTYPE       EXE_FILTER_YYLTYPE
    #ifndef YY_TYPEDEF_YY_SCANNER_T
    #define YY_TYPEDEF_YY_SCANNER_T
    typedef void* yyscan_t;
    #endif

    int exe_filter_parse(const char *input, size_t len,
            struct exe_filter_param *param);
}

%code provides {
}

%code {
    // generated header from flex
    // introduce yylex decl for later use
    #include "exe_filter.lex.h"

    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct exe_filter_param *param,           // match %parse-param
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
%define api.prefix {exe_filter_yy}
%define api.pure full
%define api.token.prefix {TOK_EXE_FILTER_}
%define locations
%define parse.error verbose
%define parse.lac full
%define parse.trace true
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct exe_filter_param *param }

// union members
%union { struct exe_filter_token token; }
%union { char *str; }
%union { char c; }

    /* %destructor { free($$); } <str> */ // destructor for `str`

%token FILTER ALL LIKE KV KEY VALUE FOR
%token LT GT LE GE NE EQ
%token SP
%token SQ "'"
%token STR CHR UNI
%token <token>  INTEGER

%left ALL
%left LT GT LE GE NE EQ LIKE ','
%left '-' '+'
%left '*' '/'
%precedence NEG /* negation--unary minus */
%precedence SQ
%precedence '\n'
%precedence SP

 /* %nterm <str>   args */ // non-terminal `input` use `str` to store
                           // token value as well

%% /* The grammar follows. */

input:
  rule
;

rule:
  exe_filter_rule
| rule SP
| rule '\n'
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

osp:
  %empty
| osp SP
;

colon:
  ':'
| colon SP
| colon '\n'
;

comma:
  ','
| comma SP
| comma '\n'
;

filter:
  FILTER
| filter SP
;

exe_filter_rule:
  filter colon ALL
| filter colon ALL for_clause
| filter colon ALL sp
| filter colon ALL sp for_clause
| filter colon subrules
| filter colon subrules for_clause
;

subrules:
  subrule
| subrules ',' subrule
| subrules ',' sp subrule
;

subrule:
  pred_exp
| LIKE sp pattern_expression
| LIKE sp pattern_expression sp
| literal_str_exp
;

for_clause:
  comma FOR osp for_param
;

for_param:
  KV
| KEY
| VALUE
;

pred_exp:
  LT sp exp
| GT sp exp
| LE sp exp
| GE sp exp
| NE sp exp
| EQ sp exp
;

pattern_expression:
  SQ sq_str SQ
| SQ sq_str SQ matching_flags
| SQ sq_str SQ matching_flags max_matching_length
| SQ sq_str SQ max_matching_length
| '/' regular_str '/'
| '/' regular_str '/' regexp_flags
;

literal_str:
  SQ sq_str SQ
;

literal_str_exp:
  literal_str matching_flags
| literal_str matching_flags max_matching_length
| literal_str max_matching_length
;

sq_str:
  STR
| CHR
| UNI
| sq_str STR
| sq_str CHR
| sq_str UNI
;

regular_str:
  STR
| CHR
| UNI
| regular_str STR
| regular_str CHR
| regular_str UNI
;

regexp_flags:
  regexp_flag
| regexp_flags regexp_flag
;

regexp_flag:
  'i'
| 'g'
| 'm'
| 's'
| 'u'
| 'y'
;

matching_flags:
  matching_flag
| matching_flags matching_flag
;

matching_flag:
  'c'
| 'i'
| 's'
;

max_matching_length:
  INTEGER
;

exp:
  INTEGER
| exp '+' ows exp
| exp '-' ows exp
| exp '*' ows exp
| exp '/' ows exp
| '(' ows exp ')'
| exp SP
| exp '\n'
;

%%

/* Called by yyparse on error. */
static void
yyerror(
    YYLTYPE *yylloc,                   // match %define locations
    yyscan_t arg,                      // match %param
    struct exe_filter_param *param,           // match %parse-param
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

int exe_filter_parse(const char *input, size_t len,
        struct exe_filter_param *param)
{
    yyscan_t arg = {0};
    exe_filter_yylex_init(&arg);
    // exe_filter_yyset_in(in, arg);
    int debug_flex = param ? param->debug_flex : 0;
    int debug_bison = param ? param->debug_bison: 0;
    exe_filter_yyset_debug(debug_flex, arg);
    yydebug = debug_bison;
    // exe_filter_yyset_extra(param, arg);
    exe_filter_yy_scan_bytes(input ? input : "", input ? len : 0, arg);
    int ret =exe_filter_yyparse(arg, param);
    exe_filter_yylex_destroy(arg);
    return ret ? 1 : 0;
}

