/*****************************************************************************/
/* readlib.h								     */
/*****************************************************************************/
/*				    Stefan Roemer,  06.02.1995 - 06.02.1995  */
/*				    Stefan Schwoon, April 1997		     */
/*****************************************************************************/

#ifndef __READLIB_H__
#define __READLIB_H__

/*****************************************************************************/

extern char *HLinput_file;	/* Name of file currently being processed. */
extern int   HLinput_line;	/* Number of current input line in file.   */

extern char *sbuf;	/* Some functions (ReadCmdToken, ReadEnclString)     */
extern int  sballoc;	/* store strings in this buffer. Memory is allocated */
			/* (and enlarged) automatically.		     */

/*****************************************************************************/

extern char ReadCharComment(FILE *file);
extern void ReadCmdToken(FILE *file);
extern void ReadNewline(FILE *file);
extern char ReadWhiteSpace(FILE *file);
extern void ReadNumber(FILE *file, int *x);
extern void ReadEnclString(FILE *file);
extern void ReadCoordinates(FILE *file, int *x, int *y);

#endif
