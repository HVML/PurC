%code top {
/*
 * @file logical.y
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
    // here to include header files required for generated logical.tab.c
}

%code requires {
    #define YYSTYPE       LOGICAL_YYSTYPE
    #define YYLTYPE       LOGICAL_YYLTYPE

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
        struct pcdvobjs_logical_param *param, // match %parse-param
        const char *errsg
    );

    static double ge(double l, double r)
    {
        bool eq = pcutils_equal_doubles(l, r);
        if (eq)
            return 1.0;

        return (l > r) ? 1.0 : 0.0;
    }

    static double le(double l, double r)
    {
        bool eq = pcutils_equal_doubles(l, r);
        if (eq)
            return 1.0;

        return (l < r) ? 1.0 : 0.0;
    }

    static double eq(double l, double r)
    {
        bool eq = pcutils_equal_doubles(l, r);
        return eq ? 1.0 : 0.0;
    }

    static double ne(double l, double r)
    {
        bool eq = pcutils_equal_doubles(l, r);
        return eq ? 0.0 : 1.0;
    }

    static double and(double l, double r)
    {
        int lz = (fpclassify(l) == FP_ZERO) ? 0 : 1;
        int rz = (fpclassify(r) == FP_ZERO) ? 0 : 1;
        return (lz && rz) ? 1.0 : 0.0;
    }

    static double or(double l, double r)
    {
        int lz = (fpclassify(l) == FP_ZERO) ? 0 : 1;
        int rz = (fpclassify(r) == FP_ZERO) ? 0 : 1;
        return (lz || rz) ? 1.0 : 0.0;
    }

    static double gt(double l, double r)
    {
        return (l > r) ? 1.0 : 0.0;
    }

    static double lt(double l, double r)
    {
        return (l < r) ? 1.0 : 0.0;
    }

    static double not(double d)
    {
        return (fpclassify(d) == FP_ZERO) ? 1.0 : 0.0;
    }

    static int eval_boolean(double d)
    {
        return (FP_ZERO == fpclassify(d)) ? false : true;
    }

    #define EVAL_FREE(v) do {            \
    } while (0)

    #define EVAL_SET(_a) do {                          \
        param->result = eval_boolean(_a);              \
    } while (0)

    #define EVAL_APPLY_1(_f, _r, _a) do {                            \
        _r = _f(_a);                                                 \
    } while (0)

    #define EVAL_APPLY_2(_f, _r, _a, _b) do {                        \
        _r = _f(_a, _b);                                             \
    } while (0)

    #define EVAL_INT(_r, _a) do {                                    \
        char   *ptr = (char*)_a[1];                                  \
        size_t  sz  = _a[0];                                         \
        char c  = ptr[sz];                                           \
        ptr[sz] = '\0';                                              \
        long long ll = atoll(ptr);                                   \
        ptr[sz] = c;                                                 \
        _r = ll;                                                     \
    } while (0)

    #define EVAL_NUM(_r, _a) do {                                    \
        char   *ptr = (char*)_a[1];                                  \
        size_t  sz  = _a[0];                                         \
        char c  = ptr[sz];                                           \
        ptr[sz] = '\0';                                              \
        double d = atof(ptr);                                        \
        ptr[sz] = c;                                                 \
        _r = d;                                                      \
    } while (0)

    #define EVAL_VAR(_r, _a) do {                                    \
        if (!param->variables)                                       \
            YYABORT;                                                 \
        char   *ptr = (char*)_a[1];                                  \
        size_t  sz  = _a[0];                                         \
        char c  = ptr[sz];                                           \
        ptr[sz] = '\0';                                              \
        purc_variant_t _v;                                           \
        _v = purc_variant_object_get_by_ckey(param->variables, ptr, true);  \
        ptr[sz] = c;                                                 \
        if (_v == PURC_VARIANT_INVALID) {                            \
            if (!param->v && !purc_variant_is_object(param->v))      \
                YYABORT;                                             \
            ptr[sz] = '\0';                                          \
            _v = purc_variant_object_get_by_ckey(param->v, ptr, true);  \
            ptr[sz] = c;                                             \
            if (_v == PURC_VARIANT_INVALID)                          \
                YYABORT;                                             \
        }                                                            \
        _r = purc_variant_numerify(_v);                             \
    } while (0)
}

/* Bison declarations. */
%require "3.0.4"
%define api.pure full
%define api.token.prefix {TOK_LOGICAL_}
%define locations
%define parse.error verbose
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct pcdvobjs_logical_param *param }

%union { uintptr_t  sz_ptr[2]; }
%union { double v; }

%destructor { EVAL_FREE($$); } <v>

/* declare tokens */
/*
 * The expression precedence is as follows: (lowest to highest)
 *        || operator, left associative
 *        && operator, left associative
 *        ! operator, nonassociative
 *        Relational operators, left associative
 *        Assignment operator, right associative
 *        + and - operators, left associative
 *        *, / and % operators, left associative
 *        ^ operator, right associative
 *        unary - operator, nonassociative
 *        ++ and -- operators, nonassociative
 */
%token <sz_ptr> VAR INT NUM
%left OR                      /* || */
%left AND                     /* && */
%precedence NEG               /* ! */
%left GE LE EQ NE '>' '<'     /* relational operators */

%nterm <v> term exp

%% /* The grammar follows. */


input:
  %empty
| statement
;

statement:
  exp                { EVAL_SET($1); }
;

exp:
  term
| exp GE exp         { EVAL_APPLY_2(ge, $$, $1, $3); }
| exp LE exp         { EVAL_APPLY_2(le, $$, $1, $3); }
| exp EQ exp         { EVAL_APPLY_2(eq, $$, $1, $3); }
| exp NE exp         { EVAL_APPLY_2(ne, $$, $1, $3); }
| exp AND exp        { EVAL_APPLY_2(and, $$, $1, $3); }
| exp OR exp         { EVAL_APPLY_2(or, $$, $1, $3); }
| exp '>' exp        { EVAL_APPLY_2(gt, $$, $1, $3); }
| exp '<' exp        { EVAL_APPLY_2(lt, $$, $1, $3); }
| '!' exp %prec NEG  { EVAL_APPLY_1(not, $$, $2); }
;

term:
  INT                { EVAL_INT($$, $1); }
| NUM                { EVAL_NUM($$, $1); }
| VAR                { EVAL_VAR($$, $1); }
| '(' exp ')'        { $$ = $2; }
;

%%

/* Called by yyparse on error. */
static void
yyerror(
    YYLTYPE *yylloc,                   // match %define locations
    yyscan_t arg,                      // match %param
    struct pcdvobjs_logical_param *param, // match %parse-param
    const char *errsg
)
{
    // to implement it here
    (void)yylloc;
    (void)arg;
    (void)param;
    fprintf(stderr, "(%d,%d)->(%d,%d): %s\n",
        yylloc->first_line, yylloc->first_column,
        yylloc->last_line, yylloc->last_column,
        errsg);
}

int pcdvobjs_logical_parse(const char *input,
        struct pcdvobjs_logical_param *param)
{
    yyscan_t arg = {0};

    yylex_init(&arg);
    // yyset_in(in, arg);
    // yyset_debug(debug, arg);
    yyset_extra(param, arg);
    yy_scan_string(input, arg);
    int ret =yyparse(arg, param);
    yylex_destroy(arg);
    if (param->variables) {
        purc_variant_unref(param->variables);
        param->variables = NULL;
    }

    return ret ? 1 : 0;
}

