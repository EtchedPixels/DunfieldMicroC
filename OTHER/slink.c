/*
 * DDS MICRO-C Source Linker
 *
 * This program perform "linking" of MICRO-C library functions, at the
 * assembly source code level. It processes an assembly language source
 * file, and uses special comments placed therein by the MICRO-C code
 * generator to determine the external references. Using an index file,
 * the external references are matched to library files which are then
 * included in the source.
 *
 * In addition to the above, all local compiler-generated symbols (which
 * begin with '?') are adjusted to be unique in each source file.
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

#include <stdio.h>
#include <ctype.h>			/* Non MICRO-C Only */
#include "microc.h"			/* Non MICRO-C Only */

/* Fixed parameters */
#define	NUM_SEG	10			/* Maximum number of segments */
#define	SYMSIZE	22			/* Maximum size of a symbol */
#define	FILESIZ	12			/* Size of a filename */
#define	LINESIZ	250			/* Maximum size of input line */
#define	MAXPRE	10			/* Maximum number of Prefix  files */
#define	MAXMID	5			/* Maximum number of Middle  files */
#define MAXSUF	5			/* Maximum number of Suffix  files */
#define	MAXCMD	50			/* Maximum number of files on the command line */
#define	POOLSIZ	3700		/* Size of string storage pool */
#ifndef DEMO
	#define	MAXLIB	200		/* Maximum number of Library files */
	#define	MAXSYMS	2000	/* Maximum number of symbols to process */
#else
	#define	MAXLIB	50		/* Maximum number of Library files */
	#define	MAXSYMS	500		/* Maximum number of symbols to process */
	#include "c:\project\demonote.txt"
#endif

/*
 * The following defines the character which is used to separate a
 * filename from the directory in a library file pathname. If not using
 * MSDOS, modify this definition, as well as the initial value of the
 * variable 'library', to constants which are suitable for your system.
 */
#define	DIRSEP	'\\'	/* directory-filename separator */

static char *library = "\\MC\\SLIB", *index_file = "EXTINDEX.LIB",
	quiet = 0, lstlib = 0, word = 0, srcfile = 0, prefix = '?',
	*tmp = "$", comment = '*', comkeep = -1, *lptr, skipstart = -1;

/* Table of external symbols and data areas */
static int sym_count = 0, data_count = MAXSYMS;
static char symbols[MAXSYMS][SYMSIZE+1];
static unsigned data_size[MAXSYMS];

/* Tables of files to include */
static int pre_count = 0, mid_count = 0, suf_count = 0, lib_count = 0,
	cmd_count = 0;
static char *pfiles[MAXPRE], *mfiles[MAXMID], *sfiles[MAXSUF],
	*lfiles[MAXLIB], *cfiles[MAXCMD], *fptr;

/* Table of reserved data areas */
static char allocate[50] = { " DS " };

static int out_count = 0;	/* Count of files processed */
static int err_count = 0;	/* Count of error occuring */

static unsigned active_libs = 0;/* Runtime library components to use */

static FILE *index_fp, *file_fp, *input_fp, *output_fp;

/* Table of segment file pointers */
static FILE *segfp[NUM_SEG] = { 0 } ;

/* Pool of storage for strings */
static char string_pool[POOLSIZ], *sp_top = &string_pool;

static char help_text[] = { "\n\
Use: slink <@infile ...> [opts] <outfile>\n\n\
Opts:	-c	remove Comments		c=char	Comment character\n\
	-l	list Libraries		i=file	Index filename\n\
	-q	Quiet mode		l=path	Library path\n\
	-s	Source file comments	p=char	Prefix character\n\
	-w	Word align data		s=@file	Startup override\n\
					t=str	Temporary file prefix\n\
" };

#ifdef DEMO
char hello[] = { "DDS MICRO-C Source Linker (Demo)\n\
?COPY.TXT 1990-2005 Dave Dunfield\n\
**See COPY.TXT**.\n" };
#else
char hello[] = { "DDS MICRO-C Source Linker\n\
?COPY.TXT 1990-2005 Dave Dunfield\n\
**See COPY.TXT**.\n" };
#endif

/*
 * Add a string to the literal pool
 */
char *add_pool(char *s)
{
	char *x;
	x = sp_top;
	while(*sp_top++ = *s++);
	return x;
}

/*
 * Add file(s) to a processing list
 */
add_file(char *list[], unsigned *index, char *f)
{
	FILE *fp;
	char buffer[100], *ptr, c;

new:
	while(isspace(*f)) ++f;
	if(*f == '@') {		/* Adding a file */
		ptr = ++f;
		while(((c = *f) != ',') && *f) ++f;
		*f++ = 0;
		if(!(fp = fopen(ptr, "r"))) {
			text_error("Cannot open", ptr);
			exit(-1); }
		while(fgets(ptr = buffer, sizeof(buffer), fp)) {
			while(isspace(*ptr)) ++ptr;
			if(*ptr)
				list[(*index)++] = add_pool(ptr); }
		fclose(fp);
		if(c)
			goto new;
		return; }

	list[(*index)++] = f;
	for(;;) {
		switch(c = *f++) {
		case 0 :
			return;
		case ',' :
			*(f-1)=0;
			goto new; } }
}

/*
 * Open a file from the library
 */
FILE *open_lib(fname)
	char *fname;
{
	char *ptr, *ptr1, c;
	FILE *fp;

	fptr = ptr1 = sp_top;	/* Build filename on end of pool */
	ptr = library;
	while(*ptr)
		*ptr1++ = *ptr++;
	*ptr1++ = DIRSEP;
	ptr = ptr1;
	while(c = *fname++) {
		if((c == '\\') || (c == ':'))
			fptr = ptr;
		*ptr1++ = c; }
	*ptr1 = 0;
	if(!(fp = fopen(fptr, "r"))) {
		text_error("Cannot open", fptr);
		++err_count; }
	return fp;
}

/*
 * Main program, process files
 */
main(argc, argv)
	int argc;
	char *argv[];
{
	int i;
	unsigned s;
	char *ptr;

/* first process any filenames and command line options */
	for(i=1; i < argc; ++i) {
		ptr = argv[i];
		switch((toupper(*ptr++) << 8) | toupper(*ptr++)) {
			case ('-'<<8)|'C' :			/* Eliminate comments */
				comkeep = 0;
				break;
			case ('-'<<8)|'L' :			/* List libraries */
				lstlib = -1;
				break;
			case ('-'<<8)|'Q' :			/* Quiet mode */
				quiet = -1;
				break;
			case ('-'<<8)|'S' :			/* Source file comments */
				srcfile = -1;
				break;
			case ('-'<<8)|'W' :			/* Word align symbols */
				word = -1;
				break;
			case ('C'<<8)|'=' :			/* Specify comment character */
				comment = *ptr;
				break;
			case ('I'<<8)|'=' :			/* Specify INDEX file */
				index_file = ptr;
				while(*ptr) {
					*ptr = toupper(*ptr);
					++ptr; }
				break;
			case ('L'<<8)|'=' :			/* Specify library */
				library = ptr;
				break;
			case ('P'<<8)|'=' :			/* Generated symbol prefix */
				prefix = *ptr;
				break;
			case ('S'<<8)|'=' :			/* Startup file override */
				add_file(pfiles, &pre_count, ptr);
				skipstart = 0;
				break;
			case ('T'<<8)|'=' :			/* Temporary file prefix */
				tmp = ptr;
				break;
			case '?' << 8 :				/* Help request */
				argc = 0;
				break;
			default:
				add_file(cfiles, &cmd_count, argv[i]); } }


	if(!quiet)
		fputs(hello, stderr);

	if(cmd_count < 2)
		abort(help_text);

	if(!(segfp[0] = output_fp = fopen(ptr = cfiles[--cmd_count], "w"))) {
		text_error("Cannot open", ptr);
		exit(-1); }

/* Open the library index */
	if(!(index_fp = open_lib(index_file)))
		exit(-1);

/* Process index file for fixed inputs */
	process_index();

/* Process any Prefix files */
	for(i = 0; i < pre_count; ++i) {
		ptr = pfiles[i];
		if(lstlib)
			text_error("Including Prefix ", ptr);
		if(file_fp = open_lib(ptr))
			process_file(file_fp); }

/* Process the users input files */
	for(i=0; i < cmd_count; ++i) {
		if(input_fp = fopen(fptr = cfiles[i], "r"))
			process_file(input_fp);
		else {
			text_error("Cannot open", fptr);
			++err_count; } }

/* Process any library files */
	for(i = 0; i < lib_count; ++i) {
		ptr = lfiles[i];
		if(lstlib)
			text_error("Including Library", ptr);
		if(file_fp = open_lib(ptr))
			process_file(file_fp); }

/* Process any middle files */
	for(i = 0; i < mid_count; ++i) {
		ptr = mfiles[i];
		if(lstlib)
			text_error("Including Middle ", ptr);
		if(file_fp = open_lib(ptr))
			process_file(file_fp); }

/* Reserve allocated memory areas */
	flush_segment();
	if(word) {			/* Word aligned output */
		for(i=MAXSYMS; i > data_count;) {
			if(!((s = data_size[--i]) & 1)) {	/* Output even sized first */
				fputs(symbols[i], output_fp);
				fputs(allocate, output_fp);
				fputn(s, output_fp);
				putc('\n', output_fp); } }
		for(i=MAXSYMS; i > data_count;) {
			if((s = data_size[--i]) & 1) {		/* Output odd sized last */
				fputs(symbols[i], output_fp);
				fputs(allocate, output_fp);
				fputn(s, output_fp);
				putc('\n', output_fp); } } }
	else for(i=MAXSYMS; i > data_count;) {
		fputs(symbols[--i], output_fp);
		fputs(allocate, output_fp);
		fputn(data_size[i], output_fp);
		putc('\n', output_fp); }

/* Process any suffix files */
	for(i=0; i < suf_count; ++i) {
		ptr = sfiles[i];
		if(lstlib)
			text_error("Including Suffix ", ptr);
		if(file_fp = open_lib(ptr))
			process_file(file_fp); }

	flush_segment();
	fclose(output_fp);
	fclose(index_fp);

	return err_count ? -2 : 0;
}

/*
 * Process index file, looking for initials
 */
process_index()
{
	char buffer[LINESIZ+1], filelist[LINESIZ];

	while(fgets(buffer, LINESIZ, index_fp)) switch(*buffer) {
		case '<' :		/* Prefix files */
			if(skipstart) {
				strcpy(lptr = filelist, &buffer[1]);
				while(parse(buffer))
					pfiles[pre_count++] = add_pool(buffer); }
			skipstart = -1;
			break;
		case '^' :		/* Middle file */
			strcpy(lptr = filelist, &buffer[1]);
			while(parse(buffer))
				mfiles[mid_count++] = add_pool(buffer);
			break;
		case '>' :		/* Suffix files */
			strcpy(lptr = filelist, &buffer[1]);
			while(parse(buffer))
				sfiles[suf_count++] = add_pool(buffer);
			break;
		case '$' :		/* Allocation directive */
			strcpy(allocate, &buffer[1]); }
}

/*
 * Check for buffer match
 */
check_file(char *f)
{
	char *p, c, flag;

	do {
		p = index_file;
		flag = -1;
		while((c = *f++) && (c != ',')) {
			if(toupper(c) != *p++)
				flag = 0; }
		if(flag) {
			return -1; } }
	while(c == ',');
	return 0;
}

/*
 * Read a file and process any external references in it,
 * also, patch any local symbols (?n) to be unique.
 */
process_file(fp)
	FILE *fp;
{
	unsigned output_enable;
	char c, buffer[LINESIZ+1], *ptr;

	output_fp = segfp[0];		/* Always begin with seg 0 */
	output_enable = -1;

	if(srcfile) {
		putc(comment, output_fp);
		fputs("#link ", output_fp);
		putc(prefix, output_fp);
		putc(out_count/26 + 'A', output_fp);
		putc(out_count%26 + 'A', output_fp);
		putc(' ', output_fp);
		fputs(fptr, output_fp);
		putc('\n', output_fp); }

	while(fgets(buffer, LINESIZ, fp)) {
		if((buffer[0] == '$') && (buffer[3] == ':'))
			switch((toupper(buffer[1]) << 8)|toupper(buffer[2])) {
				case ('E'<<8)|'X' :		/* External definition */
					lookup(&buffer[4]);
					continue;
				case ('D'<<8)|'D' :		/* Data declaration */
					if(output_enable)
						reserve_data(&buffer[4]);
					continue;
				case ('S'<<8)|'E' :		/* Segment selection */
					ptr = &buffer[4];
					while(isspace(*ptr)) ++ptr;
					select_segment(*ptr - '0');
					continue;
				case ('R'<<8)|'L' :		/* Runtime library use */
					active_libs |= get_number(buffer+4);
					continue;
				case ('R'<<8)|'S' :		/* Runtime library Section */
					if(output_enable = get_number(buffer+4))
						output_enable &= active_libs;
					else
						output_enable = -1;
					continue;
				case ('R'<<8)|'F' :		/* Runtime file select */
					output_enable = check_file(buffer+4);
					continue;
				case ('R'<<8)|'N' :		/* Not runtime file */
					output_enable = !check_file(buffer+4);
					continue;
				case ('F'<<8)|'S' :		/* Segment sequence */
					flush_segment();
					output_fp = segfp[0];
					output_enable = -1;
					continue; }

		if(output_enable) {
			if(*(ptr = buffer) == comment) {	/* Comment line */
				if(buffer[1] != '#') {			/* Process specials normally */
					if(!comkeep)				/* Disabled, remove */
						continue;
					if(isdigit(buffer[1])) {	/* MCP output line - seg0 */
				put0:	fputs(buffer, segfp[0]);
						putc('\n', segfp[0]);
						continue; }
					fputs(buffer, output_fp);
					putc('\n', output_fp);
					continue; }
				/* Output #file in segment#0 */
				for(c=2; c < 7; ++c) {
					if(buffer[c] != "##file "[c])
						goto nofile; }
				goto put0; }
		nofile:
			while(c = *ptr++) {
				putc(c, output_fp);
				if((c == prefix) && isdigit(*ptr)) {	/* Local label */
					putc(out_count/26 + 'A', output_fp);
					putc(out_count%26 + 'A', output_fp); } }
			putc('\n', output_fp); } }
	++out_count;
	fclose(fp);
}

/*
 * Reserve an unitinialized data area
 */
reserve_data(symbol)
	char *symbol;
{
	char *ptr;

	/* Parse symbol from input line */
	while(isspace(*symbol))		/* Skip leading blanks */
		++symbol;
	ptr = symbols[--data_count];
	/* If sumbol is compiler generated... convert */
	if((*symbol == prefix) && isdigit(*(symbol+1))) {
		*ptr++ = prefix;
		*ptr++ = out_count/26 + 'A';
		*ptr++ = out_count%26 + 'A';
		++symbol; }
	while(*symbol && !isspace(*symbol))
		*ptr++ = *symbol++;
	*ptr = 0;					/* Drop trailing blanks */

	data_size[data_count] = get_number(symbol);
}

/*
 * Get a number from the input stream
 */
get_number(ptr)
	char *ptr;
{
	unsigned value, base, c;
	char mflag;

	mflag = 0;
	while(isspace(*ptr))
		++ptr;

	base = 10;
	switch(*ptr++) {
		case '$' :	base = 16;	break;
		case '@' :	base = 8;	break;
		case '%' :	base = 2;	break;
		case '-' :	mflag = -1;	break;
		default:	--ptr; }

	value = 0;
	for(;;) {
		if(isdigit(c = toupper(*ptr++)))
			c -= '0';
		else if(c >= 'A')
			c -= ('A' - 10);
		else
			break;
		if(c >= base)
			break;
		value = (value * base) + c; }

	return mflag ? -value : value;
}

/*
 * Lookup an external filename
 */
lookup(symbol)
	char *symbol;
{
	int i;
	char buffer[LINESIZ+1], filelist[LINESIZ], *ptr, flag;

	/* Parse symbol from input line */
	while(isspace(*symbol))		/* Skip leading blanks */
		++symbol;
	ptr = symbol;
	while(*ptr && !isspace(*ptr))
		++ptr;
	*ptr = 0;					/* Drop trailing blanks */

	/* Check for already processed symbols */
	for(i=0; i < sym_count; ++i)
		if(!strcmp(symbol, symbols[i]))
			return;
	if(sym_count >= MAXSYMS) {
#ifdef DEMO
		fputs(demo_text, stderr);
#endif
		text_error("Severe error", "Symbol table exausted");
		exit(-1); }

	strcpy(symbols[sym_count++], symbol);

	/* Look it up in the index file */
	rewind(index_fp);

	while(fgets(buffer, LINESIZ, index_fp)) switch(*buffer) {
		case '-' :		/* Library filename entry */
			strcpy(filelist, &buffer[1]);
		case '<' :		/* Prefix filename */
		case '^' :		/* Middle filename */
		case '>' :		/* Suffix filename */
		case '$' :		/* Data definition, ignore */
			flag = *buffer;
			break;
		default:		/* All other symbols */
			if(!strcmp(symbol, buffer)) {	/* Need this file */
				if(flag == '-') {
					lptr = filelist;
					while(parse(buffer)) {
						for(i=0; i < lib_count; ++i) {
							if(!strcmp(buffer, lfiles[i]))
								break; }
						if(i == lib_count) {
							if(lib_count >= MAXLIB) {
#ifdef DEMO
								fputs(demo_text, stderr);
#endif
								text_error("Severe error", "Library index full");
								exit(-1); }
							lfiles[lib_count++] = add_pool(buffer); } } }
				return; } }
}

/*
 * Select a new output segment
 */
select_segment(s)
	unsigned s;
{
	char fname[65], *ptr, *ptr1;

	if(s >= NUM_SEG)
		abort("Invalid $SE directive");

	if(!segfp[s]) {
		ptr = fname; ptr1 = tmp;
		while(*ptr1) *ptr++ = *ptr1++;
		*ptr++ = s + '0';
		*ptr = 0;
		if(!(segfp[s] = fopen(fname, "w")))
			abort("Cannot WRITE segment file"); }

	output_fp = segfp[s];
}

/*
 * Flush the output segments
 */
flush_segment()
{
	int s;
	char fname[65], buffer[LINESIZ+1], *ptr, *ptr1;
	FILE *fp;

	output_fp = segfp[0];
	for(s = 1; s < NUM_SEG; ++s)
		if(segfp[s]) {
			fclose(segfp[s]);
			ptr = fname; ptr1 = tmp;
			while(*ptr1) *ptr++ = *ptr1++;
			*ptr++ = s + '0';
			*ptr = 0;
			if(!(fp = fopen(fname, "r")))
				abort("Cannot READ segment file");
			while(fgets(buffer, LINESIZ, fp)) {
				fputs(buffer, output_fp);
				putc('\n', output_fp); }
			fclose(fp);
			delete(fname);
			segfp[s] = 0; }
}

/*
 * Parse of next name from file list
 */
parse(dest)
	char *dest;
{
	while(isspace(*lptr))
		++lptr;
	if(!*lptr)
		return 0;
	while(!isspace(*lptr) && *lptr)
		*dest++ = *lptr++;
	*dest = 0;
	return -1;
}

/*
 * Display an error message with text
 */
text_error(message, text)
	char *message, *text;
{
	fputs(message, stderr);
	fputs(": '", stderr);
	fputs(text, stderr);
	fputs("'\n", stderr);
}

/*
 * Write a number to file
 */
fputn(value, fp)
	unsigned value;
	FILE *fp;
{
	char stack[6];
	unsigned i;

	i = 0;
	do
		stack[i++] = (value % 10) + '0';
	while(value /= 10);

	while(i)
		putc(stack[--i], fp);
}
