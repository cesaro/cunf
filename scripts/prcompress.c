#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <err.h>
#include <fcntl.h>
#include <stdarg.h>
#include <assert.h>

/***************************************************************************/
/* Miscellaneous stuff                                                     */

void error (const char * fmt, ...)
{
	va_list	args;

	va_start(args, fmt);
	verrx(EXIT_FAILURE, fmt, args);
}

char *bpos;

static unsigned int read_int ()
{
        assert (sizeof (int) == 4);
	int i = *(int*)bpos;
	bpos += sizeof(int);
	return i;
}

static void read_str ()
{
	while (*bpos++);
}

/***************************************************************************/
/* Unordered lists                                                         */

typedef struct list_t {
	int alloc;
	int size;
	long int contents;
} list_t;

void l_init (list_t **l)
{
	*l = malloc(6*sizeof(long int));
	(*l)->alloc = 4; (*l)->size = 0;
}

void l_reset (list_t **l)
{
	(*l)->size = 0;
}

void l_add (list_t **l, long int e)
{
	if ((*l)->alloc == (*l)->size)
	{
		int newsize = 2 * (*l)->alloc;
		(*l) = realloc(*l,(2+newsize)*sizeof(long int));
		(*l)->alloc = newsize;
	}
	long int *c = &((*l)->contents);
	c[(*l)->size++] = e;
}


/*****************************************************************************/
/* ordered list (from mole)                                                  */

#define LI_ALLOC_STEP 1024 /* how many nodelist elements to grab at a time  */

typedef struct nodelist_t
{
        int node;
        struct nodelist_t* next;
} nodelist_t;

typedef struct contingent_t
{
	nodelist_t *nodes;
	struct contingent_t *next;
} contingent_t;

nodelist_t *li_freelist = NULL;	  /* pointer to freed nodelist elements */
contingent_t *li_contlist = NULL; /* pointer to allocated contingents   */
int li_counter = 0;    /* num of free elements in the latest contingent */

nodelist_t* nodelist_alloc ()
{
	nodelist_t *tmp_nl;
	contingent_t *tmp_co;

	if ((tmp_nl = li_freelist))
	{
		li_freelist = tmp_nl->next;
		return tmp_nl;
	}

	if (li_counter--) return li_contlist->nodes + li_counter;

	tmp_co = malloc(sizeof(contingent_t));
	tmp_co->nodes = malloc(LI_ALLOC_STEP * sizeof(nodelist_t));
	tmp_co->next = li_contlist;
	li_contlist = tmp_co;
	li_counter = LI_ALLOC_STEP-1;

	return li_contlist->nodes + li_counter;
}

nodelist_t* nodelist_push (nodelist_t **list, int node)
{
	nodelist_t *newlist = nodelist_alloc();
	newlist->node = node;
	newlist->next = *list;
	return *list = newlist;
}

nodelist_t* nodelist_insert (nodelist_t **list, int node)
{
	while (*list && node < (*list)->node)
		list = &((*list)->next);

	if (*list && (*list)->node == node) return *list;

	return nodelist_push(list,node);
}

void nodelist_delete (nodelist_t *list)
{
	nodelist_t *tmp;

	while (list)
	{
		tmp = list;
		list = list->next;
		tmp->next = li_freelist;
		li_freelist = tmp;
	}
}

char nodelist_compare (nodelist_t *list1, nodelist_t *list2)
{
	while (list1 && list2 && list1->node == list2->node)
		list1 = list1->next, list2 = list2->next;

	if (!list1 && !list2) return 0;
	if (!list1 || (list2 && list1->node < list2->node)) return -1;
	return 1;
}

typedef struct hashcell_t
{
	nodelist_t *marking;
	int event;
	struct hashcell_t *next;
} hashcell_t;

hashcell_t **hash;
int hash_buckets;

void marking_init (int buckets)
{
	hash_buckets = buckets;
	hash = calloc(1,hash_buckets * sizeof(hashcell_t*));
}

int marking_hash (nodelist_t *marking)
{
	unsigned int val = 0, i = 0;
	while (marking)
	{
		val += marking->node * ++i;
		marking = marking->next;
	}
	return val % hash_buckets;
}

int add_marking (nodelist_t *marking, int ev)
{
	hashcell_t *newbuck;
	hashcell_t **buck = hash + marking_hash(marking);
	char cmp = 2;

	while (*buck && (cmp = nodelist_compare(marking,(*buck)->marking)) > 0)
		buck = &((*buck)->next);

	if (!cmp)	/* marking is already present */
	{
		nodelist_delete(marking);
		return (*buck)->event;
	}

	newbuck = malloc(sizeof(hashcell_t));
	newbuck->marking = marking;
	newbuck->event = ev;
	newbuck->next = *buck;
	*buck = newbuck;

	return 0;
}

/***************************************************************************/
/* Data structures for unfolding and global variables                      */

typedef struct cond_t {
	int plnum;
	list_t *postset;
	list_t *context;
	int parent;
	int ncond;  // map to folded condition
} cond_t;

typedef struct event_t {
	int trnum;
	int precounter;
	list_t *postset;
	list_t *preset;
	int nev;
	int cutoff;
} event_t;

char output_dot, output_stats;
int numpl, numtr, numev, numco, maxlen;
int numcutoffs;
cond_t *conds;
event_t *events;
char **plname;		// names of places
char **trname;		// names of places
int *evqueue;
int evbot, evtop;

int nnco = 0;
int nnev = 0;
cond_t *nconds;

/***************************************************************************
 This part traverses the list of places obtained from the prefix
 and maps them back to names before the PR-construction, removing the __i
 suffixes where necessary.

 nnpl = number of places after cleansing the suffixes
 pltranslate[] = map of place indexes in the PR net to the folded net
 newplname[] = the places of the folded net
 newcond[] = while treating an event e, this struct keeps track of the
	conditions in the new postset of e
 oldcond[] = same as newcond but for context
*/

typedef struct repcond_t {
	int cond;	// condition number (in new unfolding)
	int event;	// event number
} repcond_t;

typedef struct newevent_t {
	int trnum;
	int nchist;
	int cohist;
	int map;
} newevent_t;

int nnpl = 0;
int *pltranslate;
char **newplname;
repcond_t *newcond, *oldcond;
newevent_t *nevs;
int* ne_invmap;
int numwhites, numgrays;

void translate_places ()
{
	int p, j;

	pltranslate = malloc((numpl+1) * sizeof(int));
	newplname = malloc((numpl+1) * sizeof(char*));
	newcond = malloc((numpl+1) * sizeof(repcond_t));
	oldcond = malloc((numpl+1) * sizeof(repcond_t));

	for (p = 1; p <= numpl; p++)
	{
		char *s = plname[p], *t;
		newcond[p].event = -1;
		oldcond[p].event = -1;
		if ((t = strstr(s,"__")))
		{	// name of the form p__i
			char *u = strdup(s);

			u[t-s] = 0;
			
			// check if we've seen p before and if so, map to it
			for (j = nnpl; j > 0; j--)
				if (!strcmp(u,newplname[j]))
				{
					pltranslate[p] = j;
					free(u);
					break;
				}

			// otherwise create a new place p
			if (!j)
			{
				pltranslate[p] = ++nnpl;
				newplname[nnpl] = u;
			}
		}
		else
		{	// normal place => just copy it to the folded net
			pltranslate[p] = ++nnpl;
			newplname[nnpl] = s;
		}
		
	}
}

/***************************************************************************/

/* Condition c is in the postset of e, and it is not part of a read arc.
 * Find or generate the corresponding condition in the folded prefix. */
void insert_condition (int c, int e)
{
	int p = conds[c].plnum, np = pltranslate[p];

	// Check if e generates multiple copies of np, in which case
	// we map to the existing folded condition.
	if (newcond[np].event == e)
	{
		conds[c].ncond = newcond[np].cond;
		return;
	}

	// otherwise, generate a new f-condition
	conds[c].ncond = ++nnco;
	nconds[nnco].plnum = np;
	nconds[nnco].parent = nnev;
	l_init(&(nconds[nnco].postset));
	l_init(&(nconds[nnco].context));

	if (output_dot) printf("c%d [label=\"%s (c%d)\" shape=circle];\n",
			nnco,newplname[np],nnco);
	// FIXME production arc from e to nnco
	if (output_dot && nnev) printf("e%d -> c%d;\n",nnev,nnco);

	// record that we have generated an output place np for e
	newcond[np].event = e;
	newcond[np].cond = nnco;
}

/* decrements the counter of all events in the postset of condition c,
   putting them into the queue for a top-down traversal of the prefix */
void add_cond_postset (int c)
{
	int j;
	list_t *l = conds[c].postset;
	long int *lptr = &(l->contents);

	for (j = 0; j < l->size; j++)
	{
		int e = lptr[j];
		if (--events[e].precounter) continue;
		evqueue[evtop++] = e;
	}
}

void convert ()
{
	int c, e, e2, ne, j, p, np;
	list_t *l;
	long int *lptr;
	nodelist_t *marking;

	marking_init(numev);
	translate_places();
	nconds = malloc((numco+1) * sizeof(cond_t));
	nevs = malloc((numev+1) * sizeof(newevent_t));

	if (output_dot) printf("digraph test {\n");

	// look at the initial conditions, find some events to process
	for (c = 1; c < numco; c++)
	{
		if (conds[c].parent) continue;
		insert_condition(c,0);
		add_cond_postset(c);
	}

	/* perform a top-down traversal of the prefix that respects the
	   causal order (if e<f, then e is touched before f) */
	while (evbot < evtop)
	{
		e = evqueue[evbot++];

		marking = NULL;
		nodelist_insert(&marking,-events[e].trnum);

		// mark the places occurring in the preset of e
		l = events[e].preset;
		lptr = &(l->contents);
		for (j = 0; j < l->size; j++)
		{
			c = lptr[j];
			np = pltranslate[conds[c].plnum];
			oldcond[np].event = e;
			oldcond[np].cond = conds[c].ncond;
			nodelist_insert(&marking,conds[c].ncond);
		}

		e2 = add_marking(marking,e);
		if (e2)
		{
			// traverse e2's postset, setup oldcond
			l = events[e2].postset;
			lptr = &(l->contents);
			for (j = 0; j < l->size; j++)
			{
				c = lptr[j];
				p = conds[c].plnum;
				oldcond[p].event = e;
				oldcond[p].cond = conds[c].ncond;
			}

			// traverse e's postset, match to oldcond
			l = events[e].postset;
			lptr = &(l->contents);
			for (j = 0; j < l->size; j++)
			{
				c = lptr[j];
				p = conds[c].plnum;
				conds[c].ncond = oldcond[p].cond;
				add_cond_postset (c);
			}
			ne = events[e].nev = events[e2].nev;
			nevs[ne].cohist += events[e].cutoff;
			nevs[ne].nchist += !events[e].cutoff;
			continue;
		}

		// event does not exist yet -> insert it
		events[e].nev = ++nnev;
		nevs[nnev].trnum = events[e].trnum;
		nevs[nnev].cohist = events[e].cutoff;
		nevs[nnev].nchist = !events[e].cutoff;
		if (output_dot)
			printf("e%d [label=\"%s (e%d)\" shape=box];\n",
				nnev,trname[events[e].trnum],nnev);

		// find places consumed and produced -> read arcs
		l = events[e].postset;
		lptr = &(l->contents);
		for (j = 0; j < l->size; j++)
		{
			c = lptr[j];
			p = conds[c].plnum;
			np = pltranslate[p];

			if (oldcond[np].event == e)
			{
				// FIXME read arc between e and oldcond[np].cond
				if (output_dot)
					printf("c%d -> e%d [arrowhead=none];\n",
						oldcond[np].cond,nnev);

				// mark np as a context place
				newcond[np].event = e;
				conds[c].ncond = oldcond[np].cond;
				l_add(&(nconds[oldcond[np].cond].context),nnev);
			}
			else
				insert_condition(c,e);

			// find the next events to process
			add_cond_postset(c);
		}

		// produce consumption arcs for pre-conditions not reproduced
		l = events[e].preset;
		lptr = &(l->contents);
		for (j = 0; j < l->size; j++)
		{
			c = lptr[j];
			p = conds[c].plnum;
			np = pltranslate[p];

			if (newcond[np].event == e) continue;
			if (oldcond[np].event == -1) continue;

			// FIXME consumption arc from conds[c].ncond to e
			if (output_dot)
				printf("c%d -> e%d;\n",conds[c].ncond,nnev);
			l_add(&(nconds[conds[c].ncond].postset),nnev);

			oldcond[np].event = -1;
		}
	}

	if (output_dot) printf("}\n");

	ne_invmap = malloc((nnev+1) * sizeof(int));

	e2 = 0;
	for (e = 1; e <= nnev; e++)	// white events
		if (nevs[e].cohist == 0)
			{ nevs[e].map = ++e2; ne_invmap[e2] = e; }

	numwhites = e2;
	for (e = 1; e <= nnev; e++)	// gray events
		if (nevs[e].nchist && nevs[e].cohist)
			{ nevs[e].map = ++e2; ne_invmap[e2] = e; }

	numgrays = e2 - numwhites;
	for (e = 1; e <= nnev; e++)	// black events
		if (nevs[e].nchist == 0)
			{ nevs[e].map = ++e2; ne_invmap[e2] = e; }
}

/***************************************************************************/

void readmci (char *infile)
{
	int i, e;
	char *blob;

	/* open input and output files */
	struct stat statbuf;
	stat(infile,&statbuf);
	blob = malloc(statbuf.st_size);

	int fd = open(infile,O_RDONLY);
	if (fd < 0) { fprintf(stderr,"error opening %s\n",infile); exit(1); }

	read(fd,blob,statbuf.st_size);
	close(fd);
	bpos = blob;

	// start reading mci file
	numco = read_int();
	numev = read_int();

	conds = malloc((numco+1) * sizeof(cond_t));
	events = malloc((numev+1) * sizeof(event_t));

	evqueue = malloc(numev * sizeof(int));
	evbot = evtop = 0;

	// read events in mci file
	for (i = 1; i <= numev; i++)
	{
		events[i].trnum = read_int();
		events[i].precounter = 0;
		events[i].cutoff = 0;
		l_init(&(events[i].preset));
		l_init(&(events[i].postset));
	}

	for (i = 1; i <= numco; i++)
	{
		int p;
		conds[i].plnum = read_int();		// place label
		p = conds[i].parent = read_int();	// preset
		if (p) l_add(&(events[p].postset),i);
		l_init(&(conds[i].postset));
		while ((p = read_int()))
		{
			l_add(&(conds[i].postset),p);
			l_add(&(events[p].preset),i);
			events[p].precounter++;
		}
	}

	// read cutoff information
	numcutoffs = 0;
	while ((e = read_int()))
	{
		numcutoffs++;
		events[e].cutoff = 1;
		read_int();
	}

	while (read_int());

	// places and transitions
	numpl = read_int();
	numtr = read_int();
	maxlen = read_int();	// length of longest string

	// read place names
	plname = malloc((numpl+1) * sizeof(char*));
	trname = malloc((numtr+1) * sizeof(char*));
	for (i = 1; i <= numpl; i++) { plname[i] = strdup(bpos); read_str(); }
	read_str();
	for (i = 1; i <= numtr; i++) { trname[i] = strdup(bpos); read_str(); }

	free(blob);
}

/***************************************************************************/

void iwrite (int f, unsigned int i)
{
        i = htonl(i);
        write(f, &i, 4);
}

void writecuf (char *outfile)
{
	int i, j;
	long int *lptr;
	list_t *l;

	int f = creat(outfile,00666);

        assert(sizeof (unsigned int) == 4);

	iwrite(f,0x43554603);	// format version number
	iwrite(f,nnpl);		// number of places in the original net
	iwrite(f,numtr);	// number of transitions in the original net
	iwrite(f,nnco);		// number of conditions
	iwrite(f,nnev);		// number of events
	iwrite(f,numwhites);	// number of white events
	iwrite(f,numgrays);	// number of gray events
	iwrite(f,maxlen);	// maximum size of a place/transition name

	// list of events; each entry is the index of the associated transition
	// in the transition table, in section 10; white events are the first,
	// then gray, then black
	for (i = 1; i <= nnev; i++)
		iwrite(f,nevs[ne_invmap[i]].trnum-1);

	// list of conditions; each entry consist on:
	//     1. index of the associated place, in section 11
	//     2. 0xffffffff if initial; index of generating event otherwise
	//     3. postset size
	//     4. context size
	//     5. list of indexes of the events in the postset
	//     6. list of indexes of events in the context
	for (i = 1; i <= nnco; i++)
	{
		iwrite(f,nconds[i].plnum-1);
		iwrite(f,nconds[i].parent? nevs[nconds[i].parent].map-1
					 : 0xffffffff);
		iwrite(f,nconds[i].postset->size);
		iwrite(f,nconds[i].context->size);

		l = nconds[i].postset; lptr = &(l->contents);
		for (j = 0; j < l->size; j++)
			iwrite(f,nevs[lptr[j]].map-1);

		l = nconds[i].context; lptr = &(l->contents);
		for (j = 0; j < l->size; j++)
			iwrite(f,nevs[lptr[j]].map-1);
	}

	// list of transitions; each entry consist on the name of the transition
	//    terminated by a null character
	for (i = 1; i <= numtr; i++)
		write(f,trname[i],strlen(trname[i])+1);

	// list of places; each entry consist on the name of the place,
	//    terminated by a null character
	for (i = 1; i <= nnpl; i++)
		write(f,newplname[i],strlen(newplname[i])+1);

	close(f);
}

/***************************************************************************/

void report ()
{
	printf("net: |P|=%d, |T|=%d\n",numpl,numtr);
	printf("prefix: |C|=%d, |E|=%d (cutoffs=%d)\n",numco,numev,numcutoffs);
	printf("folded net: |P|=%d, |T|=%d\n",nnpl,numtr);
	printf("folded prefix: |C|=%d, |E|=%d (white=%d, gray=%d, black=%d)\n",
		nnco,nnev,numwhites,numgrays,nnev-numwhites-numgrays);
}

/***************************************************************************/

void usage ()
{
	fprintf(stderr,
	"\nusage: prcompress [options] <mcifile>\n\n"
	"  options:\n"
	"\t-o <filename>: output file (default: mcifile with .pr.cuf)\n"
	"\t-d: dump dot file to stdout\n"
	"\t-v: print statistics\n\n"
	);
	exit(1);
}

int main (int argc, char ** argv)
{
	int i;
        char outfile[1024];
	char *filename = NULL;

	*outfile = 0;
	output_dot = 0;
	output_stats = 0;

	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i],"-o"))
		{
			if (*outfile || ++i == argc) usage();
			strcpy(outfile,argv[i]);
		}
		else if (!strcmp(argv[i],"-d"))
			output_dot = 1;
		else if (!strcmp(argv[i],"-v"))
			output_stats = 1;
		else
		{
			if (filename) usage();
			filename = argv[i];
		}
	}

	if (!filename) usage();

	if (!*outfile || output_dot)
	{
		int l = strlen(filename);
		if (l < 5) error("'%s': filename too short!",filename);
		if (l > 1020) error("'%s': filename too long!",filename);
		strcpy(outfile,filename);
		strcpy(outfile + l - 4, ".pr.cuf");
	}

        readmci(filename);
	convert();
	if (!output_dot) writecuf(outfile);
	if (output_stats) report();

        return 0;
}
