%name-prefix "logical"
%{
#include <stdio.h>
#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/edom.h"
#include "private/html.h"
#include "purc-variant.h"
%}

%code requires {
  #include "tools.h"
  typedef void *yyscan_t;
}

%define api.pure
%parse-param { struct pcdvobjs_logical_param * myparam }
%param { yyscan_t arg }  /* because of reentrant in .l */
%define parse.error verbose

%union {
    double d;
}


/* declare tokens */
%token NUMBER
%token GT GE LT LE EQU NOEQU AND OR XOR ANTI
%token OP CP
%token EOL
%right '='
%left '+' '-'
%left '*'

%type <d> calclist exp factor anti term NUMBER

%{
  /* put here, just after all tokens defined above */
  #include "logicallex.lex.h"
  void logicalerror (struct pcdvobjs_logical_param *, yyscan_t, const char *); /* first: %parse-param; second: %param */
%}

%start calclist
%%

calclist:
  %empty           { }
| calclist exp EOL { myparam->result = $2? 1: 0; }
| calclist EOL { printf("> "); } /* blank line or a comment */
;

exp: factor
| exp AND factor { $$ = $1 && $3; }
| exp OR factor { $$ = $1 || $3; }
| exp XOR factor { $$ = (int)$1 ^ (int)$3; }
;

factor: anti 
| factor GT anti { $$ = ($1 > $3)? 1 : 0; }
| factor GE anti { $$ = ($1 >= $3)? 1 : 0; }
| factor LT anti { $$ = ($1 < $3)? 1 : 0; }
| factor LE anti { $$ = ($1 <= $3)? 1 : 0; }
| factor EQU anti { $$ = ($1 == $3)? 1 : 0; }
| factor NOEQU anti { $$ = ($1 != $3)? 1 : 0; }
;

anti: term
| ANTI term { $$ = $2? 0: 1; }
;

term: NUMBER 
| OP exp CP { $$ = $2; }
;
%%

void logicalerror (struct pcdvobjs_logical_param *p, yyscan_t arg, const char *s)
{
  (void)p;
  (void)arg;
  fprintf(stderr, "error: %s\n", s);
}

