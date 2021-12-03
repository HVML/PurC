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

%option bison-bridge bison-locations reentrant
%option noyywrap noinput nounput
%option verbose
%option debug
%option stack
%option nodefault
%option warn
%option perf-report
%option 8bit

%x IN_SQ IN_DQ IN_REGEXP IN_LIKE
%x KW
%x AFTER_STR AFTER_NUM AFTER_REGEXP

SP       [ \t]
LN       [\r\n]|[\f\r\n]
INTEGER  [0-9]+
NUMBER   ([.][0-9]+(e[-+][0-9]+)?)|([0-9]+[.][0-9]*(e[-+][0-9]+)?)

%%

<<EOF>> { int state = TOP_STATE();
          if (state != INITIAL) return -1;
          yyterminate(); }

FILTER/[ \t]*:    { R(); C(); return MKT(FILTER); }
LT        { R(); PUSH(KW); C(); return MKT(LT); }
GT        { R(); PUSH(KW); C(); return MKT(GT); }
LE        { R(); PUSH(KW); C(); return MKT(LE); }
GE        { R(); PUSH(KW); C(); return MKT(GE); }
NE        { R(); PUSH(KW); C(); return MKT(NE); }
EQ        { R(); PUSH(KW); C(); return MKT(EQ); }
AND       { R(); PUSH(KW); C(); return MKT(AND); }
OR        { R(); PUSH(KW); C(); return MKT(OR); }
XOR       { R(); PUSH(KW); C(); return MKT(XOR); }
NOT       { R(); PUSH(KW); C(); return MKT(NOT); }
LIKE      { R(); PUSH(IN_LIKE); PUSH(KW); C(); return MKT(LIKE); }
ALL       { R(); PUSH(KW); C(); return MKT(ALL); }
FOR       { R(); PUSH(KW); C(); return MKT(FOR); }
KEY       { R(); PUSH(KW); C(); return MKT(KEY); }
VALUE     { R(); PUSH(KW); C(); return MKT(VALUE); }
KV        { R(); PUSH(KW); C(); return MKT(KV); }
AS        { R(); PUSH(KW); C(); return MKT(AS); }
[']       { R(); PUSH(IN_SQ); C(); return '"'; }
["]       { R(); PUSH(IN_DQ); C(); return '"'; }
{INTEGER} { R(); PUSH(AFTER_NUM); SET_STR(); C(); return MKT(INTEGER); }
{NUMBER}  { R(); PUSH(AFTER_NUM); SET_STR(); C(); return MKT(NUMBER); }
{SP}      { R(); C(); } /* eat */
{LN}      { R(); L(); } /* eat */
.         { R(); C(); return *yytext; } /* let bison to handle */

<IN_LIKE>{
[']       { R(); PUSH(IN_SQ); C(); return '"'; }
["]       { R(); PUSH(IN_DQ); C(); return '"'; }
[/]       { R(); PUSH(IN_REGEXP); C(); return *yytext; }
[,]       { R(); C(); return *yytext; }
{SP}      { R(); C(); } /* eat */
{LN}      { R(); L(); } /* eat */
.         { R(); POP(); C(); yyless(0); }
}

<KW>{
[,]       { R(); POP(); C(); return *yytext; }
{SP}      { R(); POP(); C(); } /* eat */
{LN}      { R(); POP(); L(); } /* eat */
.         { R(); C(); return *yytext; } /* let bison to handle */
}

<IN_SQ>{
[']                { R(); CHG(AFTER_STR); C(); return '"'; }
[^'\\\f\r\n]+      { R(); SET_STR(); C(); return MKT(STR); }
\\['\\]            { R(); SET_CHR(yytext[1]); C(); return MKT(CHR); }
}

<IN_DQ>{
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

<IN_REGEXP>{
[/]             { R(); CHG(AFTER_REGEXP); C(); return *yytext; }
[^/\\\f\r\n]+   { R(); SET_STR(); C(); return MKT(STR); }
\\[/\\]         { R(); SET_CHR(yytext[1]); C(); return MKT(CHR); }
\\f             { R(); SET_CHR('\f'); C(); return MKT(CHR); }
\\r             { R(); SET_CHR('\r'); C(); return MKT(CHR); }
\\n             { R(); SET_CHR('\n'); C(); return MKT(CHR); }
\\b             { R(); SET_CHR('\b'); C(); return MKT(CHR); }
\\t             { R(); SET_CHR('\t'); C(); return MKT(CHR); }
{LN}            { R(); L(); return '\n'; } /* let bison to handle */
.               { R(); C(); return *yytext; } /* let bison to handle */
}

<AFTER_NUM>{
[-+*/,)]  { R(); POP(); C(); yyless(0); }
{SP}      { R(); POP(); C(); } /* eat */
{LN}      { R(); POP(); L(); } /* eat */
.         { R(); C(); return *yytext; } /* let bison to handle */
}

<AFTER_STR>{
[cis]     { R(); SET_CHR(*yytext); C(); return MKT(MATCHING_FLAG); }
{INTEGER} { R(); SET_STR(); C(); return MKT(MATCHING_LENGTH); }
{SP}      { R(); POP(); C(); } /* eat */
{LN}      { R(); POP(); L(); } /* eat */
.         { R(); POP(); C(); yyless(0); } /* let `upper` level to handle */
}

<AFTER_REGEXP>{
[gimsuy]  { R(); SET_CHR(*yytext); C(); return MKT(REGEXP_FLAG); }
{SP}      { R(); POP(); C(); } /* eat */
{LN}      { R(); POP(); L(); } /* eat */
.         { R(); POP(); C(); yyless(0); } /* let `upper` level to handle */
}


%%

