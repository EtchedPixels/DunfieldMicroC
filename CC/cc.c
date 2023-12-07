/*
 * DDS MICRO-C Command Coordinator
 *
 * This program integrates the various commands required to compile a
 * program into a single command.
 *
 * The following assemblers and linkers have been reported to work with
 * MICRO-C, using the indicated command line options:
 *
 * Vendor		Name	Versions(s)			Options
 * ------------------------------------------------
 * Arrowsoft	ASM	     1.00d (PD)			/ml
 * Arrowsoft	ASM      2.00c (PD)			/s/ml
 * Microsoft	MASM     3.0				/ml
 * Microsoft	MASM     4.0, 5.1			/t/ml
 * Borland		TASM     1.0, 2.01, 3.0		/t/ml					*4
 * Watcom		WASM     1.0				/q
 * Japleth      JWASM    2.07a				-q -Cp -Zm
 * Isaacson     A86      4.04				+c +O +S +E +G24 [+L]	*1
 * Isaacson     A386     4.04				+c +O +S +E +G24 [+L]	*1
 * Troendle		VAL      None (PD)			/nci [/COM]				*2
 * Microsoft	LINK     3.x, 5.x			/noi
 * Borland		TLINK    2.0, 3.01, 4.01	/c [/t]					*4
 * Watcom		WLINK    10.0				form DOS [COM]
 * Hyperkinetix	FREELINK 2.50 (freeware)	[/c]					*3
 *
 *1 A[3]86 < v4.04 does not work correctly with Micro-C.
 *2	Original VAL does not work in TINY model. Fix available from DDS.
 *3 FREELINK has no case-sensitive option. See FIXLINK.C in examples.
 *4 These are very good and free now!
 *
 * If your assembler and linker are not compatible with the command lines
 * hard coded in this program, do the following:
 *
 * - Modify CC.C, LC.BAT and PC86.IDE to use acceptable commands
 * - Compile CC.C using DDSIDE
 *         - or -
 * - Compile CC.C by running MCP, MCC, Assembler and LC commands:
 *       MCP CC.C CC.C1 l=.
 *       MCC CC.C1 CC.ASM
 *       Assemble with your assembler to get CC.OBJ
 *       LC CC
 * - Test CC.COM by re-compiling itself: CC CC -fop
 *
 * Where possible, the assembler and linker should be run with appropriate
 * options to cause them to be CASE SENSITIVE, and QUIET in operation (no
 * output messages). If either your assembler OR linker does not support
 * case sensitive operation, run BOTH of them in case insensitive mode,
 * and be sure not to use any global symbols which differ only in case.
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 *
 * Compile command: cc cc -fop
 */
#include <stdio.h>

#define	NOFILE	2		/* EXEC return code for file not found */
#define	NOPATH	3		/* EXEC return code for path not found */

static char mcdir[65], temp[65], ofile[65], tail[150], mcparm[80];
static char oasm = -1, opt = 0, pre = 0, link = -1, verb = -1, lst = 0,
	dup = 0, del = -1, com = 0, fold = 0, symb = 0, *fnptr, *mptr = &mcparm;

int model=0;
char *models[] = { "TINY", "SMALL" };

char htext[] = { "\n\
Use: CC <name> [-acdfklmopqs h= m= t=] [symbol=value]\n\n\
opts:	-Asm		-Comment	-Dupwarn	-Foldliteral\n\
	-Keeptemp	-Listing	-Module		-Optimize\n\
	-Preprocess	-Quiet		-Symbolic\n\n\
	H=homepath	M=TS(model)	T=temprefix\n\
\n\?COPY.TXT 1990-2005 Dave Dunfield\n**See COPY.TXT**.\n" };

/*
 * Main program, process options & invoke appropriate commands
 */
main(int argc, char *argv[])
{
	int i;
	char ifile[65], *ptr, c;

	/* Get default directories from environment */
	if(!getenv("MCDIR", mcdir)) {	/* Get MICRO-C directory */
		message("Environment variable MCDIR is not set!\n\n");
		strcpy(mcdir, "C:\\MC"); }
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
					case 'A' : oasm = 0;	continue;
					case 'C' : com = -1;	continue;
					case 'F' : fold = -1;	continue;
					case 'K' : del = 0;		continue;
					case 'L' : lst = -1;	continue;
					case 'M' : link = 0;	continue;
					case 'O' : opt = -1;	continue;
					case 'D' : dup = -1;
					case 'P' : pre = -1;	continue;
					case 'Q' : verb = 0;	continue;
					case 'S' : symb = -1;	continue; }
				goto badopt; }
			continue; }

		if(*(ptr+1) == '=') switch(toupper(*ptr)) {
			case 'H' : strcpy(mcdir, ptr+2);	continue;
			case 'M' :
				c = toupper(*(ptr+2));
				for(model=0; *models[model] != c; ++model)
					if(model >= (sizeof(models)/2))
						goto badopt;
				continue;
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

	message("DDS MICRO-C PC86 Compiler v3.23\n");

	if(argc < 2)
		abort(htext);

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
		message("Preprocess... ");
		sprintf(ofile,"%s%s.CP", temp, fnptr);
		sprintf(tail,"%s %s l=%s -q -l%s%s", ifile, ofile, mcdir,
			dup ? " -d" : "", mcparm);
		docmd("MCP.EXE");
		strcpy(ifile, ofile); }

	/* Compile to assembly language */
	message("Compile... ");
	sprintf(ofile,"%s%s.%s", (oasm||opt) ? temp : "", fnptr,
		opt ? "CO" : "ASM");
	sprintf(tail,"%s %s -q%s%s%s%s", ifile, ofile,
		pre ? " -l" : "", com ? " -c" : "", fold ? " -f" : "",
		symb ? " -s" : "");
	docmd("MCC.EXE");
	if(pre)
		erase(ifile);
	strcpy(ifile, ofile);

	/* Optimize the assembly language */
	if(opt) {
		message("Optimize... ");
		sprintf(ofile,"%s%s.ASM", oasm ? temp : "", fnptr);
		sprintf(tail, "%s %s -q", ifile, ofile);
		docmd("MCO.COM");
		erase(ifile);
		strcpy(ifile, ofile); }


	/* Assemble into object module */
	if(oasm) {
//		sprintf(ofile,"%s%s.OBJ", link ? temp : "", fnptr);
		sprintf(ofile,"%s%s", link ? temp : "", fnptr);
		sprintf(mcparm, lst ? ",%s.LST" : "", fnptr);
		message("Assemble... ");
		sprintf(tail,"/t/ml %s,%s%s;", ifile, ofile, mcparm);
		docmd("TASM.EXE");
		erase(ifile);
		strcpy(ifile, ofile);

	/* Link into executable program */
		if(link) {
			sprintf(tail, "Link %s...\n", models[model]);
			message(tail);
			sprintf(mcparm, lst ? "%s.MAP" : "NUL", fnptr);
			sprintf(ofile,"%s.%s", fnptr, model ? "EXE" : "COM");
			sprintf(tail,"/c%s %s\\PC86RL_%c %s,%s,%s,%s\\MCLIB;",
				model ? "" : "/t",
				mcdir, *models[model], ifile, ofile, mcparm, mcdir);
			docmd("TLINK.EXE");
			erase(ifile);
			} }

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
message(char *ptr)
{
	if(verb)
		fputs(ptr, stderr);
}

/*
 * Erase temporary file (if enabled)
 */
erase(char *file)
{
	if(del)
		delete(file);
}
