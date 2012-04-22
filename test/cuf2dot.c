
/* 
 * Copyright (C) 2010, 2011  Cesar Rodriguez <cesar.rodriguez@lsv.ens-cachan.fr>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

#include "glue.h"
#include "debug.h"
#include "global.h"
#include "netconv.h"
#include "readpep.h"
#include "unfold.h"

#define P(args...) fprintf (fout, ##args)

static unsigned long int _read_int (const char * path, FILE * f)
{
	int i;

	ASSERT (sizeof (unsigned long int) == 4);
	if (fread (&i, 4, 1, f) != 1) {
		if (errno == 0) gl_err ("'%s': a file corruption detected!", path);
		gl_err ("'%s': %s", path, strerror (errno));
	}
	return ntohl (i);
}

static void _read_str (const char * path, FILE * f, char * buff, int size)
{
	int i;
	
	for (i = 0; i < size; i++) {
		if (fread (buff + i, 1, 1, f) != 1) {
			if (errno == 0) {
				BREAK (1);
				gl_err ("'%s': b file corruption detected!", path);
			}
			gl_err ("'%s': %s", path, strerror (errno));
		}
		if (buff[i] == 0) return;
	}
	if (i >= size) gl_err ("'%s': c file corruption detected!", path);
}

static void cuf2dot (const char * infile, const char * outfile)
{
	FILE *fin, *fout;
	int i, e, max, events, conds, white, gray, magic;
	char * buff;

	/* open input and output files */
	fin = fopen (infile, "r");
	if (fin == 0) gl_err ("'%s': %s", infile, strerror (errno));
	fout = fopen (outfile, "w");
	if (fout == 0) gl_err ("'%s': %s", outfile, strerror (errno));

	/* read first four fields */
	magic = _read_int (infile, fin);
	if (magic != 0x43554602) {
		gl_err ("'%s': not a CUF2 file", outfile, strerror (errno));
	}
	conds = _read_int (infile, fin);
	events = _read_int (infile, fin);
	white = _read_int (infile, fin);
	gray = _read_int (infile, fin);
	max = _read_int (infile, fin);

	/* allocate a buffer of size max + 16 :) */
	max += 16;
	buff = gl_malloc (max);

	/* read and print events */
	P ("digraph {\n\t/* white events */\n");
	P ("\tnode\t[shape=box style=filled fillcolor=gray95];\n");
	for (i = 1; i <= white; i++) {
		_read_str (infile, fin, buff, max);
		P ("\te%-6d [label=\"%s:e%d\"];\n", i, buff, i);
	}
	P ("\n\t/* gray events */\n\tnode\t[fillcolor=gray40]\n");
	for (; i <= white + gray; i++) {
		_read_str (infile, fin, buff, max);
		P ("\te%-6d [label=\"%s:e%d\"];\n", i, buff, i);
	}
	P ("\n\t/* black events */\n\tnode\t[shape=Msquare fillcolor=gray10 fontcolor=white]\n");
	for (; i <= events; i++) {
		_read_str (infile, fin, buff, max);
		P ("\te%-6d [label=\"%s:e%d\"];\n", i, buff, i);
	}

	/* read and print conditions and the flow and context relations */
	P ("\n\t/* conditions, flow and context relations */\n");
	P ("\tnode\t[shape=circle fillcolor=gray90 fontcolor=black];\n");
	for (i = 1; i <= conds; i++) {
		_read_str (infile, fin, buff, max);
		e = _read_int (infile, fin);
		P ("\tc%-6d [label=\"%s:c%d\"];%s\n",
				i, buff, i, !e ? " /* initial */" : "");
		if (e) P ("\te%-6d -> c%d;\n", e, i);
		while (1) {
			e = _read_int (infile, fin);
			if (! e) break;
			P ("\tc%-6d -> e%d;\n", i, e);
		}
		while (1) {
			e = _read_int (infile, fin);
			if (! e) break;
			P ("\tc%-6d -> e%d [arrowhead=none color=red];\n", i, e);
		}
	}
	P ("}\n");

	fclose (fin);
	fclose (fout);
}

int main (int argc, char ** argv)
{
	int l;
	char outfile[1024];

	ASSERT (argc == 2);
	ASSERT (argv[1] != 0);

	if (argc != 2 || argv[1] == 0) {
		gl_err ("Invalid arguments, see the code!");
	}
	l = strlen (argv[1]);
	if (l < 5) gl_err ("'%s': filename too short!", argv[1]);
	if (l > 1020) gl_err ("'%s': filename too long!", argv[1]);
	strcpy (outfile, argv[1]);
	strcpy (outfile + l - 3, "dot");

	cuf2dot (argv[1], outfile);
	return EXIT_SUCCESS;
}

