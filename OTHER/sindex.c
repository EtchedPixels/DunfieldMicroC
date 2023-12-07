/*
 * DDS MICRO-C Source Indexer
 *
 * This program help with the generation of the "EXTINDEX.LIB" file
 * for the SLINK utility. All files matching the specifed pattern
 * are examined, and entries are made for each file, containing all
 * non-compiler generated labels.
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 *
 * Compile command: cc sindex -fop
 */
#include <stdio.h>

#define	LINESIZ		100		/* Maximum size of input line */
#define	FILESIZ		12		/* Maximum size of file name */

main(argc, argv)
	int argc;
	int *argv[];
{
	int i;
	char name[FILESIZ+1], buffer[LINESIZ+1], *ptr, *ptr1;
	FILE *index_fp, *fp;
	static char *pattern = "*.ASM", *index = "EXTINDEX.LIB";

	fputs("DDS MICRO-C Source Indexer\n?COPY.TXT 1990-2005 Dave Dunfield\n**See COPY.TXT**.\n", stderr);

	for(i=1; i < argc; ++i) switch(*argv[i]) {
		case '?' :	argc = 0;				break;
		case '=i' : index = &argv[i][1];	break;
		default: pattern = argv[i]; }

	if(!argc)
		abort("\nUse SINDEX [filespec i=index]\n");

	if(find_first(pattern, 0, name, &i, &i, &i, &i, &i)) {
		text_message("No files matching", pattern);
		exit(-1); }

	index_fp = fopen(index, "wvq");

	do {
		if(fp = fopen(name, "rv")) {
			text_message("Processing", name);
			putc('-', index_fp);
			fputs(name, index_fp);
			putc('\n', index_fp);
			while(fgets(ptr = buffer, LINESIZ, fp)) {
				if(strbeg(buffer, "$DD:"))
					ptr += 4;
				if(isalpha(*ptr) || (*ptr == '_')) {
					ptr1 = ptr;
					do
						++ptr1;
					while(isalnum(*ptr1) || (*ptr1 == '_'));
					if(isspace(*ptr1) || !*ptr1) {
						*ptr1 = 0;
						fputs(ptr, index_fp);
						putc('\n', index_fp); } } }
			fclose(fp); } }
	while(!find_next(name, &i, &i, &i, &i, &i));

	fclose(index_fp);
}

/*
 * Display an error message with text
 */
text_message(message, text)
	char *message, *text;
{
	fputs(message, stderr);
	fputs(": '", stderr);
	fputs(text, stderr);
	fputs("'\n", stderr);
}
