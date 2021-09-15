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
    #include <math.h>

    struct pcdvobjs_math_symbol {
        char            *symbol;
        union {
            double           d;
            long double      ld;
        };
    };

    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct pcdvobjs_math_param *param, // match %parse-param
        const char *errsg
    );

    #define SET(_r, _a) do {                           \
        if (param->is_long_double) {                   \
            _r->ld = _a.ld;                            \
        } else {                                       \
            _r->d = _a.d;                              \
        }                                              \
    } while (0)

    #define ASSIGN(_a, _b) do {                             \
        int r;                                              \
        r = pcdvobjs_math_param_set_var(param, _a, &_b);    \
        if (r)                                              \
            YYABORT;                                        \
    } while (0)

    #define ADD(_r, _a, _b) do {                       \
        if (param->is_long_double) {                   \
            _r.ld = _a.ld + _b.ld;                     \
        } else {                                       \
            _r.d = _a.d + _b.d;                        \
        }                                              \
    } while (0)

    #define SUB(_r, _a, _b) do {                       \
        if (param->is_long_double) {                   \
            _r.ld = _a.ld - _b.ld;                     \
        } else {                                       \
            _r.d = _a.d - _b.d;                        \
        }                                              \
    } while (0)

    #define MUL(_r, _a, _b) do {                       \
        if (param->is_long_double) {                   \
            _r.ld = _a.ld * _b.ld;                     \
        } else {                                       \
            _r.d = _a.d * _b.d;                        \
        }                                              \
    } while (0)

    #define DIV(_r, _a, _b) do {                       \
        if (param->is_long_double) {                   \
            _r.ld = _a.ld / _b.ld;                     \
        } else {                                       \
            _r.d = _a.d / _b.d;                        \
        }                                              \
    } while (0)

    #define EXP(_r, _a, _b) do {                       \
        if (param->is_long_double) {                   \
            _r.ld = powl(_a.ld, _b.ld);                \
        } else {                                       \
            _r.d = pow(_a.d, _b.d);                    \
        }                                              \
    } while (0)

    #define SET_BY_NUM(_r, _a) do {                    \
        if (param->is_long_double) {                   \
            _r.ld = atof(_a);                          \
        } else {                                       \
            _r.d = atof(_a);                           \
        }                                              \
    } while (0)

    static int
    _get_from_env(struct pcdvobjs_math_param *param,
        const char *key, struct pcdvobjs_math_value *val)
    {
        purc_variant_t _v;
        if (!param->v && !purc_variant_is_object(param->v))
            return -1;

        _v = purc_variant_object_get_by_ckey(param->v, key);
        if (!_v)
            return -1;

        bool ok;
        if (param->is_long_double) {
            ok = purc_variant_cast_to_long_double(_v, &val->ld, false);
        } else {
            ok = purc_variant_cast_to_number(_v, &val->d, false);
        }
        if (!ok)
            return -1;

        return 0;
    }

    #define SET_BY_VAR(_r, _a) do {                         \
        int r;                                              \
        struct pcdvobjs_math_value val = {0};               \
        r = pcdvobjs_math_param_get_var(param, _a, &val);   \
        if (r) {                                            \
            r = _get_from_env(param, _a, &val);             \
        }                                                   \
        if (r)                                              \
            YYABORT;                                        \
        _r = val;                                           \
    } while (0)
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

%union { char *str; }
%union { struct pcdvobjs_math_value v; }

%destructor { free($$); } <str>

%precedence '='
%left '-' '+'
%left '*' '/'
%precedence NEG /* negation--unary minus */
%right '^'      /* exponentiation */

%token <str> NUMBER VAR
%nterm <v> exp term


%% /* The grammar follows. */

input:
  statements
;

statements:
  %empty
| statement
| statements '\n' statement
;

statement:
  exp              { SET(param, $1); }
| assignment        
;

assignment:
  VAR '=' exp      { ASSIGN($1, $3); free($1); }
;

exp: 
  term
| exp '+' exp   { ADD($$, $1, $3); }
| exp '-' exp   { SUB($$, $1, $3); }
| exp '*' exp   { MUL($$, $1, $3); }
| exp '/' exp   { DIV($$, $1, $3); }
| exp '^' exp   { EXP($$, $1, $3); }
;

term:
  NUMBER      { SET_BY_NUM($$, $1); free($1); }
| VAR         { SET_BY_VAR($$, $1); free($1); }
| '(' exp ')' { $$ = $2; }
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

