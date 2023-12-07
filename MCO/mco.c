/*
 * DDS MICRO-C Optimizer
 *
 * This post-processor optimizes by reading the assembly source
 * code produced by the compiler, and recognizing specific instruction
 * sequences which it replaces with more efficient ones. It is entirely
 * table driven, making it fairly easy to port to any processor.
 *
 * ?COPY.TXT 1989-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

#include <stdio.h>
#include <ctype.h>			/* Non MICRO-C only */
#include "microc.h"			/* Non MICRO-C only */

#include "PC86.mco"			/* Processor specific optimization table */

#define	PEEP_SIZE	15		/* size of peephole buffer */
#define	LINE_SIZE	251		/* maximum size of input line */
#define SYMBOL_SIZE	50		/* maximum size of symbol */
#define SYMBOLS		8		/* maximum # symbols per peep */
/* Bit defintions in "special" table characters */
#define	SYMMASK		007		/* Mask for symbol number */
#define	SYMNUM		030		/* Symbol must be numeric */
#define	SYMNOT		040		/* Complement symbol */

#ifndef CPU
	#define CPU "PC86"
#endif

/* circular peep hole buffer & read/write pointers */
static char peep_buffer[PEEP_SIZE][LINE_SIZE];
static unsigned peep_read = 0, peep_write = 0;

/* Symbol table */
static char symbols[SYMBOLS][SYMBOL_SIZE], sym_used[SYMBOLS];

/* misc variables */
static char debug = 0, quiet = 0;
static FILE *input_fp = 0, *output_fp = 0;

/*
 * Read a line into the peephole buffer from the input file.
 */
static read_line()
{
	if(fgets(peep_buffer[peep_write], LINE_SIZE, input_fp)) {
		peep_write = (peep_write+1) % PEEP_SIZE;
		return 1; }
	return 0;
}

/*
 * Write a line from the peephole buffer to the output file.
 */
static write_line()
{
	fputs(peep_buffer[peep_read], output_fp);
	peep_read = (peep_read + 1) % PEEP_SIZE;
	putc('\n', output_fp);
}

/*
 * Compare an optimization table entry with a series of
 * instructions in the peephole buffer.
 * Return:	0	= No match
 *			-1	= Partial match
 *			n	= Full match ending at entry 'n'
 */
static compare(ptr, peep)
	char *ptr;
	int peep;
{
	int i, j;
	char *ptr1, *ptr2, *ptr3, c, d;
#ifdef	LIMIT1
	unsigned x;
#endif

	for(i=0; i < SYMBOLS; ++i)
		sym_used[i] = 0;

	ptr1 = peep_buffer[peep];
	while(c = *ptr) {
		if(c == '\n') {				/* end of line */
			if(*ptr1)
				return 0;
			if((peep = (peep+1) % PEEP_SIZE) == peep_write)
				return -1;
			ptr1 = peep_buffer[peep]; }
		else if(c == ' ') {			/* spaces */
			if(!isspace(*ptr1))
				return 0;
			while(isspace(*ptr1))
				++ptr1; }
		else if(c & 0x80) {			/* symbol name */
			ptr2 = ptr3 = symbols[i = c & SYMMASK];
			d = *(ptr + 1);			/* Get terminator character */
			if(sym_used[i]) {		/* Symbol is already defined */
				while(*ptr1 && (*ptr1 != d))
					if(*ptr1++ != *ptr2++)
						return 0;
				if(*ptr2)
					return 0; }
			else {					/* new symbol definition */
				while(*ptr1 && (*ptr1 != d))
					*ptr2++ = *ptr1++;
				*ptr2 = 0;
				if(c & SYMNUM) {		/* Numbers only */
					while(*ptr3)
						if(!isdigit(*ptr3++))
							return 0;
#ifdef LIMIT1
					x = atoi(symbols[i]);
					switch(c & SYMNUM) {
						case 020: if(x > LIMIT1) return 0; break;
						case 030: if(x > LIMIT2) return 0; }
#endif
				}
				if(c & SYMNOT) {		/* Must be a NOT symbol */
					j = 0;
					do {
						if(!(ptr2 = not_table[j++]))
							return 0; }
					while(strcmp(ptr2, ptr3)); }
				sym_used[i] = -1; } }
		else if(c != *ptr1++)		/* normal character */
				return 0;
		++ptr; }
	return (*ptr1) ? 0 : peep + 1;
}

/*
 * Exchange new code for old code in the peephole buffer.
 */
static exchange(old, ptr)
	unsigned old;
	char *ptr;
{
	int i, j;
	char *ptr1, *ptr2, c;

	/* if debugging, display instruction removed by optimizer */
	if(debug) {
		j = old % PEEP_SIZE;
		for(i=peep_read; i != j; i = (i+1) % PEEP_SIZE)
			fprintf(stdout,"Take: %s\n", peep_buffer[i]); }

	ptr2 = peep_buffer[peep_read = (old + (PEEP_SIZE-1)) % PEEP_SIZE];
	while(c = *ptr++) {
		if(c & 0x80) {
			ptr1 = symbols[c & SYMMASK];
			if(c & SYMNOT) {	 		/* Notted symbol */
				for(i=0; not_table[i]; ++i)
					if(!strcmp(ptr1, not_table[i])) {
						ptr1 = not_table[i ^ 0x01];
						break; } }
			while(*ptr1)
				*ptr2++ = *ptr1++; }
		else if(c == '\n') {
			*ptr2 = 0;
			ptr2 = peep_buffer[peep_read = (peep_read + (PEEP_SIZE-1)) % PEEP_SIZE]; }
		else
			*ptr2++ = c; }
	*ptr2 = 0;

	/* if debugging, display instruction given by the optimizer */
	if(debug) {
		for(i=peep_read; i != j; i = (i+1) % PEEP_SIZE)
			fprintf(stdout,"Give: %s\n", peep_buffer[i]); }
}

/*
 * Main program, read & optimize assembler source
 */
main(argc, argv)
	int argc;
	char *argv[];
{
	int i, j;
	unsigned char *ptr;
#ifdef OPT_LEVEL
	unsigned char flag;
#endif

	/* first process any filenames and command line options */
	for(i=1; i < argc; ++i) {
		ptr = argv[i];
		switch((*ptr++ << 8) | *ptr++) {
			case ('-'<<8)+'d' :			/* Debug output */
				debug = -1;
				break;
			case ('-'<<8)+'q' :			/* Quiet mode */
				quiet = -1;
				break;
#ifdef OPT_LEVEL
			case ('o'<<8)+'=' :			/* Optimization level */
				if((opt_level = atoi(ptr)) > OPT_LEVEL)
					abort("Bad o= value\n");
				break;
#endif
			default:
				if(!input_fp) {			/* Input file */
					if(!(input_fp = fopen(argv[i], "r")))
						abort("Cannot open input file\n"); }
				else if(!output_fp) {	/* Output file */
					if(!(output_fp = fopen(argv[i], "w")))
						abort("Cannot open output file\n"); }
				else
					abort("Too many parameters\n"); } }

#ifdef OPT_LEVEL
	/* Process the optimizer table, and select cases by level */
	i = j = 0;
	flag = -1;
	while(ptr = peep_table[i++]) {
		if(((unsigned)ptr & 0xFF00) == 0xFF00) {
			flag = (unsigned)ptr & opt_level;
			continue; }
		if(flag)
			peep_table[j++] = ptr; }
	peep_table[j] = 0;
#endif

	/* Any files not explicitly named default to standard I/O */
	if(!input_fp)				/* default to standard input */
		input_fp = stdin;
	if(!output_fp)				/* default to standard output */
		output_fp = stdout;

	if(!quiet)
		fputs("DDS MICRO-C "CPU" Optimizer\n?COPY.TXT 1989-2005 Dave Dunfield\n**See COPY.TXT**.\n", stderr);

	for(;;) {
		if((peep_read == peep_write) || (j == -1)) {
			if(!read_line()) {		/* End of file */
				while(peep_read != peep_write)
					write_line();
				fclose(input_fp);
				fclose(output_fp);
				exit(0); } }
		for(i=0; ptr = peep_table[i]; i += 2) {
			if(j = compare(ptr, peep_read)) {	/* we have a match */
				if(j == -1)						/* partial, wait */
					break;
				exchange(j, peep_table[i+1]);
				break; } }
		if(!ptr)			/* no matches, flush this line */
			write_line(); }
}
