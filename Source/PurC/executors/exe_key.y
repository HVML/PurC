%code top {
/*
 * @file exe_key.y
 * @author
 * @date
 * @brief The implementation of public part for key.
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
    // here to include header files required for generated exe_key.tab.c
    #ifdef _GNU_SOURCE
    #undef _GNU_SOURCE
    #endif
    #define _GNU_SOURCE
    #include <stdio.h>
    #include <stddef.h>
    #include "pcexe-helper.h"
}

%code requires {
    #ifdef _GNU_SOURCE
    #undef _GNU_SOURCE
    #endif
    #define _GNU_SOURCE
    #include <stdio.h>
    #include <stddef.h>
    #include "pcexe-helper.h"
    // related struct/function decls
    // especially, for struct exe_key_param
    // and parse function for example:
    // int exe_key_parse(const char *input,
    //        struct exe_key_param *param);
    // #include "exe_key.h"
    // here we define them locally
    struct exe_key_param {
        char *err_msg;
        int debug_flex;
        int debug_bison;
    };

    struct exe_key_token {
        const char      *text;
        size_t           leng;
    };

    #define YYSTYPE       EXE_KEY_YYSTYPE
    #define YYLTYPE       EXE_KEY_YYLTYPE
    #ifndef YY_TYPEDEF_YY_SCANNER_T
    #define YY_TYPEDEF_YY_SCANNER_T
    typedef void* yyscan_t;
    #endif

    int exe_key_parse(const char *input, size_t len,
            struct exe_key_param *param);
}

%code provides {
}

%code {
    // generated header from flex
    // introduce yylex decl for later use
    #include "exe_key.lex.h"

    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct exe_key_param *param,           // match %parse-param
        const char *errsg
    );

    #define STRLIST_INIT_STR(_list, _s) do {                          \
        pcexe_strlist_init(&_list);                                   \
        if (pcexe_strlist_append_buf(&_list, _s.text, _s.leng)) {     \
            YYABORT;                                                  \
        }                                                             \
    } while (0)

    #define STRLIST_INIT_CHR(_list, _c) do {                          \
        pcexe_strlist_init(&_list);                                   \
        if (pcexe_strlist_append_chr(&_list, _c)) {                   \
            YYABORT;                                                  \
        }                                                             \
    } while (0)

    #define STRLIST_INIT_UNI(_list, _u) do {                          \
        pcexe_strlist_init(&_list);                                   \
        if (pcexe_strlist_append_uni(&_list, _u.text, _u.leng)) {     \
            YYABORT;                                                  \
        }                                                             \
    } while (0)

    #define STRLIST_APPEND_STR(_list, _s) do {                        \
        if (pcexe_strlist_append_buf(&_list, _s.text, _s.leng)) {     \
            YYABORT;                                                  \
        }                                                             \
    } while (0)

    #define STRLIST_APPEND_CHR(_list, _c) do {                        \
        if (pcexe_strlist_append_chr(&_list, _c)) {                   \
            YYABORT;                                                  \
        }                                                             \
    } while (0)

    #define STRLIST_APPEND_UNI(_list, _u) do {                        \
        if (pcexe_strlist_append_uni(&_list, _u.text, _u.leng)) {     \
            YYABORT;                                                  \
        }                                                             \
    } while (0)
}

/* Bison declarations. */
%require "3.0.4"
%define api.prefix {exe_key_yy}
%define api.pure full
%define api.token.prefix {TOK_EXE_KEY_}
%define locations
%define parse.error verbose
%define parse.lac full
%define parse.trace true
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct exe_key_param *param }

// union members
%union { struct exe_key_token token; }
%union { char *str; }
%union { char c; }
%union { struct pcexe_strlist slist; }

%destructor { pcexe_strlist_reset(&$$); } <slist>

%token KEY ALL LIKE KV VALUE FOR AS
%token NOT
%token <c>     MATCHING_FLAG REGEXP_FLAG
%token <c>     CHR
%token <token> MATCHING_LENGTH
%token <token> STR UNI
%token <token> INTEGER NUMBER

%left '-' '+'
%left '*' '/'
%precedence UMINUS

%left AND OR XOR
%precedence NEG

%nterm <slist>  literal_char_sequence
%nterm <slist>  regular_expression


%% /* The grammar follows. */

input:
  rule
;

rule:
  key_rule
;

key_rule:
  KEY ':' subrule for_clause
;

subrule:
  ALL
| logical_expression
;

logical_expression:
  string_matching_expression
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

literal_char_sequence:
  STR  { STRLIST_INIT_STR($$, $1); }
| CHR  { STRLIST_INIT_CHR($$, $1); }
| UNI  { STRLIST_INIT_UNI($$, $1); }
| literal_char_sequence STR    { STRLIST_APPEND_STR($1, $2); $$ = $1; }
| literal_char_sequence CHR    { STRLIST_APPEND_CHR($1, $2); $$ = $1; }
| literal_char_sequence UNI    { STRLIST_APPEND_UNI($1, $2); $$ = $1; }
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
  '"' literal_char_sequence '"' matching_suffix  { pcexe_strlist_reset(&$2); }
;

string_pattern_list:
  string_pattern_expression
| string_pattern_list ',' string_pattern_expression
;

string_pattern_expression:
  '"' wildcard_expression '"' matching_suffix
| '/' regular_expression '/' regexp_suffix      { pcexe_strlist_reset(&$2); }
;

wildcard_expression:
  literal_char_sequence     { pcexe_strlist_reset(&$1); }
;

regular_expression:
  STR  { STRLIST_INIT_STR($$, $1); }
| CHR  { STRLIST_INIT_CHR($$, $1); }
| regular_expression STR    { STRLIST_APPEND_STR($1, $2); $$ = $1; }
| regular_expression CHR    { STRLIST_APPEND_CHR($1, $2); $$ = $1; }
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

%%


/* Called by yyparse on error. */
static void
yyerror(
    YYLTYPE *yylloc,                   // match %define locations
    yyscan_t arg,                      // match %param
    struct exe_key_param *param,           // match %parse-param
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

int exe_key_parse(const char *input, size_t len,
        struct exe_key_param *param)
{
    yyscan_t arg = {0};
    exe_key_yylex_init(&arg);
    // exe_key_yyset_in(in, arg);
    int debug_flex = param ? param->debug_flex : 0;
    int debug_bison = param ? param->debug_bison: 0;
    exe_key_yyset_debug(debug_flex, arg);
    yydebug = debug_bison;
    // exe_key_yyset_extra(param, arg);
    exe_key_yy_scan_bytes(input ? input : "", input ? len : 0, arg);
    int ret =exe_key_yyparse(arg, param);
    exe_key_yylex_destroy(arg);
    return ret ? 1 : 0;
}

