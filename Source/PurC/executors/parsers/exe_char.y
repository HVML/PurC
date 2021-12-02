%code top {
/*
 * @file exe_char.y
 * @author
 * @date
 * @brief The implementation of public part for char.
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
    // here to include header files required for generated exe_char.tab.c
}

%code requires {
    struct exe_char_token {
        const char      *text;
        size_t           leng;
    };

    #define YYSTYPE       EXE_CHAR_YYSTYPE
    #define YYLTYPE       EXE_CHAR_YYLTYPE
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
    #include <math.h>

    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct exe_char_param *param,           // match %parse-param
        const char *errsg
    );

    #define SET_CHAR_RULE(_rule, _from, _to, _advance, _until) do {          \
        _rule.from    = _from;                                               \
        _rule.to      = _to;                                                 \
        _rule.advance = _advance;                                            \
        _rule.until   = NULL;                                                \
        if (_until) {                                                        \
            size_t bytes, chars;                                             \
            _rule.until   = pcexe_wchar_from_utf8(_until, &bytes, &chars);   \
            free(_until);                                                    \
            if (!_rule.until)                                                \
                YYABORT;                                                     \
        }                                                                    \
    } while (0)

    #define SET_RULE(_rule) do {                            \
        if (param) {                                        \
            param->rule = _rule;                            \
        } else {                                            \
            char_rule_release(&_rule);                      \
        }                                                   \
    } while (0)
}

/* Bison declarations. */
%require "3.0.4"
%define api.pure full
%define api.token.prefix {TOK_EXE_CHAR_}
%define locations
%define parse.error verbose
%define parse.lac full
%define parse.trace true
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct exe_char_param *param }

// union members
%union { struct exe_char_token token; }
%union { char *str; }
%union { char c; }
%union { double nexp; }
%union { double to; }
%union { double advance; }
%union { char   *until; }
%union { struct char_rule rule; }
%union { struct pcexe_strlist slist; }

%destructor { free($$); } <str>
%destructor { free($$); } <until>
%destructor { char_rule_release(&$$); } <rule>
%destructor { pcexe_strlist_reset(&$$); } <slist>

%token CHAR FROM TO ADVANCE UNTIL
%token <c>     CHR
%token <token> STR UNI
%token <token> INTEGER NUMBER

%left '-' '+'
%left '*' '/'
%precedence UMINUS


%nterm <nexp>        exp
%nterm <str>         literal_str
%nterm <to>          to_clause
%nterm <advance>     advance_clause
%nterm <until>       until_clause
%nterm <rule>        char_rule
%nterm <slist>       literal_char_sequence

%% /* The grammar follows. */

input:
  char_rule          { SET_RULE($1); }
;

char_rule:
  CHAR ':' FROM exp to_clause advance_clause until_clause    { SET_CHAR_RULE($$, $4, $5, $6, $7); }
;

to_clause:
  %empty                 { $$ = NAN; }
| TO exp                 { $$ = $2; }
;

advance_clause:
  %empty                 { $$ = NAN; }
| ADVANCE exp            { $$ = $2; }
;

until_clause:
  %empty                 { $$ = NULL; }
| UNTIL literal_str      { $$ = $2; }
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

literal_str:
  '"' literal_char_sequence '"'       { STRLIST_TO_STR($$, $2); }
;

literal_char_sequence:
  STR  { STRLIST_INIT_STR($$, $1); }
| CHR  { STRLIST_INIT_CHR($$, $1); }
| UNI  { STRLIST_INIT_UNI($$, $1); }
| literal_char_sequence STR    { STRLIST_APPEND_STR($1, $2); $$ = $1; }
| literal_char_sequence CHR    { STRLIST_APPEND_CHR($1, $2); $$ = $1; }
| literal_char_sequence UNI    { STRLIST_APPEND_UNI($1, $2); $$ = $1; }
;

%%

/* Called by yyparse on error. */
static void
yyerror(
    YYLTYPE *yylloc,                   // match %define locations
    yyscan_t arg,                      // match %param
    struct exe_char_param *param,           // match %parse-param
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

int exe_char_parse(const char *input, size_t len,
        struct exe_char_param *param)
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
    return ret ? -1 : 0;
}

