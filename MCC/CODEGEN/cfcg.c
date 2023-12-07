/*
 * DDS MICRO-C Code Generator for: C-FLEA
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

#ifndef LINE_SIZE
#include "compile.h"

extern char s_name[MAX_SYMBOL][SYMBOL_SIZE+1];
extern unsigned s_type[], s_index[], s_dindex[], dim_pool[],
 global_top, local_top, function_count, next_lab;
#endif

unsigned stack_frame, stack_level, stack_offset, global_width = 0,
	global_set = 0, icall_lab = 0;

char stack_index;

char symbolic = 0;

/*
 * Write an operand to instruction
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
			out_num(stack_offset);
			out_str(stack_index ? ",I" : ",S");
			break;
		case IN_TEMP:
			out_str("?temp");
			break;
		case INDIRECT:
			out_str("I");
			break;
		case ON_STACK:
			out_str("S+");
			stack_level -= 2;
			break;
		case ION_STACK:
			out_str("[S+]");
			stack_level -= 2;
			break;
		case ISTACK_TOP:
			out_str("[S]");
			break;
		default:		/* Unknown (error) */
			out_num(token);
			out_chr('?'); }

	out_nl();
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
	unsigned size;
{
	stack_level -= size;
}

/*
 * Define beginning of module
 */
def_module() { }	/* No operation required */

/*
 * End of module definition
 */
end_module()
{
	unsigned i;
	if(symbolic) for(i=0; i < global_top; ++i) {
		out_str("*#gbl ");
		dump_symbol(i); }
}

/*
 * Begin definition of a static global variable
 */
def_static(symbol, ssize)
	unsigned symbol;
{
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

	out_str(global_width ? "," : word ? " FDB " : " FCB ");
	switch(token) {
		case SYMBOL :		/* Symbol address */
			out_symbol(value);
			global_width += 12;
			break;
		case STRING :		/* literal pool entry */
			ptr = "?0+";
			global_width += 9;
			goto doinit;
		case LABEL :		/* instruction label */
			ptr = "?";
			global_width += 4;
			goto doinit;
		default :			/* constant value */
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
		out_inst("RMB 0");

	global_set = global_width = 0;
}

/*
 * Define a global variable
 */
def_global(symbol, size)
	unsigned symbol, size;
{
	/* Output the special comment used by SLINK */
	out_str("$DD:");
	out_symbol(symbol);
	out_sp();

	/* If not using SLINK, remove above and uncomment this
	out_symbol(symbol);
	out_str(" RMB "); */

	out_num(size);
	out_nl();
}

/*
 * Define an external variable
 * In this case, we will just generate a comment, which can
 * be used by the SLINK utility to key external references.
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
	if(symbolic) {
		out_str("*#fun ");
		out_symbol(symbol);
		out_sp();
		put_num(size, -1);
		out_str(" ?");
		out_num(function_count);
		out_nl(); }

	out_symbol(symbol);
	if(stack_frame = size) {
		while(size > 255) {
			out_inst("ALLOC 255 +");
			size -= 255; }
		if(size) {
			out_str(" ALLOC ");
			out_num(size);
			out_nl(); } }
	else
		out_inst("EQU *");
	stack_level = 0;
}

/*
 * Clean up the stack & end function definition
 */
end_func()
{
	unsigned s, s1;
	if(s = s1 = stack_frame + stack_level) {
		while(s > 255)
			s -= 255;
		if(s) {
			out_str(" FREE ");
			out_num(s);
			out_nl(); }
		while(s1 > 255) {
			out_inst("FREE 255 +");
			s1 -= 255; } }
	out_inst("RET");
	if(symbolic) {
		for(s = local_top; s < MAX_SYMBOL; ++s) {
			out_str("*#lcl ");
			dump_symbol(s); }
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
	out_inst("EQU *");
}

/*
 * Define literal pool
 */
def_literal(ptr, size)
	unsigned char *ptr;
	unsigned size;
{
	unsigned i;

	if(icall_lab) {
		out_chr('?');
		out_num(icall_lab);
		out_inst("IJMP"); }

	if(size) {
		i = 0;
		out_str("?0");
		while(i < size) {
			out_str((i % 16) ? "," : " FCB ");
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
			out_str(" CALL ");
			out_num(value);
			break;
		case SYMBOL :
			if(value < global_top) {
				out_str(" CALL ");
				out_symbol(value);
				break; }
			oper_prep(token, value, type, -1);
			out_str(" LD ");
			write_oper(token, value, type);
			oper_clean();
			goto do_icall;
		case ON_STACK :
		case ION_STACK :
			out_str(" LD ");
			if(clean) {
				out_num(clean++ * 2);
				out_str(",S"); }
			else {
				out_str("S+");
				stack_level -= 2; }
			out_nl();
		do_icall:
			if(!icall_lab)
				icall_lab = ++next_lab;
			out_str(" CALL ?");
			out_num(icall_lab); }

	out_nl();

	if(clean += clean) {	/* clean up stack following function call */
		out_str(" FREE ");
		out_num(clean);
		out_nl();
		stack_level -= clean; }
}

/*
 * Unconditional jump to label
 */
jump(label, ljmp)
	unsigned label;		/* destination label */
	char ljmp;			/* long jump required */
{
	out_str(ljmp ? " JMP ?" : " SJMP ?");
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
	char *ptr;

	if(cond)			/* jump if TRUE */
		ptr = ljmp ? " JNZ ?" : " SJNZ ?";
	else				/* jump if FALSE */
		ptr = ljmp ? " JZ ?" : " SJZ ?";

	out_str(ptr);
	out_num(label);
	out_nl();
}

/*
 * Perform a switch operation
 */
do_switch(label)
	unsigned label;			/* address of switch table */
{
	out_str(" LDI #?");
	out_num(label);
	do_asm("\n SWITCH");
}

/*
 * Load index register with a pointer value
 */
index_ptr(token, value, type)
	unsigned token, value, type;
{
	if(token == IN_ACC)
		out_inst("TAI");
	else {
		oper_prep(token, value, type, 0);
		out_str(" LDI ");
		write_oper(token, value, type);
		oper_clean(); }
}

/*
 * Load index register with the address of an assignable object
 */
index_adr(token, value, type)
	unsigned token, value, type;
{
	if(token == ION_STACK) {
		out_inst("LDI S+");
		stack_level -= 2; }
	else {
		oper_prep(token, value, type, 0);
		out_str(
			((value < global_top) || (type & (STATIC|EXTERNAL)))
			? " LDI #" : " LEAI ");
		write_oper(token, value, type);
		oper_clean(); }
}

/*
 * Expand 8 bit accumulator to 16 bits if necessary.
 */
expand(type)
	unsigned type;
{
}

/*
 * Do a simple register operation
 */
accop(oper, rtype)
	unsigned oper, rtype;
{
	char *ptr;

	switch(oper) {
		case _STACK:			/* stack accumulator */
			ptr = "PUSHA";
			stack_level += 2;
			break;
		case _ISTACK:			/* stack index register */
			ptr = "PUSHI";
			stack_level += 2;
			break;
		case _TO_TEMP:			/* copy accumulator to temp */
			ptr = "ST ?temp";
			break;
		case _FROM_INDEX:		/* copy index to accumulator */
			ptr = "TIA";
			break;
		case _COM:				/* complement accumulator */
			ptr = "COM";
			break;
		case _NEG:				/* negate accumulator */
			ptr = "NEG";
			break;
		case _NOT:				/* logical complement */
			ptr = "NOT";
			break;
		case _INC:				/* increment accumulator */
			ptr = isp16(rtype) ? "INC\n INC" : "INC";
			break;
		case _DEC:				/* decrement accumulator */
			ptr = isp16(rtype) ? "DEC\n DEC" : "DEC";
			break;
		case _IADD:				/* add acc to index register */
			ptr = "ADAI";
			break;
		default:				/* Unknown (error) */
			ptr = "?S?"; }

	out_inst(ptr);
}

/*
 * Perform an operation with the accumulator and
 * the specified value;
 */
accval(oper, rtype, token, value, type)
	unsigned oper, rtype, token, value, type;
{
	char *ptr, *ptr1, byte;

	ptr1 = 0;

/* determine of length of source & result */
	byte = (type & (BYTE|POINTER)) == BYTE;
	if((token == NUMBER) & (value < 256))
		byte = 1;

	switch(oper) {
		case _LOAD:				/* load accumulator */
			ptr = byte ? "LDB" : "LD";
			break;
		case _STORE:	/* store accumulator */
			ptr = byte ? "STB" : "ST";
			break;
		case _ADD:		/* addition */
			ptr = byte ? "ADDB" : "ADD";
			break;
		case _SUB:		/* subtract */
			ptr = byte ? "SUBB" : "SUB";
			break;
		case _MULT:		/* multiply */
			ptr = byte ? "MULB" : "MUL";
			break;
		case _MOD:		/* Modulus */
			ptr1 = "ALT";
		case _DIV:		/* divide */
			ptr = byte ? "DIVB" : "DIV";
			break;
		case _AND:		/* logical and */
			ptr = byte ? "ANDB" : "AND";
			break;
		case _OR:		/* logical or */
			ptr = byte ? "ORB" : "OR";
			break;
		case _XOR:		/* exclusive or */
			ptr = byte ? "XORB" : "XOR";
			break;
		case _SHL:		/* shift left */
			ptr = "SHL";
			break;
		case _SHR:		/* shift right */
			ptr = "SHR";
			break;
		case _EQ:		/* test for equal */
		doeq:
			ptr = byte ? "CMPB" : "CMP";
			break;
		case _NE:		/* test for not equal */
			ptr1 = "NOT";
			goto doeq;
		case _LT:		/* test for less than */
			ptr1 = "LT";
			goto doeq;
		case _LE:		/* test for less or equal to */
			ptr1 = "LE";
			goto doeq;
		case _GT:		/* test for greater than */
			ptr1 = "GT";
			goto doeq;
		case _GE:		/* test for greater than or equal to */
			ptr1 = "GE";
			goto doeq;
		case _ULT:		/* unsigned less than */
			ptr1 = "ULT";
			goto doeq;
		case _ULE:		/* unsigned less than or equal to */
			ptr1 = "ULE";
			goto doeq;
		case _UGT:		/* unsigned greater than */
			ptr1 = "UGT";
			goto doeq;
		case _UGE:		/* unsigned greater than or equal to */
			ptr1 = "UGE";
			goto doeq;
		default:		/* Unknown (error) */
			ptr = "?D?"; }

/* interpret the output string & insert the operands */
	oper_prep(token, value, type, -1);
	out_sp();
	out_str(ptr);
	out_sp();
	write_oper(token, value, type);
	if(ptr1)
		out_inst(ptr1);
	oper_clean();
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
 * Write a number to the output file
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
 * Write newline/space to the output file
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
 * Compute stack offset to variable & output any special case stuff
 */
oper_prep(token, value, type, flag)
	unsigned token, value, type;
	char flag;
{
	stack_index = 0;
	if((token != SYMBOL) || (value < global_top) || (type & (STATIC|EXTERNAL)))
		return;

	stack_offset = stack_level + s_index[value];

	if(type & ARGUMENT)
		stack_offset += stack_frame + 2;

	if(stack_offset < 256) {
		return; }

	if(flag) {
		out_inst("PUSHI");
		stack_offset += 2; }

	out_inst("LEAI 255,S");
	stack_offset -= 255;
	while(stack_offset > 255) {
		out_inst("LEAI 255,I");
		stack_offset -= 255; }

	stack_index = flag | 0xF0;
}

/*
 * Clean up after large stack access
 */
oper_clean()
{
	if(stack_index & 0xF0)
		out_inst("LDI S+");
}
