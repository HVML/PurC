%code top {
/*
 * @file exe_token.y
 * @author
 * @date
 * @brief The implementation of public part for token.
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
    // here to include header files required for generated exe_token.tab.c
    #define _GNU_SOURCE
    #include <stddef.h>
    #include <stdio.h>
    #include <string.h>

    #include "purc-errors.h"

    #include "pcexe-helper.h"
    #include "exe_token.h"
}

%code requires {
    // related struct/function decls
    // especially, for struct exe_token_param
    // and parse function for example:
    // int exe_token_parse(const char *input,
    //        struct exe_token_param *param);
    // #include "exe_token.h"
    // here we define them locally
    struct exe_token_token {
        const char      *text;
        size_t           leng;
    };

    #define YYSTYPE       EXE_TOKEN_YYSTYPE
    #define YYLTYPE       EXE_TOKEN_YYLTYPE
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
    #include "exe_token.lex.h"

    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct exe_token_param *param,           // match %parse-param
        const char *errsg
    );

    #define SET_PDOUBLE(_l, _r) do {                        \
        _l = (double*)malloc(sizeof(*_l));                  \
        if (!_l) {                                          \
            YYABORT;                                        \
        }                                                   \
        *_l = _r;                                           \
    } while (0);

    #define NUMERIC_EXP_INIT_I64(_nexp, _i64) do {               \
        int64_t i64;                                             \
        STRTOLL(i64, _i64);                                      \
        _nexp = i64;                                             \
    } while (0)

    #define NUMERIC_EXP_INIT_LD(_nexp, _ld) do {                 \
        long double ld;                                          \
        STRTOLD(ld, _ld);                                        \
        _nexp = ld;                                              \
    } while (0)

    #define NUMERIC_EXP_ADD(_nexp, _l, _r) do {                  \
        _nexp = _l + _r;                                         \
    } while (0)

    #define NUMERIC_EXP_SUB(_nexp, _l, _r) do {                  \
        _nexp = _l - _r;                                         \
    } while (0)

    #define NUMERIC_EXP_MUL(_nexp, _l, _r) do {                  \
        _nexp = _l * _r;                                         \
    } while (0)

    #define NUMERIC_EXP_DIV(_nexp, _l, _r) do {                  \
        _nexp = _l / _r;                                         \
    } while (0)

    #define NUMERIC_EXP_UMINUS(_nexp, _l) do {                   \
        _nexp = -_l;                                             \
    } while (0)

    #define STRTOL(_v, _s) do {                     \
        long int v;                                 \
        char *s = (char*)malloc(_s.leng+1);         \
        if (!s) {                                   \
            YYABORT;                                \
        }                                           \
        memcpy(s, _s.text, _s.leng);                \
        s[_s.leng] = '\0';                          \
        char *end;                                  \
        v = strtol(s, &end, 0);                     \
        if (end && *end) {                          \
            free(s);                                \
            YYABORT;                                \
        }                                           \
        free(s);                                    \
        _v = v;                                     \
    } while (0)

    #define STRTOD(_v, _s) do {                     \
        double v;                                   \
        char *s = (char*)malloc(_s.leng+1);         \
        if (!s) {                                   \
            YYABORT;                                \
        }                                           \
        memcpy(s, _s.text, _s.leng);                \
        s[_s.leng] = '\0';                          \
        char *end;                                  \
        v = strtod(s, &end);                        \
        if (end && *end) {                          \
            free(s);                                \
            YYABORT;                                \
        }                                           \
        free(s);                                    \
        _v = v;                                     \
    } while (0)

    #define STRTOLD(_v, _s) do {                    \
        long double v;                              \
        char *s = (char*)malloc(_s.leng+1);         \
        if (!s) {                                   \
            YYABORT;                                \
        }                                           \
        memcpy(s, _s.text, _s.leng);                \
        s[_s.leng] = '\0';                          \
        char *end;                                  \
        v = strtold(s, &end);                       \
        if (end && *end) {                          \
            free(s);                                \
            YYABORT;                                \
        }                                           \
        free(s);                                    \
        _v = v;                                     \
    } while (0)

    #define STRTOLL(_v, _s) do {                    \
        long long int v;                            \
        char *s = (char*)malloc(_s.leng+1);         \
        if (!s) {                                   \
            YYABORT;                                \
        }                                           \
        memcpy(s, _s.text, _s.leng);                \
        s[_s.leng] = '\0';                          \
        char *end;                                  \
        v = strtoll(s, &end, 0);                    \
        if (end && *end) {                          \
            free(s);                                \
            YYABORT;                                \
        }                                           \
        free(s);                                    \
        _v = v;                                     \
    } while (0)

    #define LOGICAL_EXP_INIT_STR(_logic, _mexp) do {          \
        _logic = logical_expression_create();                 \
        if (!_logic) {                                        \
            string_matching_expression_reset(&_mexp);         \
            YYABORT;                                          \
        }                                                     \
        _logic->type = LOGICAL_EXPRESSION_STR;                \
        _logic->mexp = _mexp;                                 \
    } while (0)

    #define LOGICAL_EXP_AND(_logic, _l, _r) do {                        \
        _logic = logical_expression_create();                           \
        if (!_logic) {                                                  \
            logical_expression_destroy(_l);                             \
            logical_expression_destroy(_r);                             \
            YYABORT;                                                    \
        }                                                               \
        _logic->type = LOGICAL_EXPRESSION_OP;                           \
        _logic->op = logical_and;                                       \
        bool ok = pctree_node_append_child(&_logic->node, &_l->node);   \
        if (ok) {                                                       \
            ok = pctree_node_append_child(&_logic->node, &_r->node);    \
            if (ok)                                                     \
                break;                                                  \
        } else {                                                        \
            logical_expression_destroy(_l);                             \
        }                                                               \
        logical_expression_destroy(_r);                                 \
        logical_expression_destroy(_logic);                             \
        YYABORT;                                                        \
    } while (0)

    #define LOGICAL_EXP_OR(_logic, _l, _r) do {                         \
        _logic = logical_expression_create();                           \
        if (!_logic) {                                                  \
            logical_expression_destroy(_l);                             \
            logical_expression_destroy(_r);                             \
            YYABORT;                                                    \
        }                                                               \
        _logic->type = LOGICAL_EXPRESSION_OP;                           \
        _logic->op = logical_or;                                        \
        bool ok = pctree_node_append_child(&_logic->node, &_l->node);   \
        if (ok) {                                                       \
            ok = pctree_node_append_child(&_logic->node, &_r->node);    \
            if (ok)                                                     \
                break;                                                  \
        } else {                                                        \
            logical_expression_destroy(_l);                             \
        }                                                               \
        logical_expression_destroy(_r);                                 \
        logical_expression_destroy(_logic);                             \
        YYABORT;                                                        \
    } while (0)

    #define LOGICAL_EXP_XOR(_logic, _l, _r) do {                        \
        _logic = logical_expression_create();                           \
        if (!_logic) {                                                  \
            logical_expression_destroy(_l);                             \
            logical_expression_destroy(_r);                             \
            YYABORT;                                                    \
        }                                                               \
        _logic->type = LOGICAL_EXPRESSION_OP;                           \
        _logic->op = logical_xor;                                       \
        bool ok = pctree_node_append_child(&_logic->node, &_l->node);   \
        if (ok) {                                                       \
            ok = pctree_node_append_child(&_logic->node, &_r->node);    \
            if (ok)                                                     \
                break;                                                  \
        } else {                                                        \
            logical_expression_destroy(_l);                             \
        }                                                               \
        logical_expression_destroy(_r);                                 \
        logical_expression_destroy(_logic);                             \
        YYABORT;                                                        \
    } while (0)

    #define LOGICAL_EXP_NOT(_logic, _l) do {                            \
        _logic = logical_expression_create();                           \
        if (!_logic) {                                                  \
            logical_expression_destroy(_l);                             \
            YYABORT;                                                    \
        }                                                               \
        _logic->type = LOGICAL_EXPRESSION_OP;                           \
        _logic->op = logical_not;                                       \
        bool ok = pctree_node_append_child(&_logic->node, &_l->node);   \
        if (ok) {                                                       \
            break;                                                      \
        }                                                               \
        logical_expression_destroy(_l);                                 \
        logical_expression_destroy(_logic);                             \
        YYABORT;                                                        \
    } while (0)

    #define STR_LITERAL_RESET(_l) do {              \
        literal_expression_reset(&_l);              \
    } while (0)

    #define STR_LITERAL_SET(_l, _slist, _sfx) do {     \
        _l.literal = pcexe_strlist_to_str(&_slist);    \
        pcexe_strlist_reset(&_slist);                  \
        _l.suffix = _sfx;                              \
        if (!_l.literal) {                             \
            YYABORT;                                   \
        }                                              \
    } while (0)

    #define STR_LITERAL_DUP(_dst, _src) do {                           \
        _dst = (struct literal_expression*)calloc(1, sizeof(*_dst));   \
        if (!_dst)                                                     \
            break;                                                     \
        memcpy(_dst, _src, sizeof(*_src));                             \
    } while (0)

    #define STR_LITERAL_LIST_INIT(_literals, _l) do {        \
        _literals = string_literal_list_create();            \
        if (!_literals) {                                    \
            literal_expression_reset(&_l);                   \
            YYABORT;                                         \
        }                                                    \
        struct literal_expression *lexp;                     \
        STR_LITERAL_DUP(lexp, &_l);                          \
        if (!lexp) {                                         \
            literal_expression_reset(&_l);                   \
            string_literal_list_destroy(_literals);          \
            YYABORT;                                         \
        }                                                    \
        list_add(&lexp->node, &_literals->list);             \
    } while (0)

    #define STR_LITERAL_LIST_APPEND(_literals, _l) do {      \
        struct literal_expression *lexp;                     \
        STR_LITERAL_DUP(lexp, &_l);                          \
        if (!lexp) {                                         \
            literal_expression_reset(&_l);                   \
            string_literal_list_destroy(_literals);          \
            YYABORT;                                         \
        }                                                    \
        list_add(&lexp->node, &_literals->list);             \
    } while (0)

    #define STR_PATTERN_DUP(_dst, _src) do {                                  \
        _dst = (struct string_pattern_expression*)calloc(1, sizeof(*_dst));   \
        if (!_dst)                                                            \
            break;                                                            \
        memcpy(_dst, _src, sizeof(*_src));                                    \
    } while (0)

    #define STR_PATTERN_LIST_INIT(_patterns, _l) do {        \
        _patterns = string_pattern_list_create();            \
        if (!_patterns) {                                    \
            string_pattern_expression_reset(&_l);            \
            YYABORT;                                         \
        }                                                    \
        struct string_pattern_expression *lexp;              \
        STR_PATTERN_DUP(lexp, &_l);                          \
        if (!lexp) {                                         \
            string_pattern_expression_reset(&_l);            \
            string_pattern_list_destroy(_patterns);          \
            YYABORT;                                         \
        }                                                    \
        list_add(&lexp->node, &_patterns->list);             \
    } while (0)

    #define STR_PATTERN_LIST_APPEND(_patterns, _l) do {      \
        struct string_pattern_expression *lexp;              \
        STR_PATTERN_DUP(lexp, &_l);                          \
        if (!lexp) {                                         \
            string_pattern_expression_reset(&_l);            \
            string_pattern_list_destroy(_patterns);          \
            YYABORT;                                         \
        }                                                    \
        list_add(&lexp->node, &_patterns->list);             \
    } while (0)

    #define STRLIST_INIT_STR(_list, _s) do {                          \
        pcexe_strlist_init(&_list);                                   \
        if (pcexe_strlist_append_buf(&_list, _s.text, _s.leng)) {     \
            pcexe_strlist_reset(&_list);                              \
            YYABORT;                                                  \
        }                                                             \
    } while (0)

    #define STRLIST_INIT_CHR(_list, _c) do {                          \
        pcexe_strlist_init(&_list);                                   \
        if (pcexe_strlist_append_chr(&_list, _c)) {                   \
            pcexe_strlist_reset(&_list);                              \
            YYABORT;                                                  \
        }                                                             \
    } while (0)

    #define STRLIST_INIT_UNI(_list, _u) do {                          \
        pcexe_strlist_init(&_list);                                   \
        if (pcexe_strlist_append_uni(&_list, _u.text, _u.leng)) {     \
            pcexe_strlist_reset(&_list);                              \
            YYABORT;                                                  \
        }                                                             \
    } while (0)

    #define STRLIST_APPEND_STR(_list, _s) do {                        \
        if (pcexe_strlist_append_buf(&_list, _s.text, _s.leng)) {     \
            pcexe_strlist_reset(&_list);                              \
            YYABORT;                                                  \
        }                                                             \
    } while (0)

    #define STRLIST_APPEND_CHR(_list, _c) do {                        \
        if (pcexe_strlist_append_chr(&_list, _c)) {                   \
            pcexe_strlist_reset(&_list);                              \
            YYABORT;                                                  \
        }                                                             \
    } while (0)

    #define STRLIST_APPEND_UNI(_list, _u) do {                        \
        if (pcexe_strlist_append_uni(&_list, _u.text, _u.leng)) {     \
            pcexe_strlist_reset(&_list);                              \
            YYABORT;                                                  \
        }                                                             \
    } while (0)

    #define STRLIST_TO_STR(_str, _list) do {                          \
        _str = pcexe_strlist_to_str(&_list);                          \
        pcexe_strlist_reset(&_list);                                  \
        if (!_str) {                                                  \
            YYABORT;                                                  \
        }                                                             \
    } while (0)

    #define STR_PATTERN_RESET(_sp) do {             \
        string_pattern_expression_reset(&_sp);      \
    } while (0)

    #define STR_PATTERN_SET_WILDCARD(_sp, _s, _sfx) do {     \
        memset(&_sp, 0, sizeof(_sp));                        \
        _sp.type = STRING_PATTERN_WILDCARD;                  \
        _sp.wildcard.wildcard = _s;                          \
        _sp.wildcard.suffix = _sfx;                          \
    } while (0)

    #define STR_PATTERN_SET_REGEXP(_sp, _slist, _flags) do { \
        memset(&_sp, 0, sizeof(_sp));                        \
        _sp.type = STRING_PATTERN_REGEXP;                    \
        _sp.regexp.regexp = pcexe_strlist_to_str(&_slist);   \
        pcexe_strlist_reset(&_slist);                        \
        _sp.regexp.flags = _flags;                           \
        if (!_sp.regexp.regexp) {                            \
            YYABORT;                                         \
        }                                                    \
    } while (0)

    #define SET_RULE(_rule) do {                            \
        if (param) {                                        \
            param->rule = _rule;                            \
        } else {                                            \
            token_rule_release(&_rule);                      \
        }                                                   \
    } while (0)
}

/* Bison declarations. */
%require "3.0.4"
%define api.prefix {exe_token_yy}
%define api.pure full
%define api.token.prefix {TOK_EXE_TOKEN_}
%define locations
%define parse.error verbose
%define parse.lac full
%define parse.trace true
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct exe_token_param *param }

// union members
%union { struct exe_token_token token; }
%union { char *str; }
%union { char c; }
%union { struct token_rule rule; }
%union { double *to; }
%union { double *advance; }
%union { double nexp; }
%union { struct logical_expression *logic; }
%union { struct string_matching_expression mexp; }
%union { struct pcexe_strlist slist; }
%union { struct string_pattern_list *patterns; }
%union { struct string_literal_list *literals; }
%union { struct literal_expression lexp; }
%union { struct string_pattern_expression spexp; }
%union { unsigned char matching_flags; } // cis
%union { unsigned char regexp_flags; }   // gimsuy
%union { struct matching_suffix msfx; }
%union { long int max_matching_length; }

%destructor { token_rule_release(&$$); } <rule>
%destructor { free($$); } <to>
%destructor { free($$); } <advance>
%destructor { logical_expression_destroy($$); } <logic>
%destructor { string_matching_expression_reset(&$$); } <mexp>
%destructor { pcexe_strlist_reset(&$$); } <slist>
%destructor { string_pattern_list_destroy($$); } <patterns>
%destructor { string_literal_list_destroy($$); } <literals>
%destructor { STR_PATTERN_RESET($$); } <spexp>

%token TOKEN FROM TO ADVANCE DELIMETERS UNTIL LIKE AS
%token NOT
%token <c>     CHR
%token <token> MATCHING_LENGTH
%token <token> STR UNI
%token <c>     MATCHING_FLAG REGEXP_FLAG
%token <token> INTEGER NUMBER

%left '-' '+'
%left '*' '/'
%precedence UMINUS

%left AND OR XOR
%precedence NEG


%nterm <rule>  token_rule;
%nterm <to>          to_clause
%nterm <advance>     advance_clause
%nterm <logic>       until_clause
%nterm <nexp>        exp
%nterm <logic> logical_expression;
%nterm <mexp> string_matching_expression;
%nterm <slist>  literal_char_sequence
%nterm <patterns> string_pattern_list;
%nterm <literals> string_literal_list;
%nterm <lexp>   string_literal_expression;
%nterm <msfx>             matching_suffix;
%nterm <spexp>  string_pattern_expression;
%nterm <str>    wildcard_expression
%nterm <slist>  regular_expression
%nterm <regexp_flags>     regexp_flags regexp_suffix;
%nterm <matching_flags>   matching_flags;
%nterm <max_matching_length> max_matching_length;
%nterm <str>    delimiters_clause;


%% /* The grammar follows. */

input:
  token_rule
;

token_rule:
  TOKEN ':' FROM exp to_clause advance_clause delimiters_clause until_clause     { $$.from = $4; $$.to = $5; $$.advance = $6; $$.delimiters = $7; $$.until = $8; }
;

to_clause:
  %empty            { $$ = NULL; }
| TO exp            { SET_PDOUBLE($$, $2); }
;

advance_clause:
  %empty            { $$ = NULL; }
| ADVANCE exp       { SET_PDOUBLE($$, $2); }
;

delimiters_clause:
  %empty                                         { $$ = NULL; }
| DELIMETERS '"' literal_char_sequence '"'       { STRLIST_TO_STR($$, $3); }
;

until_clause:
  %empty                        { $$ = NULL; }
| UNTIL logical_expression      { $$ = $2; }
;

logical_expression:
  string_matching_expression   { LOGICAL_EXP_INIT_STR($$, $1); }
| logical_expression AND logical_expression { LOGICAL_EXP_AND($$, $1, $3); }
| logical_expression OR logical_expression  { LOGICAL_EXP_OR($$, $1, $3); }
| logical_expression XOR logical_expression { LOGICAL_EXP_XOR($$, $1, $3); }
| NOT logical_expression %prec NEG  { LOGICAL_EXP_NOT($$, $2); }
| '(' logical_expression ')'   { $$ = $2; }
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
    struct exe_token_param *param,           // match %parse-param
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

int exe_token_parse(const char *input, size_t len,
        struct exe_token_param *param)
{
    yyscan_t arg = {0};
    exe_token_yylex_init(&arg);
    // exe_token_yyset_in(in, arg);
    int debug_flex = param ? param->debug_flex : 0;
    int debug_bison = param ? param->debug_bison: 0;
    exe_token_yyset_debug(debug_flex, arg);
    yydebug = debug_bison;
    // exe_token_yyset_extra(param, arg);
    exe_token_yy_scan_bytes(input ? input : "", input ? len : 0, arg);
    int ret =exe_token_yyparse(arg, param);
    exe_token_yylex_destroy(arg);
    return ret ? -1 : 0;
}

