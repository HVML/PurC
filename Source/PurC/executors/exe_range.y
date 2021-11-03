%code top {
/*
 * @file exe_range.y
 * @author
 * @date
 * @brief The implementation of public part for range.
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
    // here to include header files required for generated exe_range.tab.c
    #define _GNU_SOURCE
    #include <stddef.h>
    #include <stdio.h>
    #include <string.h>

    #include "purc-errors.h"

    #include "pcexe-helper.h"
    #include "exe_range.h"
}

%code requires {
    // related struct/function decls
    // especially, for struct exe_range_param
    // and parse function for example:
    // int exe_range_parse(const char *input,
    //        struct exe_range_param *param);
    // #include "exe_range.h"
    // here we define them locally
    struct exe_range_token {
        const char      *text;
        size_t           leng;
    };

    #define YYSTYPE       EXE_RANGE_YYSTYPE
    #define YYLTYPE       EXE_RANGE_YYLTYPE
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
    #include "exe_range.lex.h"

    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct exe_range_param *param,           // match %parse-param
        const char *errsg
    );

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

    #define RULE_INIT_FROM(_rule, _from) do {        \
        _rule.from    = _from;                       \
        _rule.has_to = 0;                            \
        _rule.has_advance = 0;                       \
    } while (0)

    #define RULE_INIT_FROM_TO(_rule, _from, _to) do {        \
        _rule.from    = _from;                               \
        _rule.to      = _to;                                 \
        _rule.has_to = 1;                                    \
        _rule.has_advance = 0;                               \
    } while (0)

    #define RULE_INIT_FROM_TO_ADVANCE(_rule, _from, _to, _advance) do {  \
        _rule.from    = _from;                                           \
        _rule.to      = _to;                                             \
        _rule.advance = _advance;                                        \
        _rule.has_to = 1;                                                \
        _rule.has_advance = 1;                                           \
    } while (0)

    #define RULE_INIT_FROM_ADVANCE(_rule, _from, _advance) do {        \
        _rule.from    = _from;                                         \
        _rule.advance = _advance;                                      \
        _rule.has_to = 0;                                              \
        _rule.has_advance = 1;                                         \
    } while (0)

    #define NUMERIC_EXP_INIT_I64(_nexp, _i64) do {               \
        _nexp.type = NUMERIC_EXPRESSION_INTEGER;                 \
        STRTOLL(_nexp.i64, _i64);                                \
    } while (0)

    #define NUMERIC_EXP_INIT_LD(_nexp, _ld) do {                 \
        _nexp.type = NUMERIC_EXPRESSION_NUMERIC;                 \
        STRTOLD(_nexp.ld, _ld);                                  \
    } while (0)

    #define NUMERIC_EXP_VAL(_n)                                  \
        (_n.type == NUMERIC_EXPRESSION_NUMERIC ? _n.ld : _n.i64)                                                    \

    #define NUMERIC_EXP_ADD(_nexp, _l, _r) do {                  \
        if (_l.type == NUMERIC_EXPRESSION_NUMERIC ||             \
            _r.type == NUMERIC_EXPRESSION_NUMERIC)               \
        {                                                        \
            _l.ld = NUMERIC_EXP_VAL(_l) +                        \
                    NUMERIC_EXP_VAL(_r);                         \
            _l.type = NUMERIC_EXPRESSION_NUMERIC;                \
        } else {                                                 \
            _l.i64 = NUMERIC_EXP_VAL(_l) +                       \
                     NUMERIC_EXP_VAL(_r);                        \
            _l.type = NUMERIC_EXPRESSION_INTEGER;                \
        }                                                        \
        _nexp = _l;                                              \
    } while (0)

    #define NUMERIC_EXP_SUB(_nexp, _l, _r) do {                  \
        if (_l.type == NUMERIC_EXPRESSION_NUMERIC ||             \
            _r.type == NUMERIC_EXPRESSION_NUMERIC)               \
        {                                                        \
            _l.ld = NUMERIC_EXP_VAL(_l) -                        \
                    NUMERIC_EXP_VAL(_r);                         \
            _l.type = NUMERIC_EXPRESSION_NUMERIC;                \
        } else {                                                 \
            _l.i64 = NUMERIC_EXP_VAL(_l) -                       \
                     NUMERIC_EXP_VAL(_r);                        \
            _l.type = NUMERIC_EXPRESSION_INTEGER;                \
        }                                                        \
        _nexp = _l;                                              \
    } while (0)

    #define NUMERIC_EXP_MUL(_nexp, _l, _r) do {                  \
        if (_l.type == NUMERIC_EXPRESSION_NUMERIC ||             \
            _r.type == NUMERIC_EXPRESSION_NUMERIC)               \
        {                                                        \
            _l.ld = NUMERIC_EXP_VAL(_l) *                        \
                    NUMERIC_EXP_VAL(_r);                         \
            _l.type = NUMERIC_EXPRESSION_NUMERIC;                \
        } else {                                                 \
            _l.i64 = NUMERIC_EXP_VAL(_l) *                       \
                     NUMERIC_EXP_VAL(_r);                        \
            _l.type = NUMERIC_EXPRESSION_INTEGER;                \
        }                                                        \
        _nexp = _l;                                              \
    } while (0)

    #define NUMERIC_EXP_DIV(_nexp, _l, _r) do {                  \
        if (_l.type == NUMERIC_EXPRESSION_NUMERIC ||             \
            _r.type == NUMERIC_EXPRESSION_NUMERIC)               \
        {                                                        \
            _l.ld = NUMERIC_EXP_VAL(_l) /                        \
                    NUMERIC_EXP_VAL(_r);                         \
            _l.type = NUMERIC_EXPRESSION_NUMERIC;                \
        } else {                                                 \
            if (_r.type == NUMERIC_EXPRESSION_INTEGER &&         \
                _r.i64 == 0)                                     \
            {                                                    \
                YYABORT;                                         \
            }                                                    \
            _l.i64 = NUMERIC_EXP_VAL(_l) +                       \
                     NUMERIC_EXP_VAL(_r);                        \
            _l.type = NUMERIC_EXPRESSION_INTEGER;                \
        }                                                        \
        _nexp = _l;                                              \
    } while (0)

    #define NUMERIC_EXP_UMINUS(_nexp, _l) do {                   \
        if (_l.type == NUMERIC_EXPRESSION_NUMERIC)               \
        {                                                        \
            _l.ld = -NUMERIC_EXP_VAL(_l);                        \
        } else {                                                 \
            _l.i64 = -NUMERIC_EXP_VAL(_l);                       \
        }                                                        \
        _nexp = _l;                                              \
    } while (0)

    #define SET_RULE(_rule) do {                                          \
        if (param) {                                                      \
            param->rule.from.type = NUMERIC_EXPRESSION_INTEGER;           \
            param->rule.from.i64 = NUMERIC_EXP_VAL(_rule.from);           \
            param->rule.has_to = _rule.has_to;                            \
            if (param->rule.has_to) {                                     \
                param->rule.to.type = NUMERIC_EXPRESSION_INTEGER;         \
                param->rule.to.i64 = NUMERIC_EXP_VAL(_rule.to);           \
            }                                                             \
            param->rule.has_advance = _rule.has_advance;                  \
            if (param->rule.has_advance) {                                \
                param->rule.advance.type = NUMERIC_EXPRESSION_INTEGER;    \
                param->rule.advance.i64 = NUMERIC_EXP_VAL(_rule.advance); \
            }                                                             \
        }                                                                 \
    } while (0)
}

/* Bison declarations. */
%require "3.0.4"
%define api.prefix {exe_range_yy}
%define api.pure full
%define api.token.prefix {TOK_EXE_RANGE_}
%define locations
%define parse.error verbose
%define parse.lac full
%define parse.trace true
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct exe_range_param *param }

// union members
%union { struct exe_range_token token; }
%union { char *str; }
%union { char c; }
%union { struct numeric_expression nexp; }
%union { struct range_rule rule; }

%token RANGE FROM TO ADVANCE
%token <token> INTEGER NUMBER

%left '-' '+'
%left '*' '/'
%precedence UMINUS

%nterm <rule> range_rule subrule
%nterm <nexp> exp

%% /* The grammar follows. */

input:
  range_rule        { SET_RULE($1); }
;

range_rule:
  RANGE ':' subrule { $$ = $3; }
;

subrule:
  FROM exp                    { RULE_INIT_FROM($$, $2); }
| FROM exp TO exp             { RULE_INIT_FROM_TO($$, $2, $4); }
| FROM exp TO exp ADVANCE exp { RULE_INIT_FROM_TO_ADVANCE($$, $2, $4, $6); }
| FROM exp ADVANCE exp        { RULE_INIT_FROM_ADVANCE($$, $2, $4); }
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
    struct exe_range_param *param,           // match %parse-param
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

int exe_range_parse(const char *input, size_t len,
        struct exe_range_param *param)
{
    yyscan_t arg = {0};
    exe_range_yylex_init(&arg);
    // exe_range_yyset_in(in, arg);
    int debug_flex = param ? param->debug_flex : 0;
    int debug_bison = param ? param->debug_bison: 0;
    exe_range_yyset_debug(debug_flex, arg);
    yydebug = debug_bison;
    // exe_range_yyset_extra(param, arg);
    exe_range_yy_scan_bytes(input ? input : "", input ? len : 0, arg);
    int ret =exe_range_yyparse(arg, param);
    exe_range_yylex_destroy(arg);
    if (ret) {
        if (param->err_msg==NULL) {
            purc_set_error(PCEXECUTOR_ERROR_OOM);
        } else {
            purc_set_error(PCEXECUTOR_ERROR_BAD_SYNTAX);
        }
    }
    return ret ? -1 : 0;
}

