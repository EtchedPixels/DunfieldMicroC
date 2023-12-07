/*
 * DDS MICRO-C Command Coordinator
 *
 * This program integrates the MCP, MCCCF, MCOCF, SLINK and ASMCF
 * operations into a single command.
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#include <stdio.h>

#define	NOFILE	2		/* EXEC return code for file not found */
#define	NOPATH	3		/* EXEC return code for path not found */

char mcdir[65], temp[65], ofile[65], tail[150], mcparm[80];
char link = -1, opt = 0, pre = 0, xasm = -1, verb = -1, intel = 0, lst = 0,
	del = -1, com = 0, macro = 0, fold = 0, symb = 0, dup = 0,
	*fnptr, *mptr = &mcparm, *startup = 0;

char htext[] = { "\n\
Use: CCF <name> [-acdfiklmopqx h= s= t=] [symbol=value]\n\n\
opts:	-Asm		-Comment	-Dupwarn	-Foldliteral\n\
	-Intelhex	-Keeptemp	-Listing	-Macro\n\
	-Optimize	-Preprocess	-Quiet		-Symbolic\n\
	-eXtended-asm\n\n\
	H=homepath	S=startup	T=temprefix\n\
\n\?COPY.TXT 1990-2005 Dave Dunfield\n**See COPY.TXT**.\n" };

/*
 * Main program, process options & invoke appropriate commands
 */
main(argc, argv)
	int argc;
	int *argv[];
{
	int i;
	char ifile[65], *ptr, c;

	/* Get default directories from environment */
	if(!getenv("MCDIR", mcdir))	{	/* Get MICRO-C directory */
		message("Environment variable MCDIR is not set!\n\n");
		strcpy(mcdir,"\\MC"); }
	if(!getenv("MCTMP", temp)) {	/* Get temporary directory */
		if(getenv("TEMP", temp)) {
			if(temp[(i = strlen(temp))-1] != '\\') {
				temp[i] = '\\';
				temp[i+1] = 0; } } }

	/* parse for command line options. */
	for(i=2; i < argc; ++i) {
		if(*(ptr = argv[i]) == '-') {		/* Enable switch */
			while(*++ptr) {
				switch(toupper(*ptr)) {
					case 'A' : link = 0;		continue;
					case 'C' : com = -1;		continue;
					case 'F' : fold = -1;		continue;
					case 'I' : intel = -1;		continue;
					case 'K' : del = 0;			continue;
					case 'L' : lst = -1;		continue;
					case 'M' : macro = -1;		continue;
					case 'O' : opt = -1;		continue;
					case 'D' : dup = -1;
					case 'P' : pre = -1;		continue;
					case 'Q' : verb = 0;		continue;
					case 'S' : symb = -1;		continue;
					case 'X' : xasm = 0;		continue; }
				goto badopt; }
			continue; }

		if(*(ptr+1) == '=') switch(toupper(*ptr)) {
			case 'H' : strcpy(mcdir, ptr+2);	continue;
			case 'S' : startup = ptr+2;			continue;
			case 'T' : strcpy(temp, ptr+2);		continue; }

		*mptr++ = ' ';
		c = 0;
		while(*mptr++ = *ptr++) {
			if(*ptr == '=')
				c = pre; }
		if(c)
			continue;

	badopt:
		fprintf(stderr,"Invalid option: %s\n", argv[i]);
		exit(-1); }

#ifdef DEMO
	message("DDS MICRO-C CFLEA Cross Compiler v3.23 (Demo)\n");
#else
	message("DDS MICRO-C CFLEA Cross Compiler v3.23\n");
#endif

	if(argc < 2) {
		fputs(htext, stderr);
		exit(-1); }

	/* Parse filename & extension from passed path etc. */
	fnptr = ptr = argv[1];
	while(c = *ptr) {
		if(c == '.')
			goto noext;
		++ptr;
		if((c == ':') || (c == '\\'))
			fnptr = ptr; }
	strcpy(ptr, ".C");
noext:
	strcpy(ifile, argv[1]);
	message(fnptr);
	message(": ");
	*mptr = *ptr = 0;

	/* Pre-process to source file */
	if(pre) {
		next_step("Preprocess... ", -1);
		sprintf(tail,"%s %s l=%s -q -l%s%s",ifile, ofile, mcdir,
			dup ? " -d" : "", mcparm);
		docmd("MCP.EXE");
		strcpy(ifile, ofile); }

	/* Compile to assembly language */
	next_step("Compile... ", opt||macro||link);
	sprintf(tail, "%s %s -q%s%s%s%s", ifile, ofile, pre ? " -l" : "",
		com ? " -c" : "", fold ? " -f" : "", symb ? " -s" : "");
	docmd("MCCCF.EXE");
	if(pre)
		erase(ifile);
	strcpy(ifile, ofile);

	/* Optimize the assembly language */
	if(opt) {
		next_step("Optimize... ", macro||link);
		sprintf(tail, "%s %s -q", ifile, ofile);
		docmd("MCOCF.COM");
		erase(ifile);
		strcpy(ifile, ofile); }

	/* Run assembler MACRO processor */
	if(macro) {
#ifdef DEMO
		message("Macro... Not supported in DEMO!\n");
		return; }
#else
		next_step("Macro... ", link);
		sprintf(tail,"%s >%s", ifile, ofile);
		docmd("MACRO.EXE");
		erase(ifile);
		strcpy(ifile, ofile); }
#endif

	/* Execute the SOURCE LINKER */
	if(link) {
		next_step("Link... ", xasm);
		sprintf(mcparm, startup ? " S=%s" : "", startup);
		sprintf(tail, "%s %s t=%s l=%s\\LIBCF -q%s%s",
			ifile, ofile, temp, mcdir, symb ? " -s" : "", mcparm);
		docmd("SLINK.EXE");
		erase(ifile);
		strcpy(ifile, ofile);

	/* Assemble into object module */
		if(xasm) {
			message("Assemble...\n");
			sprintf(tail, "%s c=%s l=%s -c%s%s%s", ifile, fnptr, fnptr,
				verb ? "" : "q", intel ? "i" : "", lst ? "fs" : "t");
			docmd("ASMCF.EXE");
			erase(ifile); } }

	message("All done.\n");
}

/*
 * Execute a command, looking for it in the MICRO-C directory,
 * and also in any directories found in the PATH environment
 * variable. Operands to the command have been previously
 * defined in the global variable 'tail'.
 */
docmd(char *cmd)
{
	int rc;
	char command[65], *ptr, *ptr1, c;
	static char path[2000];

	ptr = mcdir;						/* First try MC home dir */
	if(!getenv("PATH", ptr1 = path))	/* And then search  PATH */
		ptr1 = "";

	do {	/* Search MCDIR & PATH for commands */
		sprintf(command,"%s%s%s", ptr, "\\"+(ptr[strlen(ptr)-1] == '\\'), cmd);
		rc = exec(command, tail);
		ptr = ptr1;						/* Point to next directory */
		while(c = *ptr1) {				/* Advance to end of entry */
			++ptr1;
			if(c == ';') {
				*(ptr1 - 1) = 0;		/* Zero terminate */
				break; } } }
	while(((rc == NOFILE) || (rc == NOPATH)) && *ptr);
	if(rc) {
		fprintf(stderr,"%s failed (%d)\n", cmd, rc);
		exit(-1); }
}

/*
 * Output an informational message (verbose mode only)
 */
message(ptr)
	char *ptr;
{
	if(verb)
		fputs(ptr, stderr);
}

/*
 * Erase temporary file (if enabled)
 */
erase(file)
	char *file;
{
	if(del)
		delete(file);
}

/*
 * Create a new output file name (permanent or temporary)
 */
next_step(msg, flag)
	char *msg;
	int flag;
{
	static int tnum = 0;

	message(msg);

	if(flag)
		sprintf(ofile,"%s%s.%u", temp, fnptr, ++tnum);
	else
		sprintf(ofile,"%s.ASM", fnptr);
}
