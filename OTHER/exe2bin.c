/*
 * This is a simple replacement for the EXE2BIN command which used
 * to be supplied with DOS. Newer versions of DOS supply EXE2BIN
 * only with the DOS Technical Reference, which not everybody has.
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 *
 * Compile command: cc exe2bin -fop
 */
#include <stdio.h>
#include <file.h>

/*
 * Structure of the EXE file header
 */
static struct {
	unsigned	signature;		/* Signature of file (0x5A4D) */
	unsigned	last_block;		/* Size of last block (0-512) */
	unsigned	num_blocks;		/* Number of 512 byte blocks */
	unsigned	num_reloc;		/* Number of relocation table entries */
	unsigned	header_size;	/* Size of header (in 16 byte blocks) */
	unsigned	min_mem;		/* Minimum memory requirement */
	unsigned	max_mem;		/* Maximum memory requirement */
	unsigned	stack_seg;		/* Offset to stack segment */
	unsigned	stack_ptr;		/* Initial value of stack pointer */
	unsigned	checksum;		/* EXE file checksum */
	unsigned	instr_ptr;		/* Initial value of instruction pointer */
	unsigned	code_seg;		/* Offset to code segment */
	unsigned	rel_start;		/* Offset to relocation table */
	unsigned	overlay_num;	/* overlay number */
	} exe_header;

/* Table of error messages */
static char *errmsgs[] = {
	"\nUse: exe2bin <EXEname> [BINname]\n\n?COPY.TXT 1990-2005 Dave Dunfield\n**See COPY.TXT**.\n",
	"Cannot open input file",
	"Cannot open output file",
	"Cannot read EXE header",
	"Invalid EXE file",
	"File cannot be converted",
	"Read error",
	"Write error" };

/* Misc global variables */
static char filename[65];

/*
 * Open the file, appending extension if necessary
 */
HANDLE open_file(char *name, char *extension, int options)
{
	char c, *ptr, *dot;
	HANDLE fp;

	/* Copy filename, looking for '.' extension */
	dot = 0;
	for(ptr = filename; c = *ptr = *name++; ++ptr)
		if(c == '.')
			dot = ptr;

	if(!dot) {		/* No extension, assume default */
		*(dot = ptr++) = '.';
		while(*ptr++ = *extension++); }

	fp = open(filename, options);

	*dot = 0;		/* Remove extension for later */

	return fp;
}

/*
 * Display an error message
 */
error(int errnum)
{
	lputs(errmsgs[errnum], L_stderr);
	exit(errnum);
}

/*
 * Main program - Perform EXE to BIN conversion
 */
main(int argc, char *argv[])
{
	int i;
	unsigned code_size, code_start;
	char buffer[512];
	HANDLE input, output;

/* If no arguments, issue the help line */
	if(argc < 2)
		error(0);

/* Open the input file */
	if(!(input = open_file(argv[1], "EXE", F_READ)))
		error(1);

/* Read the EXE file header */
	if(read(exe_header, sizeof(exe_header), input) != sizeof(exe_header))
		error(3);

/* Chech for valid EXE file signature */
	if(exe_header.signature != 0x5A4D)
		error(4);

/* Check that file is convertable (no relocation or stack) */
	if((exe_header.num_reloc || exe_header.stack_seg || exe_header.stack_ptr)
	  || ((exe_header.instr_ptr != 0) && (exe_header.instr_ptr != 0x100)))
		error(5);

/* Open the output file */
	if(!(output = open_file((argc > 2) ? argv[2] : filename, "BIN", F_WRITE)))
		error(2);

/* Calculate beginning and size of code block */
	code_size = (exe_header.num_blocks - (exe_header.last_block != 0)) * 512
		+ exe_header.last_block - (code_start = exe_header.header_size << 4)
		- exe_header.instr_ptr;

/* Seek to the beginning of the executable code */
	lseek(input, 0, code_start + exe_header.instr_ptr, 0);

/* Copy the image into the ".BIN" file */
	while(i = (code_size > 512) ? 512 : code_size) {
		if(read(buffer, i, input) != i)
			error(6);
		if(write(buffer, i, output))
			error(7);
		code_size -= i; }

/* Close the files */
	close(input);
	close(output);
}
