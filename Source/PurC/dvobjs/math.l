%{
/*
 * @file math.l
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
#include "math.tab.h"

#define MKT(x)    TOK_MATH_##x

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
    yylloc->last_column  = 0;                   \
} while (0)

#define R()                                       \
do {                                              \
    yylloc->first_column = yylloc->last_column ;  \
    yylloc->first_line   = yylloc->last_line;     \
} while (0)

static double get_data (char * text, struct pcdvobjs_math_param * pp);

%}

%option prefix="math_yy"
%option bison-bridge bison-locations reentrant
%option noyywrap noinput nounput
%option verbose debug
%option stack
%option nodefault
%option warn
%option perf-report
%option 8bit

%x STR

EXP ([Ee][-+]?[0-9]+)

%%
<<EOF>> { int state = TOP_STATE();
          if (state != INITIAL) return -1;
          yyterminate(); }

"+"	    { return MKT(ADD); }
"-"	    { return MKT(SUB); }
"*"	    { return MKT(MUL); }
"/"	    { return MKT(DIV); }
"("     { return MKT(OP); }
")"     { return MKT(CP); }
[0-9]+"."[0-9]*{EXP}? |
"."?[0-9]+{EXP}?	{ yylval->d = atof(yytext); return MKT(NUMBER); }

"//".*
[ \t]   { /* ignore white space */ }

[a-zA-z0-9]+  { yylval->d = get_data(yytext, yyextra); return MKT(NUMBER); }
.	{return *yytext; } /* we don't call yyerror here, because it has no way to communicate with bison */
                      /* normally, we just return character token and defer the error handling in bison */
%%


static double get_data (char * text, struct pcdvobjs_math_param * pp)
{
    double number = 0.0d;

    if (pp->v == NULL)
        return number;

    if ((pp->v != NULL) && (!purc_variant_is_object (pp->v)))
        return number;

    purc_variant_t var = NULL;

    var = purc_variant_object_get_c (pp->v, text);
    if (var)
        purc_variant_cast_to_number (var, &number, false);

    return number;
}

