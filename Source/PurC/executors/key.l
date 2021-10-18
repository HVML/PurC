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

%option prefix="key_yy"
%option bison-bridge bison-locations reentrant
%option noyywrap noinput nounput
%option verbose
%option debug
%option stack
%option nodefault
%option warn
%option perf-report
%option 8bit

%x SQ REG
%x AFTER_SQ AFTER_REG AFTER_INT
%x KEY

SP       [ \t]
LN       [\r\n]|[\f\r\n]
INTEGER  [0-9]+

%%

<<EOF>> { int state = TOP_STATE();
          if (state != INITIAL) return -1;
          yyterminate(); }

KEY       { R(); PUSH(KEY); C(); return MKT(KEY); }
LIKE      { R(); C(); return MKT(LIKE); }
ALL       { R(); C(); return MKT(ALL); }
FOR       { R(); C(); return MKT(FOR); }
VALUE     { R(); C(); return MKT(VALUE); }
KV        { R(); C(); return MKT(KV); }
[']       { R(); PUSH(SQ); C(); return MKT(SQ); }
[/]       { R(); PUSH(REG); C(); return *yytext; }
{INTEGER} { R(); PUSH(AFTER_INT); SET_STR(); C(); return MKT(INTEGER); }
{SP}      { R(); C(); return MKT(SP); }
{LN}      { R(); L(); return '\n'; }
.         { R(); C(); return *yytext; } /* let bison to handle */

<KEY>{
[:]       { R(); POP(); C(); return *yytext; }
{SP}      { R(); C(); return MKT(SP); }
{LN}      { R(); L(); return '\n'; }
.         { R(); C(); return *yytext; } /* let bison to handle */
}

<SQ>{
[']       { R(); CHG(AFTER_SQ); C(); return MKT(SQ); }
[^'\\\n]+ { R(); SET_STR(); C(); return MKT(STR); }
\\['\\]   { R(); SET_CHR(yytext[1]); C(); return MKT(CHR); }
{LN}      { R(); L(); return '\n'; }
.         { R(); C(); return *yytext; } /* let bison to handle */
}

<REG>{
[/]       { R(); CHG(AFTER_REG); C(); return *yytext; }
[^/\\\n]+ { R(); SET_STR(); C(); return MKT(STR); }
\\[/\\]   { R(); SET_CHR(yytext[1]); C(); return MKT(CHR); }
{LN}      { R(); L(); return '\n'; }
.         { R(); C(); return *yytext; } /* let bison to handle */
}

<AFTER_SQ>{
[cis]     { R(); SET_CHR(*yytext); C(); return MKT(MATCHING_FLAG); }
{INTEGER} { R(); SET_STR(); C(); return MKT(INTEGER); }
{SP}      { R(); POP(); C(); return MKT(SP); }
{LN}      { R(); POP(); L(); yyless(0); }
.         { R(); POP(); C(); yyless(0); }
}

<AFTER_REG>{
[igmsuy]  { R(); SET_CHR(*yytext); C(); return MKT(REGEXP_FLAG); }
{SP}      { R(); POP(); C(); return MKT(SP); }
{LN}      { R(); POP(); L(); yyless(0); }
.         { R(); C(); return *yytext; } /* let bison to handle */
}

<AFTER_INT>{
{SP}      { R(); POP(); C(); return MKT(SP); }
{LN}      { R(); POP(); L(); return '\n'; }
.         { R(); C(); return *yytext; } /* let bison to handle */
}


%%

