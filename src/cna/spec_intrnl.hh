
#ifndef _CNA_SPEC_INTERNL_HH_
#define _CNA_SPEC_INTERNL_HH_

#include <string>
#include <vector>

#include <stdio.h>
#include "cna/spec.hh"

/* in spec_lexer.l */
int __cna_lex (void);
void __cna_errorv (const char * fmt, ...);
void __cna_error (const char * s);
void __cna_restart (FILE * f);
extern FILE * __cna_in;

/* in spec_parser.y */
int __cna_parse (void);

/* in spec.cc, for communication with cna::Speccheck.load_spec */
extern const char * __cna_filename;
extern std::vector<cna::Spec *> * __cna_specv;

#endif
