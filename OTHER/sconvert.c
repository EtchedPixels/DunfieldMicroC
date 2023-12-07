/*
 * DDS MICRO-C ASM Source Convertor
 *
 * This program "converts" an assembly language source file to a form which
 * is similar to the MICRO-C compiler output, and is compatible with the
 * source linker (SLINK).
 *
 * The file is reduced to a minimum size (to decrease linkage time) by
 * removing all comments, and reducing all spacing to single spaces.
 * Any symbols not defined in the KEEP file will be converted to resemble
 * MICRO-C compiler generated symbols, allowing SLINK to process them.
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 *
 * Compile command: cc sconvert -fop
 */
#include <stdio.h>

#define	MAX_KEEP	200		/* Maximum number of symbols to "keep" */
#define	MAX_SYMB	300		/* Maximum number of symbols to process */
#define	RAM_POOL	25000	/* Memory pool for symbol storage */
#define	LINE_SIZE	100		/* Maximum size of input line */

static char com1 = '*', com2 = 0, prefix = '?', quiet = 0;

static int kcount = 0, scount=0, pcount=0;
static char *keep[MAX_KEEP], *symbols[MAX_SYMB], pool[RAM_POOL];

static char *inptr, inbuf[LINE_SIZE+1], *outptr, outbuf[LINE_SIZE+1];

static FILE *ifp = 0, *ofp = 0;

/*
 * Add an entry to the ram pool & return the address
 */
char *add_pool(ptr)
	char *ptr;
{
	int i;

	i = pcount;
	do
		pool[pcount++] = *ptr;
	while(*ptr++);

	return &pool[i];
}

/*
 * Main program
 */
main(argc, argv)
	int argc;
	char *argv[];

{
	int i;
	char c, *ptr;
	FILE *fp;

	for(i=1; i < argc; ++i) {
		ptr = argv[i];
		switch((*ptr++ << 8) | *ptr++) {
			case ('k'<<8)|'=' :		/* Single name to KEEP */
				keep[kcount++] = add_pool(ptr);
				break;
			case ('K'<<8)|'=' :		/* File of names to KEEP */
				fp = fopen(ptr, "rvq");
				while(fgets(inbuf, LINE_SIZE, fp))
					if(issymbol(*inbuf))
						keep[kcount++] = add_pool(inbuf);
				fclose(fp);
				break;
			case ('p'<<8)|'=' :		/* Specify prefix */
				prefix = *ptr;
				break;
			case ('C'<<8)|'=' :		/* Specify line comment */
				com1 = *ptr;
				break;
			case ('c'<<8)|'=' :		/* Specify statement comment */
				com2 = *ptr;
				break;
			case ('-'<<8)|'q' :		/* Turn off verbose message */
				quiet = -1;
				break;
			case '?' << 8 :
				argc = 0;
				break;
			default:
				if(!ifp)
					ifp = fopen(argv[i], "rvq");
				else if(!ofp)
					ofp = fopen(argv[i], "wvq");
				else
					abort("Too many arguments"); } }

	/* Default to stdin/stdout if files not specified */
	if(!ifp)
		ifp = stdin;
	if(!ofp)
		ofp = stdout;

	if(!quiet)
		fputs("DDS MICRO-C ASM Source Converter\n?COPY.TXT 1990-2005 Dave Dunfield\n**See COPY.TXT**.\n", stderr);

	if(!argc)
		abort("\nUse: SCONVERT [input [output]] [c=char C=char k=symbol K=file p=char -q]\n");

/* Pass #1 - Record the symbol names */
	if(!quiet)
		fputs("First pass... ", stderr);
	while(fgets(inptr = inbuf, LINE_SIZE, ifp)) {
		if(strbeg(inbuf, "$DD:"))
			inptr += 4;
		if(issymbol(c = *inptr)) {		/* Symbol definition */
			ptr = inptr;
			do
				c = *++ptr;
			while(issymbol(c) || isdigit(c));
			*ptr = 0;
			for(i=0; i < kcount; ++i)
				if(!strcmp(keep[i], inptr))
					break;
			if(i >= kcount)
				symbols[scount++] = add_pool(inptr); } }

/* Pass #2 - Copy the file & perform substitutions */
	rewind(ifp);
	if(!quiet)
		fputs("Second pass... ", stderr);
	while(fgets(inptr = inbuf, LINE_SIZE, ifp)) {
		outptr = outbuf;
		if(*inptr && (*inptr != com1)) {
			/* First, copy over the symbol name */
			substitute(-1);
			next_field();
			/* And then copy the instruction "verbatium" */
			while(c = *inptr) {
				if((c == com2) || issep(c))
					break;
				*outptr++ = c;
				++inptr; }
			next_field();
			/* Then, process the operands */
			substitute(!com2);
			/* Remove any trailing blanks */
			while(issep(*(outptr-1)))
				--outptr;
			*outptr = 0;
			fputs(outbuf, ofp);
			putc('\n', ofp); } }

	if(!quiet)
		fputs("Done.\n", stderr);
	fclose(ifp);
	fclose(ofp);
}

/*
 * Process text & perform substitution
 */
substitute(term)
	char term;
{
	char c, buffer[50], *ptr;
	int i;

	while(c = *inptr) {
		if((term && issep(c)) || (c == com2))
			break;
		if(issymbol(c)) {				/* A symbol name */
			i = 0;
			do {
				buffer[i++] = c;
				c = *++inptr; }
			while(issymbol(c) || isdigit(c));
			buffer[i] = 0;
			for(i=0; i < scount; ++i)
				if(!strcmp(symbols[i], buffer))
					break;
			ptr = buffer;
			if(i < scount) {			/* Substituted symbol */
				*outptr++ = prefix;
				*(ptr = &buffer[10]) = 0;
				do
					*--ptr = (i%10) + '0';
				while(i /= 10); }
			while(c = *ptr++)			/* Copy over buffer */
				*outptr++ = c;
			continue; }
		*outptr++ = c;					/* Anything else */
		++inptr;
		if((c == '\'') || (c == '"'))	/* Quoted string */
			do
				*outptr++ = *inptr;
			while(*inptr++ != c); }
}

/*
 * Skip to the next field in the input file, and
 * insert a single space in the output file.
 */
next_field()
{
	/* Skip any trailing blanks */
	while(issep(*inptr))
		++inptr;
	*outptr++ = ' ';
}

/*
 * Test for a valid symbol name
 */
issymbol(c)
	char c;
{
	return ((c >= 'a') && (c <= 'z'))
		|| ((c >= 'A') && (c <= 'Z'))
		|| (c == '_') || (c == '?');
}

/*
 * Test for a valid separator character
 */
issep(c)
	char c;
{
	return (c == ' ') || (c == '\t');
}
