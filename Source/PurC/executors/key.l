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
%option verbose debug
%option stack
%option nodefault
%option warn
%option perf-report
%option 8bit

%x LIKE
%x SQ REG
%x SQ_SUFFIX REG_SUFFIX
%x EXP_SPACE EXP_COLON

%%

<<EOF>> { int state = TOP_STATE();
          if (state != INITIAL) return -1;
          yyterminate(); }

KEY     { R(); PUSH(EXP_COLON); C(); return MKT(KEY); }
LIKE    { R(); PUSH(LIKE); PUSH(EXP_SPACE); C(); return MKT(LIKE); }
[']     { R(); PUSH(SQ); C(); return MKT(SQ); }
ALL     { R(); C(); return MKT(ALL); }
FOR     { R(); PUSH(EXP_SPACE); C(); return MKT(FOR); }
VALUE   { R(); PUSH(EXP_SPACE); C(); return MKT(VALUE); }
KV      { R(); PUSH(EXP_SPACE); C(); return MKT(KV); }
[:,]    { R(); C(); return *yytext; }
[/]     { R(); PUSH(REG); C(); return '/'; }
[ \t]   { R(); C(); } /* eat */
\n      { R(); L(); return *yytext; } /* let bison to handle */
.       { R(); C(); return *yytext; } /* let bison to handle */

<EXP_COLON>{
[:]     { R(); POP(); C(); return *yytext; }
\n      { R(); L(); return *yytext; } /* let bison to handle */
.       { R(); C(); return *yytext; } /* let bison to handle */
}

<EXP_SPACE>{
[ \t]   { R(); POP(); C(); }
\n      { R(); L(); return *yytext; } /* let bison to handle */
.       { R(); C(); return *yytext; } /* let bison to handle */
}

<LIKE>{
[']     { R(); CHG(SQ); C(); return MKT(SQ); }
[/]     { R(); CHG(REG); C(); return '/'; }
[ \t]   { R(); C(); } /* eat */
\n      { R(); L(); return *yytext; } /* let bison to handle */
.       { R(); C(); return *yytext; } /* let bison to handle */
}

<SQ>{
[']       { R(); CHG(SQ_SUFFIX); C(); return MKT(SQ); }
[^'\\\n]+ { R(); SET_STR(); C(); return MKT(STR); }
"\\'"     { R(); SET_CHR('\''); C(); return MKT(CHR); }
\n        { R(); L(); return *yytext; } /* let bison to handle */
.         { R(); C(); return *yytext; } /* let bison to handle */
}

<SQ_SUFFIX>{
[isc]          { R(); SET_CHR(*yytext); C(); return MKT(MATCHING_FLAG); }
[[:digit:]]+   { R(); SET_STR(); C(); return MKT(INTEGER); }
\n             { R(); L(); return *yytext; } /* let bison to handle */
.              { POP(); C(); yyless(0); } /* rescan */
}

<REG>{
[/]       { R(); CHG(REG_SUFFIX); C(); return *yytext; }
[^/\\\n]+ { R(); SET_STR(); C(); return MKT(STR); }
"\\/"     { R(); SET_CHR('/'); C(); return MKT(CHR); }
\n        { R(); L(); return *yytext; } /* let bison to handle */
.         { R(); C(); return *yytext; } /* let bison to handle */
}

<REG_SUFFIX>{
[gimsuy]       { R(); SET_CHR(*yytext); C(); return MKT(REGEXP_FLAG); }
[[:digit:]]+   { R(); SET_STR(); C(); return MKT(INTEGER); }
\n             { R(); L(); return *yytext; } /* let bison to handle */
.              { POP(); C(); yyless(0); } /* rescan */
}

%%

