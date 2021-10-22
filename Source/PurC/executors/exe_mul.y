%code top {
/*
 * @file exe_mul.y
 * @author
 * @date
 * @brief The implementation of public part for mul.
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
    // here to include header files required for generated exe_mul.tab.c
}

%code requires {
    #ifdef _GNU_SOURCE
    #undef _GNU_SOURCE
    #endif
    #define _GNU_SOURCE
    #include <stdio.h>
    #include <stddef.h>
    // related struct/function decls
    // especially, for struct exe_mul_param
    // and parse function for example:
    // int exe_mul_parse(const char *input,
    //        struct exe_mul_param *param);
    // #include "exe_mul.h"
    // here we define them locally
    struct exe_mul_param {
        char *err_msg;
        int debug_flex;
        int debug_bison;
    };

    struct exe_mul_token {
        const char      *text;
        size_t           leng;
    };

    #define YYSTYPE       EXE_MUL_YYSTYPE
    #define YYLTYPE       EXE_MUL_YYLTYPE
    #ifndef YY_TYPEDEF_YY_SCANNER_T
    #define YY_TYPEDEF_YY_SCANNER_T
    typedef void* yyscan_t;
    #endif

    int exe_mul_parse(const char *input, size_t len,
            struct exe_mul_param *param);
}

%code provides {
}

%code {
    // generated header from flex
    // introduce yylex decl for later use
    #include "exe_mul.lex.h"

    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct exe_mul_param *param,           // match %parse-param
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
%define api.prefix {exe_mul_yy}
%define api.pure full
%define api.token.prefix {TOK_EXE_MUL_}
%define locations
%define parse.error verbose
%define parse.lac full
%define parse.trace true
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct exe_mul_param *param }

// union members
%union { struct exe_mul_token token; }
%union { char *str; }
%union { char c; }

    /* %destructor { free($$); } <str> */ // destructor for `str`

%token MUL BY
%token LT GT LE GE NE EQ
%token <token> INTEGER NUMBER

%left '-' '+'
%left '*' '/'
%precedence UMINUS

 /* %nterm <str>   args */ // non-terminal `input` use `str` to store
                           // token value as well

%% /* The grammar follows. */

input:
  mul_rule
;

mul_rule:
  MUL ':' number_comparing_condition BY exp
;

number_comparing_condition:
  LT exp
| GT exp
| LE exp
| GE exp
| NE exp
| EQ exp
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
    struct exe_mul_param *param,           // match %parse-param
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

int exe_mul_parse(const char *input, size_t len,
        struct exe_mul_param *param)
{
    yyscan_t arg = {0};
    exe_mul_yylex_init(&arg);
    // exe_mul_yyset_in(in, arg);
    int debug_flex = param ? param->debug_flex : 0;
    int debug_bison = param ? param->debug_bison: 0;
    exe_mul_yyset_debug(debug_flex, arg);
    yydebug = debug_bison;
    // exe_mul_yyset_extra(param, arg);
    exe_mul_yy_scan_bytes(input ? input : "", input ? len : 0, arg);
    int ret =exe_mul_yyparse(arg, param);
    exe_mul_yylex_destroy(arg);
    return ret ? 1 : 0;
}

