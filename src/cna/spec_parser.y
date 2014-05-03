
%require  "2.5"
%defines "src/cna/spec_parser.h"
%name-prefix "__cna_"

%{
#include <string.h>
#include <string.h>
#include "cunf/global.h"
#include "cna/spec_intrnl.hh"
#include "cna/spec.hh"

static inline cna::Spec * __lookup_id (const char * name);
%}

%union {
	const char * id;
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
   : or_expr					{ __cna_ast = $1; }
   ;

or_expr
   : and_expr					{ $$ = $1; }
   | or_expr OR and_expr	{ $$ = new cna::Spec (cna::Spec::OR, $1, $3); }
   ;

and_expr
   : paren_expr				{ $$ = $1; }
   | and_expr AND paren_expr { $$ = new cna::Spec (cna::Spec::AND, $1, $3); }
   ;

paren_expr
   : OPEN expr CLOSE			{ $$ = $2; }
   | NOT paren_expr			{ $$ = new cna::Spec (cna::Spec::NOT, $2); }
   | ID							{ $$ = __lookup_id ($1); }
   ;

%%


/*
 * FIXME
 * these lookups could be expensive for long formulas or large nets,
 * and it is somehow simple to improve this with a hash table
 */
static inline struct place * __lookup_place (const char * name)
{
	struct ls * n;
	struct place * p;

	for (n = u.net.places.next; n; n = n->next)
	{
		p = ls_i (struct place, n, nod);
		if (strcmp (p->name, name) == 0) return p;
	}
	return 0;
}

static inline struct trans * __lookup_trans (const char * name)
{
	struct ls * n;
	struct trans * t;

	for (n = u.net.trans.next; n; n = n->next)
	{
		t = ls_i (struct trans, n, nod);
		if (strcmp (t->name, name) == 0) return t;
	}
	return 0;
}

static inline cna::Spec * __lookup_id (const char * name)
{
	struct place * p;
	struct trans * t;

	// check first whther it is a deadlock
	if (strcmp (name, "deadlock") == 0)
		return new cna::Spec ();

	// if not, a transition, otherwise, a place
	t = __lookup_trans (name);
	if (t) return new cna::Spec (t);
	p = __lookup_place (name);
	if (p) return new cna::Spec (p);

	// stop and exit
	__cna_errorv ("error: '%s': expected either \"deadlock\", " \
			"a place name, or a transition name", name);
	return 0;
}

