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

    struct logical_funcs; // forward declaration
}

%code provides {
}

%code {
    // generated header from flex
    // introduce yylex decl for later use
    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct logical_funcs *funcs,          // match %parse-param
        struct pcdvobjs_logical_param *param, // match %parse-param
        const char *errsg
    );

    struct logical_funcs {
        purc_dvariant_method not_f;
        purc_dvariant_method and_f;
        purc_dvariant_method or_f;
        purc_dvariant_method eq_f;
        purc_dvariant_method ne_f;
        purc_dvariant_method gt_f;
        purc_dvariant_method ge_f;
        purc_dvariant_method lt_f;
        purc_dvariant_method le_f;
        purc_dvariant_method streq_f;
        purc_dvariant_method strne_f;
        purc_dvariant_method strgt_f;
        purc_dvariant_method strge_f;
        purc_dvariant_method strlt_f;
        purc_dvariant_method strle_f;
    };

    static int init_logical_funcs(struct logical_funcs *funcs,
                purc_variant_t logical)
    {
        void* ptr_ptr[][2] = {
            { &funcs->not_f,    (void*)"not" },
            { &funcs->and_f,    (void*)"and" },
            { &funcs->or_f,     (void*)"or" },
            { &funcs->eq_f,     (void*)"eq" },
            { &funcs->ne_f,     (void*)"ne" },
            { &funcs->gt_f,     (void*)"gt" },
            { &funcs->ge_f,     (void*)"ge" },
            { &funcs->lt_f,     (void*)"lt" },
            { &funcs->le_f,     (void*)"le" },
            { &funcs->streq_f,  (void*)"streq" },
            { &funcs->strne_f,  (void*)"strne" },
            { &funcs->strgt_f,  (void*)"strgt" },
            { &funcs->strge_f,  (void*)"strge" },
            { &funcs->strlt_f,  (void*)"strlt" },
            { &funcs->strle_f,  (void*)"strle" },
        };

        for (size_t i=0; i<PCA_TABLESIZE(ptr_ptr); ++i) {
            purc_variant_t v = purc_variant_object_get_by_ckey(logical,
                (const char*)ptr_ptr[i][1]);
            if (v == PURC_VARIANT_INVALID)
                return -1;

            purc_dvariant_method func;
            func = purc_variant_dynamic_get_getter(v);
            if (!func)
                return -1;

            *(purc_dvariant_method*)ptr_ptr[i][0] = func;
        }

        return 0;
    }

    static int eval_variant(purc_variant_t v)
    {
        switch (v->type) {
            case PURC_VARIANT_TYPE_NULL:
                {
                    return 0;
                } break;
            case PURC_VARIANT_TYPE_BOOLEAN:
                {
                    return v->b ? 1 : 0;
                } break;
            case PURC_VARIANT_TYPE_NUMBER:
                {
                    return v->d != 0 ? 1 : 0;
                } break;
            case PURC_VARIANT_TYPE_LONGINT:
                {
                    return v->i64 ? 1 : 0;
                } break;
            case PURC_VARIANT_TYPE_ULONGINT:
                {
                    return v->u64 ? 1 : 0;
                } break;
            case PURC_VARIANT_TYPE_LONGDOUBLE:
                {
                    return v->ld != 0 ? 1 : 0;
                } break;
            case PURC_VARIANT_TYPE_ATOMSTRING:
                {
                    return v->atom ? 1 : 0;
                } break;
            case PURC_VARIANT_TYPE_STRING:
                {
                    return v->sz_ptr[0] ? 1 : 0;
                } break;
            case PURC_VARIANT_TYPE_BSEQUENCE:
                {
                    return v->sz_ptr[0] ? 1 : 0;
                } break;
            case PURC_VARIANT_TYPE_DYNAMIC:
                {
                    return (v->ptr_ptr[0] || v->ptr_ptr[1]) ? 1 : 0;
                } break;
            case PURC_VARIANT_TYPE_NATIVE:
                {
                    return (v->ptr_ptr[0]) ? 1 : 0;
                } break;
            case PURC_VARIANT_TYPE_OBJECT:
                {
                    int n = purc_variant_object_get_size(v);
                    return n>0 ? 1 : 0;
                } break;
            case PURC_VARIANT_TYPE_ARRAY:
                {
                    size_t sz = purc_variant_array_get_size(v);
                    return sz>0 ? 1 : 0;
                } break;
            case PURC_VARIANT_TYPE_SET:
                {
                    size_t sz = purc_variant_set_get_size(v);
                    return sz>0 ? 1 : 0;
                } break;
            default:
                {
                    return 0;
                }
        }
    }

    static int set_var(struct pcdvobjs_logical_param *param,
                    char *var, size_t len, purc_variant_t v)
    {
        if (!param->variables) {
            param->variables = purc_variant_make_object(0,
                        NULL, PURC_VARIANT_INVALID);
            if (!param->variables) {
                return -1;
            }
        }
        char c = var[len];
        var[len] = '\0';
        purc_variant_t key = purc_variant_make_string(var, true);
        var[len] = c;
        if (key == PURC_VARIANT_INVALID)
            return -1;

        bool ok;
        ok = purc_variant_object_set(param->variables, key, v);
        purc_variant_unref(key);

        return ok ? 0 : -1;
    }

    #define EVAL_FREE(v) do {            \
        if (v)                           \
            purc_variant_unref(v);       \
    } while (0)

    #define EVAL_SET(_a) do {                          \
        param->result = eval_variant(_a);              \
        purc_variant_unref(_a);                        \
        _a = PURC_VARIANT_INVALID;                     \
    } while (0)

    #define EVAL_ASSIGN(_a, _b) do {                        \
        int r;                                              \
        char   *ptr  = (char*)_a[1];                        \
        size_t  sz   = (size_t)_a[0];                       \
        r = set_var(param, ptr, sz, _b);                    \
        if (r)                                              \
            YYABORT;                                        \
    } while (0)

    #define EVAL_APPLY_1(_f, _r, _a) do {                            \
        purc_variant_t vars[1] = {_a};                               \
        _r = _f(PURC_VARIANT_INVALID, PCA_TABLESIZE(vars), vars, false);    \
        purc_variant_unref(_a);                                      \
        if (_r == PURC_VARIANT_INVALID)                              \
            YYABORT;                                                 \
    } while (0)

    #define EVAL_APPLY_2(_f, _r, _a, _b) do {                        \
        purc_variant_t vars[2] = {_a, _b};                           \
        _r = _f(PURC_VARIANT_INVALID, PCA_TABLESIZE(vars), vars, false);    \
        purc_variant_unref(_a);                                      \
        purc_variant_unref(_b);                                      \
        if (_r == PURC_VARIANT_INVALID)                              \
            YYABORT;                                                 \
    } while (0)

    #define EVAL_INT(_r, _a) do {                                    \
        char   *ptr = (char*)_a[1];                                  \
        size_t  sz  = _a[0];                                         \
        char c  = ptr[sz];                                           \
        ptr[sz] = '\0';                                              \
        long long ll = atoll(ptr);                                   \
        ptr[sz] = c;                                                 \
        _r = purc_variant_make_longint(ll);                          \
        if (_r == PURC_VARIANT_INVALID)                              \
            YYABORT;                                                 \
    } while (0)

    #define EVAL_NUM(_r, _a) do {                                    \
        char   *ptr = (char*)_a[1];                                  \
        size_t  sz  = _a[0];                                         \
        char c  = ptr[sz];                                           \
        ptr[sz] = '\0';                                              \
        double d = atof(ptr);                                        \
        ptr[sz] = c;                                                 \
        _r = purc_variant_make_number(d);                            \
        if (_r == PURC_VARIANT_INVALID)                              \
            YYABORT;                                                 \
    } while (0)

    #define EVAL_VAR(_r, _a) do {                                    \
        if (!param->variables)                                       \
            YYABORT;                                                 \
        char   *ptr = (char*)_a[1];                                  \
        size_t  sz  = _a[0];                                         \
        char c  = ptr[sz];                                           \
        ptr[sz] = '\0';                                              \
        _r = purc_variant_object_get_by_ckey(param->variables, ptr); \
        ptr[sz] = c;                                                 \
        if (_r == PURC_VARIANT_INVALID) {                            \
            if (!param->v && !purc_variant_is_object(param->v))      \
                YYABORT;                                             \
            ptr[sz] = '\0';                                          \
            _r = purc_variant_object_get_by_ckey(param->v, ptr);     \
            ptr[sz] = c;                                             \
            if (_r == PURC_VARIANT_INVALID)                          \
                YYABORT;                                             \
        }                                                            \
        purc_variant_ref(_r);                                        \
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
%parse-param { struct logical_funcs *funcs }
%parse-param { struct pcdvobjs_logical_param *param }

%union { uintptr_t  sz_ptr[2]; }
%union { purc_variant_t v; }

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
| assignment
;

assignment:
  VAR '=' exp        { EVAL_ASSIGN($1, $3); EVAL_FREE($3); }
;

exp:
  term
| exp GE exp         { EVAL_APPLY_2(funcs->ge_f, $$, $1, $3); }
| exp LE exp         { EVAL_APPLY_2(funcs->le_f, $$, $1, $3); }
| exp EQ exp         { EVAL_APPLY_2(funcs->eq_f, $$, $1, $3); }
| exp NE exp         { EVAL_APPLY_2(funcs->ne_f, $$, $1, $3); }
| exp AND exp        { EVAL_APPLY_2(funcs->and_f, $$, $1, $3); }
| exp OR exp         { EVAL_APPLY_2(funcs->or_f, $$, $1, $3); }
| exp '>' exp        { EVAL_APPLY_2(funcs->gt_f, $$, $1, $3); }
| exp '<' exp        { EVAL_APPLY_2(funcs->lt_f, $$, $1, $3); }
| '!' exp %prec NEG  { EVAL_APPLY_1(funcs->not_f, $$, $2); }
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
    struct logical_funcs *funcs,          // match %parse-param
    struct pcdvobjs_logical_param *param, // match %parse-param
    const char *errsg
)
{
    // to implement it here
    (void)yylloc;
    (void)arg;
    (void)funcs;
    (void)param;
    fprintf(stderr, "(%d,%d)->(%d,%d): %s\n",
        yylloc->first_line, yylloc->first_column,
        yylloc->last_line, yylloc->last_column,
        errsg);
}

int pcdvobjs_logical_parse(const char *input,
        struct pcdvobjs_logical_param *param)
{
    int r;
    yyscan_t arg = {0};
    purc_variant_t logical = purc_dvobj_logical_new();
    if (logical == PURC_VARIANT_INVALID)
        return 1;

    struct logical_funcs funcs = {0};
    r = init_logical_funcs(&funcs, logical);
    if (r)
        return 1;

    yylex_init(&arg);
    // yyset_in(in, arg);
    // yyset_debug(debug, arg);
    yyset_extra(param, arg);
    yy_scan_string(input, arg);
    int ret =yyparse(arg, &funcs, param);
    yylex_destroy(arg);
    if (param->variables) {
        purc_variant_unref(param->variables);
        param->variables = NULL;
    }

    purc_variant_unref(logical);

    return ret ? 1 : 0;
}

