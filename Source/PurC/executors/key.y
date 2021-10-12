%code top {
/*
 * @file key.y
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
    // here to include header files required for generated key.tab.c
}

%code requires {
    #include <stddef.h>
    // related struct/function decls
    // especially, for struct key_param
    // and parse function for example:
    // int key_parse(const char *input,
    //        struct key_param *param);
    // #include "key.h"
    // here we define them locally
    struct key_param {
        char      placeholder[0];
    };

    struct key_token {
        const char      *text;
        size_t           leng;
    };

    #define YYSTYPE       KEY_YYSTYPE
    #define YYLTYPE       KEY_YYLTYPE
    typedef void *yyscan_t;

    int key_parse(const char *input,
            struct key_param *param);
}

%code provides {
}

%code {
    // generated header from flex
    // introduce yylex decl for later use
    #include "key.lex.h"

    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct key_param *param,       // match %parse-param
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
%define api.prefix {key_yy}
%define api.pure full
%define api.token.prefix {TOK_KEY_}
%define locations
%define parse.error verbose
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct key_param *param }

// union members
%union { struct key_token token; }
%union { char *str; }

    /* %destructor { free($$); } <str> */ // destructor for `str`

%token KEY ALL FOR VALUE KV LIKE
%token SQ
%token <token>  INTEGER
%token <token>  STR CHR

 /* %nterm <str>   args */ // non-terminal `input` use `str` to store
                           // token value as well


%% /* The grammar follows. */

input:
  %empty
| KEY ':' ALL
| KEY ':' ALL ',' FOR VALUE
| KEY ':' ALL ',' FOR KEY
| KEY ':' ALL ',' FOR KV
| KEY ':' key_name_list
| KEY ':' key_name_list ',' FOR VALUE
| KEY ':' key_name_list ',' FOR KEY
| KEY ':' key_name_list ',' FOR KV
| "'" 
;

key_name_list:
  key_list_expression
| key_name_list ',' key_list_expression
;

key_list_expression:
  LIKE key_pattern_expression
| literal_str_exp
;

key_pattern_expression:
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
  'g'
| 'i'
| 'm'
| 's'
| 'u'
| 'y'
;

matching_flags:
  'i'
| 's'
| 'c'
;

max_matching_length:
  INTEGER
;

%%

/* Called by yyparse on error. */
static void
yyerror(
    YYLTYPE *yylloc,                   // match %define locations
    yyscan_t arg,                      // match %param
    struct key_param *param,       // match %parse-param
    const char *errsg
)
{
    // to implement it here
    (void)yylloc;
    (void)arg;
    (void)param;
    fprintf(stderr, "(%d,%d)->(%d,%d): %s\n",
        yylloc->first_line, yylloc->first_column,
        yylloc->last_line, yylloc->last_column,
        errsg);
}

int key_parse(const char *input,
        struct key_param *param)
{
    yyscan_t arg = {0};
    key_yylex_init(&arg);
    // key_yyset_in(in, arg);
    // key_yyset_debug(1, arg);
    key_yyset_debug(1, arg);
    // key_yyset_extra(param, arg);
    key_yy_scan_string(input, arg);
    int ret =key_yyparse(arg, param);
    key_yylex_destroy(arg);
    return ret ? 1 : 0;
}

