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
%}


%{
#include "${NAME}.tab.h"

#define MKT(x)    TOK_${NAME^^}_##x

#define PUSH(state)      yy_push_state(state, yyscanner)
#define POP()            yy_pop_state(yyscanner)

#define CHG(state)                           \\
do {                                         \\
    yy_pop_state(yyscanner);                 \\
    yy_push_state(state, yyscanner);         \\
} while (0)

#define TOP_STATE()                         \\
    ({  yy_push_state(INITIAL, yyscanner);  \\
        int _top = yy_top_state(yyscanner); \\
        yy_pop_state(yyscanner);            \\
        _top; })

#define C()                                     \\
do {                                            \\
    yylloc->last_column += strlen(yytext);      \\
} while (0)

#define L()                                     \\
do {                                            \\
    yylloc->last_line   += 1;                   \\
    yylloc->last_column  = 1;                   \\
} while (0)

#define R()                                       \\
do {                                              \\
    yylloc->first_column = yylloc->last_column ;  \\
    yylloc->first_line   = yylloc->last_line;     \\
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

["]     { C(); PUSH(STR); R(); return *yytext; }
[ \t]   { C(); R(); } /* eat */
\n      { L(); R(); } /* eat */
.       { C(); R(); return *yytext; } /* let bison to handle */

<STR>{
["]       { C(); POP(); R(); return *yytext; }
[^"\n]+   { C(); yylval->str=(yytext); R(); return MKT(STRING); }
\n        { L(); R(); return *yytext; } /* let bison to handle */
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
    // here to include header files required for generated ${NAME}.tab.c
}

%code requires {
    // related struct/function decls
    // especially, for struct ${NAME}_parse_param
    // and parse function for example:
    // int ${NAME}_parse(const char *input,
    //        struct ${NAME}_parse_param *param);
    // #include "${NAME}.h"
    // here we define them locally
    struct ${NAME}_parse_param {
        char      placeholder[0];
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
        struct ${NAME}_parse_param *param, // match %parse-param
        const char *errsg
    );
}

/* Bison declarations. */
%require "3.0.4"
%define api.prefix {${NAME}_yy}
%define api.pure full
%define api.token.prefix {TOK_${NAME^^}_}
%define locations
%define parse.error verbose
%defines
%verbose

%param { yyscan_t arg }
%parse-param { struct ${NAME}_parse_param *param }

%union { char *str; }      // union member
%token <str>  STRING       // token STRING use \`str\` to store semantic value
%destructor { free(\$\$); } <str> // destructor for \`str\`
%nterm <str> input         // non-terminal \`input\` use \`str\` to store
                           // semantic value as well


%% /* The grammar follows. */

input:
  %empty      { \$\$ = NULL; }
| args        { \$\$ = NULL; }
;

args:
  STRING      { free(\$1); }
| args STRING { free(\$2); }
;

%%

/* Called by yyparse on error. */
static void
yyerror(
    YYLTYPE *yylloc,                   // match %define locations
    yyscan_t arg,                      // match %param
    struct ${NAME}_parse_param *param, // match %parse-param
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

int ${NAME}_parse(const char *input,
        struct ${NAME}_parse_param *param)
{
    yyscan_t arg = {0};
    ${NAME}_yylex_init(&arg);
    // ${NAME}_yyset_in(in, arg);
    // ${NAME}_yyset_debug(debug, arg);
    // ${NAME}_yyset_extra(param, arg);
    ${NAME}_yy_scan_string(input, arg);
    int ret =${NAME}_yyparse(arg, param);
    ${NAME}_yylex_destroy(arg);
    return ret ? 1 : 0;
}

%%

EOF

if [ ! $? -eq 0 ]; then exit; fi

echo == yes ==

