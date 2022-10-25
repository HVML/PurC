#!/bin/bash

set -u

RELPATH=$1
NAME=$2



mkdir -p "${RELPATH}" &&
cat > "${RELPATH}/CMakeLists.txt" << EOF
# just leave it blank as is
EOF

if [ ! $? -eq 0 ]; then exit; fi

cat > "${RELPATH}/${NAME}.l" << EOF
%{
/*
 * @file ${NAME}.l
 * @author
 * @date
 * @brief The implementation of public part for ${NAME}.
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
%}


%{
#include "${NAME}.tab.h"

#define MKT(x)    TOK_${NAME^^}_##x

#define PUSH(state)      yy_push_state(state, yyscanner)
#define POP()            yy_pop_state(yyscanner)

#define CHG(state) do {                           \\
    yy_pop_state(yyscanner);                      \\
    yy_push_state(state, yyscanner);              \\
} while (0)

#define TOP_STATE()                               \\
    ({  yy_push_state(INITIAL, yyscanner);        \\
        int _top = yy_top_state(yyscanner);       \\
        yy_pop_state(yyscanner);                  \\
        _top; })

#define R() do {                                  \\
    yylloc->first_column = yylloc->last_column ;  \\
    yylloc->first_line   = yylloc->last_line;     \\
} while (0)

#define L() do {                                  \\
    yylloc->last_line   += 1;                     \\
    yylloc->last_column  = 1;                     \\
} while (0)

#define C()                                       \\
do {                                              \\
    yylloc->last_column += yyleng;                \\
} while (0)

#define SET_STR() do {                            \\
    yylval->token.text = yytext;                  \\
    yylval->token.leng = yyleng;                  \\
} while (0)

#define SET_CHR(chr) do {                         \\
    yylval->c = chr;                              \\
} while (0)

%}

%option prefix="${NAME}_yy"
%option bison-bridge bison-locations reentrant
%option noyywrap noinput nounput
%option verbose debug
%option stack
%option nodefault
%option warn
%option perf-report
%option 8bit

%x STR

%%

<<EOF>> { int state = TOP_STATE();
          if (state != INITIAL) return -1;
          yyterminate(); }

["]     { R(); PUSH(STR); C(); return *yytext; }
[ \t]   { R(); C(); } /* eat */
\n      { R(); L(); } /* eat */
.       { R(); C(); return *yytext; } /* let bison to handle */

<STR>{
["]       { R(); POP(); C(); return *yytext; }
[^"\n]+   { R(); SET_STR(); C(); return MKT(STR); }
\n        { R(); L(); return *yytext; } /* let bison to handle */
}

%%

EOF

if [ ! $? -eq 0 ]; then exit; fi

cat > "${RELPATH}/${NAME}.y" << EOF
%code top {
/*
 * @file ${NAME}.y
 * @author
 * @date
 * @brief The implementation of public part for ${NAME}.
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
    // here to include header files required for generated ${NAME}.tab.c
}

%code requires {
    #ifdef _GNU_SOURCE
    #undef _GNU_SOURCE
    #endif
    #define _GNU_SOURCE
    #include <stdio.h>
    #include <stddef.h>
    // related struct/function decls
    // especially, for struct ${NAME}_param
    // and parse function for example:
    // int ${NAME}_parse(const char *input,
    //        struct ${NAME}_param *param);
    // #include "${NAME}.h"
    // here we define them locally
    struct ${NAME}_param {
        char      placeholder[0];
    };

    struct ${NAME}_token {
        const char      *text;
        size_t           leng;
    };

    #define YYSTYPE       ${NAME^^}_YYSTYPE
    #define YYLTYPE       ${NAME^^}_YYLTYPE
    typedef void *yyscan_t;
}

%code provides {
}

%code {
    // generated header from flex
    // introduce yylex decl for later use
    #include "${NAME}.lex.h"

    static void yyerror(
        YYLTYPE *yylloc,                   // match %define locations
        yyscan_t arg,                      // match %param
        char **err_msg,                    // match %parse-param
        struct ${NAME}_param *param,       // match %parse-param
        const char *errsg
    );

    #define SET_ARGS(_r, _a) do {              \\
        _r = strndup(_a.text, _a.leng);        \\
        if (!_r)                               \\
            YYABORT;                           \\
    } while (0)

    #define APPEND_ARGS(_r, _a, _b) do {       \\
        size_t len = strlen(_a);               \\
        size_t n   = len + _b.leng;            \\
        char *s = (char*)realloc(_a, n+1);     \\
        if (!s) {                              \\
            free(_r);                          \\
            YYABORT;                           \\
        }                                      \\
        memcpy(s+len, _b.text, _b.leng);       \\
        _r = s;                                \\
    } while (0)
}

/* Bison declarations. */
%require "3.0.4"
%define api.prefix {${NAME}_yy}
%define api.pure full
%define api.token.prefix {TOK_${NAME^^}_}
%define locations
%define parse.error verbose
%define parse.lac full
%defines
%verbose

%param { yyscan_t arg }
%parse-param { char **err_msg }
%parse-param { struct ${NAME}_param *param }

// union members
%union { struct ${NAME}_token token; }
%union { char *str; }

%destructor { free(\$\$); } <str> // destructor for \`str\`

%token <token>  STR        // token STR use \`str\` to store token value
%nterm <str>   args        // non-terminal \`input\` use \`str\` to store
                           // token value as well


%% /* The grammar follows. */

input:
  %empty
| args            { free(\$1); }
;

args:
  '"' STR  '"'    { SET_ARGS(\$\$, \$2); }
| args STR        { APPEND_ARGS(\$\$, \$1, \$2); }
;

%%

/* Called by yyparse on error. */
static void
yyerror(
    YYLTYPE *yylloc,                   // match %define locations
    yyscan_t arg,                      // match %param
    char **err_msg,                    // match %parse-param
    struct ${NAME}_param *param,       // match %parse-param
    const char *errsg
)
{
    // to implement it here
    (void)yylloc;
    (void)arg;
    (void)err_msg;
    (void)param;
    int r = asprintf(&param->err_msg, "(%d,%d)->(%d,%d): %s",
        yylloc->first_line, yylloc->first_column,
        yylloc->last_line, yylloc->last_column - 1,
        errsg);
    (void)r;
}

int ${NAME}_parse(const char *input,
        char **err_msg,
        struct ${NAME}_param *param)
{
    yyscan_t arg = {0};
    ${NAME}_yylex_init(&arg);
    // ${NAME}_yyset_in(in, arg);
    // ${NAME}_yyset_debug(1, arg);
    // ${NAME}_yyset_extra(param, arg);
    ${NAME}_yy_scan_string(input, arg);
    int ret =${NAME}_yyparse(arg, err_msg, param);
    ${NAME}_yylex_destroy(arg);
    return ret ? 1 : 0;
}

EOF

if [ ! $? -eq 0 ]; then exit; fi

echo == yes ==

