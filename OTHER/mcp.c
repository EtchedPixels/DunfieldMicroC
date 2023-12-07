/*
 * DDS MICRO-C Preprocessor
 *
 * This filter provides nearly full pre-processor capability
 * to the MICRO-C compiler.
 *
 * The pre-processor is implemented in such a way that processed
 * input lines may be obtained simply by repeated calls to the
 * function "read_line", in much the same way as you use "fgets".
 *
 * ?COPY.TXT 1989-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

#include <stdio.h>
#ifndef _MICROC_
	#include <dos.h>		/* Non MICRO-C only */
	#include <ctype.h>		/* Non MICRO-C only */
	#include "microc.h"		/* Non MICRO-C only */
#endif

/*
 * The following two macros define strings for handling
 * directories, and are operating system dependant.
 */
#define	INCLUDE_DIR	"\\mc"	/* default 'include' directory */
#define	DIR_END		"\\"	/* string to join filename to directory */

/* Misc. fixed parameters */
#define	LINE_SIZE	200		/* maximum size of input line */
#ifndef DEMO
	#define MACROS	1000	/* number of macros allowed (500) */
	#define	MPOOL	50000	/* string space alloted to macro pool (25000) */
#else
	#define MACROS	100		/* Number of macros allowed */
	#define	MPOOL	1000	/* String space alloted to macro pool */
	#include "c:\project\demonote.txt"
#endif
#define	PARAMETERS	10		/* maximum # parameters to a macro */
#define	PARM_POOL	300		/* parameter names & definitions */
#define	INCL_DEPTH	5		/* maximum depth of include files */

/* Condition tracking 'if_flag' bits */
#define	IF_TRUE		0x80	/* If condition is currently true */
#define	IF_WAS_TRUE	0x40	/* If condition at this level has been true */
#define	IF_ELSE		0x20	/* An ELSE condition has occured */

/* String handling flags (used by copy_string()) */
#define	ST_NO_CONT	0x01	/* No continuation on next line */
#define	ST_NO_CONC	0x02	/* No concatination */
#define	ST_IN_CONC	0x04	/* Allow concat from input stream */

/* input & output buffers & pointers */
static char out_buffer[2000], in_buffer[LINE_SIZE], *input_ptr, *output_ptr;

/* macro definition: index, pool and top pointers */
static unsigned macro;
static char *define_index[MACROS], define_pool[MPOOL], *define_ptr;

/* macro parameter: index, pool and top pointers */
static unsigned parm;
static char *parm_index[PARAMETERS], parm_pool[PARM_POOL], *parm_ptr;

/* include: line stack & count, file pointers */
static unsigned include, incl_line[INCL_DEPTH];
static FILE *input_fp, *incl_fp[INCL_DEPTH], *output_fp;

/* #if condition tracking variables */
static unsigned if_top = MPOOL;
static unsigned char if_flag = IF_TRUE | IF_WAS_TRUE | IF_ELSE;

/* misc. variables and flags */
static unsigned line_number, error_count, Index;
static unsigned char comment, dupwarn, quiet, linum;
static unsigned hour, minite, second, day, month, year;

/* include library directory path */
static char *library = INCLUDE_DIR;

static char file_name[INCL_DEPTH][65], *fnptr;


/*
 * Minimal "fprintf" function
 */
#ifdef _MICROC_
register fprint(args) char *args;
{
	unsigned v, *a, sp, w;
	char stack[6], s, *fmt;
	register char c;

	a = (nargs() * 2) + &args;
	fmt = *--a;
	while(c = *fmt++) {
		if(c == '%')  {
			v = *--a;
			switch(*fmt++) {
			case 's' :		/* String output */
				while(*(char*)v)
					*output_ptr++ = *(char*)v++;
				continue;
			case '2' : w = 2; s = 0; goto do_num;
			case 'u' :		/* Numeric output */
				w = 0;
			do_num:
				sp = 0;
				do
					stack[sp++] = v % 10;
				while(v /= 10);
				while(sp < w)
					stack[sp++] = s;
				while(sp) {
					c = stack[--sp];
					c += (c > 9) ? '7' : '0';
					*output_ptr++ = c; }
				continue; } }
		*output_ptr++ = c; }
	*output_ptr = 0;
}
#else
fprint(fmt) char *fmt;
{
	unsigned v, *a, sp, w;
	char stack[6], s;
	register char c;

	a = &fmt;
	while(c = *fmt++) {
		if(c == '%')  {
			v = *++a;
			switch(*fmt++) {
			case 's' :		/* String output */
				while(*(char*)v)
					*output_ptr++ = *(char*)v++;
				continue;
			case '2' : w = 2; s = 0; goto do_num;
			case 'u' :		/* Numeric output */
				w = 0;
			do_num:
				sp = 0;
				do
					stack[sp++] = v % 10;
				while(v /= 10);
				while(sp < w)
					stack[sp++] = s;
				while(sp) {
					c = stack[--sp];
					c += (c > 9) ? '7' : '0';
					*output_ptr++ = c; }
				continue; } }
		*output_ptr++ = c; }
	*output_ptr = 0;
}

/*
 * Get current time from DOS
 */
get_time_date()
{
	union REGS r;

	r.h.ah = 0x2C;
	int86(0x21, &r, &r);
	hour = r.h.ch;
	minite = r.h.cl;
	second = r.h.dh;

	r.h.ah = 0x2A;
	int86(0x21, &r, &r);
	day = r.h.dl;
	month = r.h.dh;
	year = r.x.cx;
}
#endif

/*
 * Main program, read lines and write them to the output file
 */
main(argc, argv)
	int argc;
	char *argv[];
{
	int i;
	char *ptr;

	input_fp = output_fp = 0;	/* Default to stdio */
	define_ptr = define_pool;	/* Set up macro pool base */

#ifdef _MICROC_
	get_time(&hour, &minite, &second);
	get_date(&day, &month, &year);
#else
	get_time_date();
#endif

/* first process any filenames and command line options */
	for(i=1; i < argc; ++i) {
		input_ptr = ptr = argv[i];
		switch((*ptr++ << 8) | *ptr++) {
			case ('-'<<8)+'c' :				/* Keep comments */
				comment = -1;
				break;
			case ('-'<<8)+'d' :				/* Warn duplicates */
				dupwarn = -1;
				break;
			case ('-'<<8)+'q' :				/* Quiet mode */
				quiet = -1;
				break;
			case ('-'<<8)+'l' :				/* Output line numbers */
				linum = -1;
				break;
			case ('l'<<8)+'=' :				/* Specify library */
				library = ptr;
				break;
			default:
				define_index[macro] = define_ptr;
				copy_name(&define_ptr);
				if(*input_ptr == '=') {		/* Command line definition */
					++macro;
					*define_ptr++ = 0;
					*define_ptr++ = 0;
					do
						*define_ptr++ = *++input_ptr;
					while(*input_ptr); }
				else if(!input_fp) {		/* Input file */
					strcpy(fnptr = file_name, argv[i]);
					if(!(input_fp = fopen(argv[i], "r")))
						severe_error("Cannot open input file"); }
				else if(!output_fp) {		/* Output file */
					if(!(output_fp = fopen(argv[i], "w")))
						severe_error("Cannot open output file"); }
				else
					severe_error("Too many parameters"); } }

/* Any files not explicitly named default to standard I/O */
	if(!input_fp)				/* default to standard input */
		input_fp = stdin;
	if(!output_fp)				/* default to standard output */
		output_fp = stdout;

	if(!quiet)
#ifdef DEMO
		fputs("DDS MICRO-C Preprocessor (Demo)\n?COPY.TXT 1989-2005 Dave Dunfield\n**See COPY.TXT**.\n", stderr);
#else
		fputs("DDS MICRO-C Preprocessor\n?COPY.TXT 1989-2005 Dave Dunfield\n**See COPY.TXT**.\n", stderr);
#endif

/*
 * Read preprocessed lines from the input file and
 * write them output file. continue till EOF.
 */
	while(read_line()) {		/* copy processed input to output */
		ptr = out_buffer;
		if(linum) {
			output_ptr = in_buffer;
			if(fnptr) {
				fprint("#file %s\n", fnptr);
				fnptr = 0; }
			fprint("%u:", line_number);
			fputs(in_buffer, output_fp); }
		while(*ptr) {
			putc(*ptr, output_fp);
			if((*ptr++ == '\n') && linum) {
				output_ptr = in_buffer;
				fprint("%u:", line_number);
				fputs(in_buffer, output_fp); } }

		putc('\n', output_fp); }

	fclose(input_fp);
	fclose(output_fp);

	return error_count ? -2 : 0;
}

/*
 * Test for a valid character to start a name
 */
isname(c)
	char c;
{
	return ((c >= 'a') && (c <= 'z'))
		|| ((c >= 'A') && (c <= 'Z'))
		|| (c == '_');
}

/*
 * Test for valid character in a name.
 */
is_name(c)
	char c;
{
	return isname(c) || isdigit(c);
}

/*
 * Skip to next non-blank (space or tab) in buffer
 */
skip_blanks()
{
	while((*input_ptr == ' ') || (*input_ptr == '\t'))
		++input_ptr;
	return *input_ptr;
}

/*
 * Test for a string in input stream
 */
match(ptr)
	char *ptr;
{
	register char *ptr1;

	ptr1 = input_ptr+1;
	while(*ptr)
		if(*ptr++ != *ptr1++)	/* symbols do not match */
			return 0;
	if(is_name(*ptr1))			/* symbol continues */
		return 0;
	input_ptr = ptr1;
	skip_blanks();
	return 1;
}

/*
 * Display an error message
 */
line_error(msg)
	char *msg;
{
	char buffer[80], *ptr;
	ptr = output_ptr;
	output_ptr = buffer;
	fprint("%s(%u): %s\n", file_name[include], line_number, msg);
	output_ptr = ptr;
	fputs(buffer, stderr);
	if(++error_count == 10)
		severe_error("Too many errors");
}

/*
 * Display an error message & terminate
 */
severe_error(msg)
	char *msg;
{
	line_error(msg);
	exit(-1);
}

/*
 * Test for more macro parameters
 */
more_parms()
{
	register char c;

	if(((c = skip_blanks()) == ',') || (c == ')'))
		++input_ptr;
	else
		line_error("Invalid macro parameter");
	return c == ',';
}

/*
 * Skip over a comment + copy to the output file if necessary
 */
skip_comment(flag)
	char flag;
{
	int comment_depth;
	register char c;

	if((c = *input_ptr) == '*') {		/* C style comment */
		++input_ptr;

		if(flag) {
			*output_ptr++ = '/';
			*output_ptr++ = '*'; }

		comment_depth = 1;

		while(comment_depth) {
			if(!(c = *input_ptr++)) {					/* end of input line */
				if(flag) {
					*output_ptr = 0;
					fputs(output_ptr = out_buffer, output_fp);
					putc('\n', output_fp); }
				if(!fgets(input_ptr = in_buffer, LINE_SIZE, input_fp))
					severe_error("Unterminated comment");
				++line_number;
				continue; }
			if(flag)
				*output_ptr++ = c;
			if((c == '/') && (*input_ptr == '*')) {		/* Nested comment */
				++input_ptr;
				++comment_depth;
				if(flag)
					*output_ptr++ = '*'; }
			if((c == '*')&& (*input_ptr == '/')) {		/* End of comment */
				++input_ptr;
				--comment_depth;
				if(flag)
					*output_ptr++ = '/'; } }
		return 0; }

	if(c == '/') {						/* C++ style comment */
		if(flag) {
			do
				*output_ptr++ = c;
			while(c = *input_ptr++); }
		input_ptr = "";
		return 0; }

	return -1;							/* No comment */
}

/*
 * Copy a named symbol from the input buffer
 */
copy_name(dest_ptr)
	char **dest_ptr;
{
	register char *dest;

	dest = *dest_ptr;
	do
		*dest++ = *input_ptr++;
	while(is_name(*input_ptr));
	*dest = 0;
	*dest_ptr = dest;
}

/*
 * Copy a quoted string from the input buffer
 * with regard for "protected" characters.
 */
copy_string(dest_ptr, src_ptr, flag)
	char **dest_ptr, **src_ptr;
	char flag;
{
	char *dest, *src, delim;
	register char c;
	static char concat_flag = -1;

	src = *src_ptr;
	*(dest = *dest_ptr) = delim = *src++;

	if(concat_flag)
		++dest;

	/* Copy content of string */
	while((c = *src++) != delim) {
		if(!(*dest++ = c)) {
			unstr: severe_error("Unterminated string"); }
		if(c == '\\') {
			if(!(*dest++ = *src++)) {
				if(flag & ST_NO_CONT)
					goto unstr;
				dest -= 2;
				if(!fgets(src = in_buffer, LINE_SIZE, input_fp))
					goto unstr;
				++line_number; } } }

	*src_ptr = src;

	/* Check for string concatination */
	if(!(flag & ST_NO_CONC)) {
		while((*src == ' ') || (*src == '\t'))
			++src;
		if(*src == '#') {
			*src_ptr = src+1;
		do_conc:
			concat_flag = 0;
			goto nofix; }
		if((!*src) && (flag & ST_IN_CONC) && (skip_blanks() == '#')) {
			++input_ptr;
			goto do_conc; } }

	concat_flag = *dest++ = delim;
nofix:
	*dest_ptr = dest;
}

/*
 * Resolve special symbol names
 */
special_symbol()
{
	unsigned x;
	static char *months[] = { "???", "Jan", "Feb", "Mar", "Apr",
		"May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

	if(*(input_ptr+1) == '_') {
		if(match("_LINE__")) {	/* Current line number */
			fprint("%u", line_number);
			goto tstend; }
		if(match("_FILE__")) {	/* Current filename */
			fprint("%s", file_name[include]);
			goto tstend; }
		if(match("_TIME__")) {	/* Current time */
			if(*input_ptr != '{') {
				fprint("%2:%2:%2", hour, minite, second);
				goto tstend; }
			for(;;) {
				switch(x = *++input_ptr) {
				case 's' : x = second;		goto pr2;
				case 'm' : x = minite;		goto pr2;
				case 'H' : x = hour;		goto pr2;
				case 'D' : x = day;			goto pr2;
				case 'M' : x = month;		goto pr2;
				case 'y' : x = year%100;	goto pr2;
				case 'h' : if(x = hour%12)	goto pr2;
					x=12;
				pr2: fprint("%2", x);		continue;
				case 'Y' : fprint("%u", year); continue;
				case 'S' : fprint("%s", months[month]); continue;
				case '}' : ++input_ptr; goto tstend;
				case '\\' : if(x = *++input_ptr) break;
				case 0   : line_error("Invalid TIME code"); goto tstend;
				case 'p' :
				case 'P' : if(hour < 12) x -= ('P'-'A'); }
				*output_ptr++ = x; } }
		if(match("_DATE__")) {	/* Current date */
			fprint("%s %2 %u", months[month], day, year);
			goto tstend; }
		if(match("_INDEX__")) {	/* Index count */
			fprint("%u", Index++);
		tstend:
			if(is_name(*input_ptr))
				*output_ptr++ = ' ';
			return 0; } }
	return -1;
}

/*
 * Lookup a word from the input stream to see if it is a macro
 */
lookup_macro(eflag)
	char eflag;
{
	register int i;
	char *name;

	name = output_ptr;
	copy_name(&output_ptr);
	for(i = macro - 1; i >= 0; --i)			/* look it up */
		if(!strcmp(name, define_index[i]))
			return i;
	if(eflag)								/* not found */
		line_error("Undefined macro");
	return -1;
}

/*
 * Resolve a word into a macro definition (if it is defined)
 */
resolve_macro()
{
	char *mptr, *save_ptr, *xptr;
	int i;
	register char c;
	unsigned parm;	/* macro parameter: index, pool and top pointers */
	char *parm_index[PARAMETERS], parm_pool[PARM_POOL], *parm_ptr;

	save_ptr = output_ptr;
	if((i = lookup_macro(0)) != -1) {	/* Substitution required */
		mptr = define_index[i];
		while(*mptr++);
		parm = 0;
		parm_ptr = parm_pool;
		if(*mptr++) {					/* parameterized macro */
			if(skip_blanks() == '(') {
				++input_ptr;
				do {
					skip_blanks();
					parm_index[parm++] = parm_ptr;
					i = 0;
					while((c = *input_ptr) && (i || (c != ',') && (c != ')'))) {
						if(isname(c)) {		/* possible nested def */
							xptr = output_ptr;
							resolve_macro();
							output_ptr = xptr;
							while(*xptr)
								*parm_ptr++ = *xptr++; }
						else {						/* Normal character */
							if((c == '"') || (c == '\''))
								copy_string(&parm_ptr, &input_ptr, 0);
							else {
								*parm_ptr++ = c;
								++input_ptr;
								if(c == '(')			/* nest brackets */
									++i;
								else if(c == ')')		/* unnest brackets */
									--i; } } }
					*parm_ptr++ = 0; }
				while(more_parms()); } }
		output_ptr = save_ptr;
		save_ptr = input_ptr;
		input_ptr = mptr;
		while(c = *input_ptr) {			/* copy over definition */
			if(c & 0x80) {				/* parameter substitution */
				++input_ptr;
				if((i = c & 0x7f) < parm) {
					for(xptr = parm_index[i]; *xptr; ++xptr)
						*output_ptr++ = *xptr; }
				continue; }
			if(c == '_') {
				if(!special_symbol())
					continue; }
			if(c == '"') {
				xptr = input_ptr; input_ptr = save_ptr;
				copy_string(&output_ptr, &xptr, ST_IN_CONC);
				save_ptr = input_ptr; input_ptr = xptr;
				continue; }
			*output_ptr++ = *input_ptr++; }
		*output_ptr = 0;
		input_ptr = save_ptr; }
}

/*
 * Copy input line to output buffer while resolving macros
 */
resolve_line()
{
	register char c;
	while(c = *input_ptr) {
		if(isname(c)) {				/* symbol, could be macro */
			if((c != '_') || special_symbol())
				resolve_macro(); }
		else if((c == '"') || (c == 0x27))		/* quoted string */
			copy_string(&output_ptr, &input_ptr, 0);
		else {
			++input_ptr;
			if((c != '/') || skip_comment(comment))
				*output_ptr++ = c; } }
	*output_ptr = 0;
}

/*
 * Test for "if" processing enabled, and prepare for "if"
 */
test_if()
{
	define_pool[--if_top] = if_flag;	/* Stack current if status */
	if(!(if_flag & IF_TRUE)) {			/* If disabled...*/
		if_flag = IF_WAS_TRUE;			/* No processing at this level */
		return 0; }						/* Do not perform test */
	if_flag = 0;						/* Assume false */
	return -1;							/* And perform test */
}

/*
 * Read a line & perform pre-processing
 */
read_line()
{
	int i;
	register char c;

	for(;;) {
		if(!fgets(input_ptr = in_buffer, LINE_SIZE, input_fp)) {
			if(include) {
				fclose(input_fp);
				line_number = incl_line[--include];
				input_fp = incl_fp[include];
				fnptr = file_name[include];
				continue; }
			return 0; }
		++line_number;
		output_ptr = out_buffer;
		if(skip_blanks() != '#') {		/* No directive */
			if(if_flag & IF_TRUE) {
				input_ptr = in_buffer;
				resolve_line();
				return -1; }
			continue; }
		if(match("ifdef")) {			/* if macro defined */
			if(test_if() && (lookup_macro(0) != -1))
				if_flag = IF_TRUE | IF_WAS_TRUE; }
		else if(match("ifndef")) {		/* if macro not defined */
			if(test_if() && (lookup_macro(0) == -1))
				if_flag = IF_TRUE | IF_WAS_TRUE; }
		else if(match("if")) {			/* If expression is true */
			if(test_if()) {
				resolve_line();
				input_ptr = out_buffer;
				if(expression())
					if_flag = IF_TRUE | IF_WAS_TRUE; } }
		else if(match("else")) {		/* reverse condition */
			if(if_flag & IF_ELSE)
				severe_error("Misplaced #else");
			if_flag = (if_flag | IF_ELSE) & ~IF_TRUE;
			if(!(if_flag & IF_WAS_TRUE))
				if_flag = IF_TRUE | IF_WAS_TRUE | IF_ELSE; }
		else if(match("endif")) {		/* end conditional */
			if(if_top >= MPOOL)
				severe_error("Misplaced #endif");
			if_flag = define_pool[if_top++]; }
		else if(match("elif")) {		/* else - if */
			if(if_flag & IF_ELSE)
				severe_error("Misplaced #elif");
			if_flag &= ~IF_TRUE;
			if(!(if_flag & IF_WAS_TRUE)) {
				resolve_line();
				input_ptr = out_buffer;
				if(expression())
					if_flag |= IF_TRUE | IF_WAS_TRUE; } }
		else if(if_flag & IF_TRUE) {
			if(match("define")) {		/* define a new macro */
				if(macro >= MACROS) {
#ifdef DEMO
					fputs(demo_text, stderr);
#endif
					severe_error("Too many macro definitions"); }
				define_index[macro] = define_ptr;
				if(!isname(*input_ptr)) {
					line_error("Invalid macro name");
					continue; }
				copy_name(&define_ptr);		/* get macro name */
				++define_ptr;
				if(dupwarn) {
					for(i = macro - 1; i >= 0; --i)			/* look it up */
						if(!strcmp(define_index[macro], define_index[i])) {
							line_error("Duplicate macro");
							break; } }
				parm = 0;
				parm_ptr = parm_pool;
				if(*input_ptr == '(') {	/* parameterized macro */
					*define_ptr++ = 1;
					++input_ptr;
					if(skip_blanks() != ')')
						do {
							skip_blanks();
							if(parm >= PARAMETERS) {
								line_error("Too many macro parameters");
								break; }
							parm_index[parm++] = parm_ptr;
							copy_name(&parm_ptr);
							++parm_ptr; }
						while(more_parms());
					else
						++input_ptr; }
				else
					*define_ptr++ = 0;
				skip_blanks();
				while(c = *input_ptr) {
					if((c == '\\') && !*(input_ptr+1)) {	/* Multi-line */
						fgets(input_ptr = in_buffer, LINE_SIZE, input_fp);
						++line_number;
						*define_ptr++ = '\n';
						continue; }
					output_ptr = define_ptr;
					if(isname(c)) {
						resolve_macro();
						for(i=0; i < parm; ++i) {
							if(!strcmp(define_ptr, parm_index[i])) {
								*define_ptr = i + 0x80;
								output_ptr = ++define_ptr;
								break; } }
						if(is_name(*((define_ptr = output_ptr)-1)) && is_name(skip_blanks()))
							*define_ptr++ = ' '; }
					else if((c == '"') || (c == 0x27)) {
						copy_string(&output_ptr, &input_ptr, ST_NO_CONC);
						define_ptr = output_ptr; }
					else {
						++input_ptr;
						if((c != '/') || skip_comment(0))
							*define_ptr++ = c; }
					skip_blanks(); }
				if(define_ptr >= (define_pool+MPOOL)) {
#ifdef DEMO
					fputs(demo_text, stderr);
#endif
					severe_error("Out of memory"); }
				*define_ptr++ = 0;
				++macro; }
			else if(match("undef")) {			/* undefine a macro */
				if((i = lookup_macro(-1)) != -1) {
					if(i == (macro - 1))		/* last one, simple delete */
						define_ptr = define_index[i];
					else {						/* not last, reclaim space */
						define_ptr -= (parm = (input_ptr = define_index[i+1]) -
							(parm_ptr = define_index[i]));
						while(parm_ptr < define_ptr)
							*parm_ptr++ = *input_ptr++;
						while(i < macro) {		/* adjust index list */
							define_index[i] = define_index[i+1] - parm;
							++i; } }
					--macro; } }
			else if(match("forget")) {			/* undefine a block of macros */
				if((i = lookup_macro(-1)) != -1)
					define_ptr = define_index[macro = i]; }
			else if(match("include")) {		/* include a file */
				if(include >= (INCL_DEPTH-1))
					severe_error("Too many include files");
				if((c = skip_blanks()) == '<') {	/* library definition */
					for(parm_ptr = library; *parm_ptr; ++parm_ptr)
						*output_ptr++ = *parm_ptr;
					for(parm_ptr = DIR_END; *parm_ptr; ++parm_ptr)
						*output_ptr++ = *parm_ptr;
					c = '>'; }
				else if(c != '"') {					/* current directory */
					line_error("Invalid include file name");
					continue; }
				while(*++input_ptr && (*input_ptr != c))
					*output_ptr++ = *input_ptr;
				*output_ptr = 0;
				incl_fp[include] = input_fp;
				incl_line[include] = line_number;
				if(input_fp = fopen(out_buffer, "r")) {
					line_number = 0;
					++include;
					strcpy(fnptr = file_name[include], out_buffer); }
				else {
					line_error("Cannot open include file");
					input_fp = incl_fp[include]; } }
			else if(match("index")) {
				resolve_line();
				input_ptr = out_buffer;
				Index = expression(); }
			else if(match("error"))
				severe_error(input_ptr);
			else if(match("message")) {
				resolve_line();
				--error_count;
				line_error(out_buffer); }
			else {				/* default, pre-processing & hand back */
				line_error("Unknown directive"); } } }
}

/*
 * Get a numerical data value from the input stream
 */
get_value()
{
	unsigned num;
	num = 0;
	if(isdigit(skip_blanks())) {
		while(isdigit(*input_ptr))
			num = (num * 10) + (*input_ptr++ - '0');
		return num; }
	switch(*input_ptr++) {
		case '(' :	return expression();
		case '-' :	return -get_value();
		case '~' :	return ~get_value();
		case '!' :	return !get_value();
		case '\'' :		/* character value */
			while(*input_ptr != '\'') {
				if(!*input_ptr)
					severe_error("Unterminated string");
				num = (num << 8) + *input_ptr++; }
			++input_ptr;
			return num; }
	if(!isname(*--input_ptr))
		line_error("Invalid constant in expression");
	while(is_name(*input_ptr))	/* Skip unknown name */
		++input_ptr;			/* And return 0 */
	return 0;
}

/*
 * Process a numerical expression in the input stream.
 */
expression()
{
	unsigned value;

	value = get_value();
	for(;;) {
		skip_blanks();
		switch(*input_ptr++) {
			case ')' :
			case '\0' :
				return value;
			case '+' : value += get_value();	break;
			case '-' : value -= get_value();	break;
			case '*' : value *= get_value();	break;
			case '/' : value /= get_value();	break;
			case '%' : value %= get_value();	break;
			case '&' :
				if(*input_ptr == '&') {
					++input_ptr;
					value = get_value() && value;
					break; }
				value &= get_value();			break;
			case '|' :
				if(*input_ptr == '|') {
					++input_ptr;
					value = get_value() || value;
					break; }
				value |= get_value();			break;
			case '^' : value ^= get_value();	break;
			case '>' :
				switch(*input_ptr++) {
					case '>' : value >>= get_value();			break;
					case '=' : value = value >= get_value();	break;
					default: --input_ptr; value = value > get_value(); }
				break;
			case '<' :
				switch(*input_ptr++) {
					case '<' : value <<= get_value();			break;
					case '=' : value = value <= get_value();	break;
					default: --input_ptr; value = value < get_value(); }
				break;
			case '=' :
				if(*input_ptr++ != '=')
					goto error;
				value = value == get_value();
				break;
			case '!' :
				if(*input_ptr++ == '=') {
					value = value != get_value();
					break; }
			default:
			error:
				line_error("Invalid operator in expression");
				return 0; } }
}
