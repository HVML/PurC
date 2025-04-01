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

    #if defined(M_math)
        #define YYSTYPE        MATH_YYSTYPE
        #define YYLTYPE        MATH_YYLTYPE

        #define VALUE_TYPE     double
        #define FUNC_NAME      math_eval

        #define STRTOD         strtod
        #define CAST_TO_NUMBER purc_variant_cast_to_number
        #define MAKE_NUMBER    purc_variant_make_number

        #define VOI_FUNC       math_voi
        #define UNI_FUNC       math_uni
        #define BIN_FUNC       math_bin

        #define RANDOM         math_random

        #define SIN            sin
        #define COS            cos
        #define TAN            tan
        #define ASIN           asin
        #define ACOS           acos
        #define ATAN           atan
        #define SINH           sinh
        #define COSH           cosh
        #define TANH           tanh
        #define ASINH          asinh
        #define ACOSH          acosh
        #define ATANH          atanh
        #define LOG            log
        #define LOG10          log10
        #define LOG2           log2

        #define CBRT           cbrt
        #define SQRT           sqrt
        #define EXP            exp
        #define CEIL           ceil
        #define FLOOR          floor
        #define ROUND          round
        #define TRUNC          trunc
        #define ABS            math_abs
        #define SIGN           math_sign

        #define ATAN2          atan2
        #define HYPOT          hypot
        #define MAX            math_max
        #define MIN            math_min
        #define POW            pow

        #define PRE_DEFINED    math_pre_defined

    #elif defined(M_math_l)
        #define YYSTYPE        MATH_L_YYSTYPE
        #define YYLTYPE        MATH_L_YYLTYPE

        #define VALUE_TYPE     long double
        #define FUNC_NAME      math_eval_l

        #define STRTOD         strtold
        #define CAST_TO_NUMBER purc_variant_cast_to_longdouble
        #define MAKE_NUMBER    purc_variant_make_longdouble

        #define VOI_FUNC       math_voi_l
        #define UNI_FUNC       math_uni_l
        #define BIN_FUNC       math_bin_l

        #define RANDOM         math_random_l

        #define SIN            sinl
        #define COS            cosl
        #define TAN            tanl
        #define ASIN           asinl
        #define ACOS           acosl
        #define ATAN           atanl
        #define SINH           sinhl
        #define COSH           coshl
        #define TANH           tanhl
        #define ASINH          asinhl
        #define ACOSH          acoshl
        #define ATANH          atanhl
        #define LOG            logl
        #define LOG10          log10l
        #define LOG2           log2l

        #define CBRT           cbrtl
        #define SQRT           sqrtl
        #define EXP            expl
        #define CEIL           ceill
        #define FLOOR          floorl
        #define ROUND          roundl
        #define TRUNC          truncl
        #define ABS            math_abs_l
        #define SIGN           math_sign_l

        #define ATAN2          atan2l
        #define HYPOT          hypotl
        #define MAX            math_max_l
        #define MIN            math_min_l
        #define POW            powl

        #define PRE_DEFINED    math_pre_defined_l

    #endif

    struct internal_value {
        VALUE_TYPE      d;
    };

    struct internal_param {
        purc_variant_t param;
        purc_variant_t variables;
        VALUE_TYPE     d;
        unsigned int   divide_by_zero:1;
    };

    struct math_token {
        const char      *text;
        size_t           leng;
    };

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

    #define SET(_r, _a) do {                           \
            _r->d = _a.d;                              \
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

    #define DIV(_r, _a, _b, _loc) do {                        \
            int f = fpclassify(_b.d);                         \
            if (f & FP_ZERO) {                                \
                param->divide_by_zero = 1;                    \
                yyerror(_loc,arg,param,"Divide by zero");     \
                YYERROR;                                      \
            }                                                 \
            _r.d = _a.d / _b.d;                               \
    } while (0)

    #define SET_BY_NUM(_r, _a) do {                                 \
            /* TODO: strtod sort of func */                         \
            char *_s = (char*)_a.text;                              \
            const char _c = _s[_a.leng];                            \
            char *endptr = NULL;                                    \
            _s[_a.leng] = '\0';                                     \
            _r.d = STRTOD(_s, &endptr);                             \
            _s[_a.leng] = _c;                                       \
            if (endptr && *endptr)                                  \
                YYABORT;                                            \
    } while (0)

    #define SET_BY_VAR(_r, _a) do {                                      \
        purc_variant_t _v;                                               \
        char *_s = (char*)_a.text;                                       \
        const char _c = _s[_a.leng];                                     \
        if (param->variables) {                                          \
            _s[_a.leng] = '\0';                                          \
            _v = purc_variant_object_get_by_ckey_ex(param->variables,       \
                    _s, false);                                          \
            _s[_a.leng] = _c;                                            \
            if (_v) {                                                    \
                bool ok = CAST_TO_NUMBER(_v, &_r.d, false);              \
                if (ok)                                                  \
                    break;                                               \
            }                                                            \
        }                                                                \
        if (param->param && purc_variant_is_object(param->param)) {      \
            _s[_a.leng] = '\0';                                          \
            _v = purc_variant_object_get_by_ckey_ex(param->param,           \
                    _s, false);                                          \
            _s[_a.leng] = _c;                                            \
            if (_v) {                                                    \
                bool ok = CAST_TO_NUMBER(_v, &_r.d, false);              \
                if (ok)                                                  \
                    break;                                               \
            }                                                            \
        }                                                                \
        YYABORT;                                                         \
    } while (0)

    #define SET_BY_PRE_DEFINED(_r, _a, _s) do {                          \
        purc_variant_t _v;                                               \
        if (param->variables) {                                          \
            _v = purc_variant_object_get_by_ckey_ex(param->variables,       \
                    _s, false);                                          \
            if (_v) {                                                    \
                bool ok = CAST_TO_NUMBER(_v, &_r.d, false);              \
                if (ok)                                                  \
                    break;                                               \
            }                                                            \
        }                                                                \
        if (param->param && purc_variant_is_object(param->param)) {      \
            _v = purc_variant_object_get_by_ckey_ex(param->param, _s, false);  \
            if (_v) {                                                    \
                bool ok = CAST_TO_NUMBER(_v, &_r.d, false);              \
                fprintf(stderr, "_s: %s; ok: %d\n", _s, ok); \
                if (ok)                                                  \
                    break;                                               \
            }                                                            \
        }                                                                \
        _r.d = PRE_DEFINED(_a);                                          \
        purc_clr_error();                                                \
    } while (0)

    #define EVAL_BY_VOI_FUNC(_r, _f) do {                                \
        int r = VOI_FUNC(&_r.d, _f);                                     \
        if (r)                                                           \
            YYABORT;                                                     \
    } while (0)

    #define EVAL_BY_UNI_FUNC(_r, _f, _a) do {                            \
        int r = UNI_FUNC(&_r.d, _f, _a.d);                               \
        if (r)                                                           \
            YYABORT;                                                     \
    } while (0)

    #define EVAL_BY_BIN_FUNC(_r, _f, _a, _b) do {                        \
        int r = BIN_FUNC(&_r.d, _f, _a.d, _b.d);                         \
        if (r)                                                           \
            YYABORT;                                                     \
    } while (0)

    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct internal_param *param,      // match %parse-param
        const char *errsg
    );

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

%union { struct math_token token; }
%union { struct internal_value v; }
%union { VALUE_TYPE (*voi_func)(void); }
%union { VALUE_TYPE (*uni_func)(VALUE_TYPE a); }
%union { VALUE_TYPE (*bin_func)(VALUE_TYPE a, VALUE_TYPE b); }

%precedence '='
%left '-' '+'
%left '*' '/'
%precedence NEG /* negation--unary minus */
%right '^'      /* exponentiation */

%token SIN COS TAN ASIN ACOS ATAN
%token SINH COSH TANH ASINH ACOSH ATANH ATAN2
%token CBRT EXP HYPOT
%token LOG LOG10 LOG2
%token POW SQRT
%token CEIL FLOOR ROUND TRUNC
%token ABS MAX MIN RANDOM SIGN
%token PI E LN2 LN10 LOG2E LOG10E SQRT1_2 SQRT2

%token <token> NUMBER VAR
%nterm <v> exp term pre_defined
%nterm <voi_func> voi_func
%nterm <uni_func> uni_func
%nterm <bin_func> bin_func


%% /* The grammar follows. */

input:
  %empty
| statement
;

statement:
  exp         { SET(param, $1); }
;

exp:
  term
| exp '+' exp   { ADD($$, $1, $3); }
| exp '-' exp   { SUB($$, $1, $3); }
| exp '*' exp   { MUL($$, $1, $3); }
| exp '/' exp   { DIV($$, $1, $3, &(@3)); }
| exp '^' exp   { EVAL_BY_BIN_FUNC($$, POW, $1, $3); }
| '-' exp %prec NEG { NEG($$, $2); }
;

term:
  NUMBER      { SET_BY_NUM($$, $1); }
| VAR         { SET_BY_VAR($$, $1); }
| pre_defined { $$ = $1; }
| voi_func '(' ')' { EVAL_BY_VOI_FUNC($$, $1); }
| uni_func '(' exp ')' { EVAL_BY_UNI_FUNC($$, $1, $3); }
| bin_func '(' exp ',' exp ')' { EVAL_BY_BIN_FUNC($$, $1, $3, $5); }
| '(' exp ')' { $$ = $2; }
;

pre_defined:
  PI          { SET_BY_PRE_DEFINED($$, MATH_PI,      "PI"); }
| E           { SET_BY_PRE_DEFINED($$, MATH_E,       "E"); }
| LN2         { SET_BY_PRE_DEFINED($$, MATH_LN2,     "LN2"); }
| LN10        { SET_BY_PRE_DEFINED($$, MATH_LN10,    "LN10"); }
| LOG2E       { SET_BY_PRE_DEFINED($$, MATH_LOG2E,   "LOG2E"); }
| LOG10E      { SET_BY_PRE_DEFINED($$, MATH_LOG10E,  "LOG10E"); }
| SQRT1_2     { SET_BY_PRE_DEFINED($$, MATH_SQRT1_2, "SQRT1_2"); }
| SQRT2       { SET_BY_PRE_DEFINED($$, MATH_SQRT2,   "SQRT2"); }


voi_func:
  RANDOM      { $$ = RANDOM; }
;

uni_func:
  SIN         { $$ = SIN; }
| COS         { $$ = COS; }
| TAN         { $$ = TAN; }
| ASIN        { $$ = ASIN; }
| ACOS        { $$ = ACOS; }
| ATAN        { $$ = ATAN; }
| SINH        { $$ = SINH; }
| COSH        { $$ = COSH; }
| TANH        { $$ = TANH; }
| ASINH       { $$ = ASINH; }
| ACOSH       { $$ = ACOSH; }
| ATANH       { $$ = ATANH; }
| LOG         { $$ = LOG; }
| LOG10       { $$ = LOG10; }
| LOG2        { $$ = LOG2; }
| CBRT        { $$ = CBRT; }
| EXP         { $$ = EXP; }
| SQRT        { $$ = SQRT; }
| CEIL        { $$ = CEIL; }
| FLOOR       { $$ = FLOOR; }
| ROUND       { $$ = ROUND; }
| TRUNC       { $$ = TRUNC; }
| ABS         { $$ = ABS; }
| SIGN        { $$ = SIGN; }
;

bin_func:
  ATAN2       { $$ = ATAN2; }
| HYPOT       { $$ = HYPOT; }
| POW         { $$ = POW; }
| MAX         { $$ = MAX; }
| MIN         { $$ = MIN; }
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

int FUNC_NAME(const char *input, VALUE_TYPE *d, purc_variant_t param)
{
    struct internal_param ud = {0};
    ud.param = param;
    ud.variables = PURC_VARIANT_INVALID;

    yyscan_t arg = {0};
    yylex_init(&arg);
    // yyset_in(in, arg);
    // yyset_debug(debug, arg);
    yyset_extra(param, arg);
    yy_scan_string(input, arg);
    int ret =yyparse(arg, &ud);
    if (ud.variables) {
        purc_variant_unref(ud.variables);
        ud.variables = PURC_VARIANT_INVALID;
    }
    yylex_destroy(arg);
    if (ret==0 && d) {
        *d = ud.d;
    }
    else {
        if (ud.divide_by_zero) {
            purc_set_error(PURC_ERROR_OVERFLOW);
        }
        else {
            purc_set_error(PURC_ERROR_INTERNAL_FAILURE);
        }
    }
    return ret ? 1 : 0;
}

