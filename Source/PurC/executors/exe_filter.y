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

%token FILTER ALL LIKE KV KEY VALUE FOR AS
%token LT GT LE GE NE EQ NOT
%token <c>     MATCHING_FLAG REGEXP_FLAG
%token <token> MATCHING_LENGTH
%token <token> STR CHR UNI
%token <token> INTEGER NUMBER

%left '-' '+'
%left '*' '/'
%precedence UMINUS

%left AND OR XOR
%precedence NEG

 /* %nterm <str>   args */ // non-terminal `input` use `str` to store
                           // token value as well

%% /* The grammar follows. */

input:
  rule
;

rule:
  filter_rule
;

filter_rule:
  FILTER ':' subrule for_clause
;

subrule:
  ALL
| logical_expression
;

logical_expression:
  number_comparing_condition
| string_matching_expression
| logical_expression AND logical_expression
| logical_expression OR logical_expression
| logical_expression XOR logical_expression
| NOT logical_expression %prec NEG
| '(' logical_expression ')'
;

for_clause:
  %empty
| FOR KV
| FOR KEY
| FOR VALUE
;

number_comparing_condition:
  LT exp
| GT exp
| LE exp
| GE exp
| NE exp
| EQ exp
;

literal_char_sequence:
  STR
| CHR
| UNI
| literal_char_sequence STR
| literal_char_sequence CHR
| literal_char_sequence UNI
;

string_matching_expression:
  LIKE string_pattern_list
| AS string_literal_list
;

string_literal_list:
  string_literal_expression
| string_literal_list ',' string_literal_expression
;

string_literal_expression:
  '"' literal_char_sequence '"' matching_suffix
;

string_pattern_list:
  string_pattern_expression
| string_pattern_list ',' string_pattern_expression
;

string_pattern_expression:
  '"' wildcard_expression '"' matching_suffix
| '/' regular_expression '/' regexp_suffix
;

wildcard_expression:
  literal_char_sequence
;

regular_expression:
  STR
| CHR
| regular_expression STR
| regular_expression CHR
;

matching_suffix:
  %empty
| matching_flags
| matching_flags max_matching_length
| max_matching_length
;

regexp_suffix:
  %empty
| regexp_flags
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
  MATCHING_LENGTH
;

exp:
  INTEGER
| NUMBER
| exp '+' exp
| exp '-' exp
| exp '*' exp
| exp '/' exp
| '-' exp %prec UMINUS
| '(' exp ')'
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
    return ret ? -1 : 0;
}

