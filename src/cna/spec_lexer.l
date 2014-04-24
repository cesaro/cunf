%{

#include <stdio.h>
#include <stdarg.h>
#include <err.h>
#include "spec_lexer.h"

void yyerror (const char * fmt, ...);
int yycol = 1;
static void yyc (void);

#define yyterminate() return LEND;

// variable yyval

%}

%option nodefault 
%option noyywrap 
%option nounput
%option yylineno

%%

#.*$			{ printf ("COMMENT '%s'\n", yytext); }
\"[^"\n]*\"		{ yyc (); printf ("ID '%s'\n", yytext); return LID; }
[^!()"&| \t\n]+		{ yyc (); printf ("ID '%s'\n", yytext); return LID; }
!			{ yyc (); printf ("NOT '%s'\n", yytext); return LNOT; }
\(			{ yyc (); printf ("OPEN '%s'\n", yytext); return LOPEN; }
\)			{ yyc (); printf ("CLOSE '%s'\n", yytext); return LCLOSE; }
\|\|			{ yyc (); printf ("OR '%s'\n", yytext); return LOR; }
&&			{ yyc (); printf ("AND '%s'\n", yytext); return LAND; }
[ \t]			{ yyc (); printf ("SPACE\n"); }
\n			{ yyc (); yycol = 0; printf ("NL\n"); }

\"[^"\n]*$		{ yyc (); yyerror ("missing terminating \" character"); }
.			{ yyc (); yyerror ("'%s': unexpected character", yytext); }

%%


void yyerror (const char * fmt, ...) {
	va_list args;

	va_start (args, fmt);
	fprintf (stderr, "%s:%d:%d: ",
			spec_filename.c_str (), yylineno, yycol);
	vfprintf (stderr, fmt, args);
	fprintf (stderr, "\n");
	exit (EXIT_FAILURE);
}

// FIXME -- test that this works!
static void yyc (void) {
	static int lastlen = 0;

	yycol += lastlen;
	lastlen = yyleng;
}
