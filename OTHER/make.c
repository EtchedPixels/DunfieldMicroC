/*
 * DDS MICRO-C Make utility
 *
 * This is a VERY simple "make" utility, which provides
 * basic dependancy rules and macro capability.
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 *
 * Compile command: cc make -fop
 */

#include <stdio.h>

/* fixed parameters */
#define	LINE_SIZE	500		/* Max. size of an input line */
#define	NUM_TESTS	35		/* Number of dependancy test files */
#define	PATHSIZE	65		/* Maximum size of a file pathname */
#define	MACROS		50		/* Maximum number of defined macros */
#define	MACRO_POOL	5000	/* Storage reserved for macro definitions */
#define	INCL_DEPTH	5		/* Maximum depth of include files */

static char inline[LINE_SIZE+1], *inptr, *optr,
	mainfile[PATHSIZE+1], testfiles[NUM_TESTS][PATHSIZE+1],
	*macros[MACROS], macro_pool[MACRO_POOL],
	*makefile = "MAKEFILE";
static int testptr = 0, update = 0, macptr = 0, mpoolptr = 0, incptr = 0;
static FILE *infile[INCL_DEPTH];

static char debug = 0, quiet = 0, if_flag = 0;

/*
 * Main program, process the makefile
 */
main(argc, argv)
	int argc;
	char *argv[];
{
	int i, j;
	unsigned time, date;
	char *ptr, buffer[100], buffer1[100], doflag;

	/* Process the command line */
	for(i=1; i < argc; ++i) {
		ptr = argv[i];
		switch((*ptr++ << 8) | *ptr++) {
			case '-d' :					/* Debug mode */
				debug = -1;
				break;
			case '-q' :					/* Quiet mode */
				quiet = -1;
				break;
			case '?'<<8 :				/* Help request */
				argc = 0;
				break;
			default:
				inptr = argv[i];
				parse(buffer, "");
				if(*inptr == '=') {		/* Define a symbol */
					++inptr;
					skip();
					add_macro(buffer, inptr); }
				else					/* Specify alternate makefile */
					makefile = argv[i]; } }

	if(!quiet)
		printf("DDS MICRO-C Make utility\n?COPY.TXT 1990-2005 Dave Dunfield\n**See COPY.TXT**.\n");

	if(!argc)
		abort("\nUse: MAKE [makefile -d -q name=text]\n");

	infile[0] = fopen(makefile, "rvq");

	/* Process the makefile */
	while(readline()) {
		parse(buffer, "@-.\\:");
		if(!strcmp(buffer, "@ifdef")) {
			while((i = parse(buffer, "")) && !lookup(buffer));
			condition(i);
			continue; }
		if(!strcmp(buffer, "@ifndef")) {
			while((i = parse(buffer, "")) && lookup(buffer));
			condition(i);
			continue; }
		if(!strcmp(buffer, "@ifeq")) {
			parse(buffer,"");
			while((i = parse(buffer1, "")) && strcmp(buffer, buffer1));
			condition(i);
			continue; }
		if(!strcmp(buffer, "@ifne")) {
			parse(buffer,"");
			while((i = parse(buffer1, "")) && strcmp(buffer, buffer1));
			condition(!i);
			continue; }
		if(!strcmp(buffer, "@else")) {
			if(!(if_flag & 0x7f))
				if_flag ^= 0x80;
			continue; }
		if(!strcmp(buffer, "@endif")) {
			if_flag = (if_flag & 0x7f) ? if_flag - 1 : 0 ;
			continue; }
		if(!if_flag) {
			if(!strcmp(buffer, "@include")) {
				if(++incptr >= INCL_DEPTH)
					abort("MAKE: Too many include files.\n");
				infile[incptr] = fopen(inptr, "rvq");
				continue; }
			if(!strcmp(buffer, "@type")) {
				printf("%s\n", inptr);
				continue; }
			if(!strcmp(buffer, "@abort")) {
				fprintf(stderr,"MAKE: Aborted! %s\n", inptr);
				exit(-1); }
			switch(*inptr) {
				case ':' :					/* Dependancy line */
					++inptr;
					skip();
					doflag = testptr = 0;
					strcpy(mainfile, buffer);
					if(find_first(mainfile, 0, buffer, &j, &j, &j, &time, &date)) {
						time = date = 0;
						doflag = -1; }
					while(parse(ptr = testfiles[testptr], "-.\\:")) {
						if(*ptr != '-')
							++testptr;
						else
							++ptr;
						if(test_file(ptr, time, date))
							doflag |= 1; }
					if(doflag) {
						++update;
						if(!quiet)
							printf("%sing: '%s'\n", (doflag == -1) ? "Mak" : "Rebuild", mainfile); }
					break;
				case '=' :					/* Macro definition */
					++inptr;
					skip();
					add_macro(buffer, inptr);
					break;
				default:					/* Command to execute */
					if(doflag && *(inptr = inline)) {	/* Ignore null lines */
						if(debug) {
							fputs("Debug: ", stdout);
							fputs(inline, stdout);
							putc('\n', stdout); }
						else {
							if(skip() == '-')	/* Inhibited display */
								++inptr;
							else if(!quiet) {	/* Display this command */
								fputs(inptr, stdout);
								putc('\n', stdout); }
							if(strlen(inptr) > 123)
								abort("MAKE: Command line too long!");
							if(*inptr != '~')
								system(inptr);
							else if(i = docmd(inptr+1)) {
								printf("MAKE: Step failed with exit code %d\n", i);
								exit(i); }
							} } } } }

	if(!quiet) {
		if(update)
			printf("MAKE: %u file%s updated\n", update, &"s"[update == 1]);
		else
			printf("MAKE: All files are up to date\n"); }
}

/*
 * Test this file for later than specified time & date
 */
test_file(filename, ttime, tdate)
	char *filename;
	unsigned ttime, tdate;
{
	unsigned time, date;
	char junk[13];

	if(find_first(filename, 0, junk, junk, junk, junk, &time, &date)) {
		fprintf(stderr,"MAKE: Cannot locate: %s\n", filename);
		exit(-1); }
	if(date > tdate)
		return 1;
	if((date == tdate) && (time > ttime))
		return 1;
	return 0;
}

/*
 * Read a line, eliminating comments & performing macro substitutions.
 */
readline()
{
	int i;
	char c, s, buffer[LINE_SIZE+1], name[50], *ptr;

	/* Read line */
	while(!fgets(inptr = buffer, LINE_SIZE, infile[incptr])) {
		fclose(infile[incptr]);
		if(!incptr)
			return 0;
		--incptr; }

	optr = inline;
	while(c = *inptr++) switch(c) {
		case '$' :				/* Macro Substitution */
			s = ' ';
			switch(c = *inptr++) {
				case ',' :		/* All tested files (name.ext, comma) */
					s = ',';
				case '.' :		/* All tested files (name.ext, space) */
					for(i=0; i < testptr; ++i) {
						if(i)
							*optr++ = s;
						append(testfiles[i], 0); }
					break;
				case ';' :		/* All tested files (name, comma) */
					s = ',';
				case ':' :		/* All tested files (name, space) */
					for(i=0; i < testptr; ++i) {
						if(i)
							*optr++ = s;
						append(testfiles[i], '.'); }
					break;
				case '*' :		/* name.ext of dependant file */
					append(mainfile, 0);
					break;
				case '@' :		/* Name only of dependant file */
					append(mainfile, '.');
					break;
				default:
					--inptr;
					c = 0;
				case '$' :		/* Environment name */
					i = 0;
					while(issymbol(*inptr))
						name[i++] = *inptr++;
					name[i] = ptr = 0;
					if(c) {
						if(getenv(name, optr))
							ptr = optr; }
					else
						ptr = lookup(name);
					if(!ptr) {
						fprintf(stderr,"MAKE: Undefined: %s\n", name);
						exit(-1); }
					append(ptr, 0); }
			break;
		case '#' :				/* Comment character */
			goto exit;
		case '\\' :				/* Protect next charcter */
			c = *inptr++;
		default:				/* Ordinary character, copy over */
			*optr++ = c; }
exit:
	while(isspace(*--optr) && (optr >= inline));
	*++optr = 0;
	inptr = inline;
	return 1;
}

/*
 * Skip ahead & return next character
 */
skip()
{
	while(isspace(*inptr))
		++inptr;
	return *inptr;
}

/*
 * Parse off the next symbol from the input line,
 * Symbol consists of valid name character (alphanumerics + '_')
 * and any characters from the 'allow'ed character list.
 */
parse(dest, allow)
	char *dest, *allow
{
	char c;
	int count;

	count = 0;
	while(issymbol(c = *inptr) || isinstr(c, allow)) {
		*dest++ = c;
		++inptr;
		++count; }
	if(*(dest-1) == ':') {
		--dest;
		--inptr; }
	*dest = 0;
	skip();
	return count;
}

/*
 * Test for valid symbol character
 */
issymbol(c)
	char c;
{
	return	((c >= 'a') && (c <= 'z')) ||
			((c >= 'A') && (c <= 'Z')) ||
			((c >= '0') && (c <= '9')) ||
			(c == '_');
}

/*
 * Test for character occuring in a string
 */
isinstr(c, string)
	char c, *string;
{
	while(*string)
		if(c == *string++)
			return 1;
	return 0;
}

/*
 * Add a macro to the macro pool
 */
add_macro(name, definition)
	char *name, *definition;
{
	macros[macptr++] = &macro_pool[mpoolptr];
	do
		macro_pool[mpoolptr++] = *name;
	while(*name++);
	do
		macro_pool[mpoolptr++] = *definition;
	while(*definition++);
}

/*
 * Lookup a macro definition
 */
lookup(name)
	char *name;
{
	int i;
	char *ptr;

	for(i=0; i < macptr; ++i) {
		if(!strcmp(name, ptr = macros[i])) {
			while(*ptr++);
			return ptr; } }
	return 0;
}

/*
 * Append a string into the output line
 */
append(string, term)
	char *string, term;
{
	while(*string && (*string != term))
		*optr++ = *string++;
}

/*
 * Set up a conditional
 */
condition(flag)
	int flag;
{
	if(if_flag)
		++if_flag;
	else if(!flag)
		if_flag = 0x80;
}

docmd(char *p)
{
	char *c;
	c = p;
	while(*p) {
		if(isspace(*p)) {
			*p++ = 0;
			break; }
		++p; }
	return exec(c, p);
}

