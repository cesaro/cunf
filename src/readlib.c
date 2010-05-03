/*****************************************************************************/
/* readlib.c								     */
/*****************************************************************************/
/*				    Stefan Roemer,  06.02.1995 - 06.02.1995  */
/*				    Stefan Schwoon, April 1997		     */
/*****************************************************************************/
/* Some auxiliary functions for reading text files.			     */
/*****************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "readlib.h"
#include "glue.h"

/*****************************************************************************/

char *HLinput_file;	/* Name of file being processed.	*/
int   HLinput_line;	/* Current line of input.		*/

char *sbuf = NULL;	/* Buffer for storing strings in.	*/
int  sballoc = 0;	/* Amount of memory allocated for sbuf. */

/*****************************************************************************/
/* ReadCharComment							     */
/* Get next useful character in file (whitespace and comments are ignored.   */

char ReadCharComment (FILE *file)
{
	register char ch = '\0';

	do {
		ch = '\n';
		while (!feof(file) && strchr(" \t\b", ch = getc(file)));
		if (ch == '%')
		{
			while (!feof(file) && (ch = getc(file)) != '\n');
			if (ch == '\n')
			{
				HLinput_line++;
				ch = '\0';
			}
		}
	} while (!ch);

	return ch;
}

/*****************************************************************************/
/* ReadCmdToken								     */
/* Read the next string (an alphanumeric word) from a text file; leading     */
/* whitespace and comments are ignored. The word is stored in the global     */
/* variable sbuf.							     */

void ReadCmdToken (FILE *file)
{
	register int  len = 0;
	char *string = sbuf;

	if (!string) sbuf = string = gl_malloc (sballoc = 512);

	if (!isalnum((int)(*string++ = ReadCharComment(file))))
		gl_err ("ReadCmdToken: alphanumerical string expected");

	len++;
	while (isalnum((int)(*string = getc(file))) || *string == '_')
	{
		string++;
		len++;
		if (len >= sballoc)
		{
			sbuf = gl_realloc (sbuf,sballoc += 512);
			string = sbuf + len;
		}
	}

	ungetc(*string, file);
	*string = '\0';
}


/*****************************************************************************/
/* Find the next newline in a file.					     */

void ReadNewline (FILE *file)
{
	while (!feof(file) && getc(file) != '\n');
	HLinput_line++;
}

/*****************************************************************************/
/* Return the next char in a file, ignoring whitespace.			     */

char ReadWhiteSpace (FILE *file)
{
	register char ch = '\0';
	while (!feof(file) && isspace((int)(ch = getc(file))))
		if (ch == '\n') HLinput_line++;
	return ch;
}

/*****************************************************************************/
/* Treat the following chars in a file as a number an store its value in     */
/* *result. Leading whitespace is ignored. An error is issued if no digit    */
/* is found.								     */

void ReadNumber (FILE *file, int *result)
{
	register int  number = 0;
	register char digit;
	register int  vorz = 1;

	if ((digit = ReadWhiteSpace(file)) == '-')
	{
		vorz = -1;
		digit = ReadWhiteSpace(file);
	}

	if (isdigit((int)digit))
		number = digit - '0';
	else
		gl_err ("ReadNumber: digit expected");

	while (isdigit((int)(digit = getc(file))))
		number = number * 10 + digit - '0';

	ungetc(digit, file);
	*result = vorz * number;
}

/*****************************************************************************/
/* Read a string enclosed by (single or double) quotes. Leading whitespace   */
/* is ignored. The string is stored in the global variable sbuf. The string  */
/* in sbuf does not contain the quotes, only the chars enclosed by them.     */

void ReadEnclString (FILE *file)
{
	register char *str = sbuf;
	register int  len = 0;
	char          delimiter;

	if (!str) sbuf = str = gl_malloc (sballoc = 512);

	if ((delimiter = ReadCharComment(file)) != '\'' && delimiter != '"')
	      gl_err ("ReadEnclString: string leading ' or \" expected");

	while (!feof(file) && (*str = getc(file)) != delimiter)
	{
		str++;
		len++;
		if (len >= sballoc)
		{
			sbuf = gl_realloc (sbuf,sballoc += 512);
			str = sbuf + len;
		}
	} /* while */

	if (*str != delimiter)
	      gl_err ("ReadEnclString: closing %c is missing",delimiter);
	*str = '\0';
}

/*****************************************************************************/
/* Read a pair of coordinates (two numbers separated by @). Whitespace	     */
/* before or between the numbers and the @ is ignored.			     */

void ReadCoordinates (FILE *file, int *x, int *y)
{
	ReadNumber(file,x);
	if (ReadWhiteSpace(file) != '@')
		gl_err ("ReadCoordinates: '@' expected");
	ReadNumber(file,y);
}
