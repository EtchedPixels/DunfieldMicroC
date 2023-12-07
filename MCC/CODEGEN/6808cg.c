/*
 * MICRO-C code generator for: 6808
 *
 * SLINK segments:
 *	S0	- Code
 *	S1	- "register" variables
 *
 * ?COPY.TXT 1993-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
/* Runtime library usage bits */
#define	RL_SWITCH	0x0001	/* SWITCH statement */
#define	RL_MUL		0x0002	/* Multiply */
#define	RL_DIV		0x0004	/* Division */
#define	RL_MOD		0x0008	/* Modulus */
#define	RL_SHL		0x0010	/* Shift left */
#define	RL_SHR		0x0020	/* Shift right */
#define	RL_AND		0x0040	/* And */
#define	RL_OR		0x0080	/* Or */
#define	RL_XOR		0x0100	/* Xor */
#define	RL_SIGN		0x0200	/* Sign extension */
#define	RL_SCMP		0x0400	/* Signed compares */
#define	RL_UCMP		0x0800	/* Unsigned compares */
#define	RL_NOT		0x1000	/* NOT */
#define	RL_NEG		0x2000	/* Negate */
#define	RL_COM		0x4000	/* Compliment */
#define	RL_IADD		0x8000	/* Add acc to index */

#define	STKADJ1		1		/* Adjustment for stack addressing */
#define	STKADJ2		2		/* Adjustment for runtime library stack overhead */

#ifndef LINE_SIZE
#include "compile.h"
extern char s_name[MAX_SYMBOL][SYMBOL_SIZE+1];
extern unsigned s_type[], s_index[], s_dindex[], dim_pool[], global_top,
	local_top, next_lab, function_count;
#endif

unsigned stack_frame, stack_level, global_width = 0, global_set = 0,
	current_seg = 0, rl_flags = 0;
char is_flag = 0, zero_flag;

char symbolic = 0;			/* control output of symbolic information */

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
 * Access a word on the stack through runtime library
 */
stack_access(offset, rlname)
	unsigned offset;
	char *rlname;
{
	out_str(" LDA #");
	out_num(offset & 255);
	out_nl();
	if(offset /= 256) {
		out_str(" MOV #");
		out_num(offset);
		out_str(",?hioff\n JSR ");
		out_str(rlname);
		out_str("+2\n"); }
	else
		runtime(rlname);
}

/*
 * Execute a runtime library call
 */
runtime(ptr)
	char *ptr;
{
	out_str(" JSR ");
	out_str(ptr);
	out_nl();
}

/*
 * Get operand value into pseudo-register
 */
get_oper(token, value, type)
	unsigned token, value, type;
{
	unsigned flag;
	char byte, zflag;

	zflag = -1;
	byte = (type & (POINTER|BYTE)) == BYTE;
	switch(token) {
		case NUMBER :
			byte = zflag = 0;
			out_str(" LDHX #");
			out_num(value);
			out_nl();
			break;
		case STRING :
			byte = zflag = 0;
			out_str(" LDHX #?0+");
			out_num(value);
			out_nl();
			break;
		case SYMBOL :
			if((value < global_top) || (type & (STATIC|EXTERNAL))) {
				if(byte) {
					out_str(" LDX ");
					out_symbol(value);
					zflag = 0; }
				else {
					if(s_type[value] & REGISTER) {
						out_str(" LDHX ");
						out_symbol(value);
						zflag = 0; }
					else {
						out_str(" LDA ");
						out_symbol(value);
						out_str("\n LDX ");
						out_symbol(value);
						out_str("+1\n PSHA\n PULH"); } }
				out_nl();
				break; }
			flag = stack_level + s_index[value];
			if(type & ARGUMENT) {
				flag += stack_frame + 2;
				if((type & (BYTE|POINTER|ARRAY)) == BYTE)
					++flag; }
			if(byte) {
				out_str(" LDX ");
				out_num(flag+STKADJ1);
				out_str(",SP\n");
				break; }
			stack_access(flag + STKADJ2, "?gstkw");
			break;
		case IN_TEMP :
			byte = zflag = 0;
			out_inst("LDHX ?temp");
			break;
		case INDIRECT :
			if(byte)
				out_inst("LDHX ?idx\n LDX ,X");
			else
				runtime("?gidxw");
			break;
		case ON_STACK :
			byte = 0;
			out_inst("PULH\n PULX");
			stack_level -= 2;
			break;
		case ISTACK_TOP :
			is_flag = 1;
		case ION_STACK :
			out_inst("PULH\n PULX");
			if(token == ISTACK_TOP)
				out_inst("STHX ?temp2");
			stack_level -= 2;
			if(byte)
				out_inst("LDX ,X");
			else
				runtime("?gindhx");
			break;
		default :
			out_num(token);
			out_inst("?get_oper"); }

	if(byte) {
		out_inst((type & UNSIGNED) ?
			"CLRH" : (rl_flags |= RL_SIGN, "JSR ?sext"));
		zflag = -1; }

	return zflag;
}

/*
 * Clear a number of bytes from the processor stack
 */
clear_stack(size)
	unsigned size;
{
	unsigned x;

	if(size > 508) {
		out_str(" LDA #");
		out_num(size / 127);
		out_str("\n?");
		out_num(++next_lab);
		out_str(" AIS #127\n DBNZA ?");
		out_num(next_lab);
		out_str("\n");
		size %= 127; }

	while(size) {
		if(size == 1) {
			out_inst("PULA");
			break; }
		out_str(" AIS #");
		out_num(x = (size > 127) ? 127 : size);
		out_nl();
		size -= x; }
}

/*
 * Determine if type is pointer to 16 bits
 */
isp16(type)
	unsigned type;
{
	if(type & (POINTER-1))		/* Pointer to pointer */
		return 1;
	if(type & POINTER)			/* First level pointer */
		return !(type & BYTE);
	return 0;
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
 * Output a string to the assembler, followed by a newline
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
 * Define beginning and end of module
 */
def_module() { }	/* No operation required */
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

expand() { }		/* Always works in 16 bits */

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
		case SYMBOL :
			out_symbol(value);
			global_width += 12;
			break;
		case STRING :
			ptr = "?0+";
			global_width += 9;
			goto doinit;
		case LABEL :
			ptr = "?";
			global_width += 4;
			goto doinit;
		default :
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
	else {								/* External variables */
		out_str("$DD:");
		out_symbol(symbol);
		out_sp(); }

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
 * Enter function & allocate local stack space
 */
def_func(symbol, size)
	unsigned symbol, size;
{
	unsigned x;

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
		if(size > 512) {
			out_str(" LDA #");
			out_num(size / 128);
			out_str("\n?");
			out_num(++next_lab);
			out_str(" AIS #-128\n DBNZA ?");
			out_num(next_lab);
			out_str("\n");
			size %= 128; }
		while(size) {
			if(size == 1) {
				out_inst("PSHA");
				break; }
			out_str(" AIS #-");
			out_num(x = (size > 128) ? 128 : size);
			out_nl();
			size -= x; } }
	else
		out_inst("EQU *");
	stack_level = 0;
}

/*
 * Clean up stack & end function definition
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
 * Define compiler generated label
 */
def_label(label)
	unsigned label;
{
	out_chr('?');
	out_num(label);
	out_inst("EQU *");
}

/*
 * Define the literal pool
 */
def_literal(ptr, size)
	unsigned char *ptr;
	unsigned size;
{
	unsigned i;

	if(size) {
		set_seg(0);
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
 * Call a function
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
			out_str(" JSR ");
			out_symbol(value);
			break;
		case ON_STACK :		/* Call to computed address */
		case ION_STACK :	/* Call to indirect address */
			/* Note that for stacked operands, they were placed */
			/* on the stack BEFORE any function arguments !!!!! */
			if(clean) {
				out_str(" LDA #");
				out_num((clean++ * 2) + 2);
				out_str("\n JSR ?gstkw\n"); }
			else {
				out_inst("PULH\n PULX");
				stack_level -= 2; }
			out_str(" JSR ,X"); }

	out_nl();

	clear_stack(clean += clean);
	stack_level -= clean;
	zero_flag = -1;
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
		out_inst("CPHX #0");
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
	out_inst("PSHX\n PSHH");
	out_str(" LDHX #?");
	out_num(label);
	do_asm("\n JMP ?switch");
	rl_flags |= RL_SWITCH;
}

/*
 * Load the index register with a pointer value
 */
index_ptr(token, value, type)
	unsigned token, value, type;
{
	unsigned flag;

	switch(token) {
		case NUMBER :
			out_str(" MOV #");
			out_num(value / 256);
			out_str(",?idx\n MOV #");
			out_num(value & 255);
			out_str(",?idx+1\n");
			break;
		case STRING :
			out_str(" MOV #(?0+");
			out_num(value);
			out_str(")/256,?idx\n MOV #(?0+");
			out_num(value);
			out_str(")&255,?idx+1\n");
			break;
		case IN_ACC :
			out_inst("STHX ?idx");
			break;
		case ON_STACK :
			out_inst("PULA\n STA ?idx\n PULA\n STA ?idx+1");
			stack_level -= 2;
			break;
		case SYMBOL :
			if((value < global_top) || (type & (STATIC|EXTERNAL))) {
				out_str(" LDA ");
				out_symbol(value);
				out_str("\n STA ?idx\n LDA ");
				out_symbol(value);
				out_str("+1\n STA ?idx+1\n");
				break; }
			flag = stack_level + s_index[value] + STKADJ2;
			if(type & ARGUMENT)
				flag += stack_frame + 2;
			stack_access(flag, "?istkw");
			break;
		case IN_TEMP :
			out_inst("MOV ?temp,?idx\n MOC ?temp+1,?idx+1");
			break;
		case INDIRECT :
			runtime("?idxidx");
			break;
		default :
			out_num(token);
			out_inst("?index_ptr"); }
}

/*
 * Load the index register with the address of an assignable object
 */
index_adr(token, value, type)
	unsigned token, value, type;
{
	unsigned flag;

	switch(token) {
		case SYMBOL :
			if((value < global_top) || (type & (STATIC|EXTERNAL))) {
				out_str(" MOV #");
				out_symbol(value);
				out_str("/256,?idx\n MOV #");
				out_symbol(value);
				out_str("&255,?idx+1\n");
				break; }
			flag = stack_level + s_index[value] + STKADJ2;
			if(type & ARGUMENT)
				flag += stack_frame + 2;
			stack_access(flag, "?istka");
			break;
		case ION_STACK :
			out_inst("PULA\n STA ?idx\n PULA\n STA ?idx+1");
			stack_level -= 2;
			break;
		default :
			out_num(token);
			out_inst("?index_adr"); }
}

/*
 * Do a simple register operatino
 */
accop(oper, rtype)
	unsigned oper, rtype;
{
	switch(oper) {
		case _STACK :
			out_inst("PSHX\n PSHH");
			stack_level += 2;
			break;
		case _ISTACK :
			out_inst("LDA ?idx+1\n PSHA\n LDA ?idx\n PSHA");
			stack_level += 2;
			zero_flag = -1;
			break;
		case _TO_TEMP :
			out_inst("STHX ?temp");
			zero_flag = 0;
			break;
		case _FROM_INDEX:
			out_inst("LDHX ?idx");
			zero_flag = 0;
			break;
		case _COM :
			runtime("?com");
			rl_flags |= RL_COM;
			zero_flag = -1;
			break;
		case _NEG :
			runtime("?neg");
			rl_flags |= RL_NEG;
			zero_flag = -1;
			break;
		case _NOT :
			runtime("?not");
			rl_flags |= RL_NOT;
			zero_flag = 0;
			break;
		case _INC :
			out_inst(isp16(rtype) ? "AIX #2" : "AIX #1");
			zero_flag = -1;
			break;
		case _DEC :
			out_inst(isp16(rtype) ? "AIX #-2" : "AIX #-1");
			zero_flag = -1;
			break;
		case _IADD :
			runtime("?iadd");
			rl_flags |= RL_IADD;
			zero_flag = -1;
			break;
		default :
			out_num(oper);
			out_inst("?accop"); }
}

/*
 * Performs an operation with the accumulator and specified value
 */
accval(oper, rtype, token, value, type)
	unsigned oper, rtype, token, value, type;
{
	unsigned flag;
	char byte;

	byte = (type & (POINTER|BYTE)) == BYTE;

	zero_flag = -1;
	if(oper == _STORE) {		/* special case */
		switch(token) {
			case SYMBOL :
				if((value < global_top) || (type & (STATIC|EXTERNAL))) {
					if(byte) {
						out_str(" STX ");
						out_symbol(value); }
					else {
						if(s_type[value] & REGISTER) {
							out_str(" STHX ");
							out_symbol(value); }
						else {
							out_str(" PSHH\n PULA\n STX ");
							out_symbol(value);
							out_str("+1\n STA ");
							out_symbol(value); } }
					out_nl();
					zero_flag = 0;
					break; }
				flag = stack_level + s_index[value];
				if(type & ARGUMENT) {
					flag += stack_frame + 2;
					if((type & (BYTE|POINTER|ARRAY)) == BYTE)
						++flag; }
				if(byte) {
					out_str(" STX ");
					out_num(flag+STKADJ1);
					out_str(",SP\n");
					break; }
				stack_access(flag + STKADJ2, "?pstkw");
				break;
			case INDIRECT :
				runtime(byte ? "?pidxb" : "?pidxw");
				break;
			case ISTACK_TOP :
				is_flag = 2;
			case ION_STACK :
				if(is_flag != 1) {
					out_inst("PULA\n STA ?temp2\n PULA\n STA ?temp2+1");
					stack_level -= 2; }
				runtime(byte ? "?pindb" : "?pindw");
				if(is_flag)
					--is_flag; }

		return; }

	switch(oper) {
		case _LOAD :
			zero_flag = get_oper(token, value, type);
			return;
		case _ADD :
			if((token == NUMBER) && (value < 128)) {
				out_str(" AIX #");
				out_num(value);
				out_nl();
				zero_flag = -1;
				return; }
			break;
		case _SUB :
			if((token == NUMBER) && (value < 129)) {
				out_str(" AIX #-");
				out_num(value);
				out_nl();
				zero_flag = -1;
				return; } }

	out_inst("STHX ?acc");
	get_oper(token, value, type);

	switch(oper) {
		case _ADD :		runtime("?add");						break;
		case _SUB :		runtime("?sub");						break;
		case _MULT :	runtime("?mul");	rl_flags |= RL_MUL;	break;
		case _DIV :
			runtime((rtype & UNSIGNED) ? "?div" : "?sdiv");
			rl_flags |= RL_DIV;
			break;
		case _MOD :
			runtime((rtype & UNSIGNED) ? "?mod" : "?smod");
			rl_flags |= RL_MOD;
			break;
		case _AND :		runtime("?and");	rl_flags |= RL_AND;	break;
		case _OR :		runtime("?or");		rl_flags |= RL_OR;	break;
		case _XOR :		runtime("?xor");	rl_flags |= RL_XOR;	break;
		case _SHL :		runtime("?shl");	rl_flags |= RL_SHL;	break;
		case _SHR :		runtime("?shr");	rl_flags |= RL_SHR;	break;
		case _EQ :		runtime("?eq");		goto z0;
		case _NE :		runtime("?ne");
		z0:	zero_flag = 0;	break;
		case _LT :		runtime("?lt");		goto z1;
		case _LE :		runtime("?le");		goto z1;
		case _GT :		runtime("?gt");		goto z1;
		case _GE :		runtime("?ge");
		z1: rl_flags |= RL_SCMP; goto z0;
		case _ULT :		runtime("?ult");	goto z2;
		case _ULE :		runtime("?ule");	goto z2;
		case _UGT :		runtime("?ugt");	goto z2;
		case _UGE :		runtime("?uge");
		z2: rl_flags |= RL_UCMP; goto z0;
		default:
			out_num(oper);
			out_inst("?accop"); }
}

/*
 * Output a symbol name
 */
out_symbol(s)
	unsigned s;
{
	if(s_type[s] & STATIC) {
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
