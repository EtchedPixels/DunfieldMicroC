/*
 * DDS MICRO-C Code Generator for: 8080/8085/Z80 Family
 *
 *    The 8080 series of processors does not directly support many of the
 * basic operations required in a MICRO-C implementation such as stack
 * relative addressing and 16 bit arithmetic operations. In addition, its
 * instruction set is not very symmetrical with regard to which operations
 * may be performed on which registers, and requires that all arithmetic
 * operands be loaded into registers. This makes automatic generation of
 * efficent code a very difficult task.
 *
 *    This 8080 code generator demonstrates the simplest way of porting
 * MICRO-C to a particularly weird processor. It makes extensive use of
 * "runtime library" routines to perform the common functions. Although
 * this is not the most efficent approach, it should still execute much
 * faster than interpreted languages such as "BASIC".
 *
 * 8080 Registers:
 *	H,L	- Accumulator
 *	D,E	- Index register
 *	B,C	- Second parameter to runtime library calls
 *	A	- Misc. 8 bit temporary register & accumulator
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

#ifndef LINE_SIZE
#include "compile.h"

extern char s_name[MAX_SYMBOL][SYMBOL_SIZE+1];
extern unsigned s_type[], s_index[], s_dindex[], dim_pool[], global_top,
	local_top, function_count;
#endif

unsigned stack_frame, stack_level, global_width = 0, global_set = 0;

char symbolic = 0;			/* controls output of symbolic information */

/*
 * Execute a runtime library call
 */
runtime(ptr)
	char *ptr;
{
	out_str(" CALL ");
	out_str(ptr);
	out_nl();
}

/*
 * Get operand value into (H,L)
 */
get_oper(token, value, type)
	unsigned token, value, type;
{
	unsigned flag;
	char byte;

	byte = (type & (POINTER | BYTE)) == BYTE;

	switch(token) {
		case NUMBER:
			out_str(" LXI H,");
			out_num(value);
			out_nl();
			break;
		case STRING:
			out_str(" LXI H,?0+");
			out_num(value);
			out_nl();
			break;
		case SYMBOL:
			if((value < global_top) || (type & (STATIC|EXTERNAL))) {
				if(byte) {
					out_str(" LDA ");
					out_symbol(value);
					out_str("\n MOV L,A\n");
					sign_extend(type); }
				else {
					out_str(" LHLD ");
					out_symbol(value);
					out_nl(); }
				break; }
			flag = 2;
			if(type & ARGUMENT)
				flag += stack_frame + 2;
			out_str(" LXI H,");
			out_num(stack_level + s_index[value] + flag);
			out_nl();
			if(byte) {
				runtime("?gstkb");
				sign_extend(type); }
			else
				runtime("?gstkw");
			break;
		case IN_TEMP:
			out_inst("LHLD ?temp");
			break;
		case INDIRECT:
			if(byte) {
				out_inst("LDAX D\n MOV L,A");
				sign_extend(type); }
			else
				runtime("?gind_d");
			break;
		case ON_STACK:
			out_inst("POP H");
			stack_level -= 2;
			break;
		case ION_STACK:
			out_inst("POP H");
			stack_level -= 2;
			if(byte) {
				out_inst("MOV L,M");
				sign_extend(type); }
			else
				runtime("?gind_h");
			break;
		case ISTACK_TOP:
			out_inst("POP H\n PUSH H");
			if(byte) {
				out_inst("MOV L,M");
				sign_extend(type); }
			else
				runtime("?gind_h");
			break;
		default:		/* Unknown (error) */
			out_num(token);
			out_chr('?'); }
}

/*
 * Expand 8 bit accumulator to 16 bits
 */
sign_extend(type)
	unsigned type;
{
	out_inst((type & UNSIGNED) ? "MVI H,0" : "CALL ?sign");
}

/*
 * Clear a number of bytes from the processor stack
 */
clear_stack(size)
	unsigned size;
{
	if(size < 14) {
		while(size > 1) {
			out_inst("POP B");
			size -= 2; }
		if(size)
			out_inst("INX SP"); }
	else {
		out_str(" XCHG\n LXI H,");
		out_num(size);
		out_str("\n DAD SP\n SPHL\n XCHG\n"); }
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
 * Begin definition of a static variable
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
	out_str(" DS "); */

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
		if(size < 14) {
			while(size > 1) {
				out_inst("PUSH B");
				size -= 2; }
			if(size)
				out_inst("DCX SP"); }
		else {
			if(s_type[symbol] & REGISTER)	/* preserve arg count */
				out_inst("XCHG");
			out_str(" LXI H,-");
			out_num(size);
			out_str("\n DAD SP\n SPHL\n");
			if(s_type[symbol] & REGISTER)
				out_inst("XCHG"); } }
	else
		out_inst("EQU *");
	stack_level = 0;
}

/*
 * Clean up the stack & end function definition
 */
end_func()
{
	unsigned i;

	clear_stack(stack_frame + stack_level);
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
		case NUMBER :		/* Fixed address */
			out_str(" CALL ");
			out_num(value);
			break;
		case SYMBOL :		/* Global symbol */
			out_str(" CALL ");
			out_symbol(value);
			break;
		case ON_STACK :		/* Computer address */
		case ION_STACK :	/* Indirect address */
			/* Note that for stacked operands, they were placed */
			/* on the stack BEFORE any function arguments !!!!! */
			if(clean) {
				out_str(" LXI H,");
				out_num((clean++ * 2) + 2);
				out_str("\n CALL ?gstkw"); }
			else {
				out_str(" POP H");
				stack_level -= 2; }
			out_str("\n CALL ?idcall"); }
	out_nl();

	clear_stack(clean += clean);
	stack_level -= clean;
}

/*
 * Unconditional jump to label
 */
jump(label, ljmp)
	unsigned label;
{
	out_str(" JMP ?");
	out_num(label);
	out_nl();
}

/*
 * Conditional jump to label
 */
jump_if(cond, label, ljmp)
	char cond;
	unsigned label;
{
	out_inst("MOV A,H\n ORA L");
	out_str(cond ? " JNZ ?" : " JZ ?");
	out_num(label);
	out_nl();
}

/*
 * Perform a switch operation
 */
do_switch(label)
	unsigned label;
{
	out_str(" LXI D,?");
	out_num(label);
	do_asm("\n JMP ?switch");
}

/*
 * Load index register with a pointer value
 */
index_ptr(token, value, type)
	unsigned token, value, type;
{
	out_inst("XCHG");
	if(token == INDIRECT) {
		runtime("?gind_h");
		out_inst("XCHG"); }
	else if(token != IN_ACC) {
		get_oper(token, value, type & ~BYTE);
		out_inst("XCHG"); }
}

/*
 * Load index register with the address of an assignable object
 */
index_adr(token, value, type)
	unsigned token, value, type;
{
	unsigned flag;

	if(token == ION_STACK) {
		out_inst("POP D");
		stack_level -= 2;
		return; }

	out_str(" LXI D,");
	if((value < global_top) || (type & (STATIC|EXTERNAL))) {
		out_symbol(value);
		out_nl(); }
	else {
		flag = 0;
		if(type & ARGUMENT)
			flag += stack_frame + 2;
		out_num(stack_level + s_index[value] + flag);
		out_nl();
			out_inst("XCHG\n DAD SP\n XCHG"); }
}

/*
 * Dummy "expand", because we always have 16 bits
 */
expand() { }

/*
 * Do a simple register operation
 */
accop(oper, rtype)
	unsigned oper, rtype;
{
	switch(oper) {
		case _STACK:			/* stack accumulator */
			out_inst("PUSH H");
			stack_level += 2;
			break;
		case _ISTACK:			/* stack index register */
			out_inst("PUSH D");
			stack_level += 2;
			break;
		case _TO_TEMP:			/* copy accumulator to temp */
			out_inst("SHLD ?temp");
			break;
		case _FROM_INDEX:		/* copy index to accumulator */
			out_inst("MOV H,D\n MOV L,E");
			break;
		case _COM:				/* complement accumulator */
			runtime("?com");
			break;
		case _NEG:				/* negate accumulator */
			runtime("?neg");
			break;
		case _NOT:				/* logical complement */
			runtime("?not");
			break;
		case _INC:				/* increment accumulator */
			out_inst("INX H");
			if(isp16(rtype))
				out_inst("INX H");
			break;
		case _DEC:				/* decrement accumulator */
			out_inst("DCX H");
			if(isp16(rtype))
				out_inst("DCX H");
			break;
		case _IADD:				/* add acc to index register */
			out_inst("DAD D\n XCHG");
			break;
		default:				/* Unknown (error) */
			out_inst("?S?"); }
}

/*
 * Perform an operation with the accumulator and
 * the specified value;
 */
accval(oper, rtype, token, value, type)
	unsigned oper, rtype, token, value, type;
{
	unsigned flag;
	char byte;

	byte = (type & (POINTER | BYTE)) == BYTE;

	if(oper == _STORE) {			/* special case */
		switch(token) {
			case SYMBOL:
				if((value < global_top) || (type & (STATIC|EXTERNAL))) {
					out_str(byte ? " MOV A,L\n STA " : " SHLD ");
					out_symbol(value);
					out_nl(); }
				else {				/* Store to local */
					out_str(" LXI B,");
					flag = 2;
					if(type & ARGUMENT)
						flag += stack_frame + 2;
					out_num(stack_level + s_index[value] + flag);
					out_nl();
					runtime(byte ? "?pstkb" : "?pstkw"); }
				break;
			case INDIRECT:
				if(byte)
					out_inst("MOV A,L\n STAX D");
				else
					runtime("?pind_d");
				break;
			case ION_STACK :
				out_inst("POP B");
				stack_level -= 2;
				if(byte)
					out_inst("MOV A,L\n STAX B");
				else
					runtime("?pind_b");
				break;
			case ISTACK_TOP:
				out_inst("POP B\n PUSH B");
				if(byte)
					out_inst("MOV A,L\n STAX B");
				else
					runtime("?pind_b"); }

		return; }

	if(oper == _LOAD) {				/* special case */
		get_oper(token, value, type);
		return; }

	out_inst("MOV B,H\n MOV C,L");
	get_oper(token, value, type);
	switch(oper) {
		case _ADD:				/* Addition */
			out_inst("DAD B");
			break;
		case _SUB:		/* subtract */
			runtime("?sub");
			break;
		case _MULT:		/* multiply */
			runtime("?mul");
			break;
		case _DIV:		/* divide */
			runtime((rtype & UNSIGNED) ? "?div" : "?sdiv");
			break;
		case _MOD:		/* remainder */
			runtime((rtype & UNSIGNED) ? "?mod" : "?smod");
			break;
		case _AND:		/* logical and */
			runtime("?and");
			break;
		case _OR:		/* logical or */
			runtime("?or");
			break;
		case _XOR:		/* exclusive or */
			runtime("?xor");
			break;
		case _SHL:		/* shift left */
			runtime("?shl");
			break;
		case _SHR:		/* shift right */
			runtime("?shr");
			break;
		case _EQ:		/* test for equal */
			runtime("?eq");
			break;
		case _NE:		/* test for not equal */
			runtime("?ne");
			break;
		case _LT:		/* test for less than */
			runtime("?lt");
			break;
		case _LE:		/* test for less or equal to */
			runtime("?le");
			break;
		case _GT:		/* test for greater than */
			runtime("?gt");
			break;
		case _GE:		/* test for greater than or equal to */
			runtime("?ge");
			break;
		case _ULT:		/* unsigned less than */
			runtime("?ult");
			break;
		case _ULE:		/* unsigned less than or equal to */
			runtime("?ule");
			break;
		case _UGT:		/* unsigned greater than */
			runtime("?ugt");
			break;
		case _UGE:		/* unsigned greater than or equal to */
			runtime("?uge");
			break;
		default:		/* Unknown (error) */
			out_inst("?D?"); }

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
