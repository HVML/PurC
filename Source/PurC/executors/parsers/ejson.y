%code top {
/*
 * @file ejson.y
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
    // here to include header files required for generated ejson.tab.c
    #define _GNU_SOURCE
}

%code requires {
    #include <stddef.h>
    #include "purc-variant.h"
    // related struct/function decls
    // especially, for struct ejson_param
    // and parse function for example:
    // int ejson_parse(const char *input,
    //        struct ejson_param *param);
    // #include "ejson.h"
    // here we define them locally
    struct ejson_param {
        char      placeholder[0];

        purc_variant_t        var;
    };

    struct ejson_semantic {
        const char      *text;
        size_t           leng;
    };

    struct string_s {
        char             *str;
        size_t            len;
        size_t            sz;
    };

    #define YYSTYPE       EJSON_YYSTYPE
    #define YYLTYPE       EJSON_YYLTYPE
    typedef void *yyscan_t;

    int ejson_parse(const char *input,
            struct ejson_param *param);
}

%code provides {
}

%code {
    // generated header from flex
    // introduce yylex decl for later use
    #include "ejson.lex.h"

    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        struct ejson_param *param,       // match %parse-param
        const char *errsg
    );

    static inline purc_variant_t
    mk_integer(const char *s, size_t len)
    {
        char *end;
        char *ss = strndup(s, len);
        if (!ss)
            return PURC_VARIANT_INVALID;
        purc_variant_t vv = PURC_VARIANT_INVALID;
        char *p;
        if ((p=strcasestr(ss, "ul"))) {
            *p = '\0';
            unsigned long long int ull;
            ull = strtoull(ss, &end, 0);
            if (!end || !*end) {
                vv = purc_variant_make_ulongint(ull);
            }
        }
        else if ((p=strcasestr(ss, "l"))) {
            *p = '\0';
            long long int ll;
            ll = strtoull(ss, &end, 0);
            if (!end || !*end) {
                vv = purc_variant_make_longint(ll);
            }
        }
        free(ss);
        return vv;
    }

    static inline purc_variant_t
    mk_number(const char *s, size_t len)
    {
        char *end;
        char *ss = strndup(s, len);
        if (!ss)
            return PURC_VARIANT_INVALID;
        purc_variant_t vv = PURC_VARIANT_INVALID;
        char *p;
        if ((p=strcasestr(ss, "fl"))) {
            *p = '\0';
        }
        long double ld;
        ld = strtold(ss, &end);
        if (!end || !*end) {
            vv = purc_variant_make_longdouble(ld);
        }
        free(ss);
        return vv;
    }

    static inline void
    string_s_reset(struct string_s *ss)
    {
        free(ss->str);
        ss->str = NULL;
        ss->len = 0;
        ss->sz  = 0;
    }

    static inline int
    string_s_append_str(struct string_s *ss, const char *s, size_t len)
    {
        if (ss->len + len > ss->sz) {
            size_t n = (len + 31) / 32 * 32;
            n += ss->len;
            char *p = (char*)realloc(ss->str, n + 1);
            if (!p)
                return -1;
            ss->str = p;
            ss->sz  = n;
        }
        strncpy(ss->str + ss->len, s, len);
        ss->len += len;
        return 0;
    }

    static inline int
    string_s_append_chr(struct string_s *ss, const char c)
    {
        if (c=='\0')
            return -1;

        return string_s_append_str(ss, &c, 1);
    }

    static inline int
    string_s_append_uni(struct string_s *ss, const char *s, size_t len)
    {
        if (len != 5)
            return -1;

        char buf[5];
        ++s;
        --len;
        strncpy(buf, s, len);

        char *end;
        unsigned long int ul;
        ul = strtoul(buf, &end, 16);
        (void)ul;
        (void)ss;
        if (!end || !*end) {
            // convert UCP to utf-8
            abort();
        }

        return -1;
    }

    static purc_variant_t
    collect_str(struct string_s *ss)
    {
        purc_variant_t v;
        v = purc_variant_make_string_reuse_buff(ss->str, ss->len, true);
        if (v == PURC_VARIANT_INVALID)
            string_s_reset(ss);

        return v;
    }

    static inline int
    init_str(struct string_s *ss, const char *s, size_t len)
    {
        memset(ss, 0, sizeof(*ss));
        return string_s_append_str(ss, s, len);
    }

    static inline int
    init_chr(struct string_s *ss, const char c)
    {
        memset(ss, 0, sizeof(*ss));
        return string_s_append_chr(ss, c);
    }

    static inline int
    init_uni(struct string_s *ss, const char *s, size_t len)
    {
        memset(ss, 0, sizeof(*ss));
        return string_s_append_uni(ss, s, len);
    }

    static inline purc_variant_t
    collect_kvs(purc_variant_t vals)
    {
        size_t sz;
        if (!purc_variant_array_size(vals, &sz)) {
            purc_variant_unref(vals);
            return PURC_VARIANT_INVALID;
        }

        purc_variant_t k, v;
        k = v = PURC_VARIANT_INVALID;
        purc_variant_t obj = purc_variant_make_object(0, k, v);
        if (obj != PURC_VARIANT_INVALID) {
            for (size_t i=0; i<sz;) {
                purc_variant_t k = purc_variant_array_get(vals, i++);
                purc_variant_t v = purc_variant_array_get(vals, i++);

                bool ok = purc_variant_object_set(obj, k, v);
                if (!ok) {
                    purc_variant_unref(obj);
                    obj = PURC_VARIANT_INVALID;
                    break;
                }
            }
        }

        purc_variant_unref(vals);

        return obj;
    }

    static inline purc_variant_t
    append_kv(purc_variant_t vals, purc_variant_t kv)
    {
        purc_variant_t k = purc_variant_array_get(kv, 0);
        purc_variant_t v = purc_variant_array_get(kv, 1);

        if (!purc_variant_array_append(vals, k) ||
            !purc_variant_array_append(vals, v))
        {
            purc_variant_unref(vals);
            purc_variant_unref(kv);
            return PURC_VARIANT_INVALID;
        }

        return vals;
    }

    static inline purc_variant_t
    init_kv_kv(purc_variant_t k, purc_variant_t v)
    {
        purc_variant_t kv = purc_variant_make_array(2, k, v);

        purc_variant_unref(k);
        purc_variant_unref(k);

        return kv;
    }

    static inline purc_variant_t
    init_kv_id(const char *s, size_t len, purc_variant_t v)
    {
        char *ss = strndup(s, len);
        if (!ss) {
            purc_variant_unref(v);
            return PURC_VARIANT_INVALID;
        }

        purc_variant_t k = purc_variant_make_string_reuse_buff(ss, len, true);
        if (k == PURC_VARIANT_INVALID) {
            free(ss);
            purc_variant_unref(v);
            return PURC_VARIANT_INVALID;
        }

        return init_kv_kv(k, v);
    }

    static inline purc_variant_t
    init_kv_str(struct string_s *ss, purc_variant_t v)
    {
        purc_variant_t k = collect_str(ss);
        if (k == PURC_VARIANT_INVALID) {
            purc_variant_unref(v);
            return PURC_VARIANT_INVALID;
        }

        return init_kv_kv(k, v);
    }

    static inline purc_variant_t
    mk_vars(purc_variant_t v)
    {
        purc_variant_t vals = purc_variant_make_array(1, v);
        if (vals == PURC_VARIANT_INVALID)
            purc_variant_unref(v);
        return vals;
    }

    static inline purc_variant_t
    append_var(purc_variant_t vals, purc_variant_t v)
    {
        if (!purc_variant_array_append(vals, v)) {
            purc_variant_unref(v);
            purc_variant_unref(vals);
            return PURC_VARIANT_INVALID;
        }
        return vals;
    }

    #define MK_UNDEFINED(_r) do {                    \
        _r = purc_variant_make_undefined();          \
        if (_r == PURC_VARIANT_INVALID)              \
            YYABORT;                                 \
    } while (0)

    #define MK_NULL(_r) do {                         \
        _r = purc_variant_make_null();               \
        if (_r == PURC_VARIANT_INVALID)              \
            YYABORT;                                 \
    } while (0)

    #define MK_TRUE(_r) do {                         \
        _r = purc_variant_make_boolean(true);        \
        if (_r == PURC_VARIANT_INVALID)              \
            YYABORT;                                 \
    } while (0)

    #define MK_FALSE(_r) do {                        \
        _r = purc_variant_make_boolean(false);       \
        if (_r == PURC_VARIANT_INVALID)              \
            YYABORT;                                 \
    } while (0)

    #define MK_INTEGER(_r, _s) do {                  \
        _r = mk_integer(_s.text, _s.leng);           \
        if (_r == PURC_VARIANT_INVALID)              \
            YYABORT;                                 \
    } while (0)

    #define MK_NUMBER(_r, _s) do {                  \
        _r = mk_number(_s.text, _s.leng);           \
        if (_r == PURC_VARIANT_INVALID)             \
            YYABORT;                                \
    } while (0)

    #define MK_EMPTY_STR(_r) do {                         \
        _r = purc_variant_make_atom_string("", false);    \
        if (_r == PURC_VARIANT_INVALID)                   \
            YYABORT;                                      \
    } while (0)

    #define COLLECT_STR(_r, _s) do {                      \
        _r = collect_str(&_s);                            \
        if (_r == PURC_VARIANT_INVALID)                   \
            YYABORT;                                      \
    } while (0);

    #define INIT_STR(_r, _s) do {                         \
        if (init_str(&_r, _s.text, _s.leng))              \
            YYABORT;                                      \
    } while (0);

    #define INIT_CHR(_r, _s) do {                         \
        if (init_chr(&_r, _s))                            \
            YYABORT;                                      \
    } while (0);

    #define INIT_UNI(_r, _s) do {                         \
        if (init_uni(&_r, _s.text, _s.leng))              \
            YYABORT;                                      \
    } while (0);

    #define APPEND_STR(_r, _l, _s) do {                   \
        if (string_s_append_str(&_l, _s.text, _s.leng))   \
            YYABORT;                                      \
        _r = _l;                                          \
    } while (0);

    #define APPEND_CHR(_r, _l, _s) do {                   \
        if (string_s_append_chr(&_l, _s))                 \
            YYABORT;                                      \
        _r = _l;                                          \
    } while (0);

    #define APPEND_UNI(_r, _l, _s) do {                   \
        if (string_s_append_uni(&_l, _s.text, _s.leng))   \
            YYABORT;                                      \
        _r = _l;                                          \
    } while (0);

    #define MK_EMPTY_OBJ(_r) do {                         \
        purc_variant_t k,v;                               \
        k = v = PURC_VARIANT_INVALID;                     \
        _r = purc_variant_make_object(0, k, v);           \
        if (_r == PURC_VARIANT_INVALID)                   \
            YYABORT;                                      \
    } while (0)

    #define COLLECT_KVS(_r, _s) do {                      \
        _r = collect_kvs(_s);                             \
        if (_r == PURC_VARIANT_INVALID)                   \
            YYABORT;                                      \
    } while (0)

    #define APPEND_KV(_r, _l, _s) do {                    \
        _r = append_kv(_l, _s);                           \
        if (_r == PURC_VARIANT_INVALID)                   \
            YYABORT;                                      \
    } while (0)

    #define INIT_KV_ID(_r, _l, _s) do {                   \
        _r = init_kv_id(_l.text, _l.leng, _s);            \
        if (_r == PURC_VARIANT_INVALID)                   \
            YYABORT;                                      \
    } while (0)

    #define INIT_KV_STR(_r, _l, _s) do {                  \
        _r = init_kv_str(&_l, _s);                        \
        if (_r == PURC_VARIANT_INVALID)                   \
            YYABORT;                                      \
    } while (0)

    #define MK_EMPTY_ARR(_r) do {                         \
        purc_variant_t v;                                 \
        v = PURC_VARIANT_INVALID;                         \
        _r = purc_variant_make_array(0, v);               \
        if (_r == PURC_VARIANT_INVALID)                   \
            YYABORT;                                      \
    } while (0)

    #define MK_VARS(_r, _s) do {                          \
        _r = mk_vars(_s);                                 \
        if (_r == PURC_VARIANT_INVALID)                   \
            YYABORT;                                      \
    } while (0)

    #define APPEND_VAR(_r, _l, _s) do {                   \
        _r = append_var(_l, _s);                          \
        if (_r == PURC_VARIANT_INVALID)                   \
            YYABORT;                                      \
    } while (0)
}

/* Bison declarations. */
%require "3.0.4"
%define api.prefix {ejson_yy}
%define api.pure full
%define api.token.prefix {TOK_EJSON_}
%define locations
%define parse.error verbose
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct ejson_param *param }

%union { struct ejson_semantic sval; } // union member
%union { char c; }                     // union member
%union { purc_variant_t v; }
%union { struct string_s ss; }

%destructor { purc_variant_unref($$); } <v>

%token TQ T_UNDEFINED T_NULL T_TRUE T_FALSE
%token <sval>  STR UNI INTEGER NUMBER ID
%token <c>     CHR

%nterm <v> variant str
%nterm <v> obj kvs kv
%nterm <v> arr variants
%nterm <ss> string


%% /* The grammar follows. */

input:
  %empty             { param->var = PURC_VARIANT_INVALID; }
| variant            { param->var = $1; }
;

variant:
  T_UNDEFINED        { MK_UNDEFINED($$); }
| T_NULL             { MK_NULL($$); }
| T_TRUE             { MK_TRUE($$); }
| T_FALSE            { MK_FALSE($$); }
| INTEGER            { MK_INTEGER($$, $1);}
| NUMBER             { MK_NUMBER($$, $1); }
| str                { $$ = $1; }
| obj                { $$ = $1; }
| arr                { $$ = $1; }
;

str:
  TQ TQ              { MK_EMPTY_STR($$); }
| '"' '"'            { MK_EMPTY_STR($$); }
| '\'' '\''          { MK_EMPTY_STR($$); }
| TQ string TQ       { COLLECT_STR($$, $2); }
| '"' string '"'     { COLLECT_STR($$, $2); }
| '\'' string '\''   { COLLECT_STR($$, $2); }
;

string:
  STR          { INIT_STR($$, $1); }
| CHR          { INIT_CHR($$, $1); }
| UNI          { INIT_UNI($$, $1); }
| string STR   { APPEND_STR($$, $1, $2); }
| string CHR   { APPEND_CHR($$, $1, $2); }
| string UNI   { APPEND_UNI($$, $1, $2); }
;

obj:
  '{' '}'           { MK_EMPTY_OBJ($$); }
| '{' kvs '}'       { COLLECT_KVS($$, $2); }
| '{' kvs ',' '}'   { COLLECT_KVS($$, $2); }
;

kvs:
  kv                { $$ = $1; }
| kvs ',' kv        { APPEND_KV($$, $1, $3); }
;

kv:
  ID ':' variant                   { INIT_KV_ID($$, $1, $3); }
| '"' string '"' ':' variant       { INIT_KV_STR($$, $2, $5); }
| '\'' string '\'' ':' variant     { INIT_KV_STR($$, $2, $5); }
;

arr:
  '[' ']'                  { MK_EMPTY_ARR($$); }
| '[' variants ']'         { $$ = $2; }
| '[' variants ',' ']'     { $$ = $2; }
;

variants:
  variant                  { MK_VARS($$, $1); }
| variants ',' variant     { APPEND_VAR($$, $1, $3); }
;

%%

/* Called by yyparse on error. */
static void
yyerror(
    YYLTYPE *yylloc,                   // match %define locations
    yyscan_t arg,                      // match %param
    struct ejson_param *param,       // match %parse-param
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

int ejson_parse(const char *input,
        struct ejson_param *param)
{
    yyscan_t arg = {0};
    ejson_yylex_init(&arg);
    // ejson_yyset_in(in, arg);
    // ejson_yyset_debug(debug, arg);
    // ejson_yyset_extra(param, arg);
    ejson_yy_scan_string(input, arg);
    int ret =ejson_yyparse(arg, param);
    ejson_yylex_destroy(arg);
    return ret ? 1 : 0;
}

