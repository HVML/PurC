%{
/*
 * @file exe_filter.l
 * @author
 * @date
 * @brief The implementation of public part for exe_filter.
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
#include "exe_filter.tab.h"

#define MKT(x)    TOK_EXE_FILTER_##x

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

#define R() do {                                  \
    yylloc->first_column = yylloc->last_column ;  \
    yylloc->first_line   = yylloc->last_line;     \
} while (0)

#define L() do {                                  \
    yylloc->last_line   += 1;                     \
    yylloc->last_column  = 1;                     \
} while (0)

#define C()                                       \
do {                                              \
    yylloc->last_column += yyleng;                \
} while (0)

#define SET_STR() do {                            \
    yylval->token.text = yytext;                  \
    yylval->token.leng = yyleng;                  \
} while (0)

#define SET_CHR(chr) do {                         \
    yylval->c = chr;                              \
} while (0)

%}

%option prefix="exe_filter_yy"
%option bison-bridge bison-locations reentrant
%option noyywrap noinput nounput
%option verbose
%option debug
%option stack
%option nodefault
%option warn
%option perf-report
%option 8bit

%x IN_STR IN_REGEXP IN_LIKE
%x AFTER_INT

SP       [ \t]
LN       [\r\n]|[\f\r\n]
INTEGER  [0-9]+

%%

<<EOF>> { int state = TOP_STATE();
          if (state != INITIAL) return -1;
          yyterminate(); }

FILTER    { R(); C(); return MKT(FILTER); }
LT        { R(); C(); return MKT(LT); }
GT        { R(); C(); return MKT(GT); }
LE        { R(); C(); return MKT(LE); }
GE        { R(); C(); return MKT(GE); }
NE        { R(); C(); return MKT(NE); }
EQ        { R(); C(); return MKT(EQ); }
LIKE      { R(); PUSH(IN_LIKE); C(); return MKT(LIKE); }
ALL       { R(); C(); return MKT(ALL); }
FOR       { R(); C(); return MKT(FOR); }
KEY       { R(); C(); return MKT(KEY); }
VALUE     { R(); C(); return MKT(VALUE); }
KV        { R(); C(); return MKT(KV); }
[']       { R(); PUSH(IN_STR); C(); return MKT(SQ); }
{INTEGER} { R(); PUSH(AFTER_INT); SET_STR(); C(); return MKT(INTEGER); }
{SP}      { R(); C(); return MKT(SP); }
{LN}      { R(); L(); return '\n'; }
.         { R(); C(); return *yytext; } /* let bison to handle */

<IN_LIKE>{
[']       { R(); CHG(IN_STR); C(); return MKT(SQ); }
[/]       { R(); CHG(IN_REGEXP); C(); return *yytext; }
{SP}      { R(); C(); return MKT(SP); }
{LN}      { R(); L(); return '\n'; }
.         { R(); C(); return *yytext; } /* let bison to handle */
}

<IN_STR>{
[']                { R(); POP(); C(); return MKT(SQ); }
[^'\\\f\r\n]+      { R(); SET_STR(); C(); return MKT(STR); }
\\['\\]            { R(); SET_CHR(yytext[1]); C(); return MKT(CHR); }
\\f                { R(); SET_CHR('\f'); C(); return MKT(CHR); }
\\r                { R(); SET_CHR('\r'); C(); return MKT(CHR); }
\\n                { R(); SET_CHR('\n'); C(); return MKT(CHR); }
\\b                { R(); SET_CHR('\b'); C(); return MKT(CHR); }
\\t                { R(); SET_CHR('\t'); C(); return MKT(CHR); }
\\u[[:xdigit:]]{4} { R(); SET_STR(); C(); return MKT(UNI); }
{LN}               { R(); L(); return '\n'; }
.                  { R(); C(); return *yytext; } /* let bison to handle */
}

<IN_REGEXP>{
[/]             { R(); POP(); C(); return *yytext; }
[^/\\\f\r\n]+   { R(); SET_STR(); C(); return MKT(STR); }
\\[/\\]         { R(); SET_CHR(yytext[1]); C(); return MKT(CHR); }
\\f             { R(); SET_CHR('\f'); C(); return MKT(CHR); }
\\r             { R(); SET_CHR('\r'); C(); return MKT(CHR); }
\\n             { R(); SET_CHR('\n'); C(); return MKT(CHR); }
\\b             { R(); SET_CHR('\b'); C(); return MKT(CHR); }
\\t             { R(); SET_CHR('\t'); C(); return MKT(CHR); }
{LN}            { R(); L(); return '\n'; }
.               { R(); C(); return *yytext; } /* let bison to handle */
}

<AFTER_INT>{
{SP}      { R(); POP(); C(); return MKT(SP); }
{LN}      { R(); POP(); L(); return '\n'; }
.         { R(); C(); return *yytext; } /* let bison to handle */
}

%%

