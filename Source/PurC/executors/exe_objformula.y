%code top {
/*
 * @file exe_objformula.y
 * @author
 * @date
 * @brief The implementation of public part for objformula.
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
    // here to include header files required for generated exe_objformula.tab.c
    #define _GNU_SOURCE
    #include <stddef.h>
    #include <stdio.h>
    #include <string.h>

    #include "purc-errors.h"

    #include "pcexe-helper.h"
    #include "exe_objformula.h"
}

%code requires {
    // related struct/function decls
    // especially, for struct exe_objformula_param
    // and parse function for example:
    // int exe_objformula_parse(const char *input,
    //        struct exe_objformula_param *param);
    // #include "exe_objformula.h"
    // here we define them locally
    struct exe_objformula_token {
        const char      *text;
        size_t           leng;
    };

    #define YYSTYPE       EXE_OBJFORMULA_YYSTYPE
    #define YYLTYPE       EXE_OBJFORMULA_YYLTYPE
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
    #include "exe_objformula.lex.h"
    #include "tab.h"

    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct exe_objformula_param *param,           // match %parse-param
        const char *errsg
    );

    #define SET_RULE(_rule) do {                            \
        if (param) {                                        \
            objformula_rule_release(&_rule);                \
            break; \
            param->rule = _rule;                            \
        } else {                                            \
            objformula_rule_release(&_rule);                \
        }                                                   \
    } while (0)
}

/* Bison declarations. */
%require "3.0.4"
%define api.prefix {exe_objformula_yy}
%define api.pure full
%define api.token.prefix {TOK_EXE_OBJFORMULA_}
%define locations
%define parse.error verbose
%define parse.lac full
%define parse.trace true
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct exe_objformula_param *param }

// union members
%union { struct exe_objformula_token token; }
%union { char *str; }
%union { char c; }
%union { struct iterative_formula_expression *ife; }
%union { struct iterative_assignment_list *ial; }
%union { struct iterative_assignment_expression *iae; }
%union { struct number_comparing_condition ncc; }
%union { struct value_number_comparing_condition vncc; }
%union { struct value_number_comparing_logical_expression *vncle; }
%union { struct objformula_rule rule; }
%union { double nexp; }

%destructor { iterative_formula_expression_destroy($$); } <ife>
%destructor { ial_destroy($$); } <ial>
%destructor { iae_destroy($$); } <iae>
%destructor { vncc_release(&$$); } <vncc>
%destructor { vncle_destroy($$); } <vncle>
%destructor { objformula_rule_release(&$$); } <rule>

%left AND OR XOR

%token OBJFORMULA BY
%token LT GT LE GE NE EQ NOT
%token <token> INTEGER NUMBER ID

%right '='
%left ','
%left '-' '+'
%left '*' '/'
%precedence UMINUS

%precedence NEG


%nterm <rule>  objformula_rule;
%nterm <vncle> value_number_comparing_logical_expression;
%nterm <ife>   iterative_formula_expression;
%nterm <ial>   iterative_assignment_list;
%nterm <iae>   iterative_assignment_expression;
%nterm <ncc>   number_comparing_condition;
%nterm <vncc>  value_number_comparing_condition;
%nterm <nexp>  exp;

%% /* The grammar follows. */

input:
  objformula_rule      { SET_RULE($1); }
;

objformula_rule:
  OBJFORMULA ':' value_number_comparing_logical_expression BY iterative_assignment_list     { $$.vncle = $3; $$.ial = $5; }
;

value_number_comparing_logical_expression:
  value_number_comparing_condition   { VNCLE_INIT($$, $1); }
| value_number_comparing_logical_expression AND value_number_comparing_logical_expression { VNCLE_AND($$, $1, $3); }
| value_number_comparing_logical_expression OR value_number_comparing_logical_expression  { VNCLE_OR($$, $1, $3); }
| value_number_comparing_logical_expression XOR value_number_comparing_logical_expression { VNCLE_XOR($$, $1, $3); }
| NOT value_number_comparing_logical_expression %prec NEG  { VNCLE_NOT($$, $2); }
| '(' value_number_comparing_logical_expression ')'   { $$ = $2; }
;

value_number_comparing_condition:
  ID number_comparing_condition      { VNCC_INIT($$, $1, $2); }
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

iterative_formula_expression:
  INTEGER           { IFE_INIT_INTEGER($$, $1); }
| NUMBER            { IFE_INIT_NUMBER($$, $1); }
| ID                { IFE_INIT_ID($$, $1); }
| iterative_formula_expression '+' iterative_formula_expression     { IFE_ADD($$, $1, $3); }
| iterative_formula_expression '-' iterative_formula_expression     { IFE_SUB($$, $1, $3); }
| iterative_formula_expression '*' iterative_formula_expression     { IFE_MUL($$, $1, $3); }
| iterative_formula_expression '/' iterative_formula_expression     { IFE_DIV($$, $1, $3); }
| '-' iterative_formula_expression %prec UMINUS    { IFE_NEG($$, $2); }
| '(' iterative_formula_expression ')'             { $$ = $2; }
;

iterative_assignment_list:
  iterative_assignment_expression            { IAL_INIT($$, $1); }
| iterative_assignment_list ',' iterative_assignment_expression    { IAL_APPEND($$, $1, $3); }
;

iterative_assignment_expression:
  ID '=' iterative_formula_expression        { IAE_INIT($$, $1, $3); }
;

%%

/* Called by yyparse on error. */
static void
yyerror(
    YYLTYPE *yylloc,                   // match %define locations
    yyscan_t arg,                      // match %param
    struct exe_objformula_param *param,           // match %parse-param
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

int exe_objformula_parse(const char *input, size_t len,
        struct exe_objformula_param *param)
{
    yyscan_t arg = {0};
    exe_objformula_yylex_init(&arg);
    // exe_objformula_yyset_in(in, arg);
    int debug_flex = param ? param->debug_flex : 0;
    int debug_bison = param ? param->debug_bison: 0;
    exe_objformula_yyset_debug(debug_flex, arg);
    yydebug = debug_bison;
    // exe_objformula_yyset_extra(param, arg);
    exe_objformula_yy_scan_bytes(input ? input : "", input ? len : 0, arg);
    int ret =exe_objformula_yyparse(arg, param);
    exe_objformula_yylex_destroy(arg);
    return ret ? -1 : 0;
}

