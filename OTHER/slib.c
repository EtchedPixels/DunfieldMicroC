/*
 * DDS MICRO-C Source Librarian
 *
 * This program helps maintain the external index files used with the
 * source linker. It allows you to easily add and remove individual
 * file definitions from the index file, and most importantly, scans
 * for and reports source library problems such as duplicated symbols
 * and unresolved external references.
 *
 * ?COPY.TXT 1991-2005 Dave Dunfield
 * **See COPY.TXT**.
 *
 * Compile command: cc slib -fop
 */

#include <stdio.h>

#define	SYMSIZE	15		/* Maximum length of a symbol name */
#define	MAXSYM	2500	/* Maximum number of symbols */

/* Types of symbol entries in internal table */
#define	PREFIX	1		/* Prefix file */
#define	MIDDLE	2		/* Middle file */
#define	SUFFIX	3		/* Suffix file */
#define	LIBFUN	4		/* Standard library file */
#define	EXTRA	5		/* Extra filenames */
#define	DEFINED	6		/* Defined symbol */
#define	EXTERN	7		/* External reference */

static int stop = 0;
static unsigned char stype[MAXSYM], snames[MAXSYM][SYMSIZE+1], udata[25] = 0,
	*optr;

static unsigned fcount=0, scount=0, ecount=0;

static char verbose = -1, write = -1, changed = 0;

/*
 * Variables used for symbol display output
 */
static char *dtitle;
static int dfile, ofile, dflag;

static char htext[] = { "\n\
Use: slib [?= a= i= m= p= r= s= -q -w]\n\n\
Opts:	?=file(query)	A=Addfile	I=Index		M=addMiddle\n\
	P=addPrefix	R=Removefile	S=addSuffix\n\
	-Quiet		-Writeinhibit\n" };

/*
 * Main program
 */
main(argc, argv)
	int argc;
	int *argv[];
{
	int cmd, cmdc, i, j;
	char *cmdv[30], filename[65];

	cmdc = 0;
	optr = "EXTINDEX";
	for(i=1; i < argc; ++i) switch(*argv[i]){
		case 'q-' : verbose = 0;		break;
		case 'w-' : write = 0;			break;
		case '=i' : optr = &argv[i][1];	break;
		case '?' : argc = 0; break;
		default: cmdv[cmdc++] = argv[i]; }

	if(verbose)
		fputs("DDS MICRO-C Source Librarian\n?COPY.TXT 1991-2005 Dave Dunfield\n**See COPY.TXT**.\n", stderr);

	if(!argc)
		abort(htext);

	copy_file(filename, ".LIB");	/* Insure its uppercase */
	read_index(filename);			/* Read in the current description */

	/* Report on the library status */
	if(verbose) {
		printf("\n%s contains %u symbols in %u files with %u external references.\n",
			filename, scount, fcount, ecount);
		if(i = test_duplicate(0))
			printf("%u symbol name conflict(s)\n", i);
		if(i = test_externals(0))
			printf("%u unresolved external reference(s)\n", i); }

	/* Process any local commands */
	for(cmd=0; cmd < cmdc; ++cmd) {
		optr = cmdv[cmd];
		switch((toupper(*optr++)<<8)|toupper(*optr++)) {
			case 'A=' :		/* Add library file */
				add_file(LIBFUN);
				break;
			case 'M=' :		/* New MIDDLE file */
				add_file(MIDDLE);
				break;
			case 'P=' :		/* New PREFIX file */
				add_file(PREFIX);
				break;
			case 'R=' :		/* Remove a file */
				if((i = j = find_name(PREFIX, LIBFUN, -1)) >= 0) {
					do
						stype[j] |= 0xF0;
					while(stype[++j] >= EXTRA);
					if(verbose) printf("Removing '%s'.\n", snames[i]);
					changed = -1; }
				break;
			case 'S=' :		/* New SUFFIX file */
				add_file(SUFFIX);
				break;
			case '?=' :		/* Information request */
				if((i = find_name(PREFIX, LIBFUN, -1)) >= 0)
					display_file(i);
				break;
			default :		/* Unknown command */
				fprintf(stderr,"Unknown SLIB command '%s'\n", cmdv[cmd]);
				exit(-1); } }

	i = 0;
	if(verbose) {
		i = test_duplicate("The following symbol names are duplicated")
		  + test_externals("The following external references are unresolved"); }

	if(write && changed && ((!i) || proceed("\nWrite new library"))) {
		if(verbose) printf("\nWriting %s.\n", filename);
		write_index(filename); }
}

/*
 * Display info about file
 */
display_file(i)
	int i;
{
	int j;

	dbegin("Extra files"); dfile = i;
	for(j=i+1; stype[j] >= EXTRA; ++j)
		if(stype[j] == EXTRA)
			dshow(j);
	dend(); dbegin("Symbols Defined"); dfile = i;
	for(j=i+1; stype[j] >= EXTRA; ++j)
		if(stype[j] == DEFINED)
			dshow(j);
	dend(); dbegin("External References"); dfile = i;
	for(j=i+1; stype[j] >= EXTRA; ++j)
		if(stype[j] == EXTERN)
			dshow(j);
	dend();
}

/*
 * Lookup a name in the symbol list
 */
find_name(low, high, err)
	int low, high;
	char err;
{
	char buffer[65];
	int i;

	copy_file(buffer, ".ASM");

	for(i=0; i < stop; ++i) {
		if((stype[i] >= low) && (stype[i] <= high) && !strcmp(buffer, snames[i]))
			return i; }

	if(err)
		printf("Not found: '%s'\n", buffer);
	return -1;
}

/*
 * Prompt for proceed
 */
proceed(prompt)
	char *prompt;
{
	char buffer[25];

	for(;;) {
		printf("%s (Y/N)? ", prompt);
		fgets(optr = buffer, sizeof(buffer), stdin);
		switch(toupper(skip_blanks())) {
			case 'Y' : return -1;
			case 'N' : return 0; } }
}

/*
 * Add a source file to the archive
 */
add_file(type)
	int type;
{
	int otop;
	char buffer[200], *ptr;
	FILE *fp;

	copy_file(snames[otop = stop], ".ASM");
	optr = snames[otop];
	if(find_name(PREFIX, LIBFUN, 0) >= 0) {
		printf("File '%s' is already in the library index.\n", snames[otop]);
		return; }
	if(!(fp = fopen(snames[otop], "rv")))
		return;
	stype[stop++] = type;
	if(verbose) printf("Adding '%s'.\n", snames[otop]);

	while(fgets(optr = buffer, sizeof(buffer), fp)) {
		if(strbeg(buffer, "$EX:")) {
			stype[stop] = EXTERN;
			optr += 4;
			goto do_copy; }
		else {
			if(strbeg(buffer, "$DD:"))
				optr += 4;
			if(isalpha(*optr) || (*optr == '_')) {
			for(ptr = optr; isalnum(*ptr) || (*ptr == '_'); ++ptr);
			if(isspace(*ptr) || !*ptr) {
				stype[stop] = DEFINED;
		do_copy:
				ptr = snames[stop++];
				while(*optr && !isspace(*optr))
					*ptr++ = *optr++;
				*ptr = 0; } } } }
	stype[stop] = 0;
	fclose(fp);

	if(verbose)
		display_file(otop);

	changed = -1;
}

/*
 * Write out the index file
 */
write_index(file)
	char *file;
{
	int i, j;
	char *ptr, flag;
	FILE *fp;

	fp = fopen(file, "wvq");
	for(flag=i=0; i < stop; ++i) {
		ptr = snames[i];
		switch(stype[i]) {
			case PREFIX :
				j = '<';
				goto pfile;
			case MIDDLE :
				j = '^';
				goto pfile;
			case SUFFIX :
				j = '>';
				goto pfile;
			case LIBFUN :
				j = '-';
			pfile:
				if(flag)
					putc('\n', fp);
				putc(j, fp);
				fputs(ptr, fp);
				flag = -1;
				break;
			case EXTRA :
				putc(' ', fp);
				fputs(ptr, fp);
				flag = -1;
				break;
			case DEFINED :
				if(flag) {
					putc('\n', fp);
					flag = 0; }
				fputs(ptr, fp);
				putc('\n', fp); } }
	if(flag)
		putc('\n', fp);

	if(*udata)
		fprintf(fp,"$%s\n", udata);

	fclose(fp);
}

/*
 * Read an index file into the tables
 */
read_index(file)
	char *file;
{
	int ftype, pptr;
	char buffer[100];
	FILE *fp;

	fp = fopen(file, "rvq");

	stop = pptr = 0;
	while(fgets(optr = buffer, sizeof(buffer), fp)) switch(*optr) {
		case '$' :		/* Uninitialized data */
			strcpy(udata, optr+1);
			break;
		case '<' :		/* Prefix file */
			ftype = PREFIX;
			goto pfile;
		case '^' :		/* Middle file */
			ftype = MIDDLE;
			goto pfile;
		case '>' :		/* Suffix file */
			ftype = SUFFIX;
			goto pfile;
		case '-' :		/* Standard library file */
			ftype = LIBFUN;
		pfile:
			pptr = stop;
			++optr;
			skip_blanks();
			do {
				++fcount;
				copy_file(snames[stop], "");
				stype[stop] = ftype;
				get_externs(snames[stop++]);
				ftype = EXTRA; }
			while(skip_blanks());
			break;
		default:
			++scount;
			strcpy(snames[stop], buffer);
			stype[stop++] = DEFINED; }

	stype[stop] = 0;
	fclose(fp);
}

/*
 * Locate the external references in a file
 */
get_externs(file)
	char *file;
{
	char buffer[200], *ptr1, *ptr2;
	FILE *fp;

	if(fp = fopen(file, "rv")) {
		while(fgets(buffer, sizeof(buffer), fp)) {
			if(strbeg(buffer, "$EX:")) {
				++ecount;
				ptr1 = buffer+4;
				ptr2 = snames[stop];
				while(*ptr1 && !isspace(*ptr1))
					*ptr2++ = *ptr1++;
				*ptr2 = 0;
				stype[stop++] = EXTERN; } }
		fclose(fp); }
}

/*
 * Skip ahead to non blank
 */
skip_blanks()
{
	while(isspace(*optr))
		++optr;
	return *optr;
}

/*
 * Copy a filename from the operand pointer & convert to upper case
 */
copy_file(dest, ext)
	char *dest, *ext;
{
	char flag;
	flag = -1;
	while(*optr && !isspace(*optr)) {
		if((*dest++ = toupper(*optr++)) == '.')
			flag = 0; }
	*dest = 0;

	if(flag)
		strcpy(dest, ext);
}

/*
 * Test that externals are all resolved
 */
test_externals(title)
	char *title;
{
	int i, j, count;
	char *ptr;

	dbegin(title);
	for(i=count=0; i < stop; ++i) {
		if(stype[i] <= EXTRA)
			dfile = i;
		else if(stype[i] == EXTERN) {
			ptr = snames[i];
			for(j=0; j < stop; ++j)
				if((stype[j] == DEFINED) && !strcmp(ptr, snames[j]))
					goto extok;
			dshow(i);
			++count;
			extok: } }
	dend();

	return count;
}

/*
 * Test for multiple definitions
 */
test_duplicate(title)
	char *title;
{
	int i, j, count;
	char *ptr;

	dbegin(title);
	for(i=count=0; i < stop; ++i) {
		if(stype[i] <= EXTRA)
			dfile = i;
		else if(stype[i] == DEFINED) {
			ptr = snames[i];
			for(j=0; j < stop; ++j)
				if((j != i) && (stype[j] == DEFINED) && !strcmp(ptr, snames[j])) {
					dshow(i);
					++count; } } }
	dend();

	return count;
}

/*
 * Set up for symbol display output
 */
dbegin(title)
	char *title;
{
	dfile = ofile = dflag = -1;;
	dtitle = title;
}

/*
 * Display the next name from the list
 */
dshow(name)
	int name;
{
	if(dtitle) {
		if(dflag < 0) {
			printf("\n%s:", dtitle);
			dflag = 0; }
		if(dfile != ofile) {
			printf("\n%-12s:", snames[ofile = dfile]);
			dflag = 0; }
		if(++dflag >= 5) {
			dflag = 1;
			fputs("\n             ", stdout); }
		printf(" %-15s", snames[name]); }
}

/*
 * End symbol display output
 */
dend()
{
	if(dtitle && (dflag >= 0))
		putc('\n', stdout);
}
