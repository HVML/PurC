%option noyywrap reentrant bison-bridge
%option header-file="mathlex.lex.h"
%option prefix="math"

%{
#include "mathlex.tab.h"
#include "tools.h"

#define YY_NO_INPUT
#define YY_NO_UNPUT

static double get_data (char * text, struct pcdvobjs_math_param * pp);
static long double get_data_l (char * text, struct pcdvobjs_math_param * pp);
%}

EXP ([Ee][-+]?[0-9]+)

%%
%{
  struct pcdvobjs_math_param *pp = yyextra;
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

[a-zA-z0-9]+  { if (pp->type == 0)
                    yylval->d = get_data(yytext, pp); 
                else
                    yylval->dl = get_data_l(yytext, pp); 
                return NUMBER; }
.	{return *yytext; } /* we don't call yyerror here, because it has no way to communicate with bison */
                      /* normally, we just return character token and defer the error handling in bison */

%%

static double get_data (char * text, struct pcdvobjs_math_param * pp)
{
    double number = 0.0d;
    if ((pp->v != NULL) && (!purc_variant_is_string (pp->v))) {
        return number;
    }

    purc_variant_t var = NULL;

    var = purc_variant_object_get_c (pp->v, text);
    if (var) 
        purc_variant_cast_to_number (var, &number, false);

    return number;
}

static long double get_data_l (char * text, struct pcdvobjs_math_param * pp)
{
    long double number = 0.0d;
    if ((pp->v != NULL) && (!purc_variant_is_string (pp->v))) {
        return number;
    }

    purc_variant_t var = NULL;

    var = purc_variant_object_get_c (pp->v, text);
    if (var) 
        purc_variant_cast_to_long_double (var, &number, false);

    return number;
}
