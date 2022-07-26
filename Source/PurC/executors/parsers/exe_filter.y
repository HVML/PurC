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
}

%code provides {
}

%code {
    // generated header from flex
    // introduce yylex decl for later use
    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct exe_filter_param *param,           // match %parse-param
        const char *errsg
    );

    #define SET_RULE(_rule) do {                            \
        if (param) {                                        \
            param->rule = _rule;                            \
        } else {                                            \
            filter_rule_release(&_rule);                    \
        }                                                   \
    } while (0)
}

/* Bison declarations. */
%require "3.0.4"
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
%union { struct pcexe_strlist slist; }
%union { unsigned char matching_flags; } // cis
%union { unsigned char regexp_flags; }   // gimsuy
%union { struct matching_suffix msfx; }
%union { long int max_matching_length; }
%union { struct string_pattern_expression spexp; }
%union { struct literal_expression lexp; }
%union { struct string_literal_list *literals; }
%union { struct string_pattern_list *patterns; }
%union { struct string_matching_condition mexp; }
%union { struct number_comparing_logical_expression *ncle; }
%union { struct string_matching_logical_expression *smle; }
%union { struct filter_rule rule; }
%union { enum for_clause_type for_clause; }
%union { double nexp; }
%union { struct number_comparing_condition ncc; }

%destructor { pcexe_strlist_reset(&$$); } <slist>
%destructor { free($$); } <str>
%destructor { STR_PATTERN_RESET($$); } <spexp>
%destructor { string_literal_list_destroy($$); } <literals>
%destructor { string_pattern_list_destroy($$); } <patterns>
%destructor { string_matching_condition_reset(&$$); } <mexp>
%destructor { filter_rule_release(&$$); } <rule>
%destructor { number_comparing_logical_expression_destroy($$); } <ncle>
%destructor { string_matching_logical_expression_destroy($$); } <smle>

%token FILTER ALL LIKE KV KEY VALUE FOR AS
%token LT GT LE GE NE EQ NOT
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
%nterm <str>    wildcard_expression
%nterm <matching_flags>   matching_flags;
%nterm <regexp_flags>     regexp_flags regexp_suffix;
%nterm <msfx>             matching_suffix;
%nterm <max_matching_length> max_matching_length;
%nterm <spexp>  string_pattern_expression;
%nterm <lexp>   string_literal_expression;
%nterm <literals> string_literal_list;
%nterm <patterns> string_pattern_list;
%nterm <mexp> string_matching_condition;
%nterm <rule>  filter_rule;
%nterm <for_clause>  for_clause;
%nterm <nexp> exp
%nterm <ncc> number_comparing_condition
%nterm <ncle> number_comparing_logical_expression
%nterm <smle> string_matching_logical_expression


%% /* The grammar follows. */

input:
  filter_rule  { SET_RULE($1); }
;

filter_rule:
  FILTER ':' ALL for_clause { $$.ncle = NULL; $$.smle = NULL; $$.for_clause = $4; }
| FILTER ':' number_comparing_logical_expression for_clause { $$.ncle = $3; $$.smle = NULL; $$.for_clause = $4; }
| FILTER ':' string_matching_logical_expression for_clause { $$.ncle = NULL; $$.smle =  $3; $$.for_clause = $4; }
;

number_comparing_logical_expression:
  number_comparing_condition   { NCLE_INIT($$, $1); }
| number_comparing_logical_expression AND number_comparing_logical_expression { NCLE_AND($$, $1, $3); }
| number_comparing_logical_expression OR number_comparing_logical_expression  { NCLE_OR($$, $1, $3); }
| number_comparing_logical_expression XOR number_comparing_logical_expression { NCLE_XOR($$, $1, $3); }
| NOT number_comparing_logical_expression %prec NEG  { NCLE_NOT($$, $2); }
| '(' number_comparing_logical_expression ')'   { $$ = $2; }
;

string_matching_logical_expression:
  string_matching_condition   { SMLE_INIT($$, $1); }
| string_matching_logical_expression AND string_matching_logical_expression { SMLE_AND($$, $1, $3); }
| string_matching_logical_expression OR string_matching_logical_expression  { SMLE_OR($$, $1, $3); }
| string_matching_logical_expression XOR string_matching_logical_expression { SMLE_XOR($$, $1, $3); }
| NOT string_matching_logical_expression %prec NEG  { SMLE_NOT($$, $2); }
| '(' string_matching_logical_expression ')'   { $$ = $2; }
;

for_clause:
  %empty           { $$ = FOR_CLAUSE_VALUE; }
| FOR KV           { $$ = FOR_CLAUSE_KV; }
| FOR KEY          { $$ = FOR_CLAUSE_KEY; }
| FOR VALUE        { $$ = FOR_CLAUSE_VALUE; }
;

number_comparing_condition:
  LT exp           { $$.op_type = NUMBER_COMPARING_LT; $$.nexp = $2; }
| GT exp           { $$.op_type = NUMBER_COMPARING_GT; $$.nexp = $2; }
| LE exp           { $$.op_type = NUMBER_COMPARING_LE; $$.nexp = $2; }
| GE exp           { $$.op_type = NUMBER_COMPARING_GE; $$.nexp = $2; }
| NE exp           { $$.op_type = NUMBER_COMPARING_NE; $$.nexp = $2; }
| EQ exp           { $$.op_type = NUMBER_COMPARING_EQ; $$.nexp = $2; }
;

literal_char_sequence:
  STR  { STRLIST_INIT_STR($$, $1); }
| CHR  { STRLIST_INIT_CHR($$, $1); }
| UNI  { STRLIST_INIT_UNI($$, $1); }
| literal_char_sequence STR    { STRLIST_APPEND_STR($1, $2); $$ = $1; }
| literal_char_sequence CHR    { STRLIST_APPEND_CHR($1, $2); $$ = $1; }
| literal_char_sequence UNI    { STRLIST_APPEND_UNI($1, $2); $$ = $1; }
;

string_matching_condition:
  LIKE string_pattern_list  { $$.type = STRING_MATCHING_PATTERN; $$.patterns = $2; }
| AS string_literal_list    { $$.type = STRING_MATCHING_LITERAL; $$.literals = $2; }
;


string_literal_list:
  string_literal_expression { STR_LITERAL_LIST_INIT($$, $1); }
| string_literal_list ',' string_literal_expression { STR_LITERAL_LIST_APPEND($1, $3); $$ = $1; }
;

string_literal_expression:
  '"' literal_char_sequence '"' matching_suffix  { STR_LITERAL_SET($$, $2, $4); }
;

string_pattern_list:
  string_pattern_expression  { STR_PATTERN_LIST_INIT($$, $1); }
| string_pattern_list ',' string_pattern_expression { STR_PATTERN_LIST_APPEND($1, $3); $$ = $1; }
;


string_pattern_expression:
  '"' wildcard_expression '"' matching_suffix   { STR_PATTERN_SET_WILDCARD($$, $2, $4); }
| '/' regular_expression '/' regexp_suffix      { STR_PATTERN_SET_REGEXP($$, $2, $4); }
;


wildcard_expression:
  literal_char_sequence  { STRLIST_TO_STR($$, $1); }
;

regular_expression:
  STR  { STRLIST_INIT_STR($$, $1); }
| CHR  { STRLIST_INIT_CHR($$, $1); }
| regular_expression STR    { STRLIST_APPEND_STR($1, $2); $$ = $1; }
| regular_expression CHR    { STRLIST_APPEND_CHR($1, $2); $$ = $1; }
;


matching_suffix:
  %empty { $$.matching_flags = '\0'; $$.max_matching_length = 0; }
| matching_flags { $$.matching_flags = $1; $$.max_matching_length = 0; }
| matching_flags max_matching_length { $$.matching_flags = $1; $$.max_matching_length = $2; }
| max_matching_length { $$.matching_flags = '\0'; $$.max_matching_length = $1; }
;

regexp_suffix:
  %empty          { $$ = 0; }
| regexp_flags    { $$ = $1; }
;

regexp_flags:
  REGEXP_FLAG                  { $$ = 0; REGEXP_FLAGS_SET($$, $1); }
| regexp_flags REGEXP_FLAG     { REGEXP_FLAGS_SET($1, $2); $$ = $1; }
;

matching_flags:
  MATCHING_FLAG                { $$ = 0; MATCHING_FLAGS_SET($$, $1); }
| matching_flags MATCHING_FLAG { MATCHING_FLAGS_SET($1, $2); $$ = $1; }
;

max_matching_length:
  MATCHING_LENGTH    { STRTOL($$, $1); }
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
    int r = asprintf(&param->err_msg, "(%d,%d)->(%d,%d): %s",
        yylloc->first_line, yylloc->first_column,
        yylloc->last_line, yylloc->last_column - 1,
        errsg);
    (void)r;
}

int exe_filter_parse(const char *input, size_t len,
        struct exe_filter_param *param)
{
    yyscan_t arg = {0};
    yylex_init(&arg);
    // yyset_in(in, arg);
    int debug_flex = param ? param->debug_flex : 0;
    int debug_bison = param ? param->debug_bison: 0;
    yyset_debug(debug_flex, arg);
    yydebug = debug_bison;
    // yyset_extra(param, arg);
    yy_scan_bytes(input ? input : "", input ? len : 0, arg);
    int ret =yyparse(arg, param);
    yylex_destroy(arg);
    if (ret) {
        if (param->err_msg==NULL) {
            purc_set_error(PCEXECUTOR_ERROR_OOM);
        } else {
            purc_set_error(PCEXECUTOR_ERROR_BAD_SYNTAX);
        }
    } else {
        param->rule_valid = 1;
    }
    return ret ? -1 : 0;
}

