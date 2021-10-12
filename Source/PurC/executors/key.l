%{
/*
 * @file key.l
 * @author
 * @date
 * @brief The implementation of public part for key.
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
#include "key.tab.h"

#define MKT(x)    TOK_KEY_##x

#define PUSH(state)      yy_push_state(state, yyscanner)
#define POP()            yy_pop_state(yyscanner)

#define CHG(state) do {                           \
    yy_pop_state(yyscanner);                      \
    yy_push_state(state, yyscanner);              \
} while (0)

#define TOP_STATE()                               \
    ({  yy_push_state(INITIAL, yyscanner);        \
        int _top = yy_top_state(yyscanner);       \
        yy_pop_state(yyscanner);                  \
        _top; })

#define C() do {                                  \
    yylloc->last_column += strlen(yytext);        \
} while (0)

#define L() do {                                  \
    yylloc->last_line   += 1;                     \
    yylloc->last_column  = 1;                     \
} while (0)

#define R()                                       \
do {                                              \
    yylloc->first_column = yylloc->last_column ;  \
    yylloc->first_line   = yylloc->last_line;     \
} while (0)

#define SET_STR() do {                            \
    yylval->token.text = yytext;                  \
    yylval->token.leng = yyleng;                  \
} while (0)

#define SET_CHR(chr) do {                         \
    yylval->token.text = chr;                     \
    yylval->token.leng = 1;                       \
} while (0)

%}

%option prefix="key_yy"
%option bison-bridge bison-locations reentrant
%option noyywrap noinput nounput
%option verbose debug
%option stack
%option nodefault
%option warn
%option perf-report
%option 8bit

%x LIKE
%x SQ REG
%x ESCAPED_SQ ESCAPED_REG
%x AFTER_SQ
%x SQ_SUFFIX

%%

<<EOF>> { int state = TOP_STATE();
          if (state != INITIAL) return -1;
          yyterminate(); }

KEY     { C(); R(); return MKT(KEY); }
LIKE    { C(); PUSH(LIKE); R(); return MKT(LIKE); }
[']     { C(); PUSH(SQ); R(); return MKT(SQ); }
ALL     { C(); R(); return MKT(ALL); }
FOR     { C(); R(); return MKT(FOR); }
VALUE   { C(); R(); return MKT(VALUE); }
KV      { C(); R(); return MKT(KV); }
[:,]    { C(); R(); return *yytext; }
[/]     { C(); PUSH(REG); R(); return '/'; }
[ \t]   { C(); R(); } /* eat */
\n      { L(); R(); return *yytext; } /* let bison to handle */
.       { C(); R(); return *yytext; } /* let bison to handle */

<LIKE>{
[']     { C(); CHG(SQ); R(); return MKT(SQ); }
[/]     { C(); CHG(REG); R(); return '/'; }
[ \t]   { C(); R(); } /* eat */
\n      { L(); R(); return *yytext; } /* let bison to handle */
.       { C(); R(); return *yytext; } /* let bison to handle */
}

<SQ>{
[']       { C(); CHG(AFTER_SQ); R(); return MKT(SQ); }
[^'\\\n]+ { C(); SET_STR(); R(); return MKT(STR); }
\\        { C(); PUSH(ESCAPED_SQ); R(); }
\n        { L(); R(); return *yytext; } /* let bison to handle */
}

<ESCAPED_SQ>{
[']       { C(); SET_CHR("'"); POP(); R(); return MKT(CHR); }
\n        { L(); R(); return *yytext; } /* let bison to handle */
.         { C(); R(); return *yytext; } /* let bison to handle */
}

<AFTER_SQ>{
[/]       { C(); CHG(SQ_SUFFIX); R(); return *yytext; }
\n        { L(); R(); return *yytext; } /* let bison to handle */
.         { C(); POP(); R(); yyless(0); } /* rescan */
}

<SQ_SUFFIX>{
[isc]          { C(); R(); return *yytext; }
[[:digit:]]+   { C(); SET_STR(); R(); return MKT(INTEGER); }
\n        { L(); R(); return *yytext; } /* let bison to handle */
.         { C(); POP(); R(); yyless(0); } /* rescan */
}

<REG>{
[/]       { C(); POP(); R(); return *yytext; }
[^/\\\n]+ { C(); SET_STR(); R(); return MKT(STR); }
\\        { C(); PUSH(ESCAPED_REG); R(); }
\n        { L(); R(); return *yytext; } /* let bison to handle */
}

<ESCAPED_REG>{
[/]       { C(); SET_CHR("/"); POP(); R(); return MKT(CHR); }
\n        { L(); R(); return *yytext; } /* let bison to handle */
.         { C(); R(); return *yytext; } /* let bison to handle */
}

%%

