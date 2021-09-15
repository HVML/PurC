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

    #define SET(_r, _a) do {                           \
        if (_a.is_long_double) {                       \
            _r->ld = _a.ld;                            \
        } else {                                       \
            _r->d = _a.d;                              \
        }                                              \
        _r->is_long_double = _a.is_long_double;        \
    } while (0)

    #define ADD(_r, _a, _b) do {                       \
        if (_a.is_long_double) {                       \
            _r.ld = _a.ld + _b.ld;                     \
        } else {                                       \
            _r.d = _a.d + _b.d;                        \
        }                                              \
        _r.is_long_double = _a.is_long_double;         \
    } while (0)

    #define SUB(_r, _a, _b) do {                       \
        if (_a.is_long_double) {                       \
            _r.ld = _a.ld - _b.ld;                     \
        } else {                                       \
            _r.d = _a.d - _b.d;                        \
        }                                              \
        _r.is_long_double = _a.is_long_double;         \
    } while (0)

    #define MUL(_r, _a, _b) do {                       \
        if (_a.is_long_double) {                       \
            _r.ld = _a.ld * _b.ld;                     \
        } else {                                       \
            _r.d = _a.d * _b.d;                        \
        }                                              \
        _r.is_long_double = _a.is_long_double;         \
    } while (0)

    #define DIV(_r, _a, _b) do {                       \
        if (_a.is_long_double) {                       \
            _r.ld = _a.ld / _b.ld;                     \
        } else {                                       \
            _r.d = _a.d / _b.d;                        \
        }                                              \
        _r.is_long_double = _a.is_long_double;         \
    } while (0)

    #define SET_BY_NUM(_r, _a) do {                    \
        if (param->is_long_double) {                   \
            _r.ld = atof(_a);                          \
        } else {                                       \
            _r.d = atof(_a);                           \
        }                                              \
        _r.is_long_double = param->is_long_double;     \
    } while (0)

    #define SET_BY_VAR(_r, _a) do {                                    \
        if (!param->v && !purc_variant_is_object(param->v)) {          \
            YYABORT;                                                   \
        }                                                              \
        purc_variant_t _v;                                             \
        _v = purc_variant_object_get_by_ckey (param->v, _a);           \
        if (!_v) {                                                     \
            YYABORT;                                                   \
        }                                                              \
        if (param->is_long_double) {                                   \
            purc_variant_cast_to_long_double (_v, &_r.ld, false);      \
        } else {                                                       \
            purc_variant_cast_to_number (_v, &_r.d, false);            \
        }                                                              \
        _r.is_long_double = param->is_long_double;                     \
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

%union { const char *str; }
%union { struct pcdvobjs_math_param v; }

%precedence '='
%left '-' '+'
%left '*' '/'
%precedence NEG /* negation--unary minus */
%right '^'      /* exponentiation */

%token <str> NUMBER VAR
%nterm <v> exp factor term


%% /* The grammar follows. */

input:
  %empty           { }
| exp              { SET(param, $1); }
;

exp: 
  factor
| exp '+' exp      { ADD($$, $1, $3); }
| exp '-' factor   { SUB($$, $1, $3); }
;

factor:
  term
| factor '*' term { MUL($$, $1, $3); }
| factor '/' term { DIV($$, $1, $3); }
;

term:
  NUMBER      { SET_BY_NUM($$, $1); }
| VAR         { SET_BY_VAR($$, $1); }
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

