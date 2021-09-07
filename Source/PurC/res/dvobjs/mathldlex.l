%option noyywrap reentrant bison-bridge
%option header-file="mathldlex.lex.h"
%option prefix="mathld"

%{
#include "mathldlex.tab.h"
#include "tools.h"

#define YY_NO_INPUT
#define YY_NO_UNPUT

static long double get_data (char * text, struct pcdvobjs_mathld_param * pp);
%}

EXP ([Ee][-+]?[0-9]+)

%%
%{
  struct pcdvobjs_mathld_param *pp = yyextra;
%}

"+"	{ return ADD; }
"-"	{ return SUB; }
"*"	{ return MUL; }
"/"	{ return DIV; }
"("     { return OP; }
")"     { return CP; }
[0-9]+"."[0-9]*{EXP}? |
"."?[0-9]+{EXP}?	{ yylval->d = atof(yytext); return NUMBER; }

"\0"      { return EOL; }
"//".*  
[ \t]   { /* ignore white space */ }

[a-zA-z0-9]+  { yylval->d = get_data(yytext, pp); return NUMBER; }
.	{return *yytext; } /* we don't call yyerror here, because it has no way to communicate with bison */
                      /* normally, we just return character token and defer the error handling in bison */

%%

static long double get_data (char * text, struct pcdvobjs_mathld_param * pp)
{
    long double number = 0.0d;

    if (pp->v == NULL)
        return number;

    if ((pp->v != NULL) && (!purc_variant_is_object (pp->v))) 
        return number;

    purc_variant_t var = NULL;

    var = purc_variant_object_get_c (pp->v, text);
    if (var) 
        purc_variant_cast_to_long_double (var, &number, false);

    return number;
}
