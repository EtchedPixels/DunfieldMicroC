/*
 * DDS MICRO-C Code Generator for: 68HC16
 *
 * Inline assembly language code should preserve the contents of the
 * Z register, as it is used for stack relative addressing.
 *
 * 68HC16 Registers:
 *	A,B	- Accumulator
 *	X	- Index register
 *	Y	- Temporary register
 *	Z	- Stack addressing
 *
 * SLINK segments:
 *	S0	- Code
 *	S1	- WORD uninitialized data
 *	S2	- BYTE uninitialized data
 *	S3	- WORD "register" variables
 *	S4	- BYTE "register" varaibles
 *	S5	- WORD uninitialized storage (large model)
 *	S6	- BYTE uninitialized storage (large model)
 *
 * ?COPY.TXT 1992-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

#ifndef LINE_SIZE
#include "compile.h"

extern char s_name[MAX_SYMBOL][SYMBOL_SIZE+1];
extern unsigned s_type[], s_index[], s_dindex[], dim_pool[], global_top,
	local_top, function_count;
extern int size_of_var();
#endif

unsigned stack_frame, current_seg = 0, global_width = 0, global_set = 0,
	dsymbol = 0;
char call_buffer[50], zero_flag, last_byte = 0, model = 0, daflag;

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
 * Generate a call to a runtime library function.
 */
char *runtime(string, type, order)
	char *string;	/* name of library routine */
	unsigned type;	/* type of access */
	char order;		/* Order is important */
{
	char *ptr, *ptr1;

	if((type & (BYTE | POINTER)) == BYTE)		/* byte access */
		ptr = (type & UNSIGNED) ? "TDE\n LDAB |\n CLRA\n"
			: "TDE\n LDAB |\n SXT\n";
	else
		ptr = order ? "TDE\n LDD |\n" : "LDE |\n";

	for(ptr1 = call_buffer; *ptr; ++ptr1)
		*ptr1 = *ptr++;
	if(*string != '=') {
		*ptr1++ = ' ';
		*ptr1++ = 'J';
		*ptr1++ = 'S';
		*ptr1++ = 'R'; }
	else
		++string;
	*ptr1++ = ' ';
	while(*string)
		*ptr1++ = *string++;
	*ptr1 = 0;
	return call_buffer;
}

/*
 * Output preceeding instructions for operand access
 */
oper_prep(token)
	unsigned token;
{
	switch(token) {
		case ON_STACK:
			out_inst("TSY");
			break;
		case ION_STACK:
			out_inst("PULM Y");
			break;
		case ISTACK_TOP:
			out_inst("PULM Y\n PSHM Y"); }
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
				out_num(s_index[value] + 6 + ((type & (BYTE|POINTER|ARRAY)) == BYTE));
			else {
				out_chr('-');
				out_num(stack_frame - s_index[value]); }
			out_str(",Z");
			break;
		case IN_TEMP:
			out_str("?temp");
			break;
		case INDIRECT:
			out_str("0,X");
			break;
		case ON_STACK:
			out_str("0,Y");
			out_str("\n AIS #2");
			break;
		case ION_STACK:		/* Indirect through top of stack (destructive) */
		case ISTACK_TOP:	/* Indirect through top of stack (non-dest) */
			out_str("0,Y");
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
		out_inst((type & UNSIGNED) ? "CLRA" : "SXT");
		zero_flag = -1;
		last_byte = 0; }
}

/*
 * Clear a number of bytes from the processor stack
 */
clear_stack(size)
	unsigned size;
{
	if(size) {
		out_str(" AIS #");
		out_num(size);
		out_nl(); }
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
def_module() { }	/* No operation required */

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
	set_seg(daflag = (ssize || ((s_type[symbol] & (BYTE|POINTER)) != BYTE))
		? 1 : 2);

	if(model)
		dsymbol = symbol + 1;
	else
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
	global_set += word ? 2 : 1;

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
}

/*
 * End static storage definition
 */
end_static()
{
	if(global_width)
		out_nl();

	if(dsymbol) {
		set_seg(daflag + 4);
		out_symbol(dsymbol - 1);
		out_str(" RMB ");
		out_num(global_set);
		out_nl(); }
	else if(!global_set)
		out_inst("RMB 0");

	dsymbol = global_set = global_width = 0;
}

/*
 * Define a global variable
 */
def_global(symbol, size)
	unsigned symbol, size;
{
	if(s_type[symbol] & REGISTER) {		/* Internal variables */
		set_seg((size & 1) + 3);
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
	unsigned l, h, sp;

	size = (size + 1) & 0xFFFE;

	/* Re-sort symbol table to insure word accessed items occur first */
	for(h = MAX_SYMBOL; h > local_top; --h) {		/* Skip arguments */
		if(!(s_type[h-1] & ARGUMENT))
			break; }
	sort_symbol(local_top, h);

	/* Re-assign stack indexes, beginning with word accessed entries */
	sp = 0;
	for(l = local_top; l < h; ++l) {
		if(!(s_type[l] & (SYMTYPE|STATIC|EXTERNAL))) {
			s_index[l] = sp;
			sp += size_of_var(l); } }

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
	out_inst("PSHM Z\n TSZ");
	if(stack_frame = size) {
		out_str(" AIS #-");
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
		out_inst("TZS");
	out_inst("PULM Z\n RTS");

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
	out_str("?");
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
		set_seg(2);		/* Literal pool goes in byte segment */
		if(!model)
			out_str("?0");
		i = 0;
		while(i < size) {
			out_str((i % 16) ? "," : " FCB ");
			out_num(*ptr++);
			if(!(++i % 16))
				out_nl(); }
		if(i % 16)
			out_nl();
		if(model) { 	/* Reserve RAM area */
			set_seg(6);
			out_str("?0 RMB ");
			out_num(size);
			out_nl(); } }
}

/*
 * Call a function by name
 */
call(token, value, type, clean)
	unsigned token, value, type, clean;
{
	switch(token) {
		case NUMBER :
			out_str(" JSR ");
			out_num(value);
			break;
		case SYMBOL :
			oper_prep(token);
			out_str(" JSR ");
			write_oper(token, value, type);
			break;
		case ON_STACK :
		case ION_STACK :
			if(clean) {
				out_str(" TSX\n LDX ");
				out_num(clean++ * 2);
				out_str(",X\n"); }
			else
				out_inst("PULM X");
			out_str(" JSR 0,X"); }

	out_nl();

	clear_stack(clean += clean);
	last_byte = (type & (BYTE|POINTER)) == BYTE;
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
		out_inst(last_byte ? "TSTB" : "TSTD");
		zero_flag = 0; }

	if(ljmp)
		out_str(cond ? " LBNE ?" : " LBEQ ?");
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
}

/*
 * Load index register with a pointer value
 */
index_ptr(token, value, type)
	unsigned token, value, type;
{
	if(token == ON_STACK)
		out_inst("PULM X");
	else if(token == IN_ACC)
		out_inst("XGDX");
	else {
		oper_prep(token);
		out_str(" LDX ");
		write_oper(token, value, type);
		out_nl(); }
}

/*
 * Load index register with the address of an assignable object
 */
index_adr(token, value, type)
	unsigned token, value, type;
{
	if(token == ION_STACK) {
		out_inst("PULM X");
		return; }

	if((value < global_top) || (type & (STATIC|EXTERNAL))) {
		out_str(" LDX #");
		out_symbol(value); }
	else {
		out_str(" TZX\n AIX #");
		if(type & ARGUMENT)
			out_num(s_index[value] + 6 + ((type & (BYTE|POINTER|ARRAY)) == BYTE));
		else {
			out_chr('-');
			out_num(stack_frame - s_index[value]); } }
	out_nl();
}

/*
 * Do a simple register operation
 */
accop(oper, rtype)
	unsigned oper, rtype;
{
	char *ptr, byte, eflag, zflag;

	eflag = byte = 0;
	zflag = 100;

	if((rtype & (BYTE | POINTER)) == BYTE)
		byte = -1;

	switch(oper) {
		case _STACK:			/* stack accumulator */
			eflag = -1;
			ptr = "PSHM D";
			break;
		case _ISTACK:			/* stack index register */
			ptr = "PSHM X";
			byte = last_byte;
			break;
		case _TO_TEMP:			/* copy accumulator to temp */
			ptr = byte ? "STAB ?temp" : "STD ?temp" ;
			goto clrz;
		case _FROM_INDEX:		/* copy index to accumulator */
			ptr = "XGDX";
			last_byte = byte = 0;
			goto clrz;
		case _COM:				/* complement accumulator */
			ptr = (byte) ? "COMB" : "COMD";
			goto clrz;
		case _NEG:				/* negate accumulator */
			ptr = byte ? "NEGB" : "NEGD";
			goto clrz;
		case _NOT:				/* logical complement */
			eflag = -1;
			ptr = "JSR ?not";
			goto clrz;
		case _INC:				/* increment accumulator */
			if(isp16(rtype)) {
				ptr = "ADDD #2";
				eflag = -1; }
			else
				ptr = byte ? "INCB" : "ADDD #1";
			goto clrz;
		case _DEC:				/* decrement accumulator */
			if(isp16(rtype)) {
				ptr = "SUBD #2";
				eflag = -1; }
			else
				ptr = byte ? "DECB" : "SUBD #1";
		clrz:
			zflag = 0;
			break;
		case _IADD:				/* add acc to index register */
			zflag = eflag = -1;
			ptr = "ADX";
			break;
		default:				/* Unknown (error) */
			ptr = "?S?"; }

	if(eflag || !byte)
		expand(rtype);
	else
		last_byte = byte;

	out_inst(ptr);

	if(zflag != 100)
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
				out_inst("PULM D");
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
			ptr = runtime("=EMUL", type, 0);
			zflag = -1;
			break;
		case _DIV:		/* divide */
			zflag = eflag = -1;
			ptr = runtime(((rtype & UNSIGNED) ? "?div" : "?sdiv"), type, -1);
			break;
		case _MOD:		/* remainder */
			zflag = eflag = -1;
			ptr = runtime(((rtype & UNSIGNED) ? "?mod" : "?smod"), type, -1);
			break;
		case _AND:		/* logical and */
			if(byte) {
				ptr = "ANDB |";
				if(!rbyte)
					ptr1 = "CLRA"; }
			else
				ptr = "ANDD |";
			break;
		case _OR:		/* logical or */
			ptr = (byte) ? "ORAB |" : "ORD |";
			break;
		case _XOR:		/* exclusive or */
			ptr = (byte) ? "EORB |" : "EORD |";
			break;
		case _SHL:		/* shift left */
			zflag = eflag = -1;
			ptr = runtime("?shl", type, -1);
			break;
		case _SHR:		/* shift right */
			zflag = eflag = -1;
			ptr = runtime("?shr", type, -1);
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
			eflag = -1;
			ptr = runtime("?lt", type, -1);
			break;
		case _LE:		/* test for less or equal to */
			eflag = -1;
			ptr = runtime("?le", type, -1);
			break;
		case _GT:		/* test for greater than */
			eflag = -1;
			ptr = runtime("?gt", type, -1);
			break;
		case _GE:		/* test for greater than or equal to */
			eflag = -1;
			ptr = runtime("?ge", type, -1);
			break;
		case _ULT:		/* unsigned less than */
			eflag = -1;
			ptr = runtime("?ult", type, -1);
			break;
		case _ULE:		/* unsigned less than or equal to */
			eflag = -1;
			ptr = runtime("?ule", type, -1);
			break;
		case _UGT:		/* unsigned greater than */
			eflag = -1;
			ptr = runtime("?ugt", type, -1);
			break;
		case _UGE:		/* unsigned greater than or equal to */
			eflag = -1;
			ptr = runtime("?uge", type, -1);
			break;
		default:		/* Unknown (error) */
			ptr = "?D? |"; }

/* if necessary, extend acc before executing instruction */
	if(eflag || !rbyte)
		expand(rtype);
	else
		last_byte = rbyte;
	zero_flag = zflag;

/* interpret the output string & insert the operands */
	oper_prep(token);
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
 * Write a signed number to the output file (unsigned)
 */
out_num(value)
	unsigned value;
{
	if(value & 0x8000) {
		out_chr('-');
		value = - value; }

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
