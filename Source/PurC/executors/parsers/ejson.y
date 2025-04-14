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
    struct ejson_param {
        char      placeholder[0];
        int debug_flex;
        int debug_bison;

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

    struct kv_s {
        purc_variant_t    k;
        purc_variant_t    v;
    };

    struct kvs_s {
        struct kv_s      *kvs;
        size_t            nr;
        size_t            sz;
    };

    #define YYSTYPE       EJSON_YYSTYPE
    #define YYLTYPE       EJSON_YYLTYPE
    #ifndef YY_TYPEDEF_YY_SCANNER_T
    #define YY_TYPEDEF_YY_SCANNER_T
    typedef void* yyscan_t;
    #endif

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

    static inline purc_variant_t
    mk_by_id(const char *s, size_t len)
    {
        return purc_variant_make_string_ex(s, len, true);
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
            if (!p) {
                string_s_reset(ss);
                return -1;
            }
            ss->str = p;
            ss->sz  = n;
        }
        strncpy(ss->str + ss->len, s, len);
        ss->len += len;
        ss->str[ss->len] = '\0';
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
            abort();  // TODO: Not implemented yet
        }

        abort();  // TODO: Not implemented yet
        return -1;
    }

    static purc_variant_t
    collect_str(struct string_s *ss)
    {
        purc_variant_t v;
        v = purc_variant_make_string_reuse_buff(ss->str, ss->len + 1, true);
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

    static inline void
    init_kv_s(struct kv_s *kv)
    {
        kv->k = PURC_VARIANT_INVALID;
        kv->v = PURC_VARIANT_INVALID;
    }

    static inline void
    release_kv_s(struct kv_s *kv)
    {
        PURC_VARIANT_SAFE_CLEAR(kv->k);
        PURC_VARIANT_SAFE_CLEAR(kv->v);
    }

    static inline struct kvs_s*
    kvs_create(void)
    {
        struct kvs_s *kvs = (struct kvs_s*)calloc(1, sizeof(*kvs));
        if (!kvs)
            return NULL;

        return kvs;
    }

    static inline void
    kvs_release(struct kvs_s *kvs)
    {
        if (kvs) {
            for (size_t i=0; i<kvs->nr; ++i) {
                struct kv_s *kv = kvs->kvs + i;
                release_kv_s(kv);
            }
            free(kvs->kvs);
            kvs->kvs = NULL;
        }
    }

    static inline int
    kvs_append(struct kvs_s *kvs, struct kv_s *kv)
    {
        if (kvs->nr == kvs->sz) {
            size_t sz = kvs->sz + 16;
            struct kv_s *kv;
            kv = (struct kv_s*)realloc(kvs->kvs, sz * sizeof(*kv));
            if (!kv)
                return -1;
            kvs->kvs = kv;
            kvs->sz = sz;
        }
        kvs->kvs[kvs->nr++] = *kv;
        return 0;
    }

    static inline void
    kvs_destroy(struct kvs_s *kvs)
    {
        if (kvs) {
            kvs_release(kvs);
            free(kvs);
        }
    }

    static inline purc_variant_t
    collect_kvs(struct kvs_s *kvs)
    {
        purc_variant_t obj = purc_variant_make_object(0,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        bool ok = true;
        if (obj != PURC_VARIANT_INVALID) {
            for (size_t i=0; i<kvs->nr; ++i) {
                struct kv_s *kv = kvs->kvs + i;
                if (!purc_variant_object_set(obj, kv->k, kv->v)) {
                    ok = false;
                    break;
                }
            }
        }

        kvs_destroy(kvs);

        if (!ok) {
            PURC_VARIANT_SAFE_CLEAR(obj);
        }

        return obj;
    }

    static inline struct kvs_s*
    mk_kvs(struct kv_s *kv)
    {
        struct kvs_s *kvs = kvs_create();
        if (!kvs) {
            release_kv_s(kv);
            return NULL;
        }
        if (kvs_append(kvs, kv)) {
            release_kv_s(kv);
            kvs_destroy(kvs);
            return NULL;
        }
        return kvs;
    }

    static inline struct kvs_s*
    append_kv(struct kvs_s *kvs, struct kv_s *kv)
    {
        if (kvs_append(kvs, kv)) {
            release_kv_s(kv);
            return NULL;
        }
        return kvs;
    }

    static inline int
    init_kv_id(struct kv_s *kv, const char *s, size_t len, purc_variant_t v)
    {
        init_kv_s(kv);
        char *ss = strndup(s, len);
        if (!ss) {
            release_kv_s(kv);
            return -1;
        }

        purc_variant_t k = purc_variant_make_string_reuse_buff(ss, len + 1, true);
        if (k == PURC_VARIANT_INVALID) {
            free(ss);
            release_kv_s(kv);
            return -1;
        }

        kv->k = k;
        kv->v = v;

        return 0;
    }

    static inline int
    init_kv_str(struct kv_s *kv, struct string_s *ss, purc_variant_t v)
    {
        init_kv_s(kv);
        purc_variant_t k = collect_str(ss);
        if (k == PURC_VARIANT_INVALID) {
            init_kv_s(kv);
            purc_variant_unref(v);
            return -1;
        }

        kv->k = k;
        kv->v = v;

        return 0;
    }

    static inline purc_variant_t
    mk_vars(purc_variant_t v)
    {
        purc_variant_t vals = purc_variant_make_array(1, v);
        purc_variant_unref(v);
        return vals;
    }

    static inline purc_variant_t
    append_var(purc_variant_t vals, purc_variant_t v)
    {
        bool ok = purc_variant_array_append(vals, v);
        purc_variant_unref(v);
        if (!ok) {
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

    #define MK_BY_ID(_r, _s) do {                   \
        _r = mk_by_id(_s.text, _s.leng);            \
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

    #define MK_KVS(_r, _s) do {                           \
        _r = mk_kvs(_s);                                  \
        if (_r == NULL)                                   \
            YYABORT;                                      \
    } while (0)
    #define APPEND_KV(_r, _l, _s) do {                    \
        _r = append_kv(_l, _s);                           \
        if (_r == NULL)                                   \
            YYABORT;                                      \
    } while (0)

    #define INIT_KV_ID(_r, _l, _s) do {                   \
        if (init_kv_id(_r, _l.text, _l.leng, _s))         \
            YYABORT;                                      \
    } while (0)

    #define INIT_KV_STR(_r, _l, _s) do {                  \
        if (init_kv_str(_r, &_l, _s))                     \
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

    #define MK_EMPTY_SET(_r) do {                                         \
        purc_variant_t _k = PURC_VARIANT_INVALID;                         \
        purc_variant_t _v = PURC_VARIANT_INVALID;                         \
        _r = purc_variant_make_set(0, _k, _v);                            \
        if (_r == PURC_VARIANT_INVALID)                                   \
            YYABORT;                                                      \
    } while (0)

    #define MK_SET(_r, _k, _a) do {                                       \
        _r = purc_variant_make_set(0, _k, PURC_VARIANT_INVALID);          \
        if (_k != PURC_VARIANT_INVALID)                                   \
            purc_variant_unref(_k);                                       \
        if (_a == PURC_VARIANT_INVALID)                                   \
            break;                                                        \
        bool ok = true;                                                   \
        purc_variant_t _v;                                                \
        size_t _idx;                                                      \
        ssize_t  t;                                                       \
        foreach_value_in_variant_array(_a, _v, _idx)                      \
            (void)_idx;                                                   \
            t = purc_variant_set_add(_r, _v, PCVRNT_CR_METHOD_OVERWRITE); \
            if (t == -1)                                                  \
                break;                                                    \
        end_foreach;                                                      \
        purc_variant_unref(_a);                                           \
        if (!ok) {                                                        \
            purc_variant_unref(_r);                                       \
            YYABORT;                                                      \
        }                                                                 \
    } while (0)                                                           \

    #define MK_STRING(_r, _id) do {                                       \
        _r = purc_variant_make_string_ex(_id.text, _id.leng, true);       \
        if (_r == PURC_VARIANT_INVALID)                                   \
            YYABORT;                                                      \
    } while (0)

    #define MK_OBJS(_r, _obj) do {                        \
        _r = purc_variant_make_array(1, _obj);            \
        purc_variant_unref(_obj);                         \
        if (_r == PURC_VARIANT_INVALID)                   \
            YYABORT;                                      \
    } while (0)

    #define OBJS_APPEND(_r, _objs, _obj) do {                   \
        bool _ok = purc_variant_array_append(_objs, _obj);      \
        purc_variant_unref(_obj);                               \
        if (!_ok) {                                             \
            purc_variant_unref(_objs);                          \
            YYABORT;                                            \
        }                                                       \
        _r = _objs;                                             \
    } while (0)

    #define SET_NULL() do {                               \
        param->var = purc_variant_make_null();            \
        if (param->var == PURC_VARIANT_INVALID) {         \
            YYABORT;                                      \
        }                                                 \
    } while (0)

    #define MK_EMPTY_TUPLE(_r) do {                                       \
        _r = purc_variant_make_tuple(0, NULL);                            \
        if (_r == PURC_VARIANT_INVALID)                                   \
            YYABORT;                                                      \
    } while (0)

    #define MK_TUPLE(_r, _a) do {                                         \
        _r = purc_variant_make_tuple(0, NULL);                            \
        if (_r == PURC_VARIANT_INVALID)                                   \
            YYABORT;                                                      \
        if (_a == PURC_VARIANT_INVALID)                                   \
            break;                                                        \
        bool ok = true;                                                   \
        purc_variant_t _v;                                                \
        size_t _idx;                                                      \
        foreach_value_in_variant_array(_a, _v, _idx)                      \
            ok = purc_variant_tuple_set(_r, _idx, _v);                    \
            if (!ok)                                                      \
                break;                                                    \
        end_foreach;                                                      \
        purc_variant_unref(_a);                                           \
        if (!ok) {                                                        \
            purc_variant_unref(_r);                                       \
            YYABORT;                                                      \
        }                                                                 \
    } while (0)                                                           \
}

/* Bison declarations. */
%require "3.0.4"
%define api.pure full
%define api.token.prefix {TOK_EJSON_}
%define locations
%define parse.error verbose
%define parse.lac full
%define parse.trace true
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct ejson_param *param }

%union { struct ejson_semantic sval; } // union member
%union { char c; }                     // union member
%union { char *str; }
%union { purc_variant_t v; }
%union { struct string_s ss; }
%union { struct kvs_s *kvs; }
%union { struct kv_s kv; }

%destructor { purc_variant_unref($$); } <v>
%destructor { kvs_destroy($$); } <kvs>
%destructor { release_kv_s(&$$); } <kv>

%token TQ T_UNDEFINED T_NULL T_TRUE T_FALSE
%token <sval>  STR UNI INTEGER NUMBER ID
%token <c>     CHR

%nterm <v>   var str
%nterm <v>   obj
%nterm <v>   tuple
%nterm <kvs> kvs
%nterm <kv>  kv
%nterm <v>   arr vars
%nterm <v>   set set_key
%nterm <ss>  string


%% /* The grammar follows. */

input:
  %empty             { SET_NULL(); }
| var                { param->var = $1; }
;

var:
  T_UNDEFINED        { MK_UNDEFINED($$); }
| T_NULL             { MK_NULL($$); }
| T_TRUE             { MK_TRUE($$); }
| T_FALSE            { MK_FALSE($$); }
| INTEGER            { MK_INTEGER($$, $1);}
| NUMBER             { MK_NUMBER($$, $1); }
| ID                 { MK_BY_ID($$, $1); }
| str                { $$ = $1; }
| obj                { $$ = $1; }
| arr                { $$ = $1; }
| set                { $$ = $1; }
| tuple              { $$ = $1; }
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
  kv                { MK_KVS($$, &$1); }
| kvs ',' kv        { APPEND_KV($$, $1, &$3); }
;

kv:
  ID ':' var                   { INIT_KV_ID(&$$, $1, $3); }
| '"' string '"' ':' var       { INIT_KV_STR(&$$, $2, $5); }
;

arr:
  '[' ']'              { MK_EMPTY_ARR($$); }
| '[' vars ']'         { $$ = $2; }
;

vars:
  var              { MK_VARS($$, $1); }
| vars ','         { $$ = $1; }
| vars ',' var     { APPEND_VAR($$, $1, $3); }
;

set:
  '[' '!' ']'                   { MK_EMPTY_SET($$); }
| '[' '!' ',' vars ']'          { MK_SET($$, PURC_VARIANT_INVALID, $4); }
| '[' '!' set_key  ']'          { MK_SET($$, $3, PURC_VARIANT_INVALID); }
| '[' '!' set_key ',' vars ']'  { MK_SET($$, $3, $5); }
;

set_key:
  ID                            { MK_STRING($$, $1); }
| '"' string '"'                { COLLECT_STR($$, $2); }
;

tuple:
  '(' ')'                       { MK_EMPTY_TUPLE($$); }
| '(' vars ')'                  { MK_TUPLE($$, $2); }
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
    PC_ERROR("(%d,%d)->(%d,%d): %s\n",
        yylloc->first_line, yylloc->first_column,
        yylloc->last_line, yylloc->last_column,
        errsg);
}

int ejson_parse(const char *input,
        struct ejson_param *param)
{
    size_t len = strlen(input);
    yyscan_t arg = {0};
    yylex_init(&arg);
    // yyset_in(in, arg);
    int debug_flex = param ? param->debug_flex : 0;
    int debug_bison = param ? param->debug_bison: 0;
    yyset_debug(debug_flex, arg);
    yydebug = debug_bison;
    // yyset_extra(param, arg);
    yy_scan_bytes(input ? input : "", input ? len : 0, arg);
    int ret =yyparse(arg, param);
    yylex_destroy(arg);
    return ret ? -1 : 0;
}

purc_variant_t
pcejson_parser_parse_string(const char *str, int debug_flex, int debug_bison)
{
    struct ejson_param param;
    param.var = PURC_VARIANT_INVALID;
    param.debug_flex = debug_flex;
    param.debug_bison = debug_bison;

    int r = ejson_parse(str, &param);

    if (r)
        return PURC_VARIANT_INVALID;

    return param.var;
}

