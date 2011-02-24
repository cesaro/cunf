/*****************************************************************************/
/* readPEP.c                                                                 */
/*****************************************************************************/
/*                                  Stefan Roemer,  06.02.1995 - 06.02.1995  */
/*                                  Stefan Schwoon, April 1997		     */
/*****************************************************************************/
/* The function read_pep_net at the end reads a PEP LL net into memory.	     */
/*****************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "readlib.h"
#include "global.h"
#include "netconv.h"
#include "glue.h"

/*****************************************************************************/

enum { FT_STRING = 1, FT_COORDS = 2, FT_NUMBER = 3, FT_FLAG = 4 };
enum { TB_MANDATORY = 0, TB_OPTIONAL = 1 };
enum { TB_BLOCK = 0, TB_LINE = 1 };

/* used for specifying where data fields of interest should be stored. */
typedef struct
{
	char c;		/* Identifying character. */
	void *ptr;	/* Memory location.	  */
} t_dest;

/* what to do with the data found in a block with matching name */
typedef struct
{
	char *name;		/* Name of the block.			     */
	int  (*hookfunc)();	/* called any time an entity has been read   */
	char **restptr;		/* Where to store additional data fields.    */
	t_dest *destarray;	/* Where to store which data field.	     */
} t_blockdest;

typedef struct { int x,y; } t_coords;	/* Simple struct for coordinates. */

/* File and block identifier. */
char *filetype;
char *blocktype;

typedef struct
{
	char c;
	int type;
} t_fieldinfo;

typedef struct
{
	char *name;
	int  optional;
	int  line;
	t_fieldinfo *field;
} t_blockinfo;

typedef struct
{
	int type;
	void *ptr;
} t_lookup;

/*****************************************************************************/
/* _read_PEP_file							     */
/* This is a function for reading a file in PEP's general layout, i.e. it's  */
/* not restricted to nets (though in practice that's the only thing we'll    */
/* use it for). blocks is a data structure telling us how the layout of a    */
/* file should look like, and dest tells us what we should do with the data  */
/* we find (what and where to store it).				     */

void _read_PEP_file(char *filename, char **types,
		   t_blockinfo *blocks, t_blockdest *dest)
{
	FILE *infile;
	t_lookup *tbl;
	t_fieldinfo *fld;
	t_dest *dst;
	t_blockdest *sdest = dest;
	int i, len = 0, ch, num, num2, ralloc;
	char *restptr, *rtmp;

	HLinput_file = filename;
	HLinput_line = 1;

	/* Open the file, read the header. */
	if (!(infile = fopen(filename, "r")))
		gl_err ("'%s': cannot open for reading", filename);

	ReadCmdToken(infile);
	if (strcmp(sbuf, "PEP")) gl_err ("keyword `PEP' expected");

	/* Check if the file's type (second line of file) is one of those
	   that are allowed. */
	ReadNewline(infile);
	ReadCmdToken(infile);
	for (; *types && strcmp(sbuf,*types); types++);
	if (!*types) gl_err ("unexpected format identifier '%s'",sbuf);
	filetype = gl_strdup(sbuf);

	ReadNewline(infile);
	ReadCmdToken(infile);
	if (strncmp(sbuf, "FORMAT_N", 8))
		gl_err ("keyword 'FORMAT_N' or 'FORMAT_N2' expected");

	tbl = (t_lookup*) gl_malloc(sizeof(t_lookup)*(128-' ')) - ' ';
	ReadNewline(infile);

	while (!feof(infile))
	{
		/* Read next block id. */
		ReadCmdToken(infile);

		blocktype = gl_strdup(sbuf);

		/* Identify block. */
		for (; blocks->name && strcmp(blocks->name,sbuf); blocks++)
		    if (!blocks->optional)
			gl_err ("keyword '%s' expected",blocks->name);

		if (!blocks->name) gl_err ("unknown keyword '%s'",sbuf);

		for (dest = sdest; dest->name; dest++)
			if (!strcmp(blocks->name,dest->name)) break;

		/* Set up tbl. */
		for (i=' '; i<128; tbl[i++].type = 0);
		for (i=' '; i<128; tbl[i++].ptr = NULL);

		for (fld = blocks->field; fld->c; fld++)
			tbl[(int)fld->c].type = fld->type;

		if (!blocks->line) ReadNewline(infile);

		/* Read lines until next block begins. Every line yields one
		   new entity of the type determined by the current block. */
		for(;;)
		{
		    if (isupper(ch = ReadCharComment(infile)))
		    {
			char ch2 = getc(infile);
			ungetc(ch2,infile);
			/* We assume that uppercase letters at the start of
			   a line always indicates a new block. */
			if (isupper((int)ch2)) break;
		    }

		    if (feof(infile)) break;
		    if (ch == '\n') continue;

		    *(rtmp = restptr = gl_malloc(ralloc = 64)) = '\0';

		    /* If information about this block is wanted, take
		       the information where to store data from dest. */
		    if (dest->name)
			for (dst = dest->destarray; dst->c; dst++)
			{
			    t_lookup *l = tbl + dst->c;
			    l->ptr = dst->ptr;
			    switch(l->type)
			    {
				case FT_STRING:
				case FT_COORDS:
					*(char**)(l->ptr) = NULL;
					break;
				case FT_NUMBER:
				case FT_FLAG:
					*(int*)(l->ptr) = 0;
					break;
				default:
					gl_err ("internal error: don't "
					"know the type of field '%c'",dst->c);
				break;
			    }
			}

		    /* Parse until end of line. We assume that all information
		       about an entity (place, transition etc.) is stored in
		       one single line. */
		    while (ch != '\n')
		    {
			if (isdigit(ch) || ch == '-')
			{
				/* Numbers are treated specially.
				   x@y gets stored in the '@' field, and
				   plain numbers are stored in the '0' field. */
				ungetc(ch,infile);
				ReadNumber(infile,&num);
				if ((ch=ReadWhiteSpace(infile)) == '@'
						  || ch == '<' || ch == '>')
				{
					ch = '@';
					ReadNumber(infile,&num2);
					len = 24;
				}
				else
				{
					ungetc(ch,infile);
					ch = '0';
					len = 12;
				}
			}
			else
			{
				if (ch == '\'' || ch == '"') ungetc(ch,infile);
				switch(tbl[ch].type)
				{
				    case FT_STRING:
					ReadEnclString(infile);
					len = 3+strlen(sbuf);
					break;
				    case FT_NUMBER:
					ReadNumber(infile,&num);
					len = 12;
					break;
				    case FT_COORDS:
					ReadCoordinates(infile, &num, &num2);
					len = 24;
					break;
				    case FT_FLAG:
					len = 1;
					break;
				    default:
					gl_err ("unknown token '%c'",ch);
					break;
				}
			}

			/* Store data in the memory locations specified by
			   the dest array. If no location is given for a
			   particular field, its contents are written to
			   a 'rest' string. */
			while (!tbl[ch].ptr && rtmp-restptr+len >= ralloc)
			{
				restptr = gl_realloc(restptr, ralloc += 64);
				rtmp = restptr + strlen(restptr);
			}
			switch(tbl[ch].type)
			{
			    case FT_STRING:
				if (tbl[ch].ptr)
					*(char**)(tbl[ch].ptr) = gl_strdup(sbuf);
				else if (strchr("'\"",ch))
					sprintf(rtmp,"\"%s\"",sbuf);
				else
					sprintf(rtmp,"%c\"%s\"",ch,sbuf);
				break;
			    case FT_NUMBER:
				if (tbl[ch].ptr)
					*(int*)(tbl[ch].ptr) = num;
				else if (ch == '0')
					sprintf(rtmp,"%d ",num);
				else
					sprintf(rtmp,"%c%d",ch,num);
				break;
			    case FT_COORDS:
				if (tbl[ch].ptr)
				{
				    *((t_coords**)(tbl[ch].ptr)) =
						gl_malloc(sizeof(t_coords));
				    (*(t_coords**)(tbl[ch].ptr))->x = num;
				    (*(t_coords**)(tbl[ch].ptr))->y = num2;
				}
				else if (ch == '@')
					sprintf(rtmp,"%d@%d ",num,num2);
				else
					sprintf(rtmp,"%c%d@%d",ch,num,num2);
				break;
			    case FT_FLAG:
				if (tbl[ch].ptr)
					(*(int*)(tbl[ch].ptr))++;
				else
					sprintf(rtmp,"%c",ch);
				break;
			}
			if (!tbl[ch].ptr) rtmp += strlen(rtmp);
			ch = ReadCharComment(infile);
		    } /* end of while */

		    /* When the whole line has been read, see if information
		       about the current block is wanted - if so, call the
		       corresponding hook function. */
		    if (dest->name)
		    {
			if (dest->restptr) *(dest->restptr) = restptr;
			if (dest->hookfunc()) gl_err ("read aborted");
		    }
		    else
			gl_free(restptr);

		    HLinput_line++;
		}

		gl_free(blocktype);
		if (!feof(infile)) ungetc(ch,infile);
		blocks++;
	}

	/* Check for mandatory blocks that didn't occur in the file. */
	for (; blocks->name; blocks++)
	    if (!blocks->optional)
		gl_err ("section '%s' not found",blocks->name);

	gl_free(tbl+' ');
	fclose(infile);
}

/*****************************************************************************/
/* Define the layout of a PEP net file.					     */

/* Allowable types of nets. */
char *type_llnet[] = { "PetriBox", "PTNet", NULL };

/* Defaults for blocks/places/transitions. */
t_fieldinfo nodedefs[] =
	{ { 'n', FT_COORDS },		/* rel pos of name	  */
	  { 'a', FT_COORDS },		/* rel pos of meaning	  */
	  { 's', FT_NUMBER },		/* object size		  */
	  { 't', FT_NUMBER },		/* line weight		  */
	  {  0 ,  0 } };

/* Defaults for arcs. */
t_fieldinfo arcdefs[] =
	{ { 'n', FT_COORDS },		/* rel pos of name	  */
	  { 't', FT_NUMBER },		/* line weight		  */
	  { 'w', FT_NUMBER },		/* default weight	  */
	  {  0 ,  0 } };

/* Data fields for blocks. */
t_fieldinfo blockfields[] =
	{ { '\'',FT_STRING },		/* identifier		  */
	  { '"', FT_STRING },		/*     "		  */
	  { '@', FT_COORDS },		/* coordinates		  */
	  { '0', FT_NUMBER },		/* numeric identifier	  */
	  { 'n', FT_COORDS },		/* rel pos of name	  */
	  { 'N', FT_COORDS },		/* abs. Pos of name	  */
	  { 'b', FT_STRING },		/* Bedeutung		  */
	  { 'a', FT_COORDS },		/* rel pos of meaning	  */
	  { 'A', FT_COORDS },		/* abs pos of meaning	  */
	  { 'R', FT_STRING },		/* reference		  */
	  { 'T', FT_STRING },		/* BPN->HL ref (obsolete) */
	  { 'u', FT_STRING },		/* block list		  */
	  {  0 ,  0 } };
	  
/* Data fields for places. */
t_fieldinfo placefields[] =
	{ { '\'',FT_STRING },		/* identifier		  */
	  { '"', FT_STRING },		/*     "		  */
	  { '@', FT_COORDS },		/* coordinates		  */
	  { '0', FT_NUMBER },		/* numeric identifier	  */
	  { 'n', FT_COORDS },		/* rel pos of name	  */
	  { 'N', FT_COORDS },		/* abs pos of name	  */
	  { 'b', FT_STRING },		/* meaning		  */
	  { 'a', FT_COORDS },		/* rel pos of meaning	  */
	  { 'A', FT_COORDS },		/* abs pos of meaning	  */
	  { 'M', FT_NUMBER },		/* initial marking (int)  */
	  { 'm', FT_NUMBER },		/* current marking (int) */
	  { 'u', FT_STRING },		/* block list		  */
	  { 'Z', FT_STRING },		/* type			  */
	  { 'B', FT_COORDS },		/* rel pos of type	  */
	  { 'z', FT_STRING },		/* initial marking (str)  */
	  { 'y', FT_STRING },		/* current marking (str)  */
	  { 'R', FT_STRING },		/* reference		  */
	  { 'k', FT_NUMBER },		/* capacity		  */
	  { 'e', FT_FLAG   },		/* entry place		  */
	  { 'x', FT_FLAG   },		/* exit place		  */
	  { 'v', FT_NUMBER },		/* flags		  */
	  { 's', FT_NUMBER },		/* object size		  */
	  { 't', FT_NUMBER },		/* line weight		  */
	  { 'c', FT_NUMBER },		/* colour		  */
	  { 'T', FT_STRING },		/* BPN->HL ref (obsolete) */
	  {  0 ,  0 } };
  
/* Data fields for transitions. */
t_fieldinfo transfields[] =
	{ { 'S', FT_FLAG   },		/* synch transition	  */
	  { '\'',FT_STRING },		/* identifier		  */
	  { '"', FT_STRING },		/*     "		  */
	  { '@', FT_COORDS },		/* coordinates		  */
	  { '0', FT_NUMBER },		/* numeric identifier	  */
	  { 'n', FT_COORDS },		/* rel pos of name	  */
	  { 'N', FT_COORDS },		/* abs pos of name	  */
	  { 'b', FT_STRING },		/* meaning		  */
	  { 'a', FT_COORDS },		/* rel pos of meaning	  */
	  { 'A', FT_COORDS },		/* abs pos of meaning	  */
	  { 'v', FT_NUMBER },		/* flags		  */
	  { 'u', FT_STRING },		/* block list		  */
	  { 'P', FT_STRING },		/* list of phantom trs	  */
	  { 'g', FT_STRING },		/* value term		  */
	  { 'h', FT_COORDS },		/* rel pos of value term  */
	  { 'H', FT_COORDS },		/* rel pos of value term  */
	  { 'R', FT_STRING },		/* reference		  */
	  { 'i', FT_STRING },		/* action terms		  */
	  { 'j', FT_COORDS },		/* rel pos action terms   */
	  { 's', FT_NUMBER },		/* object size		  */
	  { 't', FT_NUMBER },		/* line weight		  */
	  { 'c', FT_NUMBER },		/* colour		  */
	  { 'T', FT_STRING },		/* BPN->HL ref (obsolete) */
	  { 'r', FT_FLAG   },		/* refined flag		  */
	  {  0 ,  0 } };
  
/* Data fields for arcs. */
t_fieldinfo arcfields[] =
	{ { '@', FT_COORDS },		/* source/dest		  */
	  { 'J', FT_COORDS },		/* coordinates		  */
	  { ',', FT_COORDS },		/* more coordinates	  */
	  { 'w', FT_NUMBER },		/* weight		  */
	  { 'n', FT_COORDS },		/* rel pos of weight	  */
	  { 'N', FT_COORDS },		/* abs pos of weight	  */
	  { 'p', FT_STRING },		/* inscription		  */
	  { 'q', FT_COORDS },		/* rel pos of inscription */
	  { 'Q', FT_COORDS },		/* abs pos of inscription */
	  { 'v', FT_NUMBER },		/* visibility		  */
	  { 't', FT_NUMBER },		/* line weight		  */
	  { 'c', FT_NUMBER },		/* colour		  */
	  {  0 ,  0 } };

/* Data fields for texts. */
t_fieldinfo textfields[] =
	{ { '\'',FT_STRING },		/* text			  */
	  { '"', FT_STRING },		/*   "			  */
	  { 'N', FT_COORDS },		/* absolute position	  */
	  {  0 ,  0 } };

/* All the data blocks that may occur in the file. */

t_blockinfo netblocks[] =
	{ { "DBL", TB_OPTIONAL,  TB_LINE,  nodedefs },
	  { "DPL", TB_OPTIONAL,  TB_LINE,  nodedefs },
	  { "DTR", TB_OPTIONAL,  TB_LINE,  nodedefs },
	  { "DPT", TB_OPTIONAL,  TB_LINE,  arcdefs },
	  { "BL",  TB_OPTIONAL,  TB_BLOCK, blockfields },
	  { "PL",  TB_MANDATORY, TB_BLOCK, placefields },
	  { "TR",  TB_MANDATORY, TB_BLOCK, transfields },
	  { "PTR", TB_OPTIONAL,  TB_BLOCK, transfields+1 },
	  { "TP",  TB_MANDATORY, TB_BLOCK, arcfields },
	  { "PT",  TB_MANDATORY, TB_BLOCK, arcfields },
	  { "RA",  TB_OPTIONAL,  TB_BLOCK, arcfields },
	  { "PTP", TB_OPTIONAL,  TB_BLOCK, arcfields },
	  { "PPT", TB_OPTIONAL,  TB_BLOCK, arcfields },
	  { "TX",  TB_OPTIONAL,  TB_BLOCK, textfields },
	  { NULL, 0, 0, NULL } };

/*****************************************************************************/

#define MBSIZE 4096
#define NAMES_START 2000
#define NAMES_OFFSET 1000

t_coords *rd_co;
struct place **PlArray;
struct trans **TrArray;
int  AnzPlNamen, MaxPlNamen, AnzTrNamen, MaxTrNamen;
int  placecount, transcount, rd_ident, rd_marked;
char autonumbering, *rd_name;

/*****************************************************************************/
/* insert_{place,trans,arc}						     */
/* These functions are called from read_PEP_net whenever a place, transition */
/* or an arc has been parsed in the net. The tables place_dest, trans_dest   */
/* and arc_dest in read_HLnet determine where read_PEP_net should store	     */
/* contents of certain fields prior to calling these functions.		     */

static int _insert_place()
{
	placecount++;
	if (rd_ident && rd_ident != placecount) autonumbering = 0;
	if (!rd_ident && autonumbering) rd_ident = placecount;
	if (!rd_ident) gl_err ("missing place identifier");
		
	if (rd_ident > AnzPlNamen)
		AnzPlNamen = rd_ident;
	else if (PlArray[rd_ident])
		gl_err ("place identifier %d used twice", rd_ident);

	while (AnzPlNamen >= MaxPlNamen)
	{
		int count;
		PlArray = gl_realloc((char*) PlArray,
			(MaxPlNamen += NAMES_OFFSET) * sizeof(struct place *));
		for (count = MaxPlNamen - NAMES_OFFSET; count < MaxPlNamen;)
			PlArray[count++] = NULL;
	}

	PlArray[rd_ident] = nc_create_place ();
	PlArray[rd_ident]->name = rd_name? gl_strdup(rd_name) : NULL;
	if (rd_marked > 1) gl_err ("place %s has more than one token",rd_name);
	PlArray[rd_ident]->m = !!rd_marked;
	return 0;
}

static int _insert_trans()
{
	if (!transcount++) autonumbering = 1;
	if (rd_ident && rd_ident != transcount) autonumbering = 0;
	if (!rd_ident && autonumbering) rd_ident = transcount;
	if (!rd_ident) gl_err ("missing transition identifier");

	if (rd_ident > AnzTrNamen)
		AnzTrNamen = rd_ident;
	else if (TrArray[rd_ident])
		gl_err ("transition identifier %d used twice",rd_ident);

	while (AnzTrNamen >= MaxTrNamen)
	{
		int count;
		TrArray = gl_realloc((char*) TrArray,
			  (MaxTrNamen += NAMES_OFFSET) * sizeof(struct trans *));
		for (count = MaxTrNamen - NAMES_OFFSET; count < MaxTrNamen;)
			TrArray[count++] = NULL;
	}

	TrArray[rd_ident] = nc_create_transition ();
	TrArray[rd_ident]->name = rd_name? gl_strdup(rd_name) : NULL;
	return 0;
}

static int _insert_arc()
{
	static int tp = 0;  /* tp = 1 means Place->Trans, 0 is Trans->Place */
	int pl, tr;

	if (*blocktype)
	{
		tp = strcmp(blocktype,"PT");
		*blocktype = '\0';
	}
		
	pl = tp? rd_co->y : rd_co->x;
	tr = tp? rd_co->x : rd_co->y;

	if (!tr || (tr > AnzTrNamen) || !TrArray[tr])
		gl_err ("arc: incorrect transition identifier");
	if (!pl || (pl > AnzPlNamen) || !PlArray[pl] )
		gl_err ("arc: incorrect place identifier");

	tp? nc_create_arc (&(TrArray[tr]->post), &(PlArray[pl]->pre),
			   TrArray[tr], PlArray[pl])
	  : nc_create_arc (&(PlArray[pl]->post), &(TrArray[tr]->pre),
			   PlArray[pl], TrArray[tr]);
	return 0;
}

static int _insert_ra()
{
	int ret;
	int tr = rd_co->x, pl = rd_co->y;

	if (!tr || (tr > AnzTrNamen) || !TrArray[tr])
		gl_err ("readarc: incorrect transition identifier");
	if (!pl || (pl > AnzPlNamen) || !PlArray[pl] )
		gl_err ("readarc: incorrect place identifier");

#if 0
	/* for now, I'm going to cheat and treat read arcs as
	   conventional arcs in both directions... */
	nc_create_arc(&(PlArray[pl]->postset),&(TrArray[tr]->preset),
			  PlArray[pl],TrArray[tr]);
	nc_create_arc(&(TrArray[tr]->postset),&(PlArray[pl]->preset),
			  TrArray[tr],PlArray[pl]);
#endif
	ret = nc_create_arc (&TrArray[tr]->cont, &PlArray[pl]->cont,
			TrArray[tr], PlArray[pl]);
	if (ret) u.net.isplain = 0;
	return 0;
}

static void _insert_t0 (void)
{
	struct trans *t0;
	struct ls *n;
	struct place *p;

	t0 = gl_malloc (sizeof (struct trans));
	ls_insert (&u.net.trans, &t0->nod);

	t0->name = "_t0_";
	t0->id = 0;
	al_init (&t0->pre);
	al_init (&t0->post);
	al_init (&t0->cont);
	ls_init (&t0->events);
	t0->m = 0;
	t0->parikhcnt1 = 0;
	t0->parikhcnt2 = 0;

	/* the postset of t0 consist on all marked places */
	for (n = u.net.places.next; n; n = n->next) {
		p = ls_i (struct place, n, nod);
		if (p->m) {
			al_add (&t0->post, p);
			al_add (&p->pre, t0);

			/* we remove the mark :) */
			p->m = 0;
		}
	}

	/* a pointer to t0 is stored in u.net */
	u.net.t0 = t0;
}

/*****************************************************************************/
/* The main function of this file: read a PEP file into a struct net.   */

void read_pep_net (char *PEPfilename)
{
	/* These tables instruct read_PEP_net where contents 
	   of certain fields should be stored.		    */
	
	t_dest place_dest[] =
		{ { '\'',&rd_name },	/* identifier		*/
		  { '"', &rd_name },	/*     "		*/
		  { '0', &rd_ident },	/* numeric name		*/
		  { 'M', &rd_marked },	/* initial marking	*/
		  {  0 ,  0 } };
	  
	t_dest trans_dest[] =
		{ { '\'',&rd_name },	/* identifier		*/
		  { '"', &rd_name },	/*     "		*/
		  { '0', &rd_ident },	/* numeric name		*/
		  {  0 ,  0 } };
	  
	t_dest arc_dest[] =
		{ { '@', &rd_co },	/* source/destination	*/
		  {  0 ,  0 } };

	/* This table is passed to read_PEP_net and instructs   */
	/* it which "hook" functions (see below) should be	*/
	/* called whenever a place, transition etc. is found.	*/

	t_blockdest netdest[] =
		{ { "PL",  _insert_place, NULL, place_dest },
		  { "TR",  _insert_trans, NULL, trans_dest },
		  { "TP",  _insert_arc,   NULL, arc_dest },
		  { "PT",  _insert_arc,   NULL, arc_dest },
		  { "RA",  _insert_ra,    NULL, arc_dest },
		  { NULL, NULL, NULL, NULL } };

	int count;

	/* Set up tables. */
	PlArray = gl_malloc((count = MaxPlNamen = MaxTrNamen = NAMES_START)
				* sizeof(struct place *));
	TrArray = gl_malloc(count * sizeof(struct trans *));
	AnzPlNamen = AnzTrNamen = 0;
	while (--count)
		PlArray[count] = NULL, TrArray[count] = NULL;

	/* Initialize net */
	nc_create_net ();

	placecount = transcount = 0;
	autonumbering = 1;

	/* Read the net */
	_read_PEP_file(PEPfilename, type_llnet, netblocks, netdest);

	gl_free(PlArray);
	gl_free(TrArray);

	/* this is cheating! we now append some initial transition t0
	 * generating the initial marking */
	_insert_t0 ();
}

