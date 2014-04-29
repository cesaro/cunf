%{

#include <stdio.h>
#include <stdarg.h>
#include <err.h>

#include "util/misc.h"
#include "cna/spec_intrnl.hh"
#include "cna/spec_parser.h"

static int __cna_column = 1;
static void __c (void);

#define yyterminate() { __c (); return 0; }

%}

%option nodefault 
%option noyywrap 
%option nounput
%option yylineno
%option prefix="__cna_"

COMMENT	#.*$
ID			[^!()"&| \t\n]+
QID		\"[^"\n]*\"

%%

{COMMENT}	{ }
{QID}		{
		__c ();
		__cna_text[__cna_leng-1] = 0;
		__cna_lval.id = new std::string (__cna_text + 1);
		return ID;
		}
{ID}		{
		__c ();
		__cna_lval.id = new std::string (__cna_text);
		return ID;
		}
!		{ __c (); return NOT; }
\(		{ __c (); return OPEN; }
\)		{ __c (); return CLOSE; }
\|\|		{ __c (); return OR; }
&&		{ __c (); return AND; }
[ \t]		{ __c (); }
\n		{ __c (); __cna_column = 0; }

\"[^"\n]*$	{ __c (); __cna_errorv ("missing terminating \" character"); }
.		{ __c (); __cna_errorv ("'%s': unexpected character", yytext); }

%%

static void __c (void) {
	static int lastlen = 0;

	__cna_column += lastlen;
	lastlen = __cna_leng;
}

void
__cna_errorv (const char * fmt, ...)
{
	va_list args;

	// FIXME consider rewriting this in terms of FATAL()
	va_start (args, fmt);
	fprintf (stderr, "%s:%d:%d: ",
			__cna_filename->c_str (), __cna_lineno,
			__cna_column);
	vfprintf (stderr, fmt, args);
	fprintf (stderr, "\n");
	exit (EXIT_FAILURE);
}

void
__cna_error (const char * s)
{
	__cna_errorv ("%s", s);
}

