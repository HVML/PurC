%code top {
/*
 * @file math.y
 * @author
 * @date
 * @brief The implementation of public part for vdom.
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
    // here to include header files required for generated math.tab.c
}

%code requires {
    #include "tools.h"

    #define YYSTYPE       MATH_YYSTYPE
    #define YYLTYPE       MATH_YYLTYPE
    typedef void *yyscan_t;
}

%code provides {
}

%code {
    // generated header from flex
    // introduce yylex decl for later use
    #include "math.lex.h"

    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct pcdvobjs_math_param *param, // match %parse-param
        const char *errsg
    );
}

/* Bison declarations. */
%require "3.0.4"
%define api.prefix {math_yy}
%define api.pure full
%define api.token.prefix {TOK_MATH_}
%define locations
%define parse.error verbose
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct pcdvobjs_math_param *param }

%union { double d; }

%token <d> NUMBER
%token ADD SUB MUL DIV
%token OP CP
%right '='
%left '+' '-'
%left '*'

%nterm <d> calclist exp factor term

%start calclist

%% /* The grammar follows. */

calclist:
  %empty           { }
| exp              { param->result = $1; }
;

exp: factor
| exp ADD exp { $$ = $1 + $3; }
| exp SUB factor { $$ = $1 - $3; }
;

factor: term
| factor MUL term { $$ = $1 * $3;}
| factor DIV term { if ($3 == 0.0) YYABORT; $$ = $1 / $3; }
;

term: NUMBER
| OP exp CP { $$ = $2; }
;
%%

/* Called by yyparse on error. */
static void
yyerror(
    YYLTYPE *yylloc,                   // match %define locations
    yyscan_t arg,                      // match %param
    struct pcdvobjs_math_param *param, // match %parse-param
    const char *errsg
)
{
    // to implement it here
    (void)yylloc;
    (void)arg;
    (void)param;
    fprintf(stderr, "%s\n", errsg);
}

int math_parse(const char *input,
        struct pcdvobjs_math_param *param)
{
    yyscan_t arg = {0};
    math_yylex_init(&arg);
    // math_yyset_in(in, arg);
    // math_yyset_debug(debug, arg);
    math_yyset_extra(param, arg);
    math_yy_scan_string(input, arg);
    int ret =math_yyparse(arg, param);
    math_yylex_destroy(arg);
    return ret ? 1 : 0;
}
