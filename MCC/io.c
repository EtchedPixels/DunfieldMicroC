/*
 * Unix I/O routines for the DDS MICRO-C compiler.
 *
 * These I/O routines should work on any "UNIX" system, or any
 * system supporting a "UNIX compatible" I/O library.
 *
 * To reduce the difficulty in porting to non-standard systems, these
 * routines use as few library functions as possible. There may be more
 * efficent ways to perform I/O in a given environment.
 *
 * For compilers which use multiple memory models (68HC16, 8051 & 8086), add
 * the line '#define MODEL' at the beginning of this file before compiling.
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

#include <stdio.h>

#ifndef LINE_SIZE
#include "compile.h"

/* Variables in the COMPILE.C module */
extern char line, comment, fold;	/* Command line switch flags */
extern char file_name[];			/* Filename table */
extern unsigned file_depth;			/* Include depth indicator */
#endif

FILE *fp_table[INCL_DEPTH+1] = { 0 }, *output_fp = 0;
char quiet = 0;

#ifdef MODEL			/* compiler's with multiple memory models only */
extern char model;					/* memory model to use */
#endif
extern char symbolic;				/* Symbolic output */

#ifndef CPU
#define	CPU	"PC86"
#endif

#ifdef DEMO
extern char demo_text[];
char hello[] = { "DDS MICRO-C "#CPU" Compiler (Demo)\n\
?COPY.TXT 1988-2005 Dave Dunfield\n\
**See COPY.TXT**.\n" };
#else
char hello[] = { "DDS MICRO-C "#CPU" Compiler\n\
?COPY.TXT 1988-2005 Dave Dunfield\n\
**See COPY.TXT**.\n" };
#endif

/*
 * Initialize I/O & execute compiler
 */
main(argc, argv)
	int argc;
	char *argv[];
{
	int i;
	char *ptr;

/* first process any filenames and command line options */
	for(i=1; i < argc; ++i) {
		ptr = argv[i];
		switch((*ptr++ << 8) + *ptr++) {
			case ('-'<<8)+'c' :		/* Include comments */
				comment = -1;
				break;
			case ('-'<<8)+'f' :		/* Fold literal strings */
				fold = -1;
				break;
			case ('-'<<8)+'l' :		/* Accept line numbers */
				line = -1;
				break;
			case ('-'<<8)+'q' :		/* Quiet mode */
				quiet = -1;
				break;
			case ('-'<<8)+'s' :		/* Symbolic output */
				symbolic = -1;
				break;
#ifdef MODEL
			case ('m'<<8)+'=' :		/* memory model to use */
				model = *ptr - '0';
				break;
#endif
			default:
				if(!fp_table[0]) {		/* Input file */
					copy_string(file_name, argv[i]);
					if(!(fp_table[0] = fopen(argv[i], "r")))
						severe_error("Cannot open input file"); }
				else if(!output_fp) {	/* Output file */
					if(!(output_fp = fopen(argv[i], "w")))
						severe_error("Cannot open output file"); }
				else
					severe_error("Too many parameters"); } }

/* Any files not explicitly named default to standard I/O */
	if(!fp_table[0])			/* default to standard input */
		fp_table[0] = stdin;
	if(!output_fp)				/* default to standard output */
		output_fp = stdout;

	if(!quiet)
		put_str(hello, 0);

	compile();
}

/*
 * Terminate the compiler
 */
terminate(rc)
	int rc;
{
	if(output_fp)
		fclose(output_fp);
	exit(rc ? -2 : 0);
}

/*
 * Write a number to file
 */
put_num(value, file)
	unsigned value;
	unsigned file;
{
	char stack[6];
	register unsigned i;

	i = 0;
	do
		stack[i++] = (value % 10) + '0';
	while(value /= 10);

	while(i)
		put_chr(stack[--i], file);
}

/*
 * Write a string to device indicated by "file"
 * (0 = console, non-0 = output file)
 */
put_str(ptr, file)
	char *ptr;
	unsigned file;
{
	while(*ptr)
		put_chr(*ptr++, file);
}

/*
 * The following routines use the standard library functions:
 * 'fopen', 'fclose', 'getc' and 'putc'
 * You may have to change these routines when porting
 * to a non-standard environment.
 */

/*
 * Stack previous input file & open a new one
 */
f_open(name)
	char *name;
{
	FILE *fp;

	if(fp = fopen(name, "r")) {
		fp_table[file_depth+1] = fp;
		return -1; }
	return 0;
}

/*
 * Close input file & return to last one
 */
f_close()
{
	fclose(fp_table[file_depth]);
}

/*
 * Read a line from the source file, return 1 if end of file,
 */
get_lin(line)
	char *line;
{
	register int chr, i;
#ifdef DEMO
	static unsigned line_count = 0;
	if(++line_count >= MAX_LINES) {
		put_str(demo_text, 0);
		severe_error("Too many source lines"); }
#endif

	i = LINE_SIZE;
	while(--i) {
		if((chr = getc(fp_table[file_depth])) < 0) {
			if(i == (LINE_SIZE-1))
				return -1;
			break; }
		if(chr == '\n')
			break;
		*line++ = chr; }
	return *line = 0;
}

/*
 * Write character to device indicated by "file"
 * (0 = console, non-0 = output file)
 */
put_chr(chr, file)
	char chr;
	char file;
{
	putc(chr, file ? output_fp : stderr);
}
