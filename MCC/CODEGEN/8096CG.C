/*
 * DDS MICRO-C Code Generator for: 8096 Family
 *
 *    The 8096 family of microprocessors has a fairly rich instruction
 * set, and employs a "register file" which provides practically unlimited
 * general purpose registers. Accessing these registers requires a byte of
 * extra code, and thus the codesize tends to be large.
 *
 *    The 8096 is also fairly fussy about WORD ALIGNMENT, and therefore this
 * code generator contains extra code to insure that word accesses to global
 * memory and the stack will remain on even boundaries. NOTE that in doing
 * so, the symbol table is re-organized (placing word accesses first), which
 * causes most of the ARRAY BOUNDS TESTS in "test.c" to fail (It assumes that
 * variables are allocated in memory in the order that they are declared).
 *
 *    For this code generator, we have defined a set of registers which
 * roughly corresponds to those of the 8086 processor:
 *	AX	- Accumulator
 *	SI	- Index register
 *	DI	- Holding register for stacked operands
 *	BP	- "Base" pointer for local variable access
 *	BX	- Location for temporary partial results + switch table access
 *	CX	- Secondary parameter to runtime library calls + shift count
 *	DX	- Used during multiply & divide instructions
 *
 * SLINK segments:
 *	S0	- Code
 *	S1	- WORD Initialized variables
 *	S2	- BYTE initialized variables
 *	S3	- WORD "register" variables
 *	S4	- BYTE "register" variables
 *
 * ?COPY.TXT 1991-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

#ifndef LINE_SIZE
#include "compile.h"

extern char s_name[MAX_SYMBOL][SYMBOL_SIZE+1];
extern unsigned s_type[], s_index[], s_dindex[], dim_pool[], global_top,
	local_top, function_count;
extern int size_of_var();
#endif

unsigned stack_frame, global_width = 0, global_set = 0, current_seg = 0,
	switch_lab[6] = { 0 }, switch_sp = 0;
char call_buffer[50], zero_flag, last_byte = 0, stack_flag = 0;

char symbolic = 0;			/* controls output of symbolic information */

/*
 * Set the current output segment
 */
set_seg(seg)
	int seg;
{
	if(seg != current_seg) {
		out_str("$SE:");
		out_chr((current_seg = seg)+'0');
		out_nl(); }
}

/*
 * Get a parameter into the secondary register
 */
char *get_parm(type)
	unsigned type;
{
	if((type & (BYTE | POINTER)) == BYTE)		/* byte access */
		return (type & UNSIGNED) ? "LDBZE CX,|" : "LDBSE CX,|";

	return  "LD CX,|";							/* all 16 bit cases */
}

/*
 * Generate a call to a runtime library function.
 */
char *runtime(string, type)
	char *string;	/* name of library routine */
	unsigned type;	/* type of access */
{
	char *ptr, *ptr1;

	if((type & (BYTE | POINTER)) == BYTE)		/* byte access */
		ptr = (type & UNSIGNED) ? "LDBZE CX,|" : "LDBSE CX,|";
	else
		ptr = "LD CX,|";

	for(ptr1 = call_buffer; *ptr; ++ptr1)
		*ptr1 = *ptr++;
	*ptr1++ = '\n';
	*ptr1++ = ' ';
	*ptr1++ = 'L';
	*ptr1++ = 'C';
	*ptr1++ = 'A';
	*ptr1++ = 'L';
	*ptr1++ = 'L';
	*ptr1++ = ' ';
	while(*string)
		*ptr1++ = *string++;
	*ptr1 = 0;
	return call_buffer;
}

/*
 * Write an operand value
 */
write_oper(token, value, type)
	unsigned token, value, type;
{

	switch(token) {
		case NUMBER:
			out_chr('#');
			out_num(value);
			break;
		case STRING:
			out_str("#?0+");
			out_num(value);
			break;
		case SYMBOL:
			if((value < global_top) || (type & (STATIC|EXTERNAL))) {
				out_symbol(value);
				break; }
			if(type & ARGUMENT)
				out_num(s_index[value] + 4);
			else {
				out_chr('-');
				out_num(stack_frame - s_index[value]); }
			out_str("[BP]");
			break;
		case IN_TEMP:
			out_str("BX");
			break;
		case INDIRECT:
			out_str("[SI]");
			break;
		case ON_STACK:
			out_str("DI");
			break;
		case ION_STACK:
		case ISTACK_TOP:
			out_str("[DI]");
			break;
		default:		/* Unknown (error) */
			out_num(token);
			out_chr('?'); }
}

/*
 * Test for operand on the stack & generate appriopriate pops
 */
test_stack(token)
	unsigned token;
{
	if((token==ON_STACK) || (token==ION_STACK) || (token==ISTACK_TOP)) {
		if(stack_flag) {
			stack_flag = 0;
			return; }
		out_inst("POP DI");
		if(token == ISTACK_TOP)
			stack_flag = -1; }
}

/*
 * Determine if a type is a pointer to 16 bits
 */
isp16(type)
	unsigned type;
{
	if(type & (POINTER-1))			/* pointer to pointer */
		return 1;
	if(type & POINTER)				/* first level pointer */
		return !(type & BYTE);
	return 0;						/* not a pointer */
}

/*
 * Output text as comment in ASM source
 */
do_comment(ptr)
	char *ptr;
{
	if(global_width) {
		out_nl();
		global_width = 0; }
	if(ptr) {
		out_chr('*');
		do_asm(ptr); }
}

/*
 * Output a string to the assembler followed by newline.
 */
do_asm(ptr)
	char *ptr;
{
	out_str(ptr);
	out_nl();
}

/*
 * Release stack allocation
 */
release_stack(size)
{
}

/*
 * Define beginning of module
 */
def_module() { }

/*
 * End of module definition
 */
end_module()
{
	unsigned i;

	if(symbolic) for(i=0; i < global_top; ++i) {
		set_seg(0);
		out_str("*#gbl ");
		dump_symbol(i); }
}

/*
 * Begin definition of a static variable
 */
def_static(symbol, ssize)
	unsigned symbol, ssize;
{
	set_seg((ssize || ((s_type[symbol] & (BYTE|POINTER)) != BYTE)) ? 1 : 2 );
	out_symbol(symbol);
}

/*
 * Initialize static storage
 */
init_static(token, value, word)
	unsigned token, value;
	char word;
{
	char *ptr;

	out_str(global_width ? "," : word ? " DW " : " DB ");
	switch(token) {
		case SYMBOL :			/* Symbol address */
			out_symbol(value);
			global_width += 12;
			break;
		case STRING :			/* literal pool entry */
			ptr = "?0+";
			global_width += 9;
			goto doinit;
		case LABEL :			/* instruction label */
			ptr = "?";
			global_width += 4;
			goto doinit;
		default :				/* constant value */
			ptr = "";
			global_width += 6;
		doinit:
			out_str(ptr);
			out_num(value); }

	if(global_width > 60) {
		global_width = 0;
		out_nl(); }

	++global_set;
}

/*
 * End static storage definition
 */
end_static()
{
	if(global_width)
		out_nl();

	if(!global_set)
		out_inst("DS 0");

	global_set = global_width = 0;
}

/*
 * Define a global non-static variable
 */
def_global(symbol, size)
	unsigned symbol, size;
{
	if(s_type[symbol] & REGISTER) {		/* Internal variables */
		set_seg((size & 1) + 3);
		out_symbol(symbol);
		out_str(" DS "); }
	else {
		out_str("$DD:");
		out_symbol(symbol);
		out_sp(); }						/* External variables */

	out_num(size);
	out_nl();
}

/*
 * Define an external variable
 */
def_extern(symbol)
	unsigned symbol;
{
	out_str("$EX:");
	out_symbol(symbol);
	out_nl();
}

/*
 * Enter function & allocate local variable stack space
 */
def_func(symbol, size)
	unsigned symbol, size;
{
	unsigned l, h, sp;

	size = (size + 1) & 0xFFFE;					/* Maintain word alignment */

	/* Re-sort symbol table to insure words accessed items occur first */
	for(h = MAX_SYMBOL; h > local_top; --h) {	/* Skip arguments */
		if(!(s_type[h-1] & ARGUMENT))
			break; }
	sort_symbol(local_top, h);					/* Position WORDS first */
	sp = 0;
	/* Re-assign stack indexes, beginnning with word accessed entries */
	for(l = local_top; l < h; ++l) {
		if(!(s_type[l] & (SYMTYPE|STATIC|EXTERNAL))) {
			s_index[l] = sp;
			sp += size_of_var(l); } }

	/* Finally, get on with outputing the function entry code */
	set_seg(0);

	if(symbolic) {
		out_str("*#fun ");
		out_symbol(symbol);
		out_sp();
		put_num(size, -1);
		out_str(" ?");
		out_num(function_count);
		out_nl(); }

	out_symbol(symbol);
	out_str(" PUSH BP\n LD BP,SP\n");
	if(stack_frame = size) {
		out_str(" SUB SP,#");
		out_num(size);
		out_nl(); }
}

/*
 * Clean up the stack & end function definition
 */
end_func()
{
	unsigned i;

	if(stack_frame)
		out_inst("LD SP,BP");
	out_inst("POP BP");
	out_inst("RET");

	if(symbolic) {
		for(i = local_top; i < MAX_SYMBOL; ++i) {
			out_str("*#lcl ");
			dump_symbol(i); }
		do_comment("#end"); }
}

/*
 * Dump a symbol definition "magic comment" to the output file
 */
dump_symbol(s)
	unsigned s;
{
	unsigned i, t;

	*s_name[s] &= 0x7F;
	out_symbol(s);
	out_sp();
	put_num(t = s_type[s], -1);
	out_sp();
	i = s_dindex[s];
	switch(t & SYMTYPE) {
		case FUNCGOTO :
			if(s < local_top)
				goto dofunc;
		case STRUCTURE :
			put_num(i, -1);
			break;
		default:
		dofunc:
			put_num(s_index[s], -1);
			if(t & ARRAY) {
				out_sp();
				put_num(t = dim_pool[i], -1);
				while(t--) {
					out_sp();
					put_num(dim_pool[++i], -1); } } }
	out_nl();
}

/*
 * Define a compiler generated label
 */
def_label(label)
	unsigned label;
{
	if(label == switch_lab[switch_sp]) {		/* Word align switch tables */
		--switch_sp;
		do_asm(" ORG (*+1)&-2"); }
	out_chr('?');
	out_num(label);
	out_inst(" EQU *");
}

/*
 * Define literal pool
 */
def_literal(ptr, size)
	unsigned char *ptr;
	unsigned size;
{
	unsigned i;

	set_seg(0);
	if(size) {
		i = 0;
		out_str("?0");
		while(i < size) {
			out_str((i % 16) ? "," : " DB ");
			out_num(*ptr++);
			if(!(++i % 16))
				out_nl(); }
		if(i % 16)
			out_nl(); }
}

/*
 * Call a function by name
 */
call(token, value, type, clean)
	unsigned token, value, type, clean;
{
	switch(token) {
		case NUMBER :
			out_str(" LCALL ");
			out_num(value);
			break;
		case SYMBOL :
			out_str(" LCALL ");
			write_oper(token, value, type);
			break;
		case ON_STACK :
		case ION_STACK :
			if(clean) {
				out_str(" MOV CX,");
				out_num(clean++ * 2);
				out_str("[SP]\n"); }
			else
				out_inst("POP CX");
			out_str(" LCALL ?idcall"); }
	out_nl();

	if(clean += clean) {	/* clean up stack following function call */
		out_str(" ADD SP,#");
		out_num(clean);
		out_nl(); }

	last_byte = (type & (POINTER | BYTE)) == BYTE;
	zero_flag = -1;
}

/*
 * Unconditional jump to label
 */
jump(label, ljmp)
	unsigned label;		/* destination label */
	char ljmp;			/* long jump required */
{
	out_str(ljmp ? " LJMP ?" : " SJMP ?");
	out_num(label);
	out_nl();
}

/*
 * Conditional jump to label
 */
jump_if(cond, label, ljmp)
	char cond;			/* condition TRUE of FALSE */
	unsigned label;		/* destination label */
	char ljmp;			/* long jump required */
{
	if(zero_flag) {		/* set up 'Z' flag if necessary */
		out_inst(last_byte ? "ANDB AX,AX" : "AND AX,AX");
		zero_flag = 0; }

	if(ljmp) {
		out_str(cond ? " JE" : " JNE");
		out_str(" *+5\n LJMP ?");
		out_num(label);
		out_nl(); }
	else {
		out_str(cond ? " JNE ?" : " JE ?");
		out_num(label);
		out_nl(); }
}

/*
 * Perform a switch operation
 */
do_switch(label)
	unsigned label;			/* address of switch table */
{
	out_str(" LD BX,#?");
	out_num(switch_lab[++switch_sp] = label);
	do_asm("\n LJMP ?switch");
}

/*
 * Load index register with a pointer value
 */
index_ptr(token, value, type)
	unsigned token, value, type;
{
	if(token == IN_ACC)
		out_inst("LD SI,AX");
	else {
		test_stack(token);
		out_str(" LD SI,");
		write_oper(token, value, type);
		out_nl(); }
}

/*
 * Load index register with the address of a symbol
 */
index_adr(token, value, type)
	unsigned token, value, type;
{
	if(token == ION_STACK) {
		out_inst("POP SI");
		return; }

	if((value < global_top) || (type & (STATIC|EXTERNAL))) {
		out_str(" LD SI,#");
		out_symbol(value);
		out_nl();
		return; }
	out_str(" ADD SI,BP,#");
	if(type & ARGUMENT)
		out_num(s_index[value] + 4);
	else {
		out_chr('-');
		out_num(stack_frame - s_index[value]); }
	out_nl();
}

/*
 * Expand 8 bit accumulator to 16 bits if necessary.
 */
expand(type)
	unsigned type;
{
	if(last_byte) {
		out_inst((type & UNSIGNED) ? zero_flag = -1, "CLRB AX+1" : "EXTB AX");
		last_byte = 0; }
}

/*
 * Do a simple register operation
 */
accop(oper, rtype)
	unsigned oper, rtype;
{
	char *ptr, byte, eflag, zflag;

	eflag = byte = zflag = 0;

	if((rtype & (POINTER | BYTE)) == BYTE)
		byte = -1;

	switch(oper) {
		case _STACK:		/* stack accumulator */
			eflag = -1;
			ptr = "PUSH AX";
			zflag = 0x55;
			break;
		case _ISTACK:		/* stack index register */
			ptr = "PUSH SI";
			byte = last_byte;
			zflag = 0x55;
			break;
		case _TO_TEMP:		/* copy accumulator to temp */
			ptr = byte ? "LDB BX,AX" : "LD BX,AX" ;
			zflag = 0x55;
			break;
		case _FROM_INDEX:	/* copy index to accumulator */
			ptr = "LD AX,SI";
			last_byte = byte = 0;
			zflag = -1;
			break;
		case _COM:			/* complement accumulator */
			ptr = byte ? "NOTB AX" : "NOT AX";
			break;
		case _NEG:			/* negate accumulator */
			ptr = byte ? "NEGB AX" : "NEG AX";
			break;
		case _NOT:			/* logical complement */
			eflag = -1;
			ptr = "LCALL ?not";
			break;
		case _INC:			/* increment accumulator */
			if(isp16(rtype))
				ptr = "ADD AX,#2";
			else
				ptr = byte ? "INCB AX" : "INC AX";
			break;
		case _DEC:			/* decrement accumulator */
			if(isp16(rtype))
				ptr = "SUB AX,#2";
			else
				ptr = byte ? "DECB AX" : "DEC AX";
			break;
		case _IADD:			/* add acc to index register */
			eflag = -1;
			ptr = "ADD SI,AX";
			zflag = -1;
			break;
		default:			/* Unknown (error) */
			ptr = "?S?"; }

/* if necessary, extend acc before executing instruction */
	if(eflag || !byte)
		expand(rtype);
	else
		last_byte = byte;

	out_inst(ptr);

/* If the instruction sets/clears the zero flag, update the status */
	if(zflag != 0x55)
		zero_flag = zflag;
}

/*
 * Perform an operation with the accumulator and
 * the specified value;
 */
accval(oper, rtype, token, value, type)
	unsigned oper, rtype, token, value, type;
{
	char *ptr, *ptr1, byte, rbyte, eflag, zflag;

	byte = rbyte = eflag = zflag = 0;
	ptr1 = 0;

	test_stack(token);		/* get stack operand if needed */

/* values on stack are always words */
	if(token == ON_STACK)
		type &= ~BYTE;

/* determine of length of source & result */
	if((type & (BYTE | POINTER)) == BYTE)
		byte = -1;
	if((rtype & (BYTE | POINTER)) == BYTE)
		rbyte = last_byte;

	switch(oper) {
		case _LOAD:				/* load accumulator */
			ptr = (rbyte = byte) ? "LDB AX,|" : "LD AX,|";
			last_byte = 0;		/* insure no pre - sign extend */
			zflag = -1;
			break;
		case _STORE:	/* store accumulator */
			ptr = byte ? "STB AX,|" : "ST AX,|";
			break;
		case _ADD:		/* addition */
			if(byte) {
				ptr = "ADDB AX,|";
				if(!rbyte)
					ptr1 = "ADDCB AX+1,#0"; }
			else
				ptr = "ADD AX,|";
			break;
		case _SUB:		/* subtract */
			if(byte) {
				ptr = "SUBB AX,|";
				if(!rbyte)
					ptr1 = "SUBCB AX+1,0"; }
			else
				ptr = "SUB AX,|";
			break;
		case _MULT:		/* multiply */
			eflag = -1;
			if((token == NUMBER) && (value == 2)) {	/* efficent *2 */
				ptr = "SHL AX,#1";
				break; }
			zflag = -1;
			ptr = get_parm(type);
			ptr1 = (rtype & UNSIGNED) ? "MULU AX,CX" : "MUL AX,CX";
			break;
		case _DIV:		/* divide */
			zflag = eflag = -1;
			ptr = get_parm(type);
			ptr1 = (rtype & UNSIGNED)
				? "CLR DX\n DIVU AX,CX"
				: "EXT AX\n DIV AX,CX";
			break;
		case _MOD:		/* remainder */
			zflag = eflag = -1;
			ptr = get_parm(type);
			ptr1 = (rtype & UNSIGNED)
				? "CLR DX\n DIVU AX,CX\n LD AX,DX"
				: "EXT AX\n DIV AX,CX\n LD AX,DX";
			break;
		case _AND:		/* logical and */
			if(byte) {
				ptr = "ANDB AX,|";
				if(!rbyte)
					ptr1 = "CLRB AX+1"; }
			else
				ptr = "AND AX,|";
			break;
		case _OR:		/* logical or */
			ptr = byte ? "ORB AX,|" : "OR AX,|";
			break;
		case _XOR:		/* exclusive or */
			ptr = byte ? "XORB AX,|" : "XOR AX,|";
			break;
		case _SHL:		/* shift left */
			zflag = eflag = -1;
			if(token == NUMBER)
				ptr = "SHL AX,|";
			else {
				ptr = get_parm(type);
				ptr1 = "SHL AX,CX"; }
			break;
		case _SHR:		/* shift right */
			zflag = eflag = -1;
			if(token == NUMBER)
				ptr = "SHR AX,|";
			else {
				ptr = get_parm(type);
				ptr1 = "SHR AX,CX"; }
			break;
		case _EQ:		/* test for equal */
			eflag = -1;
			ptr = runtime("?eq", type);
			break;
		case _NE:		/* test for not equal */
			eflag = -1;
			ptr = runtime("?ne", type);
			break;
		case _LT:		/* test for less than */
			eflag = -1;
			ptr = runtime("?lt", type);
			break;
		case _LE:		/* test for less or equal to */
			eflag = -1;
			ptr = runtime("?le", type);
			break;
		case _GT:		/* test for greater than */
			eflag = -1;
			ptr = runtime("?gt", type);
			break;
		case _GE:		/* test for greater than or equal to */
			eflag = -1;
			ptr = runtime("?ge", type);
			break;
		case _ULT:		/* unsigned less than */
			eflag = -1;
			ptr = runtime("?ult", type);
			break;
		case _ULE:		/* unsigned less than or equal to */
			eflag = -1;
			ptr = runtime("?ule", type);
			break;
		case _UGT:		/* unsigned greater than */
			eflag = -1;
			ptr = runtime("?ugt", type);
			break;
		case _UGE:		/* unsigned greater than or equal to */
			eflag = -1;
			ptr = runtime("?uge", type);
			break;
		default:		/* Unknown (error) */
			ptr = "?D? |"; }

/* if necessary, extend acc before executing instruction */
	if(eflag || !rbyte)
		expand(rtype);
	else
		last_byte = rbyte;

/* If the instruction sets/clears the zero flag, update the status */
	if(oper != _STORE)
		zero_flag = zflag;

/* interpret the output string & insert the operands */
	out_sp();
	while(*ptr) {
		if(*ptr == '|')
			write_oper(token, value, type);
		else
			out_chr(*ptr);
		++ptr; }
	out_nl();

	if(ptr1) {
		out_inst(ptr1);
		zero_flag = -1; }
}

/*
 * Output a symbol name
 */
out_symbol(s)
	unsigned s;
{
	if(s_type[s] & STATIC) {		/* Static, output local label */
		out_chr('?');
		out_num(s_index[s]); }
	out_str(s_name[s]);
}

/*
 * Write an instruction to the output file
 */
out_inst(ptr)
	char *ptr;
{
	put_chr(' ', -1);
	put_str(ptr, -1);
	put_chr('\n', -1);
}

/*
 * Write a string to the output file
 */
out_str(ptr)
	char *ptr;
{
	put_str(ptr, -1);
}

/*
 * Write a signed number to the output file
 */
out_num(value)
	unsigned value;
{
	if(value & 0x8000) {
		out_chr('-');
		value = -value; }

	put_num(value, -1);
}

/*
 * Write newline/space characters to the output file.
 */
out_nl() { put_chr('\n', -1); }
out_sp() { put_chr(' ', -1); }

/*
 * Write a character to the output file
 */
out_chr(chr)
	char chr;
{
	put_chr(chr, -1);
}

/*
 * Sort the symbol table so that all variables of an even number of bytes
 * in size occur first. This insures that WORD accessed variables and
 * structures can be aligned on word boundaries.
 */
sort_symbol(low, high)
	int low, high;
{
	char name[SYMBOL_SIZE+1];
	unsigned type, index, dindex, i;

	for(; low < high; ++low) {
		if(s_type[low] & SYMTYPE)				/* Only sort variables */
			continue;
		if(size_of_var(low) & 1) {				/* Odd sized variable */
			for(i=low+1; i < high; ++i) {
				if(s_type[i] & SYMTYPE)
					continue;
				if(!(size_of_var(i) & 1)) {
					copy_string(name, s_name[low]);
					type = s_type[low];
					index = s_index[low];
					dindex = s_dindex[low];
					copy_string(s_name[low], s_name[i]);
					s_type[low] = s_type[i];
					s_index[low] = s_index[i];
					s_dindex[low] = s_dindex[i];
					copy_string(s_name[i], name);
					s_type[i] = type;
					s_index[i] = index;
					s_dindex[i] = dindex;
					break; } } } }
}
