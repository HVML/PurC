%{
/*
 * @file ejson.l
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
#define MKT(x)    TOK_EJSON_##x

#define PUSH(state)      yy_push_state(state, yyscanner)
#define POP()            yy_pop_state(yyscanner)

#define CHG(state) do {                      \
    yy_pop_state(yyscanner);                 \
    yy_push_state(state, yyscanner);         \
} while (0)

#define TOP_STATE()                         \
    ({  yy_push_state(INITIAL, yyscanner);  \
        int _top = yy_top_state(yyscanner); \
        yy_pop_state(yyscanner);            \
        _top; })

#define C() do {                                \
    yylloc->last_column += strlen(yytext);      \
} while (0)

#define L() do {                                \
    yylloc->last_line   += 1;                   \
    yylloc->last_column  = 1;                   \
} while (0)

#define R()                                       \
do {                                              \
    yylloc->first_column = yylloc->last_column ;  \
    yylloc->first_line   = yylloc->last_line;     \
} while (0)

#define SET_STR() do {               \
    yylval->sval.text = yytext;      \
    yylval->sval.leng = yyleng;      \
} while (0)

#define SET_CHR(_c) do {             \
    yylval->c = _c;                  \
} while (0)
%}

%option bison-bridge bison-locations reentrant
%option noyywrap noinput nounput
%option verbose debug
%option stack
%option nodefault
%option warn
%option perf-report
%option 8bit

%x TQ DQ SQ ESCAPE

%%

<<EOF>> { int state = TOP_STATE();
          if (state != INITIAL) return -1;
          yyterminate(); }

"undefined"   { C(); R(); return MKT(T_UNDEFINED); }
"null"  { C(); R(); return MKT(T_NULL); }
"true"  { C(); R(); return MKT(T_TRUE); }
"false" { C(); R(); return MKT(T_FALSE); }
["]{3}  { C(); PUSH(TQ); R(); return MKT(TQ); }
["]     { C(); PUSH(DQ); R(); return *yytext; }
[']     { C(); PUSH(SQ); R(); return '"'; }
[-+]?[1-9][0-9]*(L|UL|l|ul)         { C(); SET_STR(); R(); return MKT(INTEGER); }
[-+]?[1-9][0-9]*(FL|fl)?            { C(); SET_STR(); R(); return MKT(NUMBER); }
[-+]?[0]?([.][0-9]+)?([eE][-+][0-9]+)?(FL|fl)?          { C(); SET_STR(); R(); return MKT(NUMBER); }
[-+]?[1-9][0-9]*([.][0-9]+)?([eE][-+][0-9]+)?(FL|fl)?   { C(); SET_STR(); R(); return MKT(NUMBER); }
[[:alpha:]][-_[:alnum:]]*   { C(); SET_STR(); R(); return MKT(ID); }
[{}]    { C(); R(); return *yytext; }
"["     { C(); R(); return *yytext; }
"]"     { C(); R(); return *yytext; }
","     { C(); R(); return *yytext; }
[ \t]   { C(); R(); } /* eat */
\n      { L(); R(); } /* eat */
.       { C(); R(); return *yytext; } /* let bison to handle */

<TQ>{
["""]     { C(); POP(); R(); return MKT(TQ); }
[^"\n]+   { C(); SET_STR(); R(); return MKT(STR); }
["]{1,2}  { C(); SET_STR(); R(); return MKT(STR); }
\n        { L(); SET_STR(); R(); return MKT(STR); }
}

<DQ>{
["]       { C(); POP(); R(); return *yytext; }
[^"\\\n]+ { C(); SET_STR(); R(); return MKT(STR); }
"\\\""    { C(); SET_CHR('"'); R(); return MKT(CHR); }
[\\]      { C(); PUSH(ESCAPE); R(); }
\n        { L(); R(); return *yytext; } /* let bison to handle */
}

<SQ>{
[']       { C(); POP(); R(); return '"'; }
[^'\\\n]+ { C(); SET_STR(); R(); return MKT(STR); }
"\\\'"    { C(); SET_CHR('\''); R(); return MKT(CHR); }
[\\]      { C(); PUSH(ESCAPE); R(); }
\n        { L(); R(); return *yytext; } /* let bison to handle */
}

<ESCAPE>{
[\\]      { C(); SET_CHR('\\'); POP(); R(); return MKT(CHR); }
[/]       { C(); SET_CHR('/'); POP(); R(); return MKT(CHR); }
[b]       { C(); SET_CHR('\b'); POP(); R(); return MKT(CHR); }
[f]       { C(); SET_CHR('\f'); POP(); R(); return MKT(CHR); }
[n]       { C(); SET_CHR('\n'); POP(); R(); return MKT(CHR); }
[r]       { C(); SET_CHR('\r'); POP(); R(); return MKT(CHR); }
[t]       { C(); SET_CHR('\t'); POP(); R(); return MKT(CHR); }
[uU][[:xdigit:]]{4}  { C(); SET_STR(); POP(); R(); return MKT(UNI); }
.         { C(); R(); return *yytext; } /* let bison to handle */
\n        { C(); R(); return *yytext; } /* let bison to handle */
}

%%

