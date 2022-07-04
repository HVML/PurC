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

        #define POW            pow
        #define STRTOD         strtod
        #define CAST_TO_NUMBER purc_variant_cast_to_number
        #define MAKE_NUMBER    purc_variant_make_number

        #define UNI_FUNC       math_uni

        #define SIN            sin
        #define COS            cos
        #define TAN            tan
        #define ASIN           asin
        #define ACOS           acos
        #define ATAN           atan
        #define LOG            log

    #elif defined(M_math_l)
        #define YYSTYPE        MATH_L_YYSTYPE
        #define YYLTYPE        MATH_L_YYLTYPE

        #define VALUE_TYPE     long double
        #define FUNC_NAME      math_eval_l

        #define POW            powl
        #define STRTOD         strtold
        #define CAST_TO_NUMBER purc_variant_cast_to_longdouble
        #define MAKE_NUMBER    purc_variant_make_longdouble

        #define UNI_FUNC       math_unil

        #define SIN            sinl
        #define COS            cosl
        #define TAN            tanl
        #define ASIN           asinl
        #define ACOS           acosl
        #define ATAN           atanl
        #define LOG            logl
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

    #define EXP(_r, _a, _b) do {                       \
            _r.d = POW(_a.d, _b.d);                    \
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
        char *_s = (char*)_a.text;                                       \
        const char _c = _s[_a.leng];                                     \
        if (param->variables) {                                          \
            purc_variant_t _v;                                           \
            _s[_a.leng] = '\0';                                          \
            _v = purc_variant_object_get_by_ckey(param->variables, _s);  \
            _s[_a.leng] = _c;                                            \
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
        _s[_a.leng] = '\0';                                              \
        _v = purc_variant_object_get_by_ckey(param->param, _s);          \
        _s[_a.leng] = _c;                                                \
        if (!_v)                                                         \
            YYABORT;                                                     \
                                                                         \
        bool ok = CAST_TO_NUMBER(_v, &_r.d, false);                      \
        if (!ok)                                                         \
            YYABORT;                                                     \
    } while (0)

    #define EVAL_BY_UNI_FUNC(_r, _f, _a) do {                            \
        int r = UNI_FUNC(&_r.d, _f, _a.d);                               \
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
%union { VALUE_TYPE (*uni_func)(VALUE_TYPE a); }

%precedence '='
%left '-' '+'
%left '*' '/'
%precedence NEG /* negation--unary minus */
%right '^'      /* exponentiation */

%token SIN COS TAN ASIN ACOS ATAN LOG
%token <token> NUMBER VAR
%nterm <v> exp term
%nterm <uni_func> uni_func


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
| exp '^' exp   { EXP($$, $1, $3); }
| '-' exp %prec NEG { NEG($$, $2); }
;

term:
  NUMBER      { SET_BY_NUM($$, $1); }
| VAR         { SET_BY_VAR($$, $1); }
| uni_func '(' term ')' { EVAL_BY_UNI_FUNC($$, $1, $3); }
| '(' exp ')' { $$ = $2; }
;

uni_func:
  SIN         { $$ = SIN; }
| COS         { $$ = COS; }
| TAN         { $$ = TAN; }
| ASIN        { $$ = ASIN; }
| ACOS        { $$ = ACOS; }
| ATAN        { $$ = ATAN; }
| LOG         { $$ = LOG; }
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


