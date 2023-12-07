/*
 * DDS MICRO-C Code Generator for: PC 8086 using MASM*
 *
 * The 8086 family of microprocessors has a fairly rich instruction
 * set, coupled with ample general and not-so-general purpose registers.
 * Automatic generation of efficent code is difficult however, due to
 * restrictions on register usage, and allowable operands.
 *
 * 8086 Registers:
 *	AX	- Accumulator
 *	SI	- Index register
 *	BP	- "Base" pointer for local variable access
 *	BX	- Holding register for stacked operands
 *	CX	- Secondary parameter to runtime library calls + shift count
 *	DX	- Used during multiply & divide instructions
 *	DI	- Not used
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 *
 * Note, to use with the A86 assembler: #define A86
 */

#ifndef LINE_SIZE
#include "compile.h"

extern char s_name[MAX_SYMBOL][SYMBOL_SIZE+1];
extern unsigned s_type[], s_index[], s_dindex[], dim_pool[], global_top,
	local_top, function_count;
#endif

int current_segment = 0;
unsigned stack_frame, global_width = 0, global_set = 0, asmlab = 0;
char call_buffer[50], zero_flag, last_byte = 0, stack_flag = 0;
char *segments[] = { "CSEG", "DSEG", "BSEG" };

char symbolic = 0;			/* controls output of symbolic information */

/*
 * Set the output segment (0=Code, 1=Idata, 2=Udata, -1=None)
 */
set_segment(segment)
	int segment;
{
	if(segment != current_segment) {
		if(current_segment != -1) {
			out_str(segments[current_segment]);
			out_inst("ENDS"); }
		if((current_segment = segment) != -1) {
			out_str(segments[segment]);
			out_inst("SEGMENT"); } }
}

/*
 * Get a parameter into the secondary register
 */
char *get_parm(type, order)
	unsigned type;
	char order;
{
	if((type & (BYTE | POINTER)) == BYTE)		/* byte access */
		return (type & UNSIGNED) ? "MOV CL,|\n XOR CH,CH"
			: (order) ? "MOV CX,AX\n MOV AL,|\n CBW\n XCHG AX,CX"
			: "MOV CX,AX\n MOV AL,|\n CBW";

	return  "MOV CX,|";							/* all 16 bit cases */
}

/*
 * Generate a call to a runtime library function.
 */
char *runtime(string, type, order)
	char *string;	/* name of library routine */
	unsigned type;	/* type of access */
	char order;		/* order is important flag */
{
	char *ptr, *ptr1;

	if((type & (BYTE | POINTER)) == BYTE)		/* byte access */
		ptr = (type & UNSIGNED) ? "MOV CX,AX\n MOV AL,|\n XOR AH,AH"
			: "MOV CX,AX\n MOV AL,|\n CBW";
	else
		ptr = (order) ? "MOV CX,AX\n MOV AX,|" : "MOV CX,|";

	for(ptr1 = call_buffer; *ptr; ++ptr1)
		*ptr1 = *ptr++;
	*ptr1++ = '\n';
	*ptr1++ = ' ';
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
			out_num(value);
			break;
		case STRING:
			out_str("OFFSET DGRP:?0+");
			out_num(value);
			break;
		case SYMBOL:
			if((value < global_top) || (type & (STATIC|EXTERNAL))) {
				if((type & SYMTYPE) != FUNCGOTO)
					out_str("DGRP:");
				out_symbol(value);
				break; }
#ifndef	A86
		if(type & ARGUMENT)
			out_num(s_index[value] + 4);
		else {
			out_chr('-');
			out_num(stack_frame - s_index[value]); }
		out_str("[BP]");
#else
			out_str("[BP");
			if(type & ARGUMENT) {
				out_chr('+');
				out_num(s_index[value] + 4); }
			else {
				out_chr('-');
				out_num(stack_frame - s_index[value]); }
			out_chr(']');
#endif
			break;
		case IN_TEMP:
			if((type & (POINTER | BYTE)) == BYTE)
				out_str("BYTE PTR ");
			out_str("DGRP:?temp");
			break;
		case INDIRECT:
			out_str("[SI]");
			break;
		case ON_STACK:
			out_str("BX");
			break;
		case ION_STACK:
		case ISTACK_TOP:
			out_str("[BX]");
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
		out_inst("POP BX");
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
		out_chr(';');
		do_asm(ptr); }
}

/*
 * Release allocated stack
 */
release_stack(size) { }

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
 * Define beginning of module
 */
def_module()
{
	out_str("DGRP GROUP DSEG,BSEG\n");
	out_str("DSEG SEGMENT BYTE PUBLIC 'IDATA'\nDSEG ENDS\n");
	out_str("BSEG SEGMENT BYTE PUBLIC 'UDATA'\nBSEG ENDS\n");
	out_str("CSEG SEGMENT BYTE PUBLIC 'CODE'\n");
	out_inst("ASSUME CS:CSEG, DS:DGRP, SS:DGRP");
	out_inst("EXTRN ?eq:NEAR,?ne:NEAR,?lt:NEAR,?le:NEAR,?gt:NEAR");
	out_inst("EXTRN ?ge:NEAR,?ult:NEAR,?ule:NEAR,?ugt:NEAR,?uge:NEAR");
	out_inst("EXTRN ?not:NEAR,?switch:NEAR,?temp:WORD");
}

/*
 * End of module definition
 */
end_module()
{
	unsigned i;

	set_segment(-1);
	out_inst("END");

	if(symbolic) for(i=0; i < global_top; ++i) {
		out_str(";#gbl ");
		dump_symbol(i); }
}

/*
 * Begin definition of a static variable
 */
def_static(symbol, ssize)
	unsigned symbol;
{
	out_global(symbol, 1);
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
			out_str("OFFSET DGRP:");
			out_symbol(value);
			global_width += 20;
			break;
		case STRING :			/* Literal pool entry */
			ptr = "OFFSET DGRP:?0+";
			global_width += 19;
			goto doinit;
		case LABEL :			/* instruction label */
			ptr = "OFFSET ?";
			global_width += 12;
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
	out_global(symbol, 2);
	if((s_type[symbol] & (POINTER | BYTE)) == BYTE) {
		out_str(" DB ");
		put_num(size, -1); }
	else {
		out_str(" DW ");
		put_num(size/2, -1); }
	out_str(" DUP(?)\n");
}

/*
 * Define an external variable
 */
def_extern(symbol)
	unsigned symbol;
{
#ifndef A86
	unsigned type;

	set_segment(-1);
	out_str(" EXTRN ");
	out_symbol(symbol);
	out_chr(':');
	if(((type = s_type[symbol]) & SYMTYPE) == FUNCGOTO)
		out_str("NEAR");
	else
		out_str((type & (POINTER | BYTE)) == BYTE ? "BYTE" : "WORD");
	out_chr('\n');
#endif
}

/*
 * Enter function & allocate local variable stack space
 */
def_func(symbol, size)
	unsigned symbol, size;
{
	set_segment(0);

	if(symbolic) {
		out_str(";#fun ");
		out_symbol(symbol);
		out_sp();
		put_num(size, -1);
		out_sp();
		out_num(function_count);
		out_nl(); }

	out_global(symbol, 0);
	out_str(": PUSH BP\n MOV BP,SP\n");
	if(stack_frame = size) {
		out_str(" SUB SP,");
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
		out_inst("MOV SP,BP");
	out_inst("POP BP");
	out_inst("RET");

	if(symbolic) {
		for(i = local_top; i < MAX_SYMBOL; ++i) {
			out_str(";#lcl ");
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
	out_chr('?');
	out_num(label);
	out_str(":\n");
}

/*
 * Define literal pool
 */
def_literal(ptr, size)
	unsigned char *ptr;
	unsigned size;
{
	unsigned i;

	if(size) {
		set_segment(1);
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
		case SYMBOL :		/* Calling a symbol */
		case NUMBER :		/* Calling a fixed address */
			out_str(" CALL ");
			write_oper(token, value, type);
			out_nl();
			break;
		case ON_STACK :		/* Calling a computed address */
		case ION_STACK :	/* Calling a indirect address */
			/* Note that for stacked operands, they were placed */
			/* on the stack BEFORE any function arguments !!!!! */
			if(clean) {		/* Function has arguments */
#ifndef	A86
				out_str(" MOV BX,SP\n MOV BX,");
				out_num(clean++ * 2);
				out_str("[BX]\n"); }
#else
				out_str(" MOV BX,SP\n MOV BX,[BX+");
				out_num(clean++ * 2);
				out_str("]\n"); }
#endif
			else			/* No arguments - simple */
				out_inst("POP BX");
			out_inst("CALL BX"); }

	if(clean += clean) {	/* clean up stack following function call */
		out_str(" ADD SP,");
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
	out_str(ljmp ? " JMP ?" : " JMP SHORT ?");
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
		out_inst(last_byte ? "AND AL,AL" : "AND AX,AX");
		zero_flag = 0; }

	if(ljmp) {
		out_str(cond ? " JZ ??" : " JNZ ??");
		out_num(++asmlab);
		out_str("\n JMP ?");
		out_num(label);
		out_str("\n??");
		out_num(asmlab);
		out_str(":\n"); }
	else {
		out_str(cond ? " JNZ ?" : " JZ ?");
		out_num(label);
		out_nl(); }
}

/*
 * Perform a switch operation
 */
do_switch(label)
	unsigned label;			/* address of switch table */
{
	out_str(" MOV BX,OFFSET ?");
	out_num(label);
	do_asm("\n JMP ?switch");
}

/*
 * Load index register with a pointer value
 */
index_ptr(token, value, type)
	unsigned token, value, type;
{
	if(token == IN_ACC)
		out_inst("MOV SI,AX");
	else {
		test_stack(token);
		out_str(" MOV SI,");
		write_oper(token, value, type);
		out_nl(); }
}

/*
 * Load index register with the address of an assignable object
 */
index_adr(token, value, type)
	unsigned token, value, type;
{
	if(token == ION_STACK)
		out_inst("POP SI");
	else {
		out_str(
			((value < global_top) || (type & (STATIC|EXTERNAL)))
			? " MOV SI,OFFSET " : " LEA SI,");
		write_oper(token, value, type);
		out_nl(); }
}

/*
 * Expand 8 bit accumulator to 16 bits if necessary.
 */
expand(type)
	unsigned type;
{
	if(last_byte) {
		out_inst((type & UNSIGNED) ? zero_flag = -1, "XOR AH,AH" : "CBW");
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
			ptr = byte ? "MOV BYTE PTR DGRP:?temp,AL" : "MOV DGRP:?temp,AX" ;
			zflag = 0x55;
			break;
		case _FROM_INDEX:	/* copy index to accumulator */
			ptr = "MOV AX,SI";
			last_byte = byte = 0;
			zflag = -1;
			break;
		case _COM:			/* complement accumulator */
			ptr = byte ? "NOT AL" : "NOT AX";
			zflag = -1;
			break;
		case _NEG:			/* negate accumulator */
			ptr = byte ? "NEG AL" : "NEG AX";
			break;
		case _NOT:			/* logical complement */
			eflag = -1;
			ptr = "CALL ?not";
			break;
		case _INC:			/* increment accumulator */
			if(isp16(rtype)) {
				ptr = "ADD AX,2";
				eflag = -1; }
			else
				ptr = byte ? "INC AL" : "INC AX";
			break;
		case _DEC:			/* decrement accumulator */
			if(isp16(rtype)) {
				ptr = "SUB AX,2";
				eflag = -1; }
			else
				ptr = byte ? "DEC AL" : "DEC AX";
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
			ptr = (rbyte = byte) ? "MOV AL,|" : "MOV AX,|";
			last_byte = 0;		/* insure no pre - sign extend */
			zflag = -1;
			break;
		case _STORE:	/* store accumulator */
			ptr = byte ? "MOV |,AL" : "MOV |,AX";
			break;
		case _ADD:		/* addition */
			if(byte) {
				ptr = "ADD AL,|";
				if(!rbyte)
					ptr1 = "ADC AH,0"; }
			else
				ptr = "ADD AX,|";
			break;
		case _SUB:		/* subtract */
			if(byte) {
				ptr = "SUB AL,|";
				if(!rbyte)
					ptr1 = "SBB AH,0"; }
			else
				ptr = "SUB AX,|";
			break;
		case _MULT:		/* multiply */
			eflag = -1;
			if((token == NUMBER) && (value == 2)) {	/* efficent *2 */
				ptr = "SAL AX,1";
				break; }
			zflag = -1;
			ptr = get_parm(type, 0);
			ptr1 = (rtype & UNSIGNED) ? "MUL CX" : "IMUL CX";
			break;
		case _DIV:		/* divide */
			ptr = get_parm(type, zflag = eflag = -1);
			ptr1 = (rtype & UNSIGNED) ? "XOR DX,DX\n DIV CX" : "CWD\n IDIV CX";
			break;
		case _MOD:		/* remainder */
			ptr = get_parm(type, zflag = eflag = -1);
			ptr1 = (rtype & UNSIGNED) ? "XOR DX,DX\n DIV CX\n MOV AX,DX"
				: "CWD\n IDIV CX\n MOV AX,DX";
			break;
		case _AND:		/* logical and */
			if(byte) {
				ptr = "AND AL,|";
				if(!rbyte)
					ptr1 = "XOR AH,AH"; }
			else
				ptr = "AND AX,|";
			break;
		case _OR:		/* logical or */
			ptr = byte ? "OR AL,|" : "OR AX,|";
			break;
		case _XOR:		/* exclusive or */
			ptr = byte ? "XOR AL,|" : "XOR AX,|";
			break;
		case _SHL:		/* shift left */
			ptr = (byte || (token == NUMBER)) ? "MOV CL,|" : "MOV CX,|";
			ptr1 = "SHL AX,CL";
			zflag = eflag = -1;
			break;
		case _SHR:		/* shift right */
			ptr = (byte || (token == NUMBER)) ? "MOV CL,|" : "MOV CX,|";
			ptr1 = "SHR AX,CL";
			zflag = eflag = -1;
			break;
		case _EQ:		/* test for equal */
			eflag = -1;
			ptr = runtime("?eq", type, 0);
			break;
		case _NE:		/* test for not equal */
			eflag = -1;
			ptr = runtime("?ne", type, 0);
			break;
		case _LT:		/* test for less than */
			ptr = runtime("?lt", type, eflag = -1);
			break;
		case _LE:		/* test for less or equal to */
			ptr = runtime("?le", type, eflag = -1);
			break;
		case _GT:		/* test for greater than */
			ptr = runtime("?gt", type, eflag = -1);
			break;
		case _GE:		/* test for greater than or equal to */
			ptr = runtime("?ge", type, eflag = -1);
			break;
		case _ULT:		/* unsigned less than */
			ptr = runtime("?ult", type, eflag = -1);
			break;
		case _ULE:		/* unsigned less than or equal to */
			ptr = runtime("?ule", type, eflag = -1);
			break;
		case _UGT:		/* unsigned greater than */
			ptr = runtime("?ugt", type, eflag = -1);
			break;
		case _UGE:		/* unsigned greater than or equal to */
			ptr = runtime("?uge", type, eflag = -1);
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
 * Output a global symbol name
 */
out_global(symbol, segment)
	int symbol, segment;
{
	set_segment(segment);
	if(!(s_type[symbol] & STATIC)) {
		out_str(" PUBLIC ");
		out_symbol(symbol);
		out_chr('\n'); }
	out_symbol(symbol);
}

/*
 * Insure symbol is not a reserved word by preceeding it with '_'.
 */
out_symbol(symbol)
	int symbol;
{
	if(s_type[symbol] & STATIC) {
		out_chr('?');
		out_num(s_index[symbol]); }
	else
		out_chr('_');
	out_str(s_name[symbol]);
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
out_chr(c)
	char c;
{
	put_chr(c, -1);
}
