%require  "2.5"
%debug 
%defines 

%code{

#include <iostream>
#include <cstdlib>
#include <fstream>

static Spec * lookup_id (std::string * s);

}

/* token types */
%union {
	std::string id;
	Spec * s;
}

%token <id> ID
%type <s> expr or_expr and_expr paren_expr


%%

expr
   : or_expr			{ $$ = $1; }
   ;

or_expr
   : and_expr			{ $$ = $1; }
   | or_expr OR and_expr	{ $$ = new Spec (Spec::OR, $1, $3); }
   ;

and_expr
   : paren_expr			{ $$ = $1; }
   | and_expr AND paren_expr	{ $$ = new Spec (Spec::AND, $1, $3); }
   ;

paren_expr
   : OPEN expr CLOSE		{ $$ = $2; }
   | NOT paren_expr		{ $$ = new Spec (Spec::NOT, $2, 0); }
   | ID				{ $$ = lookup_id ($1); }
   ;

%%

static Spec * lookup_id (std::string * s)
{
	/* FIXME find the string or deadlock */
	return new Spec (10);
}

void yyerror (const char * fmt, ...) {
	va_list args;

	va_start (args, fmt);
	fprintf (stderr, "%s:%d:%d: ",
			spec_filename.c_str (), yylineno, yycol);
	vfprintf (stderr, fmt, args);
	fprintf (stderr, "\n");
	exit (EXIT_FAILURE);
}



/* include for access to scanner.yylex
#include "mc_scanner.hpp"
static int yylex(MC::MC_Parser::semantic_type *yylval,
                 MC::MC_Scanner  &scanner,
                 MC::MC_Driver   &driver)
{
   return( scanner.yylex(yylval) );
}
 */
