/*
 * List of tokens supported by the DDS MICRO-C compiler:
 *
 * MAKE SURE that this list agrees with the "tokens"
 * table in the module "compile.c"
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#define SEMI	0		/* ; */
#define COMMA	1		/* , */
#define COLON	2		/* : */
#define OCB		3		/* { */
#define CCB		4		/* } */
#define ORB		5		/* ( */
#define CRB		6		/* ) */
#define OSB		7		/* [ */
#define CSB		8		/* ] */
#define INC		9		/* '++' */
#define DEC		10		/* '--' */
#define	DOT		11		/* '.' */
#define	DASHP	12		/* '->' */
#define ADDE	13		/* '+='	(Marks start of binaries) */
#define SUBE	14		/* '-=' */
#define STARE	15		/* '*=' */
#define DIVE	16		/* '/=' */
#define MODE	17		/* '%=' */
#define ANDE	18		/* '&=' */
#define ORE		19		/* '|=' */
#define XORE	20		/* '^=' */
#define SHLE	21		/* '<<=' */
#define SHRE	22		/* '>>=' */
#define DAND	23		/* '&&' */
#define DOR		24		/* '||' */
#define ADD		25		/* '+' */
#define SUB		26		/* '-' - subtract & negate */
#define STAR	27		/* '*' - multiply & pointer */
#define DIV		28		/* '/' */
#define MOD		29		/* '%' */
#define AND		30		/* '&' - and & address of */
#define OR		31		/* '|' */
#define XOR		32		/* '^' */
#define SHL		33		/* '<<' */
#define SHR		34		/* '>>' */
#define LE		35		/* '<=' */
#define GE		36		/* '>=' */
#define LT		37		/* '<' */
#define GT		38		/* '>' */
#define EQ		39		/* '==' */
#define NE		40		/* '!=' */
#define ASSIGN	41		/* '=' */
#define QUEST	42		/* '?' (marks end of binaries) */
#define NOT		43		/* '!' */
#define COM		44		/* '~' */
#define INT		45		/* 'int' */
#define UNSIGN	46		/* 'unsigned' */
#define CHAR	47		/* 'char' */
#define STAT    48		/* 'static' */
#define EXTERN	49		/* 'extern' */
#define	CONST	50		/* 'const' */
#define REGIS	51		/* 'register' */
#define IF		52		/* 'if' statement */
#define ELSE	53		/* 'else' modifier */
#define WHILE	54		/* 'while' statement */
#define DO		55		/* 'do' statement */
#define FOR		56		/* 'for' statement */
#define SWITCH	57		/* 'switch' statement */
#define CASE	58		/* 'case' statement */
#define DEFAULT	59		/* 'default' statement */
#define RETURN	60		/* 'return' statement */
#define BREAK	61		/* 'break' statement */
#define CONTIN	62		/* 'continue' statement */
#define GOTO	63		/* 'goto' statement */
#define	SIZEOF	64		/* 'sizeof' operator */
#define ASM		65		/* 'asm' statement */
#define	STRUCT	66		/* 'struct' */
#define	UNION	67		/* 'union' */
#define	VOID	68		/* 'void' */
