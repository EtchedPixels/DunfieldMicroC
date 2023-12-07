/*
 * DDS MICRO-C Code Generator for: AVR
 *
 * AVR Registers:
 *	R0		  - Used to access constant memory
 *	R20:21    - Temporary location (not used with TRYT)
 *	R22:23    - Secondary parameter to runtime library calls
 *	R24:25(W) - Accumulator
 *	R26:27(X) - Misc indexing register & parameters
 *	R28:29(Y) - Stack frame addressing
 *	R30:31(Z) - Index register
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#define	TRYT			/* Try new templess way */
#define	SCODE	0		/* Code segment */
#define	SCDATA	1		/* Data in code segment */

/* #define debug(a) printf a; */
#define debug(a)

#ifndef LINE_SIZE
#include "compile.h"

extern char s_name[MAX_SYMBOL][SYMBOL_SIZE+1];
extern unsigned s_type[], s_index[], s_dindex[], dim_pool[], global_top,
	local_top, function_count, next_lab;
/* extern unsigned expr_type[], expr_ptr; */
#endif

unsigned stack_frame, stack_level, global_width = 0, global_set = 0,
	current_seg = 0;
char last_byte = 0, zero_flag;

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

/* Make an operand addressable */
oper_prep(reg, word, token, value, type)
	unsigned reg, token, value, type;
	char word;
{
	unsigned flag;

	debug(("oper_prep: %04x %04x %04x %04x %04x\n", reg, word, token, value, type))

	switch(token) {
		case NUMBER :
			out_str(" LDI R");
			out_num(reg);
			out_chr(',');
			out_num(value);
			out_nl();
			if(word) {
				out_str(" LDI R");
				out_num(reg+1);
				out_chr(',');
				out_chr('=');
				out_num(value);
				out_nl(); }
			break;
		case STRING :
			out_str(" LDI R");
			out_num(reg);
			out_str(",?0+");
			out_num(value);
			out_str("&255\n LDI R");
			out_num(reg+1);
			out_str(",?0+");
			out_num(value);
			out_str("/256\n");
			break;
		case SYMBOL :
			if((value < global_top) || (type & (STATIC|EXTERNAL))) {
				if(s_type[value] & INITED) {
					out_str(" LDI R26,");
					out_symbol(value);
					out_str("\n LDI R27,=");
					out_symbol(value);
					out_str("\n CALL ?getcsx\n MOV R");
					out_num(reg);
					out_str(",R0\n");
					if(word) {
						out_str(" CALL ?getcsx\n MOV R");
						out_num(reg+1);
						out_str(",R0\n"); }
					break; }
				out_str(" LDS R");
				out_num(reg);
				out_chr(',');
				out_symbol(value);
				out_nl();
				if(word) {
					if((type & (POINTER|BYTE)) == BYTE)
						extend_sign(reg, type);
					else {
						out_str(" LDS R");
						out_num(reg+1);
						out_chr(',');
						out_symbol(value);
						out_str("+1\n"); } }
				break; }
			flag = 0;
			if(type & ARGUMENT)
				flag += stack_frame /* + 4 */;
			flag += stack_level + s_index[value];
			if((flag > 63) || ((flag == 63) && word)) {
				out_str(" LDI R26,");
				out_num(flag);
				out_str("\n LDI R27,=");
				out_num(flag);
				out_str("\n ADD R26,R28\n ADC R27,R29\n LD R");
				out_num(reg);
				out_str(",X+\n");
				if(word) {
					if((type & (POINTER|BYTE)) == BYTE)
						extend_sign(reg, type);
					else {
						out_str(" LD R");
						out_num(reg+1);
					out_str(",X\n"); } }
					break; }
			out_str(" LDD R");
			out_num(reg);
			out_str(",Y+");
			out_num(flag);
			out_nl();
			if(word) {
				if((type & (POINTER|BYTE)) == BYTE)
					extend_sign(reg, type);
				else {
					out_str(" LDD R");
					out_num(reg+1);
					out_str(",Y+");
					out_num(flag + 1);
					out_nl(); } }
			break;
		case IN_TEMP :
#ifdef TRYT
			if(reg != 22) {
				out_str(" MOV R");
				out_num(reg);
				out_str(",R22\n MOV R");
				out_num(reg+1);
				out_str(",R23\n"); }
#else
			out_str(" MOV R");
			out_num(reg);
			out_str(",R20\n MOV R");
			out_num(reg+1);
			out_str(",R21\n");
#endif
			break;
		case INDIRECT :
			if(type & CONSTANT) {
				out_str(" LPM\n MOV R");
				out_num(reg);
				out_str(",R0\n");
				if(word) {
					out_str(" ADIW Z,1\n LPM\n MOV R");
					out_num(reg+1);
					out_str(",R0\n"); }
				break; }
			out_str(" LDD R");
			out_num(reg);
			out_str(",Z+0\n");
			if(word) {
				if((type & (POINTER|BYTE)) == BYTE)
					extend_sign(reg, type);
				else {
					out_str(" LDD R");
					out_num(reg+1);
					out_str(",Z+1\n"); } }
			break;
		case ON_STACK :
			out_str(" LD R");
			out_num(reg);
			out_str(",Y+\n LD R");
			out_num(reg+1);
			out_str(",Y+\n");
			stack_level -= 2;
			break;
		case ION_STACK :
			out_inst("LD R26,Y+\n LD R27,Y+");
			stack_level -= 2;
		load_ind:
			if(type & CONSTANT) {
				out_str(" CALL ?getcsx\n MOV R");
				out_num(reg);
				out_str(",R0\n");
				if(word) {
					out_str(" CALL ?getcsx\n MOV R");
					out_num(reg+1);
					out_str(",R0\n"); }
				break; }
			out_str(" LD R");
			out_num(reg);
			out_str(",X+\n");
			if(word) {
				out_str(" LD R");
				out_num(reg+1);
				out_str(",X\n"); }
			break;
		case ISTACK_TOP :
			out_inst("LDD R26,Y+0\n LDD R27,Y+1");
			goto load_ind;
		default:
			out_inst("?oper_prep?"); }
}

extend_sign(unsigned reg, unsigned type)
{
	if(!(type & UNSIGNED)) switch(reg) {
		case 24 : out_inst("CALL ?sign"); 	return;
		case 22 : out_inst("CALL ?psign");	return; }
	out_str(" LDI R");
	out_num(reg+1);
	out_str(",0\n");
}

/*
 * Clear a number of bytes from the processor stack
 */
clear_stack(size)
	unsigned size;
{
	if(size) {
		if(size < (64*4)) {
			while(size > 63) {
				out_inst("ADIW Y,63");
				size -= 63; }
			out_str(" ADIW Y,");
			out_num(size);
			out_nl(); }
		else {
			out_str(" LDI R22,");
			out_num(size);
			out_str("\n LDI R23,=");
			out_num(size);
			out_str("\n ADD R28,R22\n ADC R29,R23\n"); } }
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
def_module() { }

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
	set_seg(SCDATA);
	out_symbol(symbol);

	if((s_type[symbol] & (INITED|ARRAY)) == (INITED|ARRAY))
		s_type[symbol] |= CONSTANT;
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
			out_chr('?');
			out_num(value);
			out_str("/2");
			global_width += 7;
			break;
/*			ptr = "?";
			global_width += 5;
			goto doinit; */
		default :				/* constant value */
			ptr = "";
			global_width += 6;
		doinit:
			out_str(ptr);
			out_num(value); }

	if(global_width > 60) {
		global_width = 0;
		out_nl(); }

	global_set += word ? 2 : 1;
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
	out_str("$DD:");
	out_symbol(symbol);
	out_sp();

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
	set_seg(SCODE);
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
		if(size < (64*4)) {
			while(size > 63) {
				out_inst("SBIW Y,63");
				size -= 63; }
			out_str(" SBIW Y,");
			out_num(size);
			out_nl(); }
		else {
			out_str(" LDI R22,");
			out_num(size);
			out_str("\n LDI R23,=");
			out_num(size);
			out_str("\n SUB R28,R22\n SBC R29,R23\n"); } }
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
		set_seg(SCDATA);		/* Literal pool goes in segment 1 */
		out_str("?0");
		i = 0;
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
			out_num(value/2);
			break;
		case SYMBOL :		/* Global symbol */
			out_str(" CALL ");
			out_symbol(value);
			break;
		case ON_STACK :		/* Computed address */
		case ION_STACK :	/* Indirect address */
			/* Note that for stacked operands, they were placed */
			/* on the stack BEFORE any function arguments !!!!! */
			if(clean) {
				value = clean++ * 2;
				out_str(" LDD R30,Y+");
				out_num(value);
				out_str("\n LDD R31,Y+");
				out_num(value+1); }
			else {
				out_str(" LD R30,Y+\n LD R31,Y+");
				stack_level -= 2; }
			out_str("\n LSR R31\n ROR R30\n ICALL"); }
	out_nl();

	clear_stack(clean += clean);
	stack_level -= clean;
	last_byte = (type & (BYTE | POINTER)) == BYTE;
	zero_flag = -1;
}

/*
 * Unconditional jump to label
 */
jump(label, ljmp)
	unsigned label, ljmp;
{
	out_str(ljmp ? " JMP ?" : " RJMP ?");
	out_num(label);
	out_nl();
}

/*
 * Conditional jump to label
 */
jump_if(cond, label, ljmp)
	char cond;
	unsigned label, ljmp;
{
	if(zero_flag) {
		out_inst(last_byte ? "TST R24" : "MOV R22,R24\n OR R22,R25");
		zero_flag = 0; }
	if(ljmp) {
		out_str(cond ? " BREQ ?" : " BRNE ?");
		out_num(++next_lab);
		out_str("\n JMP ?");
		out_num(label);
		out_str("\n?");
		out_num(next_lab);
		out_str(" EQU *"); }
	else {
		out_str(cond ? " BRNE ?" : " BREQ ?");
		out_num(label); }
	out_nl();
}

/*
 * Perform a switch operation
 */
do_switch(label)
	unsigned label;
{
	out_str(" LDI R30,?");
	out_num(label);
	out_str("\n LDI R31,=?");
	out_num(label);
	do_asm("\n JMP ?switch");
}

/*
 * Load index register with a pointer value
 */
index_ptr(token, value, type)
	unsigned token, value, type;
{
	if(token == INDIRECT) {
		if(type & CONSTANT)
			out_inst("LPM\n MOV R22,R0\n ADIW Z,1\n LPM\n MOV R31,R0\n MOV R30,R22");
		else
			out_inst("LD R22,Z+\n LD R31,Z\n MOV R30,R22"); }
	else if(token == IN_ACC)
		out_inst("MOV R30,R24\n MOV R31,R25");
	else
		oper_prep(30, -1, token, value, type);
}

/*
 * Load index register with the address of an assignable object
 */
index_adr(token, value, type)
	unsigned token, value, type;
{
	unsigned flag;

	if(token == ION_STACK) {
		out_inst("LD R31,Y+\n LD R30,Y+");
		stack_level -= 2;
		return; }

	if((value < global_top) || (type & (STATIC|EXTERNAL))) {
		out_str(" LDI R30,");
		out_symbol(value);
		out_str("\n LDI R31,=");
		out_symbol(value);
		out_nl(); }
	else {
		flag = 0;
		if(type & ARGUMENT)
			flag += stack_frame /* + 4 */;
		flag += stack_level + s_index[value];
		if(flag > 63) {
			out_str(" LDI R30,");
			out_num(flag);
			out_str("\n LDI R31,=");
			out_num(flag);
			out_str("\n ADD R30,R28\n ADC R31,R29"); }
		else {
			out_inst("MOV R30,R28\n MOV R31,R29");
			out_str(" ADIW Z,");
			out_num(flag); }
		out_nl(); }
}

/*
 * Dummy "expand", because we always have 16 bits
 */
expand(type)
	unsigned type;
{
	debug(("EXPAND: %04x %u\n", type, last_byte))
	if(last_byte) {
		out_inst((type & UNSIGNED) ? zero_flag = -1, "LDI R25,0"
			: "CALL ?sign");
		last_byte = 0; }
}

/*
 * Do a simple register operation
 */
accop(oper, rtype)
	unsigned oper, rtype;
{
	char *ptr, byte, eflag;
	debug(("ACCOP: %04x %04x\n", oper, rtype))

	eflag = byte = 0;
	if((rtype & (BYTE | POINTER)) == BYTE)
		byte = -1;

	switch(oper) {
		case _STACK:			/* stack accumulator */
			eflag = -1;
			ptr = "ST -Y,R25\n ST -Y,R24";
			stack_level += 2;
			break;
		case _ISTACK:			/* stack index register */
			stack_level += 2;
			ptr = "ST -Y,R31\n ST -Y,R30";
			byte = last_byte;
			break;
		case _TO_TEMP:			/* copy accumulator to temp */
#ifdef TRYT
			ptr = byte ? "MOV R22,R24" : "MOV R22,R24\n MOV R23,R25";
#else
			ptr = byte ? "MOV R20,R24" : "MOV R20,R24\n MOV R21,R25";
#endif
			break;
		case _FROM_INDEX:		/* copy index to accumulator */
			ptr = "MOV R24,R30\n MOV R25,R31";
			last_byte = byte = 0;
			zero_flag = -1;
			break;
		case _COM:				/* complement accumulator */
			if(byte) {
				ptr = "COM R24";
				zero_flag = 0; }
			else {
				ptr = "COM R24\n COM R25";
				zero_flag = -1; }
			break;
		case _NEG:				/* negate accumulator */
			ptr = byte ? "NEG R24" : "COM R24\n COM R25\n ADIW W,1";
			zero_flag = 0;
			break;
		case _NOT:				/* logical complement */
			eflag = -1;
			ptr = "CALL ?not";
			zero_flag = 0;
			break;
		case _INC:				/* increment accumulator */
			zero_flag = 0;
			if(isp16(rtype)) {
				ptr = "ADIW W,2";
				eflag = -1; }
			else {
				ptr = "ADIW W,1";
				if(byte)
					zero_flag = -1; }
			break;
		case _DEC:				/* decrement accumulator */
			zero_flag = 0;
			if(isp16(rtype)) {
				ptr = "SBIW W,2";
				eflag = -1; }
			else {
				ptr = "SBIW W,1";
				if(byte)
					zero_flag = -1; }
			break;
		case _IADD:				/* add acc to index register */
			eflag = -1;
			ptr = "ADD R30,R24\n ADC R31,R25";
			zero_flag = -1;
			break;
		default:				/* Unknown (error) */
			out_inst("?S?"); }

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
	unsigned flag;
	char byte, xbyte, *ptr, *ptr1, c, eflag, rbyte, rflag;
	debug(("ACCVAL: %04x %04x %04x %04x %04x\n", oper, rtype, token, value, type))

	ptr = ptr1 = eflag = byte = xbyte = rbyte = 0;

	if(token == ON_STACK)
		type &= ~BYTE;

	if((type & (POINTER | BYTE)) == BYTE)
		xbyte = byte = -1;
	if((rtype & (POINTER | BYTE)) == BYTE)
		rbyte = last_byte;

	rflag = (byte && !rbyte);

	if(oper == _LOAD) {				/* special case */
		oper_prep(24, !byte, token, value, type);
		rbyte = byte;
		last_byte = 0;
		zero_flag = -1;
		goto dotype; }

	if(oper == _STORE) {			/* special case */
		if(!rbyte)
			expand(rtype);
		switch(token) {
			case SYMBOL:
				if((value < global_top) || (type & (STATIC|EXTERNAL))) {
					if(s_type[value]&INITED)
						line_error("Non-assignable");
					out_str(" STS ");
					out_symbol(value);
					out_str(",R24\n");
					if(!byte) {
						out_str(" STS ");
						out_symbol(value);
						out_str("+1,R25\n"); } }
				else {					/* Store to local */
					flag = 0;	/* ??? was 2 */
					if(type & ARGUMENT)
						flag += stack_frame /* + 4 */;
					flag += stack_level + s_index[value];
					if((flag > 63) || ((flag == 63) && !byte)) {
						out_str(" LDI R26,");
						out_num(flag);
						out_str("\n LDI R27,=");
						out_num(flag);
						out_str("\n ADD R26,R28\n ADC R27,R29\n");
						if(!byte)
							out_inst("ST X+,R24\n ST X,R25");
						else
							out_inst("ST X,R24");
						zero_flag = -1;
						break; }
					out_str(" STD Y+");
					out_num(flag);
					out_str(",R24\n");
					if(!byte) {
						out_str(" STD Y+");
						out_num(flag+1);
						out_str(",R25\n"); } }
				break;
			case INDIRECT:
/*				if(expr_type[expr_ptr] & CONSTANT)	*/
/*					line_error("Non-assignable");	*/
				out_inst("STD Z+0,R24");
				if(!byte)
					out_inst("STD Z+1,R25");
				break;
			case ION_STACK :
				out_inst("LD R26,Y+\n LD R27,Y+");
				stack_level -= 2;
			wristack:
/*				if(expr_type[expr_ptr] & REGISTER)	*/
/*					line_error("Non-assignable");	*/
				if(byte)
					out_inst("ST X,R24");
				else
					out_inst("ST X+,R24\n ST X,R25");
				break;
			case ISTACK_TOP:
				out_inst("LDD R26,Y+0\n LDD R27,Y+1");
				goto wristack; }
/*		zero_flag = -1;	*/
		goto dotype; }

/*	oper_prep(22, !byte, token, value, type); */
	zero_flag = 0;
	switch(oper) {
		case _ADD:				/* Addition */
			if((token == NUMBER) && (value < 64)) {
				ptr = "$ADIW W,[";
				if(byte)
					zero_flag = -1;
				break; }
			ptr = rflag ? "%LDI R23,0\n ADD R24,R22\n ADC R25,R23\\"
				: "ADD R24,R22|ADC R25,R23";
			break;
		case _SUB:		/* subtract */
			if(token == NUMBER) {
				if(value < 64) {
					ptr = "$SBIW W,[";
					if(byte)
						zero_flag = -1;
					break; }
				ptr = rflag ? "$SUBI R24,[\n SBCI R25,]\\"
					: "$SUBI R24,[|SBCI R25,]";
				break; }
			ptr = rflag ? "%LDI R23,0\n SUB R24,R22\n SBC R25,R23\\"
				: "SUB R24,R22|SBC R25,R23";
			break;
		case _MULT:		/* multiply */
			if((token == NUMBER) && (value == 2)) {
				ptr = "$LSL R24|ROL R25";
				goto eflag1; }
			ptr = "?mul";
			goto eflag1;
		case _DIV:		/* divide */
			ptr = (rtype & UNSIGNED) ? "?div" : "?sdiv";
			goto eflag1;
		case _MOD:		/* remainder */
			ptr = (rtype & UNSIGNED) ? "?mod" : "?smod";
			goto eflag1;
		case _AND:		/* logical and */
			if(token == NUMBER) {
				ptr = rflag ? "$ANDI R24,[\n LDI R25,0\\"
					: "$ANDI R24,[|ANDI R25,]";
				break; }
			ptr = rflag ? "%AND R24,R22\n LDI R25,0\\"
				: "AND R24,R22|AND R25,R23";
			break;
		case _OR:		/* logical or */
			if(token == NUMBER) {
				ptr = "$ORI R24,[|ORI R25,]";
				break; }
			ptr = "OR R24,R22|OR R25,R23";
			break;
		case _XOR:		/* exclusive or */
			ptr = "EOR R24,R22|EOR R25,R23";
			break;
		case _SHL:		/* shift left */
			if(token == NUMBER) {
				if(byte) {
					if(value < 5) {
						if(value > 0) out_inst("LSL R24");
						if(value > 1) out_inst("LSL R24");
						if(value > 2) out_inst("LSL R24");
						if(value > 3) out_inst("LSL R24");
						break; } }
				else if(value < 3) {
					if(value > 0) out_inst("LSL R24\n ROL R25");
					if(value > 1) out_inst("LSL R24\n ROL R25");
					goto eflag1; } }
			ptr = "?shl";
			goto eflag1;
		case _SHR:		/* shift right */
			if(token == NUMBER) {
				if(byte) {
					if(value < 5) {
						if(value > 0) out_inst("LSR R24");
						if(value > 1) out_inst("LSR R24");
						if(value > 2) out_inst("LSR R24");
						if(value > 3) out_inst("LSR R24");
						break; } }
				else if(value < 3) {
					if(value > 0) out_inst("LSR R25\n ROR R24");
					if(value > 1) out_inst("LSR R25\n ROR R24");
					goto eflag1; } }
			ptr = "?shr";
			goto eflag1;
		case _EQ:		/* test for equal */
			ptr = "?eq/";
			goto eflag1;
		case _NE:		/* test for not equal */
			ptr = "?ne/";
			goto eflag1;
		case _LT:		/* test for less than */
			ptr = "?lt/";
			goto eflag1;
		case _LE:		/* test for less or equal to */
			ptr = "?le/";
			goto eflag1;
		case _GT:		/* test for greater than */
			ptr = "?gt/";
			goto eflag1;
		case _GE:		/* test for greater than or equal to */
			ptr = "?ge/";
			goto eflag1;
		case _ULT:		/* unsigned less than */
			ptr = "?ult/";
			goto eflag1;
		case _ULE:		/* unsigned less than or equal to */
			ptr = "?ule/";
			goto eflag1;
		case _UGT:		/* unsigned greater than */
			ptr = "?ugt/";
			goto eflag1;
		case _UGE:		/* unsigned greater than or equal to */
			ptr = "?uge/";
		eflag1:
			eflag = -1;
			break;
		default:		/* Unknown (error) */
			out_inst("?D?"); }

dotype:
	if(eflag || !rbyte) {
		expand(rtype);
		byte = 0; }
	else
		last_byte = rbyte;

	if(ptr) {
		switch(*ptr) {
			case '%' :	oper_prep(22, 0, token, value, type);
			case '$' :	++ptr;	break;
			default:
				oper_prep(22, !xbyte, token, value, type);
				if(xbyte && !byte)
					extend_sign(22, type); }
		if(*ptr == '?') {
			out_str(" CALL");
			zero_flag = -1; }
		out_sp(' ');
		while(c = *ptr++) {
			switch(c) {
			case '|' :
				out_nl();
				if(byte)
					return;
				out_sp();
			case '\\' :
				zero_flag = -1;
				continue;
			case '/' :
				zero_flag = 0;
				continue;
			case '[' :
				out_num(value & 255);
				continue;
			case ']' :
				out_num(value >> 8);
				continue;
			case '^' :
				out_num(value);
				continue; }
			out_chr(c); }
		out_nl(); }
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
