
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glue.h"
#include "debug.h"
#include "global.h"
#include "netconv.h"
#include "readpep.h"
#include "unfold.h"

/* all gloabal data is stored in this structure */
struct u u;

/*****************************************************************************/

void usage(char *myname)
{
	fprintf(stderr,
		"%s -- low level net unfolder\n\n"
		"Usage: %s [options] <LLnetfile> [FileOptions]\n\n"

	"     Options:\n"
	"      -T <name>      stop when transition <name> is inserted\n"
	"      -d <depth>     unfold up to given depth\n"
	"      -i             interactive mode\n\n"

	"     FileOptions:\n"
	"      -m <filename>  file to store the unfolding in\n\n"

	"Unless specified otherwise, all filenames will default to\n"
	"the basename of <LLnetfile> plus appropriate extensions.\n\n"

	"Version 1.0.6 (23.03.2006)\n", myname, myname);

	exit(1);
}

/*****************************************************************************/

int main (int argc, char **argv)
{
	int	 i;
	char    *llnet = NULL, *mcifile;
	char    **dptr = &llnet;
	char	*tmpname, *idx;

	/* initialize global parameters */
	u.stoptr = 0;
	u.depth = 0;
	u.interactive = 0;
	u.exitcode = 0;

	/* parse command line */
	for (i = 1; i < argc; i++)
		if (!strcmp(argv[i],"-m"))
			dptr = &mcifile;
		else if (!strcmp(argv[i],"-T"))
			u.stoptr = argv[++i];
		else if (!strcmp(argv[i],"-d"))
			u.depth = atoi(argv[++i]);
		else if (!strcmp(argv[i],"-i"))
			u.interactive = 1;
		else
		{
			if (!dptr) usage(argv[0]);
			*dptr = argv[i];

			tmpname = gl_malloc (strlen(argv[i])+5);
			strcpy(tmpname,argv[i]);
			idx = strrchr(tmpname,'.');
			if (!idx) strcat(tmpname,".");
			idx = strrchr(tmpname,'.');

			if (dptr == &llnet)
			{
				strcpy(idx,".mci");
				mcifile = gl_strdup (tmpname);
			}

			dptr = NULL;
		}

	if (!llnet) usage(argv[0]);

	read_pep_net (llnet);
	nc_static_checks ();
	db_net ();
	/* unfold();
	write_mci_file(mcifile); */

	return u.exitcode;
}

