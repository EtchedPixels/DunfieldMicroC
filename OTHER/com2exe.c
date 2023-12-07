/*
 * This program converts a .COM format executable file (such as produced
 * by MICRO-C's TINY model) into a .EXE format executable. It is essentially
 * the reverse of the EXE2BIN utility.
 *
 * ?COPY.TXT 1993-2005 Dave Dunfield
 * **See COPY.TXT**.
 *
 * Compile command: cc com2exe -fop
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
	char		spare[4]; } exe_header;

static char buffer[50000], filename[65];

/*
 * Error messages
 */
char *errmsgs[] = {
	"\nUse: com2exe <COMname> [EXEname]\n\n?COPY.TXT 1993-2005 Dave Dunfield\n**See COPY.TXT**.\n",
	"Cannot open input file",
	"Cannot open output file",
	"Write error",
	"File too large" };

/*
 * Display an error message
 */
error(int errnum)
{
	lputs(errmsgs[errnum], L_stderr);
	exit(errnum);
}

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
 * Build an EXE header & prefix file contents
 */
main(int argc, char *argv[])
{
	unsigned size, csize;
	HANDLE fh;

	if(argc < 2)
		error(0);

	if(!(fh = open_file(argv[1], "COM", F_READ)))
		error(1);

	csize = (size = read(buffer, sizeof(buffer), fh)) + 32;
	close(fh);
	if(size == sizeof(buffer))
		error(4);

	memset(exe_header, 0, sizeof(exe_header));
	exe_header.signature = 'ZM';
	exe_header.last_block = csize % 512;
	exe_header.num_blocks = (csize + 511) / 512;
	exe_header.header_size = 2;
	exe_header.min_mem = 4096 - ((size + 15) / 16);
	exe_header.max_mem = 0xFFFF;
	exe_header.code_seg = exe_header.stack_seg = -16;
	exe_header.stack_ptr = -2;
	exe_header.instr_ptr = 0x0100;
	exe_header.rel_start = 0x001C;

	if(!(fh = open_file((argc > 2) ? argv[2] : filename, "EXE", F_WRITE)))
		error(2);
	if(write(exe_header, sizeof(exe_header), fh) ||	write(buffer, size, fh))
		error(3);
	close(fh);
}
