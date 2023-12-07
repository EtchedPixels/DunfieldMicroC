/*
 * Global definitions for the DDS MICRO-C compiler.
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

#define FALSE		0
#define TRUE		1

/*
 * Misc. fixed compiler parameters.
 *
 * When porting to systems with limited memory, you may have to reduce
 * the size of some of the tables in order to make it fit. The most
 * suitable candidates for memory reduction are (in this order):
 *
 *	LITER_POOL, SYMBOL_SIZE, MAX_SYMBOL, DEFINE_POOL, MAX_DEFINE
 */
#define LINE_SIZE	250			/* size of input line */
#define	FILE_SIZE	64			/* maximum size of file name */
#define SYMBOL_SIZE	15			/* # significant chars in symbol name */
#define INCL_DEPTH	5			/* maximum depth of include files */
#define DEF_DEPTH	5			/* maximum depth of define macros */
#define EXPR_DEPTH	20			/* maximum depth of expression stack */
#define LOOP_DEPTH	10			/* maximum depth of nested loops */
#define MAX_ARGS	25			/* maximum # arguments to a function */
#define MAX_SWITCH	50			/* maximum # active switch-case statements */
#define MAX_ERRORS	10			/* # error before termination forced */
#define MAX_DIMS	500			/* maximum # active array dimensions */
#define MAX_DEFINE	150			/* maximum # define sumbols */
#define DEFINE_POOL 2000		/* size of define string space */

#ifndef DEMO
	#define MAX_SYMBOL	1000	/* maximum # active symbols */
	#define LITER_POOL	20000	/* size of literal string space */
#else
	#define MAX_SYMBOL	50		/* maximum # active symbols */
	#define LITER_POOL	500		/* size of literal string space */
	#define	MAX_LINES	500		/* Maximum # source lines */
#endif

/*
 * Bits found in the "type" entry of symbol table, also
 * used on the expression stack to keep track of element types,
 * and passed to the code generation routines to identify types.
 *
 * The POINTER definition indicates the level of pointer indirection,
 * and should contain all otherwise unused bits, in the least significant
 * (rightmost) bits of the word.
 */
#define REFERENCE	0x8000		/* symbol has been referenced */
#define INITED		0x4000		/* symbol is initialized */
#define ARGUMENT	0x2000		/* symbol is a function argument */
#define EXTERNAL	0x1000		/* external attribute */
#define STATIC		0x0800		/* static attribute */
#define REGISTER	0x0400		/* register attribute */
#define	TVOID		0x0200		/* void attribute */
#define UNSIGNED	0x0100		/* unsigned attribute */
#define BYTE		0x0080		/* 0=16 bit, 1=8 bit */
#define	CONSTANT	0x0040		/* constant attribute */
#define	SYMTYPE		0x0030		/* symbol type (see below) */
#define ARRAY		0x0008		/* symbol is an array */
#define POINTER		0x0007		/* level of pointer indirection */

/*
 * Symbol type designator bits (SYMTYPE field above)
 */
#define	VARIABLE	0x00		/* Symbol is a simple variable */
#define	MEMBER		0x10		/* Symbol is a structure member */
#define	STRUCTURE	0x20		/* Symbol is a structure template */
#define	FUNCGOTO	0x30		/* function(global) or gotolabel(local) */

/*
 * Non-literal tokens identifing value types.
 */
#define NUMBER		100			/* numeric constant */
#define STRING		101			/* literal constant */
#define LABEL		102			/* label address */
#define SYMBOL		103			/* symbol value */
#define IN_ACC		104			/* value in accumulator */
#define IN_TEMP		105			/* value is in temporary register */
#define INDIRECT	106			/* indirect through index register */
#define ON_STACK	107			/* value in on stack */
#define ION_STACK	108			/* indirect through stack top */
#define ISTACK_TOP	109			/* indirect through stack top, leave on */

/*
 * Function compilation state of the parser (0 = No function)
 */
#define	FUNC_ARGS	1			/* Declaring arguments */
#define	FUNC_VARS	2			/* Declaring local variables */
#define	FUNC_CODE	3			/* Outputing code */

/*
 * Register stacking identifiers
 */
#define	STACK_ACC	0x01		/* Stack the accumulator */
#define	STACK_IDX	0x02		/* Stack the index register */

/*
 * No operand operations passed to the
 * code generation routine "accop".
 */
#define _STACK		0			/* place accumulator on stack */
#define _ISTACK		1			/* place index register on stack */
#define _TO_TEMP	2			/* place acc in temporary location */
#define _FROM_INDEX	3			/* get acc from index */
#define _COM		4			/* one's complement acc */
#define _NEG		5			/* two's complement acc */
#define _NOT		6			/* logical complement acc */
#define _INC		7			/* Increment accumulator */
#define _DEC		8			/* decrement accumulator */
#define _IADD		9			/* Add accumulator to index */

/*
 * One operand accumulator operations passed
 * to the code generation routine "accval".
 */
#define _LOAD		0			/* Load register contents */
#define _STORE		1			/* Store the register value */
#define _ADD		2			/* Addition */
#define _SUB		3			/* Subtraction */
#define _MULT		4			/* Multiplication */
#define _DIV		5			/* Division */
#define _MOD		6			/* Modular division */
#define _AND		7			/* Logical AND */
#define _OR			8			/* Logical OR */
#define _XOR		9			/* Exclusive OR */
#define _SHL		10			/* Shift left */
#define _SHR		11			/* Shift right */
#define _EQ			12			/* Test for equals */
#define _NE			13			/* Test for not equals */
#define _LT			14			/* Test for less than */
#define _LE			15			/* Test for less than or equals */
#define _GT			16			/* Test for greater than */
#define _GE			17			/* Test for greater than or equals */
#define _ULT		18			/* Unsigned LT */
#define _ULE		19			/* Unsigned LE */
#define _UGT		20			/* Unsigned GT */
#define _UGE		21			/* Unsigned GE */
