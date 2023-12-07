/*
 * DDS MICRO-C Code Generator for: 68HC11
 *
 *    The 68HC11 is a powerful single-chip micro-controller, which
 * would make a good MICRO-C platform, except for its lack of stack
 * relative addressing. Simulating such addressing not only requires
 * extra code, but it ties up one of the two available 16 bit index
 * registers, resulting in more code generation difficulties.
 *
 *    Since the 68HC11 only supports 8 bit offsets for indexed memory
 * accesses, access to stack variables which are greater than 256 bytes
 * away from the stack pointer requires more (less efficent) code. For
 * maximum efficency, declare small and frequently used local variables
 * before any larger or less frequently used variables in a function.
 *
 *    Arrays <= 256 bytes in size will automatically be indexed using an
 * efficent 8 bit add (ABX). All other indexing is performed using less
 * efficent 16 bit arithmetic. This code generator extends the "register"
 * keyword to apply to pointers and arrays. If you know that you will not
 * be indexing through a pointer with offsets greater that 256 bytes, you
 * can declare it "register" to force the more efficent 8-bit arithmetic
 * to be used at all times.
 *
 *    Do to the lack of spare registers, the second parameter to run time
 * library functions is passed in a temporary ram location called '?parm'.
 * Place this and the '?temp' location in the direct (0) page of memory,
 * for maximum efficency.
 *
 *    To differentiate accesses to INTERNAL and EXTERNAL memory, the code
 * generator makes use of the "register" attribute. Any unitialized global
 * variables which are declared as "register" are allocated in INTERNAL
 * memory. To preserve the notion that "register" means "efficent", internal
 * memory should be placed within the first 256 bytes of memory address
 * space ($0000-$00FF).
 *
 *    Inline assembly language code should preserve the contents of the
 * Y register, as it is used for stack relative addressing.
 *
 * 68HC11 Registers:
 *	A,B	- Accumulator
 *	X	- Index register
 *	Y	- Temporary register & Stack addressing
 *
 * SLINK segments:
 *	S0	- Code
 *	S1	- "register" variables
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

/* Runtime library usage bits */
#define	RL_SWITCH	0x0001	/* SWITCH statement */
#define	RL_MUL		0x0002	/* Multiply */
#define	RL_UDIV		0x0004	/* Unsigned Division */
#define	RL_SDIV		0x0008	/* Signed division */
#define	RL_MOD		0x0010	/* Modulus */
#define	RL_SHL		0x0020	/* Shift left */
#define	RL_SHR		0x0040	/* Shift right */
#define	RL_SIGN		0x0080	/* Sign extension */
#define	RL_SCMP		0x0100	/* Signed compares */
#define	RL_UCMP		0x0200	/* Unsigned compares */
#define	RL_NOT		0x0400	/* NOT */
#define	RL_NEG		0x0800	/* Negate */
#define	RL_COM		0x1000	/* Compliment */
#ifndef LINE_SIZE

#include "compile.h"

extern char s_name[MAX_SYMBOL][SYMBOL_SIZE+1];
extern unsigned s_type[], s_index[], s_dindex[], dim_pool[], global_top,
	local_top, function_count;
extern size_of_var();
#endif

unsigned stack_frame, stack_level, y_stack_level, current_seg = 0,
	global_width = 0, global_set = 0, rl_flags = 0;
char call_buffer[50], zero_flag, last_byte = 0, y_stack_flag = 0,
	index16;

char symbolic = 0;		/* controls output of symbolic information */

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
 * Generate a call to a runtime library function.
 */
char *runtime(string, type)
	char *string;	/* name of library routine */
	unsigned type;	/* type of access */
{
	char *ptr, *ptr1;

	if((type & (BYTE | POINTER)) == BYTE)		/* byte access */
		ptr = (type & UNSIGNED) ? "STD ?parm\n LDAB |\n CLRA\n"
			: (rl_flags |= RL_SIGN, "STD ?parm\n LDAB |\n JSR ?sign\n");
	else
		ptr = "STD ?parm\n LDD |\n";

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
 * Output preceeding instructions for operand access
 */
oper_prep(token, value)
	unsigned token, value;
{
	unsigned i;

	switch(token) {
		case SYMBOL:
			if((value < global_top) || (s_type[value] & (STATIC|EXTERNAL)))
				break;
			if(!y_stack_flag) {
				out_inst("TSY");
				y_stack_level = stack_level;
				y_stack_flag = -1; }
			i = (s_type[value] & ARGUMENT) ? stack_frame+2 : 0;
			if(((i += s_index[value])+y_stack_level) > 254) {
				out_str(" XGDY\n ADDD #");
				out_num(i);
				out_str("\n XGDY\n");
				y_stack_level -= i;
				y_stack_flag = 0; }
			break;
		case ON_STACK:
			if((y_stack_level != stack_level) || !y_stack_flag) {
				out_inst("TSY");
				y_stack_level = stack_level;
				y_stack_flag = -1; }
			break;
		case ION_STACK:
			out_inst("PULY");
			y_stack_flag = 0;
			stack_level -= 2;
			break;
		case ISTACK_TOP:
			out_inst("PULY\n PSHY");
			y_stack_flag = 0; }
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
				if((type & (BYTE|POINTER|ARRAY)) == BYTE)	/* char byte 2 */
					flag1 = 1;
				flag1 += stack_frame + 2; }
			out_num(y_stack_level + s_index[value] + flag1);
			out_str(",Y");
			break;
		case IN_TEMP:
			out_str(flag1 ? "?temp+1" : "?temp");
			break;
		case INDIRECT:
			out_str(flag1 ? "1,X" : ",X");
			break;
		case ON_STACK:
			out_str(flag1 ? "1,Y" : ",Y");
			if(flag != 1) {
				out_str("\n INS\n INS");
				stack_level -= 2; }
			break;
		case ION_STACK:		/* Indirect through top of stack (destructive) */
		case ISTACK_TOP:	/* Indirect through top of stack (non-dest) */
			out_str(",Y");
			break;
		default:		/* Unknown (error) */
			out_num(token);
			out_chr('?'); }
}

/*
 * Expand 8 bit accumulator to 16 bits
 */
expand(type)
	unsigned type;
{
	if(last_byte) {
		out_inst((type & UNSIGNED) ? zero_flag = -1, "CLRA" :
			(rl_flags |= RL_SIGN, "JSR ?sign"));
		last_byte = 0; }
}

/*
 * Clear a number of bytes from the processor stack
 */
clear_stack(size)
	unsigned size;
{
	if(size < 13) {
		while(size > 1) {
			out_inst("PULX");
			size -= 2; }
		if(size)
			out_inst("INS"); }
	else {
		out_str(" TSX\n XGDX\n ADDD #");
		out_num(size);
		out_str("\n XGDX\n TXS\n"); }
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

	out_str("$RL:");
	put_num(rl_flags, -1);
	out_nl();

	if(symbolic) for(i=0; i < global_top; ++i) {
		set_seg(0);
		out_str("*#gbl ");
		dump_symbol(i); }
}

/*
 * Begin definition of a static variable
 */
def_static(symbol, ssize)
	unsigned symbol;
{
	set_seg(0);
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
		out_inst("RMB 0");

	global_set = global_width = 0;
}

/*
 * Define a global variable
 */
def_global(symbol, size)
	unsigned symbol, size;
{
	if(s_type[symbol] & REGISTER) {		/* Internal variables */
		set_seg(1);
		out_symbol(symbol);
		out_str(" RMB "); }
	else {		/* Output the special comment used by SLINK */
		out_str("$DD:");
		out_symbol(symbol);
		out_sp(); }

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
	if(stack_frame = size) {
		if(size < 13) {
			while(size > 1) {
				out_inst("PSHX");
				size -= 2; }
			if(size)
				out_inst("DES"); }
		else {
			out_str(" TSX\n XGDX\n SUBD #");
			out_num(size);
			out_str("\n XGDX\n TXS\n"); } }
	else
		out_inst("EQU *");
	stack_level = y_stack_flag = 0;
}

/*
 * Clean up the stack & end function definition
 */
end_func()
{
	unsigned i;

	clear_stack(stack_frame + stack_level);
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
	y_stack_flag = 0;
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
		case NUMBER :		/* Call to fixed address */
			out_str(" JSR ");
			out_num(value);
			break;
		case SYMBOL :		/* Call to normal symbol */
			oper_prep(token, value);
			out_str(" JSR ");
			write_oper(token, value, type, 0);
			break;
		case ON_STACK :		/* Call to computed address */
		case ION_STACK :	/* Call to indirect address */
			/* Note that for stacked operands, they were placed */
			/* on the stack BEFORE any function arguments !!!!! */
			if(clean) {
				out_str(" TSX\n LDX ");
				out_num(clean++ * 2);
				out_str(",X\n"); }
			else {
				out_inst("PULX");
				stack_level -= 2; }
			out_str(" JSR ,X"); }
	out_nl();

	clear_stack(clean += clean);
	stack_level -= clean;
	last_byte = (type & (BYTE|POINTER)) == BYTE;
	zero_flag = -1;
	y_stack_flag = 0;
}

/*
 * Unconditional jump to label
 */
jump(label, ljmp)
	unsigned label;
	char ljmp;
{
	out_str(ljmp ? " JMP ?" : " BRA ?");
	out_num(label);
	out_nl();
}

/*
 * Conditional jump to label
 */
jump_if(cond, label, ljmp)
	char cond, ljmp;
	unsigned label;
{
	if(zero_flag) {
		out_inst(last_byte ? "TSTB" : "CPD #0");
		zero_flag = 0; }

	if(ljmp)
		out_str(cond ? " BEQ *+5\n JMP ?" : " BNE *+5\n JMP ?");
	else
		out_str(cond ? " BNE ?" : " BEQ ?");
	out_num(label);
	out_nl();
}

/*
 * Perform a switch operation
 */
do_switch(label)
	unsigned label;
{
	out_str(" LDX #?");
	out_num(label);
	do_asm("\n JMP ?switch");
	rl_flags |= RL_SWITCH;
	y_stack_flag = 0;
}

/*
 * Load index register with a pointer value
 */
index_ptr(token, value, type)
	unsigned token, value, type;
{
	if(token == ON_STACK) {
		out_inst("PULX");
		stack_level -= 2; }
	else if(token == IN_ACC)
		out_inst("XGDX");
	else {
		oper_prep(token, value);
		out_str(" LDX ");
		write_oper(token, value, type, 0);
		out_nl(); }
	index16 = (type & REGISTER) ? 0 : -1;
}

/*
 * Load index register with the address of an assignable variable
 */
index_adr(token, value, type)
	unsigned token, value, type;
{
	unsigned flag, t;

	if(token == ION_STACK) {
		out_inst("PULX");
		stack_level -= 2;
		index16 = (type & REGISTER) ? 0 : -1;
		return; }

	if((value < global_top) || (type & (STATIC|EXTERNAL))) {
		out_str(" LDX #");
		out_symbol(value);
		out_nl(); }
	else {
		out_str(" TSX\n XGDX\n ADDD #");
		flag = 0;
		if(type & ARGUMENT) {
			flag = stack_frame + 2;
			if((type & (BYTE|POINTER|ARRAY)) == BYTE)
				++flag; }
		out_num(stack_level + s_index[value] + flag);
		out_str("\n XGDX\n"); }

	index16 =	(((t = s_type[value]) & REGISTER) ||
				((t & ARRAY) && (size_of_var(value) <= 256)))
			? 0 : -1;
}

/*
 * Do a simple register operation
 */
accop(oper, rtype)
	unsigned oper, rtype;
{
	char *ptr, byte, eflag;

	eflag = byte = 0;

	if((rtype & (BYTE | POINTER)) == BYTE)
		byte = -1;

	switch(oper) {
		case _STACK:			/* stack accumulator */
			eflag = -1;
			ptr = "PSHB\n PSHA";
			stack_level += 2;
			break;
		case _ISTACK:			/* stack index register */
			ptr = "PSHX";
			stack_level += 2;
			byte = last_byte;
			break;
		case _TO_TEMP:			/* copy accumulator to temp */
			ptr = byte ? "STAB ?temp" : "STD ?temp" ;
			zero_flag = 0;
			break;
		case _FROM_INDEX:		/* copy index to accumulator */
			ptr = "XGDX";
			last_byte = byte = 0;
			zero_flag = -1;
			break;
		case _COM:				/* complement accumulator */
			if(byte) {
				ptr = "COMB";
				zero_flag = 0; }
			else {
				ptr = "COMA\n COMB";
				zero_flag = -1; }
			break;
		case _NEG:				/* negate accumulator */
			ptr = byte ? "NEGB" : "COMA\n COMB\n ADDD #1";
			zero_flag = 0;
			break;
		case _NOT:				/* logical complement */
			eflag = -1;
			ptr = "JSR ?not";
			rl_flags |= RL_NOT;
			zero_flag = 0;
			break;
		case _INC:				/* increment accumulator */
			if(isp16(rtype)) {
				ptr = "ADDD #2";
				eflag = -1; }
			else
				ptr = byte ? "INCB" : "ADDD #1";
			zero_flag = 0;
			break;
		case _DEC:				/* decrement accumulator */
			if(isp16(rtype)) {
				ptr = "SUBD #2";
				eflag = -1; }
			else
				ptr = byte ? "DECB" : "SUBD #1";
			zero_flag = 0;
			break;
		case _IADD:				/* add acc to index register */
			eflag = -1;
			ptr = index16 ? "PSHX\n TSX\n ADDD 0,X\n PULX\n XGDX"
				: "ABX";
			zero_flag = -1;
			break;
		default:				/* Unknown (error) */
			ptr = "?S?"; }

	if(eflag || !byte)
		expand(rtype);
	else
		last_byte = byte;

	out_inst(ptr);
}

/*
 * Perform an operation with the accumulator and
 * the specified value;
 */
accval(oper, rtype, token, value, type)
	unsigned oper, rtype, token, value, type;
{
	char *ptr, *ptr1, byte, rbyte, eflag;

	zero_flag = byte = rbyte = eflag = 0;
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
			if(token == ON_STACK) {
				out_inst("PULA\n PULB");
				stack_level -= 2;
				zero_flag = -1;
				return; }
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
			eflag = -1;
			if((token == NUMBER) && (value == 2)) {	/* efficent *2 */
				ptr = "LSLD";
				break; }
			ptr = runtime("?mul", type);
			rl_flags |= RL_MUL;
			zero_flag = -1;
			break;
		case _DIV:		/* divide */
			zero_flag = eflag = -1;
			if(rtype & UNSIGNED) {
				ptr = runtime("?div", type);
				rl_flags |= RL_UDIV; }
			else {
				ptr = runtime("?sdiv", type);
				rl_flags |= RL_SDIV; }
			break;
		case _MOD:		/* remainder */
			zero_flag = eflag = -1;
			ptr = runtime((rtype & UNSIGNED) ? "?mod" : "?smod", type);
			rl_flags |= RL_MOD;
			break;
		case _AND:		/* logical and */
			if(byte) {
				ptr = "ANDB |";
				if(!rbyte)
					ptr1 = "CLRA"; }
			else {
				ptr = "ANDA [\n ANDB ]";
				zero_flag = -1; }
			break;
		case _OR:		/* logical or */
			if(byte)
				ptr = "ORAB |";
			else {
				ptr = "ORAA [\n ORAB ]";
				zero_flag = -1; }
			break;
		case _XOR:		/* exclusive or */
			if(byte)
				ptr = "EORB |";
			else {
				ptr = "EORA [\n EORB ]";
				zero_flag = -1; }
			break;
		case _SHL:		/* shift left */
			zero_flag = eflag = -1;
			ptr = runtime("?shl", type);
			rl_flags |= RL_SHL;
			break;
		case _SHR:		/* shift right */
			zero_flag = eflag = -1;
			ptr = runtime("?shr", type);
			rl_flags |= RL_SHR;
			break;
		case _EQ:		/* test for equal */
			ptr = runtime("?eq", type);
			goto eflag2;
		case _NE:		/* test for not equal */
			ptr = runtime("?ne", type);
			goto eflag2;
		case _LT:		/* test for less than */
			ptr = runtime("?lt", type);
			goto scomp;
		case _LE:		/* test for less or equal to */
			ptr = runtime("?le", type);
			goto scomp;
		case _GT:		/* test for greater than */
			ptr = runtime("?gt", type);
			goto scomp;
		case _GE:		/* test for greater than or equal to */
			ptr = runtime("?ge", type);
		scomp:
			rl_flags |= RL_SCMP;
			goto eflag2;
		case _ULT:		/* unsigned less than */
			ptr = runtime("?ult", type);
			goto ucomp;
		case _ULE:		/* unsigned less than or equal to */
			ptr = runtime("?ule", type);
			goto ucomp;
		case _UGT:		/* unsigned greater than */
			ptr = runtime("?ugt", type);
			goto ucomp;
		case _UGE:		/* unsigned greater than or equal to */
			ptr = runtime("?uge", type);
		ucomp:
			rl_flags |= RL_UCMP;
		eflag2:
			eflag = -2;
			break;
		default:		/* Unknown (error) */
			ptr = "?D? |"; }

/* if necessary, extend acc before executing instruction */
	if(eflag || !rbyte)
		expand(rtype);
	else
		last_byte = rbyte;
	if(eflag == -2)
		zero_flag = 0;

/* interpret the output string & insert the operands */
	oper_prep(token, value);
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
