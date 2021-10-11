%{
/*
 * @file logical.l
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
#include "logical.tab.h"

#define MKT(x)    TOK_LOGICAL_##x

#define PUSH(state)      yy_push_state(state, yyscanner)
#define POP()            yy_pop_state(yyscanner)

#define CHG(state)                           \
do {                                         \
    yy_pop_state(yyscanner);                 \
    yy_push_state(state, yyscanner);         \
} while (0)

#define TOP_STATE()                         \
    ({  yy_push_state(INITIAL, yyscanner);  \
        int _top = yy_top_state(yyscanner); \
        yy_pop_state(yyscanner);            \
        _top; })

#define C()                                     \
do {                                            \
    yylloc->last_column += strlen(yytext);      \
} while (0)

#define L()                                     \
do {                                            \
    yylloc->last_line   += 1;                   \
    yylloc->last_column  = 1;                   \
} while (0)

#define R()                                       \
do {                                              \
    yylloc->first_column = yylloc->last_column ;  \
    yylloc->first_line   = yylloc->last_line;     \
} while (0)

#define SET_INT() do {                            \
    yylval->sz_ptr[0] = yyleng;                   \
    yylval->sz_ptr[1] = (uintptr_t)yytext;        \
} while (0)

#define SET_NUM() do {                            \
    yylval->sz_ptr[0] = yyleng;                   \
    yylval->sz_ptr[1] = (uintptr_t)yytext;        \
} while (0)

#define SET_VAR() do {                            \
    yylval->sz_ptr[0] = yyleng;                   \
    yylval->sz_ptr[1] = (uintptr_t)yytext;        \
} while (0)
%}


%option prefix="logical_yy"
%option bison-bridge bison-locations reentrant
%option noyywrap noinput nounput
%option verbose debug
%option stack
%option nodefault
%option warn
%option perf-report
%option 8bit

%%

<<EOF>> { int state = TOP_STATE();
          if (state != INITIAL) return -1;
          yyterminate(); }

[!<>();]       { C(); R(); return *yytext; }
">="	       { C(); R(); return MKT(GE); }
"<="	       { C(); R(); return MKT(LE); }
"=="	       { C(); R(); return MKT(EQ); }
"!="	       { C(); R(); return MKT(NE); }
"&&"	       { C(); R(); return MKT(AND); }
"||"	       { C(); R(); return MKT(OR); }



[-+]?0                { C(); SET_INT(); R(); return MKT(INT); }
[-+]?[1-9][0-9]*      { C(); SET_INT(); R(); return MKT(INT); }
[-+]?"0."[0-9]*       { C(); SET_NUM(); R(); return MKT(NUM); }
[-+]?[0-9]+"."[0-9]*  { C(); SET_NUM(); R(); return MKT(NUM); }
[-+]?[0-9]+("."[0-9]*)?[eE][+-]?[0-9]+ { C(); SET_NUM(); R(); return MKT(NUM); }





[_[:alpha:]][[:alnum:]]*  { C(); SET_VAR(); R(); return MKT(VAR); }

[ \t]   { C(); R(); /* ignore white space */ }

.	    { C(); R(); return *yytext; } /* let bison to handle it */
\n      { L(); R(); return *yytext; }

%%

