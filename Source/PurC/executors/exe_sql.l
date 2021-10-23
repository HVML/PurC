%{
/*
 * @file exe_sql.l
 * @author
 * @date
 * @brief The implementation of public part for exe_sql.
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
#include "exe_sql.tab.h"

#define MKT(x)    TOK_EXE_SQL_##x

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

%option prefix="exe_sql_yy"
%option bison-bridge bison-locations reentrant
%option noyywrap noinput nounput
%option verbose
%option debug
%option stack
%option nodefault
%option warn
%option perf-report
%option 8bit

%x KW
%x IN_SQ IN_DQ AFTER_NUM AFTER_STR AFTER_ID

SP       [ \t]
LN       [\r\n]|[\f\r\n]
INTEGER  [0-9]+
NUMBER   ([.][0-9]+(e[-+][0-9]+)?)|([0-9]+[.][0-9]*(e[-+][0-9]+)?)
ID       [_[:alpha:]][_[:alnum:]]*
OP       [-+*/,()=<>!.:]

%%

<<EOF>> { int state = TOP_STATE();
          if (state != INITIAL) return -1;
          yyterminate(); }

SQL/[ \t]*:    { R(); C(); return MKT(SQL); }
UNION     { R(); PUSH(KW); C(); return MKT(UNION); }
SELECT    { R(); PUSH(KW); C(); return MKT(SELECT); }
AS        { R(); PUSH(KW); C(); return MKT(AS); }
WHERE     { R(); PUSH(KW); C(); return MKT(WHERE); }
GROUP     { R(); PUSH(KW); C(); return MKT(GROUP); }
ORDER     { R(); PUSH(KW); C(); return MKT(ORDER); }
BY        { R(); PUSH(KW); C(); return MKT(BY); }
ASC       { R(); PUSH(KW); C(); return MKT(ASC); }
DESC      { R(); PUSH(KW); C(); return MKT(DESC); }
TRAVEL    { R(); PUSH(KW); C(); return MKT(TRAVEL); }
IN        { R(); PUSH(KW); C(); return MKT(IN); }
SIBLINGS  { R(); PUSH(KW); C(); return MKT(SIBLINGS); }
DEPTH     { R(); PUSH(KW); C(); return MKT(DEPTH); }
BREADTH   { R(); PUSH(KW); C(); return MKT(BREADTH); }
LEAVES    { R(); PUSH(KW); C(); return MKT(LEAVES); }
LIKE      { R(); PUSH(KW); C(); return MKT(LIKE); }
AND       { R(); PUSH(KW); C(); return MKT(AND); }
OR        { R(); PUSH(KW); C(); return MKT(OR); }
NOT       { R(); PUSH(KW); C(); return MKT(NOT); }
">="      { R(); C(); return MKT(GE); }
"<="      { R(); C(); return MKT(LE); }
"<>"      { R(); C(); return MKT(NE); }
{OP}      { R(); C(); return *yytext; }
[']       { R(); PUSH(IN_SQ); C(); return '"'; }
["]       { R(); PUSH(IN_DQ); C(); return '"'; }
{INTEGER} { R(); PUSH(AFTER_NUM); SET_STR(); C(); return MKT(INTEGER); }
{NUMBER}  { R(); PUSH(AFTER_NUM); SET_STR(); C(); return MKT(NUMBER); }
{ID}      { R(); PUSH(AFTER_ID); SET_STR(); C(); return MKT(ID); }
{SP}      { R(); C(); } /* eat */
{LN}      { R(); L(); } /* eat */
.         { R(); C(); return *yytext; } /* let bison to handle */

<IN_SQ>{
[']{2}             { R(); SET_CHR('\''); C(); return MKT(CHR); }
[']                { R(); CHG(AFTER_STR); C(); return '"'; }
[^'\\\f\r\n]+      { R(); SET_STR(); C(); return MKT(STR); }
\\['\\]            { R(); SET_CHR(yytext[1]); C(); return MKT(CHR); }
}

<IN_DQ>{
["]{2}             { R(); SET_CHR('"'); C(); return MKT(CHR); }
["]                { R(); CHG(AFTER_STR); C(); return '"'; }
[^"\\\f\r\n]+      { R(); SET_STR(); C(); return MKT(STR); }
\\["\\]            { R(); SET_CHR(yytext[1]); C(); return MKT(CHR); }
}

<IN_SQ,IN_DQ>{
\\f                { R(); SET_CHR('\f'); C(); return MKT(CHR); }
\\r                { R(); SET_CHR('\r'); C(); return MKT(CHR); }
\\n                { R(); SET_CHR('\n'); C(); return MKT(CHR); }
\\b                { R(); SET_CHR('\b'); C(); return MKT(CHR); }
\\t                { R(); SET_CHR('\t'); C(); return MKT(CHR); }
\\u[[:xdigit:]]{4} { R(); SET_STR(); C(); return MKT(UNI); }
{LN}               { R(); L(); return '\n'; } /* let bison to handle */
.                  { R(); C(); return *yytext; } /* let bison to handle */
}

<KW,AFTER_NUM,AFTER_STR,AFTER_ID>{
">="      { R(); POP(); C(); return MKT(GE); }
"<="      { R(); POP(); C(); return MKT(LE); }
"<>"      { R(); POP(); C(); return MKT(NE); }
{OP}      { R(); POP(); C(); return *yytext; }
{SP}      { R(); POP(); C(); } /* eat */
{LN}      { R(); POP(); L(); } /* eat */
.         { R(); C(); return *yytext; } /* let bison to handle */
}

%%

