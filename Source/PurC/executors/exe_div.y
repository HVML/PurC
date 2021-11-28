%code top {
/*
 * @file exe_div.y
 * @author
 * @date
 * @brief The implementation of public part for div.
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
    // here to include header files required for generated exe_div.tab.c
    #define _GNU_SOURCE
    #include <stddef.h>
    #include <stdio.h>
    #include <string.h>

    #include "purc-errors.h"

    #include "pcexe-helper.h"
    #include "exe_div.h"
}

%code requires {
    // related struct/function decls
    // especially, for struct exe_div_param
    // and parse function for example:
    // int exe_div_parse(const char *input,
    //        struct exe_div_param *param);
    // #include "exe_div.h"
    // here we define them locally
    struct exe_div_token {
        const char      *text;
        size_t           leng;
    };

    #define YYSTYPE       EXE_DIV_YYSTYPE
    #define YYLTYPE       EXE_DIV_YYLTYPE
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
    #include "exe_div.lex.h"
    #include "tab.h"

    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct exe_div_param *param,           // match %parse-param
        const char *errsg
    );

    #define SET_RULE(_rule) do {                            \
        if (param) {                                        \
            param->rule = _rule;                            \
        } else {                                            \
            div_rule_release(&_rule);                       \
        }                                                   \
    } while (0)
}

/* Bison declarations. */
%require "3.0.4"
%define api.prefix {exe_div_yy}
%define api.pure full
%define api.token.prefix {TOK_EXE_DIV_}
%define locations
%define parse.error verbose
%define parse.lac full
%define parse.trace true
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct exe_div_param *param }

// union members
%union { struct exe_div_token token; }
%union { char *str; }
%union { char c; }
%union { double nexp; }
%union { struct number_comparing_condition ncc; }
%union { struct number_comparing_logical_expression *ncle; }
%union { struct div_rule rule; }

%destructor { number_comparing_logical_expression_destroy($$); } <ncle>
%destructor { div_rule_release(&$$); } <rule>

%token DIV BY
%token LT GT LE GE NE EQ NOT
%token <token> INTEGER NUMBER

%left '-' '+'
%left '*' '/'
%precedence UMINUS

%left AND OR XOR
%precedence NEG


%nterm <nexp>  exp
%nterm <ncc>   number_comparing_condition
%nterm <ncle>  number_comparing_logical_expression
%nterm <rule>  div_rule;



 /* %nterm <str>   args */ // non-terminal `input` use `str` to store
                           // token value as well

%% /* The grammar follows. */

input:
  div_rule  { SET_RULE($1); }
;

div_rule:
  DIV ':' number_comparing_logical_expression BY exp     { $$.ncle = $3; $$.nexp = $5; }
;

number_comparing_logical_expression:
  number_comparing_condition   { NCLE_INIT($$, $1); }
| number_comparing_logical_expression AND number_comparing_logical_expression { NCLE_AND($$, $1, $3); }
| number_comparing_logical_expression OR number_comparing_logical_expression  { NCLE_OR($$, $1, $3); }
| number_comparing_logical_expression XOR number_comparing_logical_expression { NCLE_XOR($$, $1, $3); }
| NOT number_comparing_logical_expression %prec NEG  { NCLE_NOT($$, $2); }
| '(' number_comparing_logical_expression ')'   { $$ = $2; }
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

%%

/* Called by yyparse on error. */
static void
yyerror(
    YYLTYPE *yylloc,                   // match %define locations
    yyscan_t arg,                      // match %param
    struct exe_div_param *param,           // match %parse-param
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

int exe_div_parse(const char *input, size_t len,
        struct exe_div_param *param)
{
    yyscan_t arg = {0};
    exe_div_yylex_init(&arg);
    // exe_div_yyset_in(in, arg);
    int debug_flex = param ? param->debug_flex : 0;
    int debug_bison = param ? param->debug_bison: 0;
    exe_div_yyset_debug(debug_flex, arg);
    yydebug = debug_bison;
    // exe_div_yyset_extra(param, arg);
    exe_div_yy_scan_bytes(input ? input : "", input ? len : 0, arg);
    int ret =exe_div_yyparse(arg, param);
    exe_div_yylex_destroy(arg);
    return ret ? -1 : 0;
}

