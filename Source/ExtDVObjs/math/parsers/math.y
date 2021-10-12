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
    #include "mathlib.h"

    struct internal_value {
#ifdef M_math
        double d;
#endif
#ifdef M_math_l
        long double d;
#endif
    };

    struct internal_param {
        purc_variant_t param;
        purc_variant_t variables;
#ifdef M_math
        double d;
#endif
#ifdef M_math_l
        long double d;
#endif
    };

#ifdef M_math
    #define YYSTYPE       MATH_YYSTYPE
    #define YYLTYPE       MATH_YYLTYPE
#endif
#ifdef M_math_l
    #define YYSTYPE       MATH_L_YYSTYPE
    #define YYLTYPE       MATH_L_YYLTYPE
#endif

    typedef void *yyscan_t;
}

%code provides {
}

%code {
    // generated header from flex
    // introduce yylex decl for later use
#ifdef M_math
    #include "math.lex.h"
#endif
#ifdef M_math_l
    #include "math_l.lex.h"
#endif
    #include <math.h>

    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct internal_param *param,      // match %parse-param
        const char *errsg
    );

#ifdef M_math
    #define POW pow
    #define STRTOD         strtod
    #define CAST_TO_NUMBER purc_variant_cast_to_number
    #define MAKE_NUMBER    purc_variant_make_number
#endif
#ifdef M_math_l
    #define POW powl
    #define STRTOD         strtold
    #define CAST_TO_NUMBER purc_variant_cast_to_long_double
    #define MAKE_NUMBER    purc_variant_make_longdouble
#endif

    #define PL(x,y)                                            \
      fprintf(stderr, "location[%d]: (%d,%d)->(%d,%d)\n",      \
                            x,                                 \
                            y.first_line, y.first_column-1,    \
                            y.last_line,  y.last_column-1)

    #define SET(_r, _a) do {                           \
            _r->d = _a.d;                              \
    } while (0)

    #define ASSIGN(_a, _b) do {                                           \
        if (!param->variables) {                                          \
            param->variables = purc_variant_make_object(0, NULL, NULL);   \
            if (!param->variables)                                        \
                YYABORT;                                                  \
        }                                                                 \
        purc_variant_t v;                                                 \
        v = MAKE_NUMBER(_b.d);                                            \
        if (!v)                                                           \
            YYABORT;                                                      \
                                                                          \
        purc_variant_t k = purc_variant_make_string(_a, true);            \
        bool ok;                                                          \
        ok = purc_variant_object_set(param->variables, k, v);             \
        purc_variant_unref(k);                                            \
        purc_variant_unref(v);                                            \
                                                                          \
        if (!ok)                                                          \
            YYABORT;                                                      \
    } while (0)

    #define NEG(_r, _a) do {                           \
            _r.d = -_a.d;                              \
    } while (0)

    #define ADD(_r, _a, _b) do {                       \
            _r.d = _a.d + _b.d;                        \
    } while (0)

    #define SUB(_r, _a, _b) do {                       \
            _r.d = _a.d - _b.d;                        \
    } while (0)

    #define MUL(_r, _a, _b) do {                       \
            _r.d = _a.d * _b.d;                        \
    } while (0)

    #define DIV(_r, _a, _b) do {                       \
            _r.d = _a.d / _b.d;                        \
    } while (0)

    #define EXP(_r, _a, _b) do {                       \
            _r.d = POW(_a.d, _b.d);                    \
    } while (0)

    #define SET_BY_NUM(_r, _a) do {                                 \
            /* TODO: strtod sort of func */                         \
            char *endptr = NULL;                                    \
            _r.d = STRTOD(_a, &endptr);                             \
            if (endptr && *endptr)                                  \
                YYABORT;                                            \
    } while (0)

    #define SET_BY_VAR(_r, _a) do {                                      \
        if (param->variables) {                                          \
            purc_variant_t _v;                                           \
            _v = purc_variant_object_get_by_ckey(param->variables, _a);  \
            if (_v) {                                                    \
                bool ok = CAST_TO_NUMBER(_v, &_r.d, false);              \
                if (ok)                                                  \
                    break;                                               \
            }                                                            \
        }                                                                \
        purc_variant_t _v;                                               \
        if (!param->param || !purc_variant_is_object(param->param))      \
            YYABORT;                                                     \
                                                                         \
        _v = purc_variant_object_get_by_ckey(param->param, _a);          \
        if (!_v)                                                         \
            YYABORT;                                                     \
                                                                         \
        bool ok = CAST_TO_NUMBER(_v, &_r.d, false);                      \
        if (!ok)                                                         \
            YYABORT;                                                     \
    } while (0)
}

/* Bison declarations. */
%require "3.0.4"
    /* %define api.prefix {math_yy} */
%define api.pure full
%define api.token.prefix {TOK_MATH_}
%define locations
%define parse.error verbose
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct internal_param *param }

%union { char *str; }
%union { struct internal_value v; }

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
  %empty
| statements
| statements nop
| nop statements
| nop statements nop
;

statements:
  statement
| statements nop statement
;

nop:
  '\n'
| ';'
| nop '\n'
| nop ';'
;

statement:
  exp         { SET(param, $1); }
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
| '-' exp %prec NEG { NEG($$, $2); }
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
    struct internal_param *param,      // match %parse-param
    const char *errsg
)
{
    // to implement it here
    (void)yylloc;
    (void)arg;
    (void)param;
    fprintf(stderr, "(%d,%d)->(%d,%d): %s\n",
        yylloc->first_line, yylloc->first_column-1,
        yylloc->last_line,  yylloc->last_column-1,
        errsg);
}

#ifdef M_math
int math_eval(const char *input, double *d, purc_variant_t param)
{
    struct internal_param ud = {0};
    ud.param = param;
    ud.variables = PURC_VARIANT_INVALID;

    yyscan_t arg = {0};
    math_yylex_init(&arg);
    // math_yyset_in(in, arg);
    // math_yyset_debug(debug, arg);
    math_yyset_extra(param, arg);
    math_yy_scan_string(input, arg);
    int ret =math_yyparse(arg, &ud);
    if (ud.variables) {
        purc_variant_unref(ud.variables);
        ud.variables = PURC_VARIANT_INVALID;
    }
    math_yylex_destroy(arg);
    if (ret==0 && d) {
        *d = ud.d;
    }
    return ret ? 1 : 0;
}
#endif
#ifdef M_math_l
int math_eval_l(const char *input, long double *d, purc_variant_t param)
{
    struct internal_param ud = {0};
    ud.param = param;
    ud.variables = PURC_VARIANT_INVALID;

    yyscan_t arg = {0};
    math_l_yylex_init(&arg);
    // math_l_yyset_in(in, arg);
    // math_l_yyset_debug(debug, arg);
    math_l_yyset_extra(param, arg);
    math_l_yy_scan_string(input, arg);
    int ret =math_l_yyparse(arg, &ud);
    if (ud.variables) {
        purc_variant_unref(ud.variables);
        ud.variables = PURC_VARIANT_INVALID;
    }
    math_l_yylex_destroy(arg);
    if (ret==0 && d) {
        *d = ud.d;
    }
    return ret ? 1 : 0;
}
#endif


