/*
 * DDS MICRO-C Code Generator for: 8051 family
 *
 *	 The 8051, with its two memory spaces and restricted instruction set
 * makes for a challenging MICRO-C implementation. This code generator
 * provides examples of various techniques whereby the basic processor model
 * employed by MICRO-C is adapted to conform to a hardware platform which
 * is quite different from that model.
 *
 * To differentiate accesses to INTERNAL and EXTERNAL memory, the code
 * generator makes use of the "register" attribute. Although this is
 * inconsistent with the standard 'C' use of "register", this method was
 * chosen because it requires no changes to the main compiler. Where
 * possible, the general notion that "register" means "efficent" has
 * been preserved.
 *
 * GLOBAL variables which are declared as "register" will be allocated
 * in internal memory. These variables should NOT be initialized in the
 * declaration.
 *
 * GLOBAL variables which are NOT declared as "register" are allocated in
 * EXTERNAL memory. Any such variables which are initialized in the declaration
 * are allocated in ROM, following the program code, and thus will not be
 * alterable. If not initialized, they are allocated in external RAM.
 *
 * LOCAL variables are allocated on the 8051 stack. Since there is only
 * 120 bytes of memory for internal variables and the stack, care should
 * be exercised is the use of local variables.
 *
 * By default, a reference via a pointer occurs to INTERNAL memory if that
 * pointer is declared as "register", and to EXTERNAL memory if it is not.
 *
 * For GLOBAL pointer variables, this means that the pointer will reference
 * the same type of memory in which it is defined. You may use casts to cause
 * a pointer to reference a specific type of memory.
 *
 * Where possible, use INTERNAL (register) global variables. Local (auto) and
 * EXTERNAL (non register) variables both incurr significant overhead to access
 * Due to the memory limitations of the 8051, you may have to "double up" those
 * internal variables which can be shared between functions. (The pre-processor
 * can be used to refer to the same variable by multiple names if you like).
 *
 * Assembly language code may be used at any time, either as INLINE code which
 * occurs within a 'C' functions, or as an ASSEMBLY language FUNCTION which
 * you can then call from 'C'.
 *
 * An assembly language function returning a value should return it as a 16
 * bit quantity in B:A (B=High byte, A=Low byte).
 *
 * The following restrictions apply to the use of assembly language code:
 *
 * If you modify the REGISTER BANK in ANY assembly language code (INLINE or
 * FUNCTION), you must reset it to ZERO before your code completes. It is
 * also your responsibility to make sure that you do not "clobber" INTERNAL
 * memory if you move the register bank. If necessary, the startup code can
 * be modified to allow space for additional register banks (This would also
 * be a good idea if you employ interrupt handlers that use the registers).
 *
 * INLINE code should preserve the contents of R0. This restriction does not
 * apply to assembly language functions. All other registers may be modified..
 *
 * Most CPU's have a downward growing stack, while the 8051 has an upward
 * growing stack. In most cases, this is of no concern to the 'C' programmer,
 * since the compiler takes care of addressing the stack. There are however
 * two instances when you must take the forward growing stack into account:
 *
 * When writeing assembly language functions, you have to SUBTRACT from
 * the stack pointer to address parameters instead of ADD. See the example
 * library functions supplied in this demo package.
 *
 * The MICRO-C library functions which use a variable number of arguments such
 * as "printf" address the stack directly through a pointer which is set up
 * to point to the FIRST (furthest away) argument. If you wish to include those
 * functions in the 8051 library, the 'C' source must be modified to INCREMENT
 * this pointer instead of DECREMENT it. Any functions from the library which
 * are declared as "register" are affected.
 *
 * 8051 Registers:
 *	A	- Accumulator LOW
 *	B	- Accmulator HIGH
 *	DPTR- External memory access
 *	R0	- Stack addressing
 *	R1	- Index register LOW
 *	R2	- Index register HIGH
 *	R3	- Temporary location LOW
 *	R4	- Temporary location HIGH
 *	R7	- Temporary storage of 'A' during RL calls
 *
 * SLINK segments:
 *	S0	- Code
 *	S1	- Initialized data
 *	S2	- "register" variables
 *	S3	- Uninitialized storage (MEDIUM and LARGE models)
 *
 * ?COPY.TXT 1991-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#define	SEG_CODE	0		/* Code storage segment */
#define	SEG_IDATA	1		/* Iniitialized data segment */
#define	SEG_RDATA	2		/* "register" variables */
#define	SEG_UDATA	3		/* Unitialized storage (MEDIUM and LARGE) */

#define	MDL_TINY	0		/* Tiny   : No external data */
#define	MDL_SMALL	1		/* Small  : CODE+DATA overlaid, stack internal */
#define	MDL_COMPACT	2		/* Compact: CODE+DATA overlaid, stack external */
#define	MDL_MEDIUM	3		/* Medium : CODE+DATA separate, stack internal */
#define	MDL_LARGE	4		/* Large  : CODE+DATA separate, stack external */

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

#define	RL_ADDI		0x2000	/* Add ACC to index */
#define	RL_INC		0x4000	/* Increment */
#define	RL_DEC		0x8000	/* Decrement */

#ifndef LINE_SIZE
#include "compile.h"

extern char s_name[MAX_SYMBOL][SYMBOL_SIZE+1];
extern unsigned s_type[], s_index[], s_dindex[], dim_pool[], global_top,
	local_top, next_lab, function_count;
#endif

#define	STKOFF	1		/* Overhead offset to correctly access stack (1) */
#define	STKARG	4		/* Offset when argument (4) */

/* Operand location identifiers */
#define	R0		1		/* Operand is indirect through R0 */
#define	R1		2		/* Operand is indirect through R1 */
#define	DPTR	3		/* Operand is indirect through DPTR */
#define	DPTRC	4		/* Operand is constant through DPTR */
#define	TEMP	5		/* Operand is in temporary register */

unsigned stack_frame, stack_level, global_width = 0, global_set = 0,
	operand = 0, r0 = 32767, current_seg = 0, local_sp = 0, dsymbol = 0,
	rl_flags = 0;
char last_byte = 0, r1 = 0, local_idx = 0, local_acc = 0, local_stk[30];

/*
 * This variable controls the memory model:
 *
 * 0  = TINY model: external memory is READ-ONLY, and accessed via MOVC.
 *		Only "register" globals may be modified by the program. All others
 *		must be initialized with "static" data which will not be modified.
 *		Local variables are allocated on the internal CPU stack.
 *
 * 1  = SMALL model: external memory is READ/WRITE, and accessed via
 *		MOVX. To use this model, you must have external PROGRAM and DATA
 *		memory OVERLAYED into a single 64K address space which can be
 *		accessed with either MOVX or MOVC. Local variables are allocated
 *		on the internal CPU stack.
 *
 * 2  = COMPACT model: Similar to SMALL model, except that local variables
 *		are allocated in a "pseudo" stack in external ram.
 *
 * 3  = MEDIUM: Similar as SMALL, except that initialized data is copied from
 *		the ROM to the RAM, eliminating the requirement that ROM and RAM be
 *		overlapped, and allowing initialized globals to be changed at runtime.
 *
 * 4  = LARGE: Similar as COMPACT, expect that initialized data is copied from
 *		the ROM to the RAM, eliminating the requirement that ROM and RAM be
 *		overlapped, and allowing initialized globals to be changed at runtime.
 *
 * You may wish to add code to IO.C to allow selecting the memory model
 * when you execute the compiler (via a command line switch).
 */
char model = MDL_MEDIUM;	/* Default to MEDIUM model */
char symbolic = 0;			/* Controls output of symbolic information */

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
 * Select appropriate DTRP access mode for external memory.
 */
dptr(unsigned t)
{
	if(!model)
		return DPTRC;
	if(model > MDL_COMPACT) {
		if((t & (CONSTANT|INITED)) == (CONSTANT|INITED))
			return DPTRC;
		if((t & (CONSTANT|POINTER)) == CONSTANT)
			return DPTRC; }
	return DPTR;
}

/*
 * Output preceeding instruction(s) for operand access
 */
oper_prep(token, value, type, hold)
	unsigned token, value, type;
	char hold;
{
	int reg, i, s;

	reg = type & REGISTER;
	operand = 0;

	switch(token) {
		case SYMBOL :			/* A symbol reference */
			if((value < global_top) || (type & (STATIC|EXTERNAL))) {/* Global */
				if(!(s_type[value] & REGISTER)) {		/* External memory */
					out_str(" MOV DPTR,#");
					out_symbol(value);
					out_nl();
					operand = dptr(type); } }
			else {							/* Local symbol */
				if((model == MDL_COMPACT) || (model == MDL_LARGE)) {
					s = s_index[value];
					if(!(type & ARGUMENT)) {
						out_str(" MOV R7,#");
						out_num(s & 255);
						if(s & 0xff00) {
							out_str("\n MOV R6,#");
							out_num((unsigned)s >> 8);
							do_asm("\n LCALL ?accex2"); }
						else
							do_asm("\n LCALL ?accex1");
						operand = dptr(type);
						break; }
					s += STKARG; }
				else {
					s = type & ARGUMENT ?
						stack_frame + s_index[value] + STKARG:
						stack_frame - s_index[value]; }
				if((i = abs(reg = s - r0)) < 5) {
					while(i) {
						out_inst(reg < 0 ? "INC R0" : "DEC R0");
						--i; } }
				else {
					out_str(" MOV R0,#");
					out_num(-(stack_level + s + STKOFF));
					do_asm("\n LCALL ?auto0"); }
				r0 = s;
				operand = R0; }
			break;
		case INDIRECT :			/* Through the index register */
			operand = R1;
			if(r1) {			/* Index has been inc'd without reload */
				out_inst("DEC R1");
				r1 = 0; }
			if(!(local_idx || reg)) {
				out_inst("MOV DPL,R1\n MOV DPH,R2");
				operand = dptr(type); }
			break;
		case ON_STACK :			/* Value is on the stack */
			out_inst("POP ?R4\n POP ?R3");
			stack_level -= 2;
			--local_sp;
		case IN_TEMP :			/* Value is in temporary register */
			operand = TEMP;
			break;
		case ION_STACK :		/* Indirect through stack top */
		case ISTACK_TOP :		/* "" "" and leave it on */
			if(local_stk[--local_sp] || reg) {
				r0 = 32767;
				out_inst("POP ?R7\n POP ?R0");
				operand = R0;
				reg = -1; }
			else {
				out_inst("POP DPH\n POP DPL");
				operand = dptr(type); }
			if(token == ISTACK_TOP) {
				out_inst(reg ? "PUSH ?R0\n PUSH ?R7" : "PUSH DPL\n PUSH DPH");
				++local_sp; }
			else {
				stack_level -= 2; } }

	if(hold) {		/* Load into temp register */
		reg = (type & (BYTE | POINTER)) == BYTE;
		if(operand == DPTR) {
			do_call(reg ? "?extb" : "?extw");
			operand = TEMP; }
		else if(operand == DPTRC) {
			do_call(reg ? "?extbc" : "?extwc");
			operand = TEMP; }
		else if((hold > 1) && (operand != TEMP)) {
				out_str(" MOV ");
				if(operand == R0 || operand == R1)
					out_chr('?');
				out_str("R3,");
				write_oper(token, value, 2, "\n");
				if(!reg) {
					incoper();
					out_str(" MOV ");
					if(operand == R0 || operand == R1)
						out_chr('?');
					out_str("R4,");
					write_oper(token, value, 1, "\n"); }
			operand = TEMP; }
		if(reg && (hold > 1))
			out_inst(type & UNSIGNED ? "MOV R4,#0"
				: (rl_flags |= RL_SIGN, "LCALL ?tsign")); }
}

/*
 * Write the operand value
 * flag: 0=16 Bit, 1=High 8 bits, 2=Low 8 bits
 */
write_oper(token, value, flag, tail)
	unsigned token, value;
	char flag, *tail;
{
	switch(operand) {
		case R0 :	out_str("[R0]");					break;
		case R1 :	out_str("[R1]");					break;
		case TEMP:	out_str(flag == 1 ? "R4" : "R3");	break;
		default:	switch(token) {
			case NUMBER :		/* A numeric value */
				if(flag == 1)		/* High 8 bits */
					value >>= 8;
				else if(flag)		/* Low 8 bits */
					value &= 0xff;
				out_chr('#');
				out_num(value);
				break;
			case STRING :		/* A text string offset */
				out_str("#?0+");
				out_num(value);
				if(flag)
					out_str(flag == 1 ? "/256" : "&255");
				break;
			case SYMBOL :		/* A named symbol */
				out_symbol(value);
				if(flag == 1)
					out_str("+1");
				break;
			default:
				out_str("?"); } }

	out_str(tail);
}

/*
 * Advance operand pointers if necessary
 */
incoper()
{
	switch(operand) {
		case R0	:	out_inst("INC R0");	--r0;		break;
		case R1	:	out_inst("INC R1");	r1 = -1;	break;
		case DPTR:
		case DPTRC:	out_inst("INC DPTR"); }
}

/*
 * Expand 8 bit accumulator to 16 bits
 */
expand(type)
	unsigned type;
{
	if(last_byte) {
		out_inst(type & UNSIGNED ? "MOV B,#0"
			: (rl_flags |= RL_SIGN, "LCALL ?sign"));
		last_byte = 0; }
}

/*
 * Determine if a type is a pointer to 16 bits
 */
isp16(type)
	unsigned type;
{
	if(type & (POINTER-1))
		return 1;
	if(type & POINTER)
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
 * Output a string to the assembler
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
 * Perform an assembly language call
 */
do_call(label)
	char *label;
{
	out_str(" LCALL ");
	out_str(label);
	out_nl();
}

/*
 * Define beginning of module
 */
def_module() { };

/*
 * End of module definition
 * Write out the runtime library usage bits
 */
end_module()
{
	unsigned i;

	out_str("$RL:");
	put_num(rl_flags, -1);
	out_nl();
	if(symbolic) for(i=0; i < global_top; ++i) {
		set_seg(SEG_CODE);
		out_str("*#gbl ");
		dump_symbol(i); }
}

/*
 * Begin definition of a static variable
 */
def_static(symbol, ssize)
	unsigned symbol;
{
	if(s_type[symbol] & CONSTANT) {
		set_seg(SEG_CODE);
		out_symbol(symbol);
		return; }
	set_seg(SEG_IDATA);
	if(model > MDL_COMPACT)
		dsymbol = symbol+1;
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

	out_str(global_width ? "," : word ? " DRW " : " DB ");
	global_set += word ? 2 : 1 ;

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
}

/*
 * End static storage definition
 */
end_static()
{
	if(global_width)
		out_nl();

	if(dsymbol) {
		set_seg(SEG_UDATA);
		out_symbol(dsymbol-1);
		out_str(" DS ");
		out_num(global_set);
		out_nl(); }
	else if(!global_set)
		out_inst("DS 0");

	dsymbol = global_set = global_width = 0;
}

/*
 * Define a global variable
 */
def_global(symbol, size)
	unsigned symbol, size;
{
	if(s_type[symbol] & REGISTER) {		/* Internal variables */
		set_seg(SEG_RDATA);
		out_symbol(symbol);
		out_str(" DS "); }
	else {
		out_str("$DD:");
		out_symbol(symbol);
		out_sp(); }					/* External variables */

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
	set_seg(SEG_CODE);		/* Code goes in segment 0 */

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
		if((model == MDL_COMPACT) || (model == MDL_LARGE)) {
			size = -size;
			out_str(" MOV R7,#");
			out_num(size & 255);
			if(size & 0xff00) {
				out_str("\n MOV R6,#");
				out_num(size >> 8);
				do_asm("\n LCALL ?adjex2"); }
			else
				do_asm("\n LCALL ?adjex1"); }
		else {
			if(size <= 5) {
				while(size--)
					out_inst("INC SP"); }
			else {
				out_str(" MOV R7,#");
				out_num(size);
				do_asm("\n LCALL ?adjstk"); } } }
	else
		out_inst("EQU *");
	local_sp = stack_level = r1 = 0;
	r0 = 32767;
}

/*
 * Clean up the stack & end function definition
 */
end_func()
{
	unsigned size;

	if((model == MDL_COMPACT) || (model == MDL_LARGE)) {
		while(stack_level) {
			out_inst("DEC SP");
			--stack_level; }
		if(size = stack_frame) {
			out_str(" MOV R7,#");
			out_num(size & 255);
			if(size & 0xff00) {
				out_str("\n MOV R6,#");
				out_num(size >> 8);
				do_asm("\n LJMP ?adjex2"); }
			else
				do_asm("\n LJMP ?adjex1"); }
		else
			out_inst("RET"); }
	else {
		if((size = stack_frame + stack_level) <= 4) {
			while(size--)
				out_inst("DEC SP");
			out_inst("RET"); }
		else {
			out_str(" MOV R7,#");
			out_num(-size);
			do_asm("\n LJMP ?exit"); } }

	if(symbolic) {
		for(size = local_top; size < MAX_SYMBOL; ++size) {
			out_str("*#lcl ");
			dump_symbol(size); }
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
	r0 = 32767;
	r1 = 0;
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
#if 0
		set_seg(SEG_IDATA);	/* Literal pool goes in the code segment */
		if(model <= MDL_COMPACT)
			out_str("?0");
		i = 0;
		while(i < size) {
			out_str((i % 16) ? "," : " DB ");
			out_num(*ptr++);
			if(!(++i % 16))
				out_nl(); }
		if(i % 16)
			out_nl();
		if(model > MDL_COMPACT) {	/* MEDIUM and LARGE, reserve RAM area */
			set_seg(SEG_UDATA);
			out_str("?0 DS ");
			out_num(size);
			out_nl(); } }
#else
		set_seg((model <= MDL_COMPACT) ? SEG_IDATA : SEG_CODE);
		out_str("?0");
		i = 0;
		while(i < size) {
			out_str((i % 16) ? "," : " DB ");
			out_num(*ptr++);
			if(!(++i % 16))
				out_nl(); }
		if(i % 16)
			out_nl(); }
#endif
}

/*
 * Call a function by name
 */
call(token, value, type, clean)
	unsigned token, value, type, clean;
{
	switch(token) {
		case NUMBER :		/* Call to fixed address */
			out_str(" LCALL ");
			out_num(value);
			out_nl();
			break;
		case SYMBOL :		/* Call to fixed symbol */
			out_str(" LCALL ");
			out_symbol(value);
			out_nl();
			break;
		case ON_STACK :		/* Call to computed value */
		case ION_STACK :	/* Call to indirect value */
			/* Note that for stacked operands, they were placed */
			/* on the stack BEFORE any function arguments !!!!! */
			if(clean) {
				out_str(" MOV R0,#-");
				out_num((clean++ * 2) + (STKOFF + 2));
				do_asm("\n LCALL ?auto0");
				out_inst("MOV DPL,[R0]\n INC R0\n MOV DPH,[R0]"); }
			else {
				out_inst("POP DPH\n POP DPL");
				stack_level -= 2;
				--local_sp; }
			do_call("?idcall");
			rl_flags |= RL_SWITCH; }

	local_sp -= clean;
	stack_level -= (clean += clean);

	if(clean <= 5) {
		while(clean--)
			out_inst("DEC SP"); }
	else {
		out_str(" MOV R7,#");
		out_num(-clean);
		do_asm("\n LCALL ?adjstk"); }

	last_byte = (type & (BYTE|POINTER)) == BYTE;
	r0 = 32767;
	r1 = 0;
}

/*
 * Unconditional jump to label
 */
jump(label, ljmp)
	unsigned label;
	char ljmp;
{
	out_str(ljmp ? " LJMP ?" : " SJMP ?");
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

	if(!last_byte)
		out_inst("ORL A,B");

	if(ljmp) {
		out_str(cond ? " JZ ?" : " JNZ ?");
		out_num(++next_lab);
		out_str("\n LJMP ?");
		out_num(label);
		out_str("\n?");
		out_num(next_lab);
		out_str(" EQU *"); }
	else {
		out_str(cond ? " JNZ ?" : " JZ ?");
		out_num(label); }
	out_nl();
}

/*
 * Perform a switch operation
 */
do_switch(label)
	unsigned label;
{
	out_str(" MOV DPTR,#?");
	out_num(label);
	do_asm("\n LJMP ?switch");
	r0 = 32767;
	r1 = 0;
	rl_flags |= RL_SWITCH;
}

/*
 * Load the index register with the address of an assignable object
 */
index_adr(token, value, type)
	unsigned token, value, type;
{
	unsigned s;

	if(token == ION_STACK) {
		out_inst("POP ?R2\n POP ?R1");
		stack_level -= 2;
		--local_sp;
		return; }

	local_idx = r1 = 0;
	if((value < global_top) || (type & (STATIC|EXTERNAL))) {	/* Globals */
		out_str(" MOV R1,#");
		out_symbol(value);
		out_str("\n MOV R2,#=");
		out_symbol(value);
		out_nl(); }
	else {							/* Locals */
		if((model == MDL_COMPACT) || (model == MDL_LARGE)) {
			s = s_index[value];
			if(!(type & ARGUMENT)) {
				out_str(" MOV R7,#");
				out_num(s & 255);
				if(s & 0xff00) {
					out_str("\n MOV R6,#");
					out_num(s >> 8);
					do_asm("\n LCALL ?idxex2"); }
				else
					do_asm("\n LCALL ?idxex1");
				return; }
			s += STKARG; }
		else {
			s = type & ARGUMENT ?
				stack_frame + s_index[value] + STKARG:
				stack_frame - s_index[value]; }
		out_str(" MOV R1,#");
		out_num(-(stack_level + s + STKOFF));
		do_asm("\n LCALL ?auto1");
		local_idx = -1; }
}

/*
 * Load the index register with the contents of a symbol
 */
index_ptr(token, value, type)
	unsigned token, value, type;
{
	if(token == ON_STACK) {
		out_inst("POP ?R2\n POP ?R1");
		stack_level -= 2;
		local_idx = local_stk[--local_sp]; }
	else if(token == IN_ACC) {
		out_inst("MOV R1,A\n MOV R2,B");
		local_idx = local_acc; }
	else {
		oper_prep(token, value, type, 0);
		if(operand == DPTR)
			do_call("?exti");
		else if(operand == DPTRC)
			do_call("?extic");
		else if(operand == R0) {
			out_inst("MOV ?R1,[R0]\n INC R0\n MOV ?R2,[R0]");
			--r0; }
		else if(operand == R1)
			out_inst("INC R1\n MOV ?R2,[R1]\n DEC R1\n MOV ?R1,[R1]");
		else {
			out_str(" MOV R1,");
			write_oper(token, value, 2, "\n");
			incoper();
			out_str(" MOV R2,");
			write_oper(token, value, 1, "\n"); }
		local_idx = 0; }
	r1  = 0;
}

/*
 * Do a one simple accumulator operation
 */
accop(oper, rtype)
	unsigned oper, rtype;
{
	char *ptr, byte, eflag;

	eflag = byte = 0;

	if((rtype & (BYTE | POINTER)) == BYTE)
		byte = -1;
	switch(oper) {
		case _STACK :			/* Stack accumulator */
			eflag = -1;
			ptr = "PUSH A\n PUSH B";
			stack_level += 2;
			local_stk[local_sp++] = local_acc;
			break;
		case _ISTACK :			/* Stack index register */
			ptr = "PUSH ?R1\n PUSH ?R2";
			stack_level += 2;
			byte = last_byte;
			local_stk[local_sp++] = local_idx;
			break;
		case _TO_TEMP :			/* Copy accumulator to temporary */
			ptr = byte ? "MOV R3,A" : "MOV R3,A\n MOV R4,B" ;
			break;
		case _FROM_INDEX :		/* Copy index register to accumulator */
			ptr = "MOV A,R1\n MOV B,R2";
			last_byte = byte = 0;
			local_acc = local_idx;
			break;
		case _COM :				/* Complement accumulator */
			ptr = byte ? " CPL A" : (rl_flags |= RL_COM, "LCALL ?com");
			break;
		case _NEG :				/* Negate accumulator */
			ptr = byte ? "CPL A\n INC A" : (rl_flags |= RL_NEG, "LCALL ?neg");
			break;
		case _NOT :				/* Logical NOT of ACC */
			eflag = -1;
			rl_flags |= RL_NOT;
			ptr = "LCALL ?not";
			break;
		case _INC :				/* Increment accumulator */
			if(isp16(rtype)) {
				rl_flags |= RL_INC;
				ptr = "LCALL ?inc2"; }
			else
				ptr = byte ? "INC A" : (rl_flags |= RL_INC, "LCALL ?inc");
			break;
		case _DEC :				/* Decrement accumulator */
			if(isp16(rtype)) {
				rl_flags |= RL_DEC;
				ptr = "LCALL ?dec2"; }
			else
				ptr = byte ? "DEC A" : (rl_flags |= RL_DEC, "LCALL ?dec");
			break;
		case _IADD :			/* Add accumulator to index */
			eflag = -1;
			ptr = "LCALL ?addi";
			r0 = 32767;
			rl_flags |= RL_ADDI;
			break;
		default:
			ptr = "?S?"; }

	if(eflag || !byte)
		expand(rtype);
	else
		last_byte = byte;

	out_inst(ptr);
}

/*
 * Perform an operation with the accumulator and
 * the specified value.
 */
accval(oper, rtype, token, value, type)
	unsigned oper, rtype, token, value, type;
{
	char *ptr, byte, rbyte, hold;

	byte = rbyte = ptr = 0;
	hold = 1;		/* For now, assume grab only external ref's */

	/* Values on the stack are always words */
	if(token == ON_STACK)
		type &= ~BYTE;

	/* determine the length of source & result */
	if((type & (BYTE | POINTER)) == BYTE)
		byte = -1;
	if((rtype & (BYTE | POINTER)) == BYTE)
		rbyte = last_byte;

	switch(oper) {
		case _LOAD :		/* Load the accumulator */
			local_acc = 0;
			if(token == ON_STACK) {
				out_inst("POP B\n POP A");
				stack_level -= 2;
				--local_sp;
				return; }
			oper_prep(token, value, type, 0);
			if(operand == DPTR)
				out_inst("MOVX A,[DPTR]");
			else if(operand == DPTRC)
				out_inst("CLR A\n MOVC A,[A+DPTR]");
			else {
				out_str(" MOV A,");
				write_oper(token, value, 2, "\n"); }
			if(!byte) {
				incoper();
				if(operand == DPTR) {
					out_inst("MOV B,A");
					out_inst("MOVX A,[DPTR]");
					out_inst("XCH A,B"); }
				else if(operand == DPTRC) {
					out_inst("MOV B,A");
					out_inst("CLR A\n MOVC A,[A+DPTR]");
					out_inst("XCH A,B"); }
				else {
					out_str(" MOV B,");
					write_oper(token, value, 1, "\n"); }
				last_byte = 0; }
			else
				last_byte = -1;
			return;
		case _STORE :		/* Store the accumulator */
			if(!rbyte)
				expand(rtype);
			oper_prep(token, value, type, 0);
			if(operand == DPTR)
				out_inst("MOVX [DPTR],A");
			else if(operand == DPTRC)
				line_error("Non-assignable");
			else {
				out_str(" MOV ");
				write_oper(token, value, 2, ",A\n"); }
			if(!byte) {
				incoper();
				if(operand == DPTR)
					out_inst("XCH A,B\n MOVX [DPTR],A\n XCH A,B");
				else {
					out_str(" MOV ");
					write_oper(token, value, 1, ",B\n"); } }
			return;
		case _ADD :			/* Add the 16 bit accumulator */
			ptr = byte ? rbyte ?
				"ADD A,}" :
				"ADD A,}\n XCH A,B\n ADDC A,#0\n XCH A,B" :
				"ADD A,}\n XCH A,B\n| ADDC A,{\n XCH A,B" ;
			break;
		case _SUB :			/* Subtract */
			ptr = byte ? rbyte ?
				"CLR C\n SUBB A,}" :
				"CLR C\n SUBB A,}\n XCH A,B\n SUBB A,#0\n XCH A,B" :
				"CLR C\n SUBB A,}\n XCH A,B\n| SUBB A,{\n XCH A,B";
			break;
		case _AND :			/* Logical AND */
			ptr = byte ? rbyte ?
				"ANL A,}" :
				"ANL A,}\n XCH A,B\n CLR A\n XCH A,B" :
				"ANL A,}\n XCH A,B\n| ANL A,{\n XCH A,B" ;
			break;
		case _OR :			/* Logical OR */
			ptr = byte ?
				"ORL A,}" :
				"ORL A,}\n XCH A,B\n| ORL A,{\n XCH A,B";
			break;
		case _XOR :			/* Exclusive OR */
			ptr = byte ?
				"XRL A,}" :
				"XRL A,}\n XCH A,B\n| XRL A,{\n XCH A,B";
			break;
		case _MULT:
			rbyte = 0;
			if((token == NUMBER) && (value == 2))	/* Fast *2 */
				ptr = "CLR C\n RLC A\n XCH A,B\n| RLC A\n XCH A,B";
			else {
				ptr = "LCALL ?mul";
				rl_flags |= RL_MUL;
				hold = 2; }
			break;
		case _DIV :
			rbyte = 0;
			if(rtype & UNSIGNED) {
				if((token == NUMBER) && (value == 2))	/* Fast /2 */
					ptr = "CLR C\n XCH A,B\n RRC A\n XCH A,B\n RRC A";
				else {
					ptr = "LCALL ?div";
					rl_flags |= RL_UDIV;
					hold = 2; }
				break; }
			rl_flags |= RL_SDIV;
			ptr = "LCALL ?sdiv";
			hold = 2;
			break;
		case _MOD :
			ptr = (rtype & UNSIGNED) ? "LCALL ?mod" : "LCALL ?smod";
			rl_flags |= RL_MOD;
			goto runtime;
		case _SHL :	ptr = "LCALL ?shl";	rl_flags |= RL_SHL; goto runtime;
		case _SHR :	ptr = "LCALL ?shr";	rl_flags |= RL_SHR; goto runtime;
		case _EQ :	ptr = "LCALL ?eq";	goto runtime;
		case _NE :	ptr = "LCALL ?ne";	goto runtime;
		case _LT :	ptr = "LCALL ?lt";	goto scomp;
		case _LE :	ptr = "LCALL ?le";	goto scomp;
		case _GT :	ptr = "LCALL ?gt";	goto scomp;
		case _GE :	ptr = "LCALL ?ge";
		scomp:		rl_flags |= RL_SCMP;
					goto runtime;
		case _ULT :	ptr = "LCALL ?ult";	goto ucomp;
		case _ULE :	ptr = "LCALL ?ule";	goto ucomp;
		case _UGT :	ptr = "LCALL ?ugt";	goto ucomp;
		case _UGE :	ptr = "LCALL ?uge";
		ucomp:		rl_flags |= RL_UCMP;
		runtime:
			rbyte = 0;		/* Insure we work with 16 bits */
			hold = 2;		/* Get result in temporary register */
			break;
		default:
			ptr = "?D?"; }

	/* If necessary, extend the ACC before executing the instruction */
	if(!rbyte)
		expand(rtype);
	else
		last_byte = rbyte;

	oper_prep(token, value, type, hold);	/* Set up addressing */

	out_sp();
	while(*ptr) {
		if(*ptr == '|') {	/* Advance to part 2 */
			incoper();
			++ptr;
			continue; }
		if(*ptr == '^')			/* Use entire operand */
			write_oper(token, value, 0, "");
		else if(*ptr == '{')	/* Use first portion */
			write_oper(token, value, 1, "");
		else if(*ptr == '}')	/* Use second portion */
			write_oper(token, value, 2, "");
		else
			out_chr(*ptr);
		++ptr; }

	out_nl();
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
