/*
 * DDS MICRO-C Code Generator for: 68HC12
 *
 *    The 68HC12 is part-way between the 68HC11 and the 6809 in instruction
 * set capability. In particular, it adds the stack relative addressing mode
 * which allows access to the stack much easier than on the 68HC11, and makes
 * for an efficent C platform.
 *
 *    This code generator demonstrates a fairly complete implementation
 * of MICRO-C on the 68HC12. Although runtime library routines are used
 * for some common functions, most of the generated code is "inline",
 * resulting in fast execution.
 *
 * 6809 Registers:
 *	D (A,B)	- Accumulator
 *	X		- Index register
 *	Y		- Indirect stack addressing, 2nd parm to runtime library calls
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
char call_buffer[50], zero_flag, last_byte = 0, y_stack = 0;

char symbolic = 0;		/* controls output of symbolic information */

/*
 * Generate a call to a runtime library function.
 */
char *runtime(string, type, order)
	char *string;	/* name of library routine */
	unsigned type;	/* type of access */
	char order;		/* order is important flag */
{
	char *ptr, *ptr1;

	y_stack = -1;
	if((type & (BYTE | POINTER)) == BYTE)		/* byte access */
		ptr = (type & UNSIGNED) ? "TFR D,Y\n LDAB |\n CLRA\n"
			: "TFR D,Y\n LDAB |\n SEX B,D\n";
	else
		ptr = (order) ? "TFR D,Y\n LDD |\n" : "LDY |\n";

	for(ptr1 = call_buffer; *ptr; ++ptr1)
		*ptr1 = *ptr++;
	*ptr1++ = ' ';
	*ptr1++ = 'J';
	*ptr1++ = 'S';
	*ptr1++ = 'R';
	*ptr1++ = ' ';
	while(*string)
		*ptr1++ = *string++;
	*ptr1 = 0;
	return call_buffer;
}

/*
 * Write an operand value
 * flag: (0=16 bit, 1=first 8 bits, 2 = second 8 bits)
 */
write_oper(token, value, type, flag)
	unsigned token, value, type, flag;
{
	int flag1;

	flag1 = (flag == 2);

	switch(token) {
		case NUMBER:
			if(flag == 1)
				value >>= 8;
			else if(flag1)
				value &= 0xff;
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
				if(flag1)
					out_str("+1");
				break; }
			if(type & ARGUMENT) {
				if((type & (BYTE|POINTER|ARRAY)) == BYTE)	/* 8of16 byte 2 */
					flag1 = 1;
				flag1 += stack_frame + 2; }
			out_num(stack_level + s_index[value] + flag1);
			out_str(",SP");
			break;
		case IN_TEMP:
			out_str(flag1 ? "?temp+1" : "?temp");
			break;
		case INDIRECT:
			out_str(flag1 ? "1,X" : ",X");
			break;
		case ON_STACK:
			if(flag) {
				out_str("1,SP+");
				stack_level -= 1; }
			else {
				out_str("2,SP+");
				stack_level -= 2; }
			break;
		case ION_STACK:
			if(!y_stack) {
				out_str(",Y");
				break; }
		case ISTACK_TOP:
			out_str("[,SP]");
			break;
		default:		/* Unknown (error) */
			out_num(token);
			out_chr('?'); }
}

/*
 * Prepare for special addressing modes
 */
oper_prep(token)
	unsigned token;
{
	if((token == ION_STACK) && !y_stack) {
		out_inst("PULY");
		stack_level -= 2; }
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

	out_str("$RL:65535\n");

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
		out_str(" LEAS -");
		out_num(size);
		do_asm(",SP"); }
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

	if(stack_frame || stack_level) {
		out_str(" LEAS ");
		out_num(stack_frame + stack_level);
		do_asm(",SP"); }
	out_inst("RTS");

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
			out_num(value);
			break;
		case SYMBOL :
			out_str(" JSR ");
			write_oper(token, value, type, 0);
			break;
		case ON_STACK :
		case ION_STACK :
			if(clean) {
				out_str(" JSR ");
				out_chr('[');
				out_num(clean++ *2);
				out_str(",SP]"); }
			else {
				out_inst("PULY\n JSR ,Y");
				stack_level -= 2; } }
	out_nl();

	if(clean += clean) {	/* clean up stack following function call */
		out_str(" LEAS ");
		out_num(clean);
		do_asm(",SP");
		stack_level -= clean; }

	last_byte = (type & (BYTE | POINTER)) == BYTE;
	zero_flag = -1;
}

/*
 * Unconditional jump to label
 */
jump(label, ljmp)
	unsigned label;		/* destination label */
	char ljmp;			/* long jump required */
{
	out_str(ljmp ? " JMP ?" : " BRA ?");
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

	if(zero_flag) {		/* set up 'Z' flag if necessary */
		out_inst(last_byte ? "TSTB" : "CPD #0");
		zero_flag = 0; }

	if(cond)			/* jump if TRUE */
		ptr = ljmp ? " LBNE ?" : " BNE ?";
	else				/* jump if FALSE */
		ptr = ljmp ? " LBEQ ?" : " BEQ ?";

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
	out_str(" LDX #?");
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
		out_inst("TFR D,X");
	else {
		oper_prep(token);
		out_str(" LDX ");
		write_oper(token, value, type, 0);
		out_nl(); }
}

/*
 * Load index register with the address of an assignable object
 */
index_adr(token, value, type)
	unsigned token, value, type;
{
	if(token == ION_STACK) {
		out_inst("PULX");
		stack_level -= 2; }
	else {
		oper_prep(token);
		out_str(((value < global_top) || (type & (STATIC|EXTERNAL))) ?
			" LDX #" : " LEAX ");
		write_oper(token, value, type, 0);
		out_nl(); }
}

/*
 * Expand 8 bit accumulator to 16 bits if necessary.
 */
expand(type)
	unsigned type;
{
	if(last_byte) {
		out_inst((type & UNSIGNED) ? zero_flag = -1, "CLRA" : "SEX B,D");
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

	if((rtype & (BYTE | POINTER)) == BYTE)
		byte = -1;

	switch(oper) {
		case _STACK:			/* stack accumulator */
			eflag = -1;
			ptr = "PSHD";
			stack_level += 2;
			zflag = 0x55;
			break;
		case _ISTACK:			/* stack index register */
			ptr = "PSHX";
			stack_level += 2;
			byte = last_byte;
			zflag = 0x55;
			break;
		case _TO_TEMP:			/* copy accumulator to temp */
			ptr = byte ? "STAB ?temp" : "STD ?temp" ;
			break;
		case _FROM_INDEX:		/* copy index to accumulator */
			ptr = "TFR X,D";
			last_byte = byte = 0;
			zflag = -1;
			break;
		case _COM:				/* complement accumulator */
			if(byte)
				ptr = "COMB";
			else {
				ptr = "COMA\n COMB";
				zflag = -1; }
			break;
		case _NEG:				/* negate accumulator */
			ptr = byte ? "NEGB" : "COMA\n COMB\n ADDD #1";
			break;
		case _NOT:				/* logical complement */
			eflag = -1;
			ptr = "JSR ?not";
			break;
		case _INC:				/* increment accumulator */
			if(isp16(rtype)) {
				ptr = "ADDD #2";
				eflag = -1; }
			else
				ptr = byte ? "INCB" : "ADDD #1";
			break;
		case _DEC:				/* decrement accumulator */
			if(isp16(rtype)) {
				ptr = "SUBD #2";
				eflag = -1; }
			else
				ptr = byte ? "DECB" : "SUBD #1";
			break;
		case _IADD:				/* add acc to index register */
			eflag = -1;
			ptr = "LEAX D,X";
			zflag = 0x55;
			break;
		default:				/* Unknown (error) */
			ptr = "?S?"; }

	if(eflag || !byte)
		expand(rtype);
	else
		last_byte = byte;

	if(zflag != 0x55)
		zero_flag = zflag;

	out_inst(ptr);
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
			ptr = (rbyte = byte) ? "LDAB |" : "LDD |";
			last_byte = 0;		/* insure no pre - sign extend */
			break;
		case _STORE:	/* store accumulator */
			ptr = byte ? "STAB |" : "STD |";
			break;
		case _ADD:		/* addition */
			if(byte) {
				ptr = "ADDB |";
				if(!rbyte)
					ptr1 = "ADCA #0"; }
			else
				ptr = "ADDD |";
			break;
		case _SUB:		/* subtract */
			if(byte) {
				ptr = "SUBB |";
				if(!rbyte)
					ptr1 = "SBCA #0"; }
			else
				ptr = "SUBD |";
			break;
		case _MULT:		/* multiply */
			zflag = eflag = -1;
			if((token == NUMBER) && (value == 2)) {	/* efficent *2 */
				ptr = "LSLD";
				break; }
			ptr = runtime("?mul", type, 0);
			break;
		case _DIV:		/* divide */
			ptr = runtime(((rtype & UNSIGNED) ? "?div" : "?sdiv"),
				type, zflag = eflag = -1);
			break;
		case _MOD:		/* remainder */
			ptr = runtime(((rtype & UNSIGNED) ? "?mod" : "?smod"),
				type, zflag = eflag = -1);
			break;
		case _AND:		/* logical and */
			if(byte) {
				ptr = "ANDB |";
				if(!rbyte)
					ptr1 = "CLRA"; }
			else {
				ptr = "ANDA [\n ANDB ]";
				zflag = -1; }
			break;
		case _OR:		/* logical or */
			if(byte)
				ptr = "ORAB |";
			else {
				ptr = "ORAA [\n ORAB ]";
				zflag = -1; }
			break;
		case _XOR:		/* exclusive or */
			if(byte)
				ptr = "EORB |";
			else {
				ptr = "EORA [\n EORB ]";
				zflag = -1; }
			break;
		case _SHL:		/* shift left */
			ptr = runtime("?shl", type, zflag = eflag = -1);
			break;
		case _SHR:		/* shift right */
			ptr = runtime("?shr", type, zflag = eflag = -1);
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

/* interpret the output string & insert the operands */
	oper_prep(token);
	out_sp();
	while(*ptr) {
		if(*ptr == '|')
			write_oper(token, value, type, 0);
		else if(*ptr == '[')
			write_oper(token, value, type, 1);
		else if(*ptr == ']')
			write_oper(token, value, type, 2);
		else
			out_chr(*ptr);
		++ptr; }
	out_nl();
	if((token == ION_STACK) && y_stack) {
		out_inst("PULY");
		stack_level -= 2; }
	y_stack = 0;

	zero_flag = zflag;

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
