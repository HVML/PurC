%code top {
/*
 * @file logical.y
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
    // here to include header files required for generated logical.tab.c
}

%code requires {
    #include "helper.h"

    #define YYSTYPE       LOGICAL_YYSTYPE
    #define YYLTYPE       LOGICAL_YYLTYPE
    typedef void *yyscan_t;
}

%code provides {
}

%code {
    // generated header from flex
    // introduce yylex decl for later use
    #include "logical.lex.h"

    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct pcdvobjs_logical_param *param, // match %parse-param
        const char *errsg
    );
}

/* Bison declarations. */
%require "3.0.4"
%define api.prefix {logical_yy}
%define api.pure full
%define api.token.prefix {TOK_LOGICAL_}
%define locations
%define parse.error verbose
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct pcdvobjs_logical_param *param }

%union { double d; }

/* declare tokens */
%token <d> NUMBER
%token GT GE LT LE EQU NOEQU AND OR XOR ANTI
%token OP CP
%token EOL
%right '='
%left '+' '-'
%left '*'

%nterm <d> calclist exp factor anti term

%start calclist

%% /* The grammar follows. */

calclist:
  %empty      { }
| exp         { param->result = $1 ? 1 : 0; }
;

exp: factor
| exp AND factor { $$ = $1 && $3; }
| exp OR factor { $$ = $1 || $3; }
| exp XOR factor { $$ = (int)$1 ^ (int)$3; }
;

factor: anti
| factor GT anti { $$ = ($1 > $3)? 1 : 0; }
| factor GE anti { $$ = ($1 >= $3)? 1 : 0; }
| factor LT anti { $$ = ($1 < $3)? 1 : 0; }
| factor LE anti { $$ = ($1 <= $3)? 1 : 0; }
| factor EQU anti { $$ = ($1 == $3)? 1 : 0; }
| factor NOEQU anti { $$ = ($1 != $3)? 1 : 0; }
;

anti: term
| ANTI term { $$ = $2? 0: 1; }
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
    struct pcdvobjs_logical_param *param, // match %parse-param
    const char *errsg
)
{
    // to implement it here
    (void)yylloc;
    (void)arg;
    (void)param;
    fprintf(stderr, "%s\n", errsg);
}

int logical_parse(const char *input,
        struct pcdvobjs_logical_param *param)
{
    yyscan_t arg = {0};
    logical_yylex_init(&arg);
    // logical_yyset_in(in, arg);
    // logical_yyset_debug(debug, arg);
    logical_yyset_extra(param, arg);
    logical_yy_scan_string(input, arg);
    int ret =logical_yyparse(arg, param);
    logical_yylex_destroy(arg);
    return ret ? 1 : 0;
}

