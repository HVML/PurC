%{
/*
 * @file exe_add.l
 * @author
 * @date
 * @brief The implementation of public part for exe_add.
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
#define MKT(x)    TOK_EXE_ADD_##x

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

%x KW
%x AFTER_NUM

SP       [ \t]
LN       [\r\n]|[\f\r\n]
INTEGER  [0-9]+
NUMBER   ([.][0-9]+(e[-+][0-9]+)?)|([0-9]+[.][0-9]*(e[-+][0-9]+)?)

%%

<<EOF>> { int state = TOP_STATE();
          if (state != INITIAL) return -1;
          yyterminate(); }

ADD/[ \t]*:    { R(); C(); return MKT(ADD); }
BY        { R(); PUSH(KW); C(); return MKT(BY); }
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
{INTEGER} { R(); PUSH(AFTER_NUM); SET_STR(); C(); return MKT(INTEGER); }
{NUMBER}  { R(); PUSH(AFTER_NUM); SET_STR(); C(); return MKT(NUMBER); }
{SP}      { R(); C(); } /* eat */
{LN}      { R(); L(); } /* eat */
.         { R(); C(); return *yytext; } /* let bison to handle */

<KW>{
{SP}      { R(); POP(); C(); } /* eat */
{LN}      { R(); POP(); L(); } /* eat */
.         { R(); C(); return *yytext; } /* let bison to handle */
}

<AFTER_NUM>{
[-+*/)]   { R(); POP(); C(); yyless(0); }
{SP}      { R(); POP(); C(); } /* eat */
{LN}      { R(); POP(); L(); } /* eat */
.         { R(); C(); return *yytext; } /* let bison to handle */
}

%%

