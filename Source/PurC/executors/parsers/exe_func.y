%code top {
/*
 * @file exe_func.y
 * @author
 * @date
 * @brief The implementation of public part for func.
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
    // here to include header files required for generated exe_func.tab.c
}

%code requires {
    struct exe_func_token {
        const char      *text;
        size_t           leng;
    };

    #define YYSTYPE       EXE_FUNC_YYSTYPE
    #define YYLTYPE       EXE_FUNC_YYLTYPE
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
        struct exe_func_param *param,      // match %parse-param
        const char *errsg
    );

    #define SET_RULE(_rule) do {                            \
        if (param) {                                        \
            param->rule = _rule;                            \
        } else {                                            \
            func_rule_release(&_rule);                      \
        }                                                   \
    } while (0)

    #define SET_NAME(_r, _name) do {                             \
        _r.name = strndup(_name.text, _name.leng);               \
        if (!_r.name) {                                          \
            func_rule_release(&_r);                              \
            YYABORT;                                             \
        }                                                        \
    } while (0)

    #define SET_NAMES(_r, _name, _module) do {                   \
        SET_NAME(_r, _name);                                     \
        _r.module_name = strndup(_module.text, _module.leng);    \
        if (!_r.module_name) {                                   \
            func_rule_release(&_r);                              \
            YYABORT;                                             \
        }                                                        \
    } while (0)
}

/* Bison declarations. */
%require "3.0.4"
%define api.pure full
%define api.token.prefix {TOK_EXE_FUNC_}
%define locations
%define parse.error verbose
%define parse.lac full
%define parse.trace true
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct exe_func_param *param }

// union members
%union { struct exe_func_token token; }
%union { char *str; }
%union { struct func_rule rule; }

%destructor { func_rule_release(&$$); } <rule>

%token FUNC CLASS
%token <token> NAME

%nterm <rule>  func_rule;



 /* %nterm <str>   args */ // non-terminal `input` use `str` to store
                           // token value as well

%% /* The grammar follows. */

input:
  func_rule  { SET_RULE($1); }
;

func_rule:
  FUNC  ':' NAME          { SET_NAME($$, $3); }
| FUNC  ':' NAME '@' NAME { SET_NAMES($$, $3, $5); }
;

%%

/* Called by yyparse on error. */
static void
yyerror(
    YYLTYPE *yylloc,                   // match %define locations
    yyscan_t arg,                      // match %param
    struct exe_func_param *param,      // match %parse-param
    const char *errsg
)
{
    // to implement it here
    (void)yylloc;
    (void)arg;
    (void)param;
    if (!param)
        return;
    int r = asprintf(&param->err_msg, "(%d,%d)->(%d,%d): %s",
        yylloc->first_line, yylloc->first_column,
        yylloc->last_line, yylloc->last_column - 1,
        errsg);
    (void)r;
}

int exe_func_parse(const char *input, size_t len,
        struct exe_func_param *param)
{
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
    PC_ASSERT(0);
    return ret ? -1 : 0;
}

