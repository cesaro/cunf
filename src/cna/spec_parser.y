
%require  "2.5"
%defines "src/cna/spec_parser.h"
%name-prefix "__cna_"

%{
#include <string>
#include "cna/spec_intrnl.hh"
#include "cna/spec.hh"

static inline cna::Spec * __lookup_id (std::string * s);
%}

%union {
	std::string * id;
	cna::Spec * spec;
}

%token <id>	ID
%token		NOT;
%token		OPEN;
%token		CLOSE;
%token		OR;
%token		AND;

%type <spec> expr or_expr and_expr paren_expr

%%

expr
   : or_expr			{ __cna_ast = $1; }
   ;

or_expr
   : and_expr			{ $$ = $1; }
   | or_expr OR and_expr	{ $$ = new cna::Spec (cna::Spec::OR, $1, $3); }
   ;

and_expr
   : paren_expr			{ $$ = $1; }
   | and_expr AND paren_expr	{ $$ = new cna::Spec (cna::Spec::AND, $1, $3); }
   ;

paren_expr
   : OPEN expr CLOSE		{ $$ = $2; }
   | NOT paren_expr		{ $$ = new cna::Spec (cna::Spec::NOT, $2, 0); }
   | ID				{ $$ = __lookup_id ($1); delete $1; }
   ;

%%

static cna::Spec *
__lookup_id (std::string * s)
{
	printf ("lookup '%s'\n", s->c_str ());
	/* FIXME find the string or deadlock */
	return new cna::Spec (10);
}

