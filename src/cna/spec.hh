
#ifndef _SPEC_HH_
#define _SPEC_HH_

#include <string>

/* in spec_lexer.l */
#define YY_DECL extern int yylex (void)
YY_DECL;

extern std::string spec_filename;
extern int yycol;
extern int yylineno;

#define LID	1
#define LNOT	2
#define LOPEN	3
#define LCLOSE	4
#define LOR	5
#define LAND	6

#define LEND	7


/* in spec_parser.y */

void yyerror (const char * fmt, ...);


/* in spec.cc */

std::string fmt (const std::string fmt_str, ...);

class Spec
{
public:
	typedef enum {PLACE, TRANS, DEADLOCK, OR, AND, NOT} type_t;

	type_t type;
	Spec * l;
	Spec * r;
	int trans;
	int place;

	Spec (int pl);
	Spec (type_t t, Spec * left, Spec * right);
	void str (std::string &s);
	~Spec (void);
};


#endif
