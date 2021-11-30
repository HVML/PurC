%code top {
/*
 * @file exe_sql.y
 * @author
 * @date
 * @brief The implementation of public part for sql.
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
    // here to include header files required for generated exe_sql.tab.c
}

%code requires {
    struct exe_sql_param {
        char *err_msg;
        int debug_flex;
        int debug_bison;
    };

    struct exe_sql_token {
        const char      *text;
        size_t           leng;
    };

    #define YYSTYPE       EXE_SQL_YYSTYPE
    #define YYLTYPE       EXE_SQL_YYLTYPE
    #ifndef YY_TYPEDEF_YY_SCANNER_T
    #define YY_TYPEDEF_YY_SCANNER_T
    typedef void* yyscan_t;
    #endif

    int exe_sql_parse(const char *input, size_t len,
            struct exe_sql_param *param);
}

%code provides {
}

%code {
    // generated header from flex
    // introduce yylex decl for later use
    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct exe_sql_param *param,           // match %parse-param
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
%define api.prefix {exe_sql_yy}
%define api.pure full
%define api.token.prefix {TOK_EXE_SQL_}
%define locations
%define parse.error verbose
%define parse.lac full
%define parse.trace true
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct exe_sql_param *param }

// union members
%union { struct exe_sql_token token; }
%union { char *str; }
%union { char c; }

    /* %destructor { free($$); } <str> */ // destructor for `str`

%token SQL SELECT WHERE GROUP BY ORDER TRAVEL IN LIKE UNION AS ASC DESC
%token SIBLINGS DEPTH BREADTH LEAVES
%token NOT GE LE NE AT
%token <c> CHR UNI
%token <token> STR INTERIOR
%token <token> INTEGER NUMBER ID

%left UNION
%left IN LIKE
%left AND OR
%right '=' '<' '>' GE LE NE
%left '-' '+'
%left '*' '/'
%precedence UMINUS
%precedence NEG

 /* %nterm <str>   args */ // non-terminal `input` use `str` to store
                           // token value as well

%% /* The grammar follows. */

input:
  rule
;

rule:
  sql_rule
;

sql_rule:
  SQL ':' union_clause
;

select_clause:
  SELECT select_list where_clause group_by_clause order_by_clause travel_in_clause
;

union_clause:
  select_clause
| '(' union_clause ')'
| union_clause UNION union_clause
;

select_list:
  select_item
| select_list ',' select_item
;

select_item:
  exp
| exp AS ID
;

var:
  ID
| ID '.' ID
;

var_list:
  var
| var_list ',' var
;

where_clause:
  %empty
| WHERE exp
;

group_by_clause:
  %empty
| GROUP BY var_list
;

order_by_clause:
  %empty
| ORDER BY var_list
| ORDER BY var_list ASC
| ORDER BY var_list DESC
;

travel_in_clause:
  %empty
| TRAVEL IN SIBLINGS
| TRAVEL IN DEPTH
| TRAVEL IN BREADTH
| TRAVEL IN LEAVES
;

exp:
  INTEGER
| NUMBER
| var
| '*'
| '&'
| '"' str '"'
| AT ID
| exp LIKE exp
| exp IN '(' exp_list ')'
| exp AND exp
| exp OR exp
| NOT exp %prec NEG
| exp '=' exp
| exp NE exp
| exp LE exp
| exp GE exp
| exp '>' exp
| exp '<' exp
| exp '+' exp
| exp '-' exp
| exp '*' exp
| exp '/' exp
| '-' exp %prec UMINUS
| '(' exp ')'
;

exp_list:
  exp
| exp ',' exp
;

str:
  STR
| CHR
| UNI
| str STR
| str CHR
| str UNI
;

%%

/* Called by yyparse on error. */
static void
yyerror(
    YYLTYPE *yylloc,                   // match %define locations
    yyscan_t arg,                      // match %param
    struct exe_sql_param *param,           // match %parse-param
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

int exe_sql_parse(const char *input, size_t len,
        struct exe_sql_param *param)
{
    yyscan_t arg = {0};
    exe_sql_yylex_init(&arg);
    // exe_sql_yyset_in(in, arg);
    int debug_flex = param ? param->debug_flex : 0;
    int debug_bison = param ? param->debug_bison: 0;
    exe_sql_yyset_debug(debug_flex, arg);
    yydebug = debug_bison;
    // exe_sql_yyset_extra(param, arg);
    exe_sql_yy_scan_bytes(input ? input : "", input ? len : 0, arg);
    int ret =exe_sql_yyparse(arg, param);
    exe_sql_yylex_destroy(arg);
    return ret ? -1 : 0;
}

