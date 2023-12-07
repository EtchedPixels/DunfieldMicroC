/*
 * DDS MICRO-C compiler
 *
 * This is the main "compile" module which contains the statement analyser,
 * input scanner, expression parser, and symbol table management routines.
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 *
 ** Notes concerning portability are identified by **
 */

#ifndef LINE_SIZE	/* if stand alone, include headers */
#include "compile.h"
#include "tokens.h"
#endif

/*
 * Tokens recognized by the parser:
 *
 * MAKE SURE that this list agrees with the definitions in "tokens.h".
 */
static char *tokens[] = {
	";", ",", ":", "{", "}", "(", ")", "[", "]", "++", "--", ".", "->",
	"+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=",
	"&&", "||", "+", "-", "*", "/", "%", "&", "|", "^",
	"<<", ">>", "<=", ">=", "<", ">", "==", "!=", "=", "?" , "!", "~",
	"int", "unsigned", "char", "static", "extern", "const", "register",
	"if", "else", "while", "do", "for", "switch", "case",
	"default", "return", "break", "continue", "goto", "sizeof",
	"asm", "struct", "union", "void", 0 };			/* end of table */

/* Table defining expression operator precedence */
static char priority[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 15, 15, 15,	/* SEMI to DASHP */
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,				/* ADDE to SHRE */
	5, 4, 12, 12, 13, 13, 13, 8, 6, 7,			/* DAND to XOR */
	11, 11, 10, 10, 10, 10, 9, 9, 2, 3 };		/* SHL to QUEST */

/* File tracking variables */
char file_name[INCL_DEPTH][FILE_SIZE+1];
unsigned file_line[INCL_DEPTH], file_depth = 0, line_number = 0;

/* Input buffer & input pointer */
char input_line[LINE_SIZE], *input_pos;

/* Global value storage locations (results from scanner) */
char gsymbol[SYMBOL_SIZE+1];
unsigned gvalue;

/* Macro define tables and associated variables */
char *define_index[MAX_DEFINE], define_pool[DEFINE_POOL],
	*define_stack[DEF_DEPTH];
unsigned define_top = 0, define_ptr = 0, define_depth = 0;

/* Symbol table and associated variables */
char s_name[MAX_SYMBOL][SYMBOL_SIZE+1], arg_list[MAX_ARGS][SYMBOL_SIZE+1];
unsigned s_type[MAX_SYMBOL], s_index[MAX_SYMBOL], s_dindex[MAX_SYMBOL],
	global_top = 0, local_top = MAX_SYMBOL, arg_count, sptr, fptr;

/* Literal + Dimension pools and associated variables */
unsigned dim_top = 0, literal_top = 0, dim_pool[MAX_DIMS];
char literal_pool[LITER_POOL];

/* Expression evaluation stacks */
char expr_token[EXPR_DEPTH];
unsigned expr_value[EXPR_DEPTH], expr_type[EXPR_DEPTH], expr_ptr = 0;

/* Misc. global variables */
char in_function = 0, if_flag = 0, pend_opr = 0, union_flag, afix;
unsigned break_stack[LOOP_DEPTH], continue_stack[LOOP_DEPTH],
	switch_stack[MAX_SWITCH*2], exit_label, loop_ptr = 0, switch_ptr = 0,
	sdefault = 0, exit_flag = 0, next_lab = 0, ungot_token = 0,
	error_count = 0, function_count = 0, local_stack, struct_offset,
	pend_type;

/* Option flags */
char comment = 0,		/* Include 'C' source as comments */
	fold = 0,			/* Fold literal strings */
	line = 0;			/* Accept line numbers */

#ifdef DEMO
#include "c:\project\demonote.txt"
#endif

/*
 * Copy a string - Return pointer to zero terminator
 */
char *copy_string(dest, source)
	char *dest, *source;
{
	while(*dest = *source) {
		++dest;
		++source; }
	return dest;
}

/*
 * Test two strings for equality
 */
equal_string(str1, str2)
	char *str1, *str2;
{
	do {
		if(*str1 != *str2++)
		return 0; }
	while(*str1++);
	return -1;
}

/*
 * Determine if a character is an numeric digit
 */
is_num(chr)
	char chr;
{
	return (chr >= '0') && (chr <= '9');
}

/*
 * Determine if a character is a valid symbol character
 */
is_symbol(chr)
	char chr;
{
	return ((chr >= 'a') && (chr <= 'z'))
		|| ((chr >= 'A') && (chr <= 'Z'))
		|| (chr == '_') || is_num(chr);
}

/*
 * Determine if a character is a "skip" character
 */
is_skip(chr)
	char chr;
{
	return (chr == ' ') || (chr == '\t');
}

/*
 * Main compile loop, process statements until end of file
 */
compile()
{
	*(input_pos = input_line) = 0;
	def_module();
	for(;;)
		statement(get_token());
}

/*
 * Handle inline assembly language
 */
inline_asm()
{
	char c, f, *i;
	f = fold;
	fold = 0;
	if(test_next(STRING))
		do_asm(&literal_pool[literal_top = gvalue]);
	else {
		expect(OCB);
		for(;;) {
			read_line();
			i = input_pos;
			while(is_skip(c = *input_pos++));
			if(c == '}') {
				if(afix)
					*--input_pos = ';';
				break; }
			do_asm(i); } }
	fold = f;
	afix = 0;
}

/*
 * Process a language statement
 */
statement(token)
	unsigned token;
{
	unsigned a, b, c, d;

	test_exit();		/* generate any preceeding exit */

	switch(token) {		/* act upon the token */
		case SEMI:		/* ';' - null statement */
			check_func();
			return;
		case OCB:		/* '{' - begin a block */
			while((token = get_token()) != CCB)
				statement(token);
			break;
		case INT:		/* integer variable declaration */
		case UNSIGN:	/* unsigned variable declaration */
		case CHAR:		/* 8 bit variable declaration */
		case STAT:		/* static modifier */
		case EXTERN:	/* external modifier */
		case CONST:		/* constant modifier */
		case REGIS:		/* register modifier */
		case STRUCT:	/* Structure declaration */
		case UNION:		/* Union declaration */
		case VOID :		/* void declaration */
			declare(token, 0);
			break;
		case IF:
			check_func();
			expect(ORB);
			cond_jump(CRB, FALSE, a = ++next_lab);
			statement(get_token());
			if(test_next(ELSE)) {
				test_jump(b = ++next_lab);
				def_label(a);
				a = b;
				statement(get_token()); }
			test_exit();
			def_label(a);
			break;
		case WHILE:
			check_func();
			def_label(continue_stack[loop_ptr] = a = ++next_lab);
			break_stack[loop_ptr++] = b = ++next_lab;
			expect(ORB);
			cond_jump(CRB, FALSE, b);
			statement(get_token());
			test_jump(a);
			def_label(b);
			--loop_ptr;
			break;
		case DO:
			check_func();
			def_label(a = ++next_lab);
			continue_stack[loop_ptr] = b = ++next_lab;
			break_stack[loop_ptr++] = c = ++next_lab;
			statement(get_token());
			test_exit();
			def_label(b);
			expect(WHILE);
			cond_jump(SEMI, TRUE, a);
			def_label(c);
			--loop_ptr;
			break;
		case FOR:
			check_func();
			expect(ORB);
			if(!test_next(SEMI))			/* initial */
				evaluate(SEMI, 0);
			def_label(d = a = ++next_lab);
			break_stack[loop_ptr] = b = ++next_lab;
			if(!test_next(SEMI))			/* test */
				cond_jump(SEMI, FALSE, b);
			if(!test_next(CRB)) {			/* end */
				jump(c = ++next_lab, 0);
				def_label(d = ++next_lab);
				evaluate(CRB, 0);
				jump(a, 0);
				def_label(c); }
			continue_stack[loop_ptr++] = d;
			statement(get_token());
			test_jump(d);
			def_label(b);					/* exit */
			--loop_ptr;
			break;
		case SWITCH:
			check_func();
			a = sdefault;
			continue_stack[loop_ptr] = loop_ptr ? continue_stack[loop_ptr-1]:0;
			break_stack[loop_ptr++] = sdefault = b = ++next_lab;
			expect(ORB);
			evaluate(CRB, -1);
			expand(gvalue);
			check_void(gvalue);
			do_switch(c = ++next_lab);
			d = switch_ptr;
			statement(get_token());
			test_jump(b);
			def_label(c);
			while(switch_ptr > d) {
				init_static(LABEL, switch_stack[--switch_ptr], -1);
				init_static(NUMBER, switch_stack[--switch_ptr], -1); }
			init_static(NUMBER, 0, -1);
			init_static(LABEL, sdefault, -1);
			end_static();
			def_label(b);
			--loop_ptr;
			sdefault = a;
			break;
		case CASE:
			a = check_switch();
			if(switch_ptr >= (MAX_SWITCH*2))
				severe_error("Too many active cases");
			get_constant(&c);
			switch_stack[switch_ptr++] = c;
			switch_stack[switch_ptr++] = a;
			expect(COLON);
			break;
		case DEFAULT:
			sdefault = check_switch();
			expect(COLON);
			break;
		case RETURN:
			check_func();
			if(!test_next(SEMI)) {
				evaluate(SEMI, -1);
				check_void(gvalue);
				expand(gvalue); }
			exit_flag = exit_label ? exit_label : (exit_label = ++next_lab);
			break;
		case BREAK:
			check_loop(break_stack);
			break;
		case CONTIN:
			check_loop(continue_stack);
			break;
		case GOTO:
			check_func();
			expect_symbol();
			if(lookup_local())
				check_type(FUNCGOTO);
			else
				define_symbol(REFERENCE|EXTERNAL|FUNCGOTO, ++next_lab);
			expect(SEMI);
			exit_flag = s_dindex[sptr];
			break;
		case SYMBOL:	/* a symbol table entry */
			if(!in_function) {			/* global definition */
				declare(token, 0);
				break; }
			if(*input_pos == ':') {		/* label definition */
				check_func();
				++input_pos;
				if(lookup_local()) {
					check_type(FUNCGOTO);
					s_type[sptr] &= ~EXTERNAL; }
				else
					define_symbol(FUNCGOTO, ++next_lab);
				def_label(s_dindex[sptr]);
				break; }
		case ASM :
			afix = -1;
			if(!in_function) {
				inline_asm();
				expect(SEMI);
				break; }
		default:		/* expression evaluation */
			check_func();
			unget_token(token);
			evaluate(SEMI, 0); }
}

/*
 * Check that a loop is active & setup exit condition
 */
check_loop(stack)
	unsigned stack[];
{
	expect(SEMI);
	if(!(loop_ptr && (exit_flag = stack[loop_ptr-1])))
		line_error("No active loop");
}

/*
 * Check that a switch is active & allocate label
 */
check_switch()
{
	if(!sdefault)
		line_error("No active switch");
	def_label(++next_lab);
	return next_lab;
}

/*
 * Compile a jump only if last statement compiled was not an "exit"
 * statement ("return", "break", or "continue"). This prevents the
 * generation of an unaccessable jump following these statements.
 */
test_jump(label)
	unsigned label;
{
	if(test_exit())
		jump(label, -1);
}

/*
 * Test to see if the last statement compiled was an "exit"
 * statement, and if so, generate a jump to the appriopriate
 * label. This prevents generation of a spurious jump when a
 * "return" statement is used at the end of a function.
 */
test_exit()
{
	if(exit_flag) {
		jump(exit_flag, -1);
		return(exit_flag = 0); }
	return -1;
}

/*
 * Compile a conditional evaluation & jump
 */
cond_jump(term, cond, label)
	unsigned term, cond, label;
{
	unsigned token, value, type;

	expr_ptr = pend_opr = 0;
	sub_eval(term);
	pop(&token, &value, &type);
	check_void(type);

	if(token == NUMBER) {
		if(!((value != 0) ^ cond ^ test_not()))
			jump(label, -1); }
	else {
		if(token != IN_ACC)
			load(type, token, value, type);
		jump_if(cond ^ test_not(), label, -1); }
}

/*
 * Match a keyword from the input line
 */
char *match(ptr)
	char *ptr;
{
	char *ptr1;

	ptr1 = input_pos;
	while(*ptr) {
		if(*ptr++ != *ptr1++)
			return 0; }

	if(is_symbol(*ptr1))			/* if symbol continues, no match */
		return 0;

	while(is_skip(*ptr1))		/* skip trailing nulls */
		++ptr1;
	input_pos = ptr1;
	return ptr+1;
}

/*
 * Add an entry to the define (macro) pool
 */
char *add_pool(string)
	char *string;
{
	char *ptr;

	ptr = &define_pool[define_ptr];
	do {
		if(define_ptr >= DEFINE_POOL)
			severe_error("Macro space exhausted");
		define_pool[define_ptr++] = *string; }
	while(*string++);
	return ptr;
}

/*
 * Lookup macro in the macro definition table
 */
char *lookup_macro()
{
	unsigned i;
	char *ptr;

	for(i=0; i < define_top; ++i) {
		if(ptr = match(define_index[i]))
			return ptr; }
	return 0;
}

/*
 * Parse off string from the input line
 */
char *parse()
{
	char *dptr;

	dptr = input_line;
	while(is_skip(*input_pos))
		++input_pos;
	while((*input_pos) && !is_skip(*input_pos))
		*dptr++ = *input_pos++;
	*dptr = 0;

	return input_line;
}

/*
 * Allow a single token to be returned to the input stream
 */
unget_token(token)
	unsigned token;
{
	ungot_token = token + 1;
}

/*
 * Test the next token in the input stream, put it
 * back if its not the one we are looking for.
 */
test_next(token)
	unsigned token;
{
	unsigned token1;

	if((token1 = get_token()) == token)
		return -1;
	unget_token(token1);
	return 0;
}

/*
 * Get a token from the input stream, and return it as a simple value
 * (indicating type). If it a special token type (NUMBER, STRING, SYMBOL),
 * global variables "gvalue" and "gsymbol" are set to appriopriate values.
 */
get_token()
{
	unsigned i, j;
	char *ptr, *last_pos, chr;

/* if a token has been ungot, re-get it */
/* (gvalue & gsymbol) will not have changed */
	if(ungot_token) {
		i = ungot_token - 1;
		ungot_token = 0;
		return i; }

/* skip any leading blanks and comments */
test_token:	for(;;) {
		while(is_skip(chr = read_char()));
		if(chr == '/') {
			if(*input_pos == '*') {		/* Standard comment */
				++input_pos;
				skip_comment();
				continue; }
			if(*input_pos == '/') {		/* C++ style comment */
				*input_pos = 0;
				continue; } }
		--input_pos;
		break; }

/* lookup token in token table */
	last_pos = input_pos;				/* remember where we were */
	for(i=0; ptr = tokens[i]; ++i) {	/* look through entire table */
		while((chr = *input_pos) && (*ptr == chr)) {
			++ptr;
			++input_pos; }
		if(!*ptr) {						/* we found a token */
			if(is_symbol(*(ptr-1)) && is_symbol(*input_pos))
				break;					/* not a token */
			return i; }
		input_pos = last_pos; }			/* reset pointer */

/* we didn't find a token, check out special cases */
	input_pos = last_pos;
	gvalue = 0;

	if((chr = *input_pos) == '"')	{	/* string value */
		++input_pos;
		gvalue = literal_top;
cs:		do {
			if(literal_top >= LITER_POOL) {
#ifdef DEMO
				put_str(demo_text, 0);
#endif
				severe_error("String space exausted"); }
			literal_pool[literal_top++] = i = read_special('"'); }
		while(!(i & 0xff00));
/* Handle concatinated strings */
		while(is_skip(chr = read_char()));
		if(chr == '"') {
			--literal_top;
			goto cs; }
		--input_pos;
		if(fold) {
			for(i=0; i < gvalue; ++i) {
				ptr = literal_pool+i;
				last_pos = literal_pool + gvalue;
				j = literal_top - gvalue;
				while(*ptr++ == *last_pos++) {
					if(!--j) {
						literal_top = gvalue;
						gvalue = i;
						break; } } } }
		return STRING; }

	if(chr == 0x27) {					/* quoted value */
		++input_pos;
		while(!((i = read_special(0x27)) & 0xff00))
			gvalue = (gvalue << 8) + i;
		return NUMBER; }

	if(is_num(chr)) {					/* numeric constant */
		if(chr == '0') {
			if(*++input_pos == 'x') {		/* hex */
				++input_pos;
				gvalue = get_number(16, 0); }
			else							/* octal */
				gvalue = get_number(8, 0); }
		else								/* decimal */
			gvalue = get_number(10, 0);
		return NUMBER; }

	if(is_symbol(chr)) {					/* macro or symbol name */
		if(ptr = lookup_macro()) {				/* macro name */
			if(define_depth >= DEF_DEPTH)
				severe_error("Macro expansion to deep");
			define_stack[define_depth++] = input_pos;
			input_pos = ptr;
			goto test_token; }
		while(is_symbol(chr = *input_pos)) {		/* symbol name */
			if(gvalue < SYMBOL_SIZE)
				gsymbol[gvalue++] = chr;
			++input_pos; }
		gsymbol[gvalue] = 0;
		return SYMBOL; }

/* not a token or special value */
	++input_pos;			/* skip offending character */
	return -1;				/* report "unknown" token type */
}

/*
 * Get a number in a number base for a maximum # of digits
 * (digits = 0 means no limit)
 */
get_number(base, digits)
	unsigned base, digits;
{
	unsigned value, c;

	value = 0;
	do {
		if(is_num(c = *input_pos))		/* convert numeric digits */
			c -= '0';
		else if(c >= 'a')				/* convert lower case alphabetics */
			c -= ('a' - 10);
		else if(c >= 'A')				/* convert upper case alphabetics */
			c -= ('A' - 10);
		else							/* not a valid "digit" */
			break;
		if(c >= base)					/* outside of base */
			break;
		value = (value * base) + c;		/* include in total */
		++input_pos; }
	while(--digits);					/* enforce maximum digits */
	return value;
}

/*
 * Skip past comments in the input stream
 *
 ** 32 bit compiler users... you must insure that 'x' will only
 ** contain the last 2 characters read from the input file.
 */
skip_comment()
{
	unsigned x;

	x = 0;
	do
		if((x = (x << 8) + read_char()) == (('/' << 8) + '*')) {
			skip_comment();
			x = 0; }
	while(x != (('*' << 8) + '/'));
}

/*
 * Read special character (with translations)
 */
read_special(delim)
	char delim;
{
	int c;

	if((c = read_char()) == delim)
		return 0xff00;
	if(c == '\\') switch(c = read_char()) {
		case 'n':				/* newline */
			c = 0x0a;
			break;
		case 'r':				/* return */
			c = 0x0d;
			break;
		case 't':				/* tab */
			c = 0x09;
			break;
		case 'f' :				/* formfeed */
			c = 0x0c;
			break;
		case 'b':				/* backspace */
			c = 0x08;
			break;
		case 'x' :				/* hex value */
			c = get_number(16, 2);
			break;
		default:
			if(is_num(c)) {		/* octal value */
				--input_pos;
				c = get_number(8, 3); } }

	return c & 0xff;
}

/*
 * Read a character from the input file
 */
read_char()
{
	char c;

	while(!(c = *input_pos++)) {		/* end of this line */
		if(define_depth)				/* return from macro input */
			input_pos = define_stack[--define_depth];
		else							/* read another line from file */
			read_line(); }

	return c;
}

/*
 * Read a line & interpret preprocessor commands.
 * At end of file, backup through any "#includes", and at the top,
 * end compilation!
 */
read_line()
{
	unsigned i;

	++line_number;

/* If end of file, back up through include files */
	if(get_lin(input_pos = input_line)) {	/* end of file encountered */
		if(file_depth) {
			f_close();
			line_number = file_line[--file_depth];
			input_line[0] = 0;
			return; }

		/* End of entire program input has occured */
		if(in_function)
			severe_error("Unterminated function");
		def_literal(literal_pool, literal_top);		/* Dump literal pool */

		/* Generate external & uninitalized global variable definitions */
		for(sptr = 0; sptr < global_top; ++sptr) {
			if((i = s_type[sptr]) & EXTERNAL) {
				if(i & REFERENCE)
					def_extern(sptr); }
			else if(!(i & (SYMTYPE | INITED)))
				def_global(sptr, size_of_var(sptr)); }

		end_module();
		terminate(error_count); }

/* Output 'C' source as comments if required */
	if(comment) {
		test_exit();
		do_comment(input_line); }

/* If present (& enabled), decode line numbers */
	if(line && is_num(*input_line)) {
		i = get_number(10, 0);
		if(*input_pos == ':') {
			++input_pos;
			line_number = i; }
		else
			input_pos = input_line; }

/* handle '#' preprocessor directives */
	if(match("#define")) {					/* define statement */
		if(define_top >= MAX_DEFINE)
			severe_error("Too many defines");
		define_index[define_top++] = add_pool(parse());
		add_pool(parse()); }
	else if(match("#include")) {			/* include file */
		if(file_depth >= INCL_DEPTH)
			severe_error("Too many includes");
		if(f_open(input_pos = parse())) {
			file_line[file_depth++] = line_number;
			copy_string(file_name[file_depth], input_pos);
			line_number = 0; }
		else
			text_error("Unable to open", input_pos); }
	else if(match("#ifdef")) {				/* if symbol defined */
		if_flag = 2;
		if(!lookup_macro())
			skip_cond(); }
	else if(match("#ifndef")) {				/* if not defined */
		if_flag = 2;
		if(lookup_macro())
			skip_cond(); }
	else if(match("#else")) {				/* after an if */
		test_if(2);
		skip_cond(); }
	else if(match("#endif"))				/* end of if */
		test_if(1);
	else if(match("#file"))					/* Filename override */
		copy_string(file_name[file_depth], parse());
	else									/* normal input line */
		return;

	*(input_pos = input_line) = 0;	/* fake null line to force another read */
}

/*
 * Test #end/#else for validity
 */
test_if(flag)
	char flag;
{
	if(if_flag < flag)
		severe_error("Improper #else/#endif");
	if_flag = flag-1;
}

/*
 * Skip to end of a conditional section
 */
skip_cond()
{
	do {
		if(match("#else")) {
			test_if(2);
			return; }
		++line_number;
		if(get_lin(input_pos = input_line))		/* end of file */
			severe_error("Unterminated conditional"); }
	while(!match("#endif"));
	test_if(1);
}

/*
 * Check for a certain token occuring next in the input stream.
 * If it is not found, report an error.
 */
expect(token)
	unsigned token;
{
	if(!test_next(token))
		text_error("Expected", tokens[token]);
}

/*
 * Get a SYMBOL token & report error if not found
 */
expect_symbol()
{
	if(!test_next(SYMBOL))
		syntax_error();
}

/*
 * Get structure/member name & check its type
 */
get_smname(type)
	unsigned type;
{
	expect_symbol();
	special_name();
	if(lookup())
		check_type(type);
	else
		symbol_error("Unknown structure/member");
}

/*
 * Report an undefined symbol
 */
undef_error()
{
	symbol_error("Undefined");
}

/*
 * Report an error involving a freshly parsed symbol name
 *
 ** See note at beginning of 'special_name' function.
 */
symbol_error(msg)
	char *msg;
{
	*gsymbol &= 0x7f;	/* Remove high bit in-case special name */
	text_error(msg, gsymbol);
}

/*
 * Output an error message with quoted text
 */
text_error(msg, txt)
	char *msg, *txt;
{
	char emsg[50];

	copy_string(copy_string(copy_string(emsg, msg), ": "), txt);
	line_error(emsg);
}

/*
 * Report a general syntax error
 */
syntax_error()
{
	line_error("Syntax error");
}

/*
 * Report an error in indirection
 */
index_error()
{
	line_error("Illegal indirection");
}

/*
 * Report incompatible types
 */
type_error()
{
	line_error("Type clash");
}

/*
 * Report illegal pointer operations
 */
pointer_error()
{
	line_error("Illegal pointer operation");
}

/*
 * Report illegal void use
 */
check_void(type)
	unsigned type;
{
	if( (type & (TVOID|POINTER)) == TVOID)
		type_error();
}

/*
 * Report a compile error
 */
line_error(message)
	char *message;
{
	put_str(file_name[file_depth], 0);
	put_chr('(', 0);
	put_num(line_number, 0);
	put_str("): ", 0);
	put_str(message, 0);
	put_chr('\n', 0);

	if(++error_count == MAX_ERRORS)
		severe_error("Too many errors");
}

/*
 * Report a non-recoverable compile error
 */
severe_error(string)
	char *string;
{
	line_error(string);
	put_str("Compilation aborted\n", 0);
	terminate(-1);
}

/*
 * Get a type declaration with all modifiers
 */
get_type(token, type)
	unsigned token, type;
{
	for(;;) {
		switch(token) {
			case CHAR:
				type |= BYTE;
				break;
			case INT:
				type &= ~BYTE;
				break;
			case UNSIGN:
				type |= UNSIGNED;
				break;
			case STAT:
				type |= STATIC;
				break;
			case EXTERN:
				type |= EXTERNAL;
				break;
			case CONST :
				type |= CONSTANT;
				break;
			case REGIS:
				type |= REGISTER;
				break;
			case VOID :
				type |= TVOID;
				break;
			case STAR:		/* pointer reference */
				do {
					++type;
					if(!(type & POINTER))
						line_error("Too many pointer levels"); }
				while(test_next(STAR));
				return type;
			default:
				unget_token(token);
				return type; }
		token = get_token(); }
}

/*
 * Declare a symbol (function or variable)
 */
declare(token, type)
	unsigned token, type;
{
	char union_save;
	unsigned ssize, lstack_save;

	union_save = union_flag;
	ssize = 0;

	for(;;) {
		type = get_type(token, type);
		if(test_next(STRUCT)) {		/* Structure declaration */
			union_flag = 0;
			goto dostruct; }
		if(test_next(UNION)) {		/* Union declaration */
			union_flag = -1;
		dostruct:
			lstack_save = local_stack;
			if(test_next(OCB))
				ssize = define_structure();
			else {
				expect_symbol();
				special_name();
				if(lookup()) {
					check_type(STRUCTURE);
					ssize = s_dindex[sptr]; }
				else {
					define_symbol(STRUCTURE|BYTE|REFERENCE, 0);
					token = sptr;
					expect(OCB);
					ssize = s_dindex[token] = define_structure(); } }
			local_stack = lstack_save;
			union_flag = union_save;
			type |= BYTE;
			if(test_next(SEMI))
				return;
			while(test_next(STAR))
				++type; }
		expect_symbol();
		if((type & SYMTYPE) == MEMBER)
			special_name();
		switch(token = get_token()) {
		case ORB:	define_func(type);	break;	/* Function definition */
		case SYMBOL: syntax_error();	break;	/* Detect double name */
		default:								/* Variable */
			unget_token(token);
			if(in_function == FUNC_ARGS) {
				if(!(type & SYMTYPE))
					copy_string(arg_list[arg_count++], gsymbol);
				define_var(type, ssize);
				return; }
			check_void(type);
			define_var(type, ssize); }
		if(!test_next(COMMA)) {			/* No more definitions */
			test_next(SEMI);
			return; }
		type &= ~(POINTER | ARRAY);
		token = get_token(); }
}

/*
 * Define a structure template...
 *
 ** If you need structures to be an even number of bytes (for WORD
 ** ALIGNMENT), replace the "return size;" statement at the end of
 ** this function with:  return (size + 1) & -2;
 */
define_structure()
{
	unsigned token, size, save_offset;

	save_offset = struct_offset;
	struct_offset = size = 0;
	while((token = get_token()) != CCB) {
		declare(token, MEMBER|REFERENCE);
		if(struct_offset > size)
			size = struct_offset; }
	struct_offset = save_offset;

#ifdef WALIGN
	return (size+1) & -2;
#else
	return size;
#endif
}

/*
 * Define a variable
 */
define_var(type, ssize)
	unsigned type, ssize;
{
	unsigned token, value, index, size, i, j, k;
	char iflag, nflag, fflag, temp[SYMBOL_SIZE+1];

	if(in_function >= FUNC_CODE)
		line_error("Declaration must preceed code");

/* calculate base variable size */
	size = ((type & (BYTE | ARGUMENT | POINTER)) != BYTE) + 1;

/* evaluate any array indexes */
	copy_string(temp, gsymbol);
	i = dim_top;
	nflag = index = 0;
	while(test_next(OSB)) {			/* array definition */
		++index;
		++dim_top;
		if(test_next(CSB)) {		/* null definition */
			if(index != 1)
				line_error("Illegal null dimension");
			--nflag;
			dim_pool[dim_top] = value = 0;
			continue; }
		if(get_constant(&value) != NUMBER)
			line_error("Numeric constant required");
		size *= dim_pool[dim_top] = value;
		expect(CSB); }
	copy_string(gsymbol, temp);

/* for structures, add an extra dimension of structure size */
	if(ssize && !(type & POINTER)) {
		++index;
		size *= dim_pool[++dim_top] = ssize; }

	if(index) {						/* defining an array */
		type |= ARRAY;
		dim_pool[i] = index;
		if(++dim_top > MAX_DIMS)
			severe_error("Dimension table exhausted"); }

	if(test_next(ASSIGN))			/* initialized variable */
		type |= INITED;

	if(union_flag)					/* If a union, reset structure offset */
		struct_offset = 0;

	define_symbol(type, i);			/* Create the symbol table entry */

	struct_offset += size_of_var(sptr);		/* Advance offset */

	index = 0;
	j = (i = (type & (POINTER | BYTE)) != BYTE) + 1;

	if(type & INITED) {				/* force immediate allocation */
		k = sptr;
		if(in_function && !(type & STATIC))
			line_error("Illegal initialization");
		fflag = fold;
		if(!(type & POINTER))
			fold = 0;
		iflag = test_next(OCB);
		do {
			if(ssize) {
				if(test_next(CHAR)) {		/* Force 8-bit initialization */
					do_comment(i = 0);
					j = 1; }
				else if(test_next(INT)) {	/* Force 16-bit initialization */
					do_comment(0);
					i=1;
					j = 2; } }
			if(test_next(AND)) {			/* Special case to allow & */
				expect_symbol();
				if(!lookup())
					undef_error();
				token = SYMBOL;
				value = sptr; }
			else
				token = get_constant(&value);
			if(!index)
				def_static(k, ssize);
			if((token == STRING) && (j == 1)) {
				do {
					init_static(NUMBER, literal_pool[value], i);
					index += j; }
				while(++value < literal_top);
				literal_top = gvalue; }
			else {
				init_static(token, value, i);
				index += j; } }
		while(iflag && test_next(COMMA));
		if(iflag)
			expect(CCB);
		fold = fflag;

		if(nflag)		/* Fixup null dimension */
			size = (dim_pool[s_dindex[k]+1] = ((index+size)-1)/size) * size;

		if(index > size)
			line_error("Too many initializers");

		if(i && (size & 1)) {		/* Align if necessary ... */
			do_comment(i = 0);
			j = 1; }
		while(size > index) {		/* And fill uninitialized storage */
			init_static(NUMBER, 0, i);
			index += j; }

		end_static(); }

	if(!(type & (STATIC|EXTERNAL)))		/* Allocate stack space for locals */
		local_stack += ((type & ARGUMENT) ? 2 : size);
}

/*
 * Define a function
 */
define_func(type)
	unsigned type;
{
	unsigned token, dim_save;

/* do not allow functions within functions */
	if(in_function) {
		line_error("Illegal nested function");
		return; }

/* Make a symbol table entry for the function */
	define_symbol(type | FUNCGOTO, arg_count = 0);
	fptr = sptr;

/* Declare ANSI style arguments, remember K&R style for later */
	local_top = MAX_SYMBOL;
	dim_save = dim_top;
	in_function = FUNC_ARGS;
	if(!test_next(CRB)) {
		do if(!declare_arg()) {
			expect_symbol();
			copy_string(arg_list[arg_count++], gsymbol); }
		while(test_next(COMMA));
		expect(CRB); }

/* For null definition, assume external (for prototype) */
	unget_token(token = get_token());
	if((token == SEMI) || (token == COMMA))
		type = (s_type[fptr] |= EXTERNAL);

/* If not an external definition, compile the body of the function */
	if(!(type & EXTERNAL)) {
		++function_count;
		in_function = FUNC_VARS;	/* indicate inside a function */
		/* accept K&R style declarations for local arguments */
		while(declare_arg());		/* Declare arguments */

		/* Adjust argument offsets */
		--arg_count;
		for(token = local_top; token < MAX_SYMBOL; ++token) {
			if(!(s_type[token] & SYMTYPE))
				s_index[token] = (arg_count - s_index[token]) * 2; }

		/* Compile single statement into function */
		local_stack = exit_label = exit_flag = 0;
		statement(get_token());
		check_func();	/* insure enter gets written in null func */
		if(exit_flag != exit_label)	/* Clean up last transfer */
			test_exit();
		if(exit_label)				/* Return label is required */
			def_label(exit_label);
		end_func();

		/* Check for symbol errors, define any STATIC variables */
		while(local_top < MAX_SYMBOL) {
			copy_string(gsymbol, s_name[local_top]);
			if(!((token = s_type[local_top]) & REFERENCE)) {
				symbol_error("Unreferenced");
				--error_count; }	/* Only a warning */
			if((token & (EXTERNAL|SYMTYPE)) == (EXTERNAL|FUNCGOTO))
				symbol_error("Unresolved");
			if((token & (STATIC|INITED)) == STATIC)
				def_global(local_top, size_of_var(local_top));
			++local_top; } }

/* End definition, clear locals, return to global scope */
	in_function = exit_label = exit_flag = 0;
	local_top = MAX_SYMBOL;
	dim_top = dim_save;
}

/*
 * Test for an argument declaration & declare it if present
 */
declare_arg()
{
	int token;
	switch(token = get_token()) {
		case VOID:
			if(test_next(CRB)) {
				unget_token(CRB);
				return -1; }
		case INT:
		case UNSIGN:
		case CHAR:
		case CONST:
		case REGIS:
		case STRUCT:
		case UNION:
			declare(token, ARGUMENT);
			return -1; }
	unget_token(token);
	return 0;
}

/*
 * Enter a symbol in the symbol table with specified name & type
 */
define_symbol(type, dim_index)
	unsigned type, dim_index;
{
	unsigned index;

	if(in_function) {				/* within a function, use local */
		if(lookup_local()) {
			test_redefine(type, dim_index, "Duplicate Local");
			return; }
		sptr = --local_top;
		if(type & ARGUMENT) {		/* calculate argument variable index */
			for(index = 0; index < arg_count; ++index)
				if(equal_string(arg_list[index], gsymbol))
					goto found_index;
			symbol_error("Not an argument"); }
		index = (type & STATIC) ? function_count : local_stack; }
	else {							/* outside of function, use global */
		if(index = lookup_global()) {		/* symbol already exists */
			test_redefine(type, dim_index, "Duplicate global");
			return; }
		sptr = global_top++; }

found_index:
	if((type & SYMTYPE) == MEMBER)
		index = struct_offset;

	if(global_top > local_top) {
#ifdef DEMO
		put_str(demo_text, 0);
#endif
		severe_error("Symbol table full"); }

	/* Record symbol table entry */
	copy_string(s_name[sptr], gsymbol);
	s_type[sptr] = type;
	s_index[sptr] = index;
	s_dindex[sptr] = dim_index;
}

/*
 * Test for valid redefinition of a symbol
 */
test_redefine(type, dim_index, message)
	unsigned type, dim_index;
	char *message;
{
	unsigned i, rtype, rdim;

	rtype = s_type[sptr];

	/* Test for structure member redefinitions */
	if((type & SYMTYPE) == MEMBER) {
		message = "Inconsistent member type/offset";
		if((rtype != type) || (s_index[sptr] != struct_offset))
			goto fail;
		goto testdim; }

	/* Test for external symbol redefinitions */
	if((rtype & EXTERNAL) && !in_function) {
		message = "Inconsistent redeclaration";
		if( (rtype | (INITED|REFERENCE|EXTERNAL))
		==	(type  | (INITED|REFERENCE|EXTERNAL))) {
			s_type[sptr] = (rtype & REFERENCE) | type;
	/* Test for matching array dimensions */
	testdim:
			if(type & ARRAY) {
				rtype = 0;
				rdim = s_dindex[sptr];
				i = dim_pool[dim_index];
				do {
					if(dim_pool[rdim] != dim_pool[dim_index]) {
						if(dim_pool[rdim] || (rtype != 1))
							goto fail;
						dim_pool[rdim] = dim_pool[dim_index]; }
					++rtype;
					++rdim;
					++dim_index; }
				while(i--); }
			return; } }

	/* No excuse... Issue error message */
fail: symbol_error(message);
}

/*
 * Locate a symbol in the local symbol table
 */
lookup_local()
{
	unsigned i;

	i = MAX_SYMBOL;
	while(i > local_top)
		if(equal_string(gsymbol, s_name[--i]))
			return s_type[sptr = i] |= REFERENCE;
	return 0;
}

/*
 * Locate a symbol in the global symbol table
 */
lookup_global()
{
	unsigned i;

	for(i=0; i < global_top; ++i)
		if(equal_string(gsymbol, s_name[i]))
			return s_type[sptr = i] |= REFERENCE;
	return 0;
}

/*
 * Locate a symbol in local or global symbol tables
 */
lookup()
{
	return lookup_local() || lookup_global();
}

/*
 * Modify global symbol name to unique structure/member name space
 *
 ** As is, this function assumes that your environment has at least
 ** 8-bit characters, and uses 7-bit character representations. If
 ** this is not the case, modify the following code to append a special
 ** non-C character (like '$') to the symbol name, and remove the &=
 ** from the beginning of the 'symbol_error' function.
 */
special_name()
{
	*gsymbol |= 0x80;	/* Set high bit to indicate special name */
}

/*
 * Check type of last symbol looked up.
 */
check_type(type)
	unsigned type;
{
	if((s_type[sptr] & SYMTYPE) != type)
		symbol_error("Improper type of symbol");
}

/*
 * Check that we are within a function definition
 */
check_func()
{
	if(in_function) {
		if(in_function < FUNC_CODE) {	/* first invocation */
			in_function = FUNC_CODE;
			def_func(fptr, local_stack); } }
	else
		line_error("Incorrect declaration");
}

/*
 * Calculate the size of a variable
 */
size_of_var(index)
	unsigned index;
{
	unsigned type, size, i;

	if((type = s_type[index]) == STRUCTURE)
		return s_dindex[index];

	size = ((type & (POINTER | BYTE)) != BYTE) + 1;
	if(type & ARRAY) {
		type = dim_pool[i = s_dindex[index]];
		while(type--)
			size *= dim_pool[++i]; }
	return size;
}

/*
 * Get a constant value (NUMBER or STRING)
 * which can be evaluated at compile time.
 */
get_constant(value)
	unsigned *value;
{
	unsigned token, type;

	unget_token(do_oper(SEMI));

	pop(&token, value, &type);

	if((token != NUMBER) && (token != STRING))
		line_error("Constant expression required");

	return token;
}

/*
 * Evaluate a full expression at the highest level, and
 * insure that the result is loaded into the accumulator.
 */
evaluate(term, xpend)
	unsigned term;
	int xpend;
{
	unsigned token, value;

	expr_ptr = pend_opr = 0;
	sub_eval(term);
	pop(&token, &value, &gvalue);
	if(xpend || (pend_opr <= _NOT))
		load(gvalue, token, value, gvalue);
}

/*
 * Evaluate a sub expression & handle COMMA operator. This must
 * be done as a special case, because the function performed by
 * COMMA differs with the context of the expression, and can not
 * therefore be handled as a general operator.
 */
sub_eval(term)
	unsigned term;
{
	unsigned token;

	while((token = do_oper(SEMI)) == COMMA)
		pop(&token, &token, &token);

	unget_token(token);
	expect(term);
}

/*
 * Perform an operation & all higher priority operations.
 */
do_oper(token)
	unsigned token;
{
	unsigned token1, t, v, tt, tt1, exit_lab;

	/* Handle "special" binary operators which involve jumps */
	if((token == DAND) || (token == DOR) || (token == QUEST)) {
		pop(&t, &v, &tt);
		stack_register(STACK_ACC | STACK_IDX);
		if(t != IN_ACC)
			load(tt, t, v, tt);
		check_void(tt);
		exit_lab = ++next_lab;
		if(token == QUEST) {		/* conditional expression */
			jump_if(test_not(), token1 = ++next_lab, 0);
			sub_eval(COLON);
			pop(&t, &v, &tt);
			load(tt, t, v, tt);
			check_void(tt);
			jump(exit_lab, 0);
			def_label(token1); }
		else {						/* && and || */
			do_pending();
			expand(tt);				/* Incase mixed char/int */
			jump_if(token == DOR, exit_lab, 0); }
		token1 = do_oper(SEMI);
		pop(&t, &v, &tt1);
		load(tt1, t, v, tt1);
		check_void(tt1);
		def_label(exit_lab);
		push(IN_ACC, v, combine(tt, tt1));
		return token1; }

	get_value();			/* stack the value */

/* Handle operator precedence and grouping */
	token1 = get_token();	/* Look at next operator */
	while((token1 >= ADDE) && (token1 <= QUEST) && (priority[token1] >= priority[token])) {
		if((priority[token1] == priority[token]) && ((token >= ADD) && (token <= NE))) {
			do_binary(token);
			return do_oper(token1); }
		token1 = do_oper(token1); }

/* Perform the operation */
	if(token)
		do_binary(token);

	return token1;
}

/*
 * Push a value on the expression stack
 */
push(token, value, type)
	unsigned token, value, type;
{
	if(expr_ptr >= EXPR_DEPTH)
		severe_error("Expression stack overflow");

	expr_type[expr_ptr]		= type;
	expr_value[expr_ptr]	= value;
	expr_token[expr_ptr++]	= token;
}

/*
 * Pop a value from the expression stack
 */
pop(token, value, type)
	int *token, *value, *type;
{
	if(!expr_ptr)
		severe_error("Expression stack underflow");

	*token	= expr_token[--expr_ptr];
	*value	= expr_value[expr_ptr];
	*type	= expr_type[expr_ptr];
}

/*
 * Gets the next token and perform any processing
 * required to evaluate it.
 */
get_value()
{
	int ndim;
	unsigned token, t, v, tt, tt1, tt2, dptr;
	char flag, opflag;

	switch(token = get_token()) {
		case NUMBER:			/* a constant number */
			t = NUMBER;
			v = gvalue;
			tt = ((int)v < 0) ? UNSIGNED : 0;
			break;
		case STRING:			/* a literal string */
			t = STRING;
			v = gvalue;
			tt = BYTE | CONSTANT | 1;
			break;
		case SYMBOL:			/* symbol value */
			if(!lookup()) {						/* Not defined */
				if(test_next(ORB)) {			/* function, assume external */
					unget_token(ORB);
					t = in_function;
					in_function = 0;
					define_symbol(REFERENCE|EXTERNAL|FUNCGOTO, 0);
					in_function = t; }
				else							/* variable, report error */
					undef_error(); }
			t = SYMBOL;
			tt = s_type[v = sptr];
			break;
		case STAR:				/* pointer reference */
			get_value();
			pop(&t, &v, &tt);
			do_pending();
			stack_register(STACK_IDX);
			index_ptr(t, v, tt);
			t = INDIRECT;
			if(tt & POINTER)
				--tt;
			else
				index_error();
			break;
		case AND:				/* address of */
			get_value();
			pop(&t, &v, &tt);
			if(t == SYMBOL) {
				stack_register(STACK_ACC | STACK_IDX);
				index_adr(t, v, tt); }
			else if(t != INDIRECT)
				line_error("Invalid '&' operation");
			stack_register(STACK_ACC);
			accop(_FROM_INDEX, pend_opr = 0);
			tt = (tt + 1) & ~SYMTYPE;
			t = IN_ACC;
			break;
		case ORB:				/* typecast or sub-expression */
			switch(token = get_token()) {	/* typecast */
				case INT:		/* cast to integer */
				case UNSIGN:	/* cast in unsigned */
				case CHAR:		/* cast to character */
				case CONST:		/* cast to constant */
				case REGIS:		/* cast to register */
				case VOID:		/* cast to void */
					tt = get_type(token, 0) | REFERENCE;
					expect(CRB);
					get_value();
					pop(&t, &v, &tt1);
					tt |= tt1 & (SYMTYPE|ARGUMENT|STATIC);
					if(	((tt1 & (REGISTER|CONSTANT)) != (tt&(REGISTER|CONSTANT)))
					||	(((tt1&(BYTE|POINTER))==BYTE) != ((tt&(BYTE|POINTER))==BYTE))) {
						load(tt, t, v, tt1);
						t = IN_ACC;
						expand(tt); }
					break;
				default:					/* sub-expression */
					unget_token(token);
					sub_eval(CRB);
					pop(&t, &v, &tt);
					break; }
			break;
		case SIZEOF:			/* Sizeof operator */
			flag = 0;
			v = 1;
			while((token = get_token()) == ORB)
				++flag;
			switch(token) {
				case INT:		/* integer type */
				case UNSIGN:	/* unsigned type */
				case CHAR:		/* character type */
				case CONST:		/* constant type */
				case REGIS:		/* register type */
				case VOID:		/* void type */
					tt = get_type(token, 0);
					break;
				case STRUCT:	/* Structure type */
				case UNION:		/* Union type */
					get_smname(STRUCTURE);
					tt = s_type[sptr];
					v = s_dindex[sptr];
					break;
				case SYMBOL:	/* A variable name */
					if(lookup()) {
						check_type(VARIABLE);
					sizesym:
						if((tt = s_type[sptr]) & ARRAY) {
							dptr = s_dindex[sptr];
							ndim = dim_pool[dptr++];
							while(test_next(OSB)) {
								if(!(ndim--))
									index_error();
								++dptr;
								if(!test_next(CSB)) {
									get_constant(&tt1);
									expect(CSB); } }
							while(ndim--)
								v *= dim_pool[dptr++]; }
						if(test_next(DOT) || test_next(DASHP)) {
							get_smname(MEMBER);
							v = 1;
							goto sizesym; }
						break; }
					undef_error();
					break;
				default:
					syntax_error(); }
			v *= ((tt & (POINTER | BYTE)) != BYTE) + 1;
			while(flag--)
				expect(CRB);
			t = NUMBER;
			tt = 0;
			break;
		case ASM :				/* Inline assembly */
			stack_register(STACK_ACC | STACK_IDX);
			if(test_next(ORB)) {
				opflag = ndim = 0;
				do {
					if(opflag) {
						accop(_STACK, tt);
						ndim += 2; }
					token = do_oper(SEMI);
					pop(&t, &v, &tt);
					load(tt, t, v, tt);
					check_void(tt);
					opflag = -1; }
				while(token == COMMA);
				if(token != CRB)
					syntax_error();
				release_stack(ndim);
				expand(tt); }
			inline_asm();
			v = tt = 0;
			t = IN_ACC;
			break;
		default:				/* anything else (operators) */
			get_value();		/* look for a value */
			do_unary(token);	/* Perform operation */
			return; }

	do {
		opflag = 0;

	/* handle function calls */
		if(test_next(ORB)) {
			push(t, v, tt);
			stack_register(STACK_ACC | STACK_IDX);
			ndim = 0;
			if(!test_next(CRB)) {		/* evaluate function operands */
				do {
					token = do_oper(SEMI);
					pop(&t, &v, &tt);
					load(tt, t, v, tt);
					accop(_STACK, tt);
					check_void(tt);
					++ndim; }
				while(token == COMMA);
				if(token != CRB)
					syntax_error(); }
			pop(&t, &v, &tt);
			if(tt & REGISTER)			/* "register" function */
				load(0, NUMBER, ndim, 0);
			call(t, v, tt, ndim);
			t = IN_ACC;
			tt &= ~(SYMTYPE|ARRAY); }

	/* handle indexing operations */
		flag = 0;
		if(test_next(OSB)) {
			if(tt & ARRAY) {				/* array, get # dimensions */
				dptr = s_dindex[v];
				ndim = dim_pool[dptr++]; }
			else							/* pointer, fake # dims as 1 */
				dptr = ndim = 1;

			stack_register(STACK_ACC | STACK_IDX);
			push(t, v, tt);
			do {							/* we have an index to calculate */
				v = tt & (POINTER | ARRAY);
				token = ((tt & BYTE) && ((v < 2) || (v == ARRAY))) ? 1 : 2;
				--ndim;
				if(tt & ARRAY) {			/* array reference */
					t = ++dptr;
					if(!(v = ndim))			/* all indices given, load pointer */
						tt &= ~ARRAY;
					while(v--)
						token *= dim_pool[t++]; }
				else {
					if(tt & POINTER) {		/* pointer reference */
						--tt;
						if(flag) {			/* array of pointers */
							pop(&t, &v, &tt1);
							load(tt1, t, v, tt1);
							pop(&t, &v, &tt2);
							load_index(t, v, tt2);
							accop(_IADD, tt1);
							push(t = INDIRECT, v, tt);
							flag = 0; } }
					else					/* invalid indexing */
						index_error(); }
				sub_eval(CSB);
				if(token != 1) {			/* calculate element offset */
					push(NUMBER, token, 0);
					do_binary(STAR); }
				if(flag)
					do_binary(ADD);
				flag = -1; }
			while(test_next(OSB));

			pop(&token, &v, &tt1);
			load(t, token, v, tt1);
			pop(&t, &v, &tt2);
			load_index(t, v, tt2);
			accop(_IADD, tt1);
			t = INDIRECT;
			opflag = -1; }

	/* Handle structure member reference (pointer type) */
		if(test_next(DASHP)) {
			if(!(tt & POINTER))
				index_error();
			goto domember; }

	/* Handle structure member reference (array type) */
		if(test_next(DOT)) {
			if(!(tt & ARRAY))
				type_error();
			if(t != INDIRECT) {
			domember:
				load_index(t, v, tt); }
			get_smname(MEMBER);
			if(tt1 = s_index[v = sptr]) {				/* Adjust offset */
				load(0, NUMBER, tt1, 0);
				accop(_IADD, 0); }
			tt = ((tt & (CONSTANT|REGISTER)) | s_type[sptr]) & ~SYMTYPE;	/* Set new type */
			t = INDIRECT;
			opflag = -1;
			continue; }

	/* convert any array references to address values */
		if(tt & ARRAY) {
			tt = (tt + 1) & ~ARRAY;
			if(flag || !(tt & ARGUMENT)) {
				if((t != INDIRECT) && !flag) {
					stack_register(STACK_ACC | STACK_IDX);
					index_adr(t, v, tt); }
				accop(_FROM_INDEX, pend_opr = 0);
				t = IN_ACC; } }

	/* handle any post operators (++ and --) if present */
		if(test_next(INC)) {			/* post '++' */
			load(tt, partial_stack(t), v, tt);
			accop(_INC, tt);
			store(tt, t, v, pend_type = tt);
			pend_opr = _DEC;
			t = IN_ACC;
			check_void(tt & ~POINTER); }
		else if(test_next(DEC)) {		/* post '--' */
			load(tt, partial_stack(t), v, tt);
			accop(_DEC, tt);
			store(tt, t, v, pend_type = tt);
			pend_opr = _INC;
			t = IN_ACC;
			check_void(tt & ~POINTER); } }
	while(opflag);

	push(t, v, tt);
}

/*
 * Load the index register with the address
 * needed to perform an indexing operation.
 */
load_index(t, v, tt)
	unsigned t, v, tt;
{
	if((tt & ARGUMENT) || !(tt & ARRAY)) {	/* pointer or argument */
		stack_register(STACK_IDX);
		index_ptr(t, v, tt); }
	else if(t != INDIRECT) {				/* standard array */
		stack_register(STACK_IDX);
		index_adr(t, v, tt); }
}

/*
 * Evaluate a unary operation, if possible, evaluate constant expressions
 * into another constant. Produce code to perform operation if necessary.
 */
do_unary(oper)
	unsigned oper;
{
	unsigned token, value, type;
	char flag;

	pop(&token, &value, &type);
	flag = 0;
	/* Evaluate any operations that can be performed at compile time */
	if(token == NUMBER) {
		flag = -1;
		switch(oper) {
			case SUB :		/* unary minus */
				value = -value;
				break;
			case COM:		/* ones complement */
				value = ~value;
				break;
			case NOT:		/* logical complement */
				value = !value;
				break;
			default:
				flag = 0; } }

	/* Generate code to perform operation */
	if(!flag) {
		switch(oper) {
			case SUB:				/* unary minus */
				flag = _NEG;
				break;
			case COM:				/* ones complement */
				flag = _COM;
				break;
			case NOT:				/* logical complement */
				load(type, token, value, pend_type = type);
				pend_opr = _NOT;
				break;
			case INC:				/* '++' increment & store */
				load(type, partial_stack(token), value, type);
				accop(_INC, type);
				store(type, token, value, type);
				break;
			case DEC:				/* '--' decrement & store */
				load(type, partial_stack(token), value, type);
				accop(_DEC, type);
				store(type, token, value, type);
				break;
			default:
				syntax_error(); }
		if(flag) {
			if(type & POINTER)
				pointer_error();
			load(type, token, value, type);
			accop(flag, type); }
		token = IN_ACC; }

	push(token, value, type);

	check_void(type & ~POINTER);
}

/*
 * Evaluate a binnary operation, if possible, evaluate constant expressions
 * into another constant. Produce code to perform operation if necessary.
 */
do_binary(oper)
	unsigned oper;
{
	unsigned token, value, type, token1, value1, type1;
	unsigned atoken, avalue, atype, temp, ctype;
	int v, v1;
	char flag, uflag, oflag, swap, order;

	pop(&token1, &value1, &type1);
	pop(&token,  &value,  &type);

	flag = uflag = oflag = swap = order = atoken = 0;

	/* Unmodified constants assume the type of the other operand */
	if((token == NUMBER) && !type) {
		swap = -1;
		if(!(type1 & POINTER))
			type = type1 & ~TVOID; }
	if((token1 == NUMBER) && !type1) {
		swap = 0;
		if(!(type & POINTER))
			type1 = type & ~TVOID; }

	ctype = combine(type, type1);

	/* Use unsigned compares if needed */
	if((type | type1) & (POINTER | UNSIGNED))
		uflag = _ULT - _LT;

	/* Do any operations that can be performed at compile time */
	if((token == NUMBER) && (token1 == NUMBER) && (oper != DAND) && (oper != DOR)) {
		v = value;
		v1 = value1;
		switch(oper) {
			case ADD :
				value += value1;
				break;
			case SUB :
				value -= value1;
				break;
			case STAR:
				value *= value1;
				break;
			case DIV:
				value /= value1;
				v /= v1;
				goto ucomp;
			case MOD:
				value %= value1;
				break;
			case AND:
				value &= value1;
				break;
			case OR:
				value |= value1;
				break;
			case XOR:
				value ^= value1;
				break;
			case SHL:
				value <<= value1;
				break;
			case SHR:
				value >>= value1;
				break;
			case EQ:
				value = value == value1;
				break;
			case NE:
				value = value != value1;
				break;
			case LT :
				value = value < value1;
				v = v < v1;
				goto ucomp;
			case LE:
				value = value <= value1;
				v = v <= v1;
				goto ucomp;
			case GT:
				value = value > value1;
				v = v > v1;
				goto ucomp;
			case GE:
				value = value >= value1;
				v = v >= v1;
			ucomp:
				if(!uflag)
					value = v;
				break;
			default:
				syntax_error(); } }
	else {
		/* Generate code to perform the operation */
		avalue = value;
		atype = type;

		/* Try and rearrange for partial results already in ACC */
		if(token1 == IN_ACC)
			swap = -1;

		switch(oper) {
			case DAND:
			case DOR:
				push(token1, value1, type1);
				return;
			case ADDE:
				atoken = token;
			case ADD:
				flag = _ADD;
				break;
			case SUBE:
				atoken = token;
			case SUB:
				order = flag = _SUB;
				break;
			case STARE:
				atoken = token;
			case STAR:
				flag = _MULT;
				goto noptr;
			case DIVE:
				atoken = token;
			case DIV:
				order = flag = _DIV;
				goto noptr;
			case MODE:
				atoken = token;
			case MOD:
				order = flag = _MOD;
				goto noptr;
			case ANDE:
				atoken = token;
			case AND:
				flag = _AND;
				goto noptr;
			case ORE:
				atoken = token;
			case OR:
				flag = _OR;
				goto noptr;
			case XORE:
				atoken = token;
			case XOR:
				flag = _XOR;
				goto noptr;
			case SHLE:
				atoken = token;
			case SHL:
				order = flag = _SHL;
				goto noptr;
			case SHRE:
				atoken = token;
			case SHR:
				order = flag = _SHR;
			noptr: oflag = 1;
				break;
			case ASSIGN:
				load(ctype, token1, value1, type1);
				atoken = token;
				oflag = 2;
				break;
			case EQ:
				flag = _EQ;
				goto sett;
			case NE:
				flag = _NE;
				goto sett;
			case LT:
				flag = (swap ? _GT : _LT) + uflag;
				goto sett;
			case LE:
				flag = (swap ? _GE : _LE) + uflag;
				goto sett;
			case GT:
				flag = (swap ? _LT : _GT) + uflag;
				goto sett;
			case GE:
				flag = (swap ? _LE : _GE) + uflag;
			sett:
				oflag = 3;
				break;
			default:
				syntax_error(); }

		if(flag) {							/* simple arithmetic operations */
			if(atoken == ION_STACK)			/* if on stack, don't remove */
				token = ISTACK_TOP;
			if((token1 == IN_ACC) && order) {	/* out of order - use temp */
				do_pending();
				accop(_TO_TEMP, type1);
				token1 = IN_TEMP; }

	/* swap operands to generate more optimal code */
			if(swap && !order) {
				temp = token; token = token1; token1 = temp;
				temp = value; value = value1; value1 = temp;
				temp = type;  type  = type1;  type1  = temp; }

			if(type1 & SYMTYPE)
				type_error();
			load(ctype, token, value, type);	/* insure its loaded */
			accval(flag, ctype, token1, value1, type1); }

	/* perform any pending assignment */
		if(atoken)
			store(ctype, atoken, avalue, atype);

		token = IN_ACC; }

	/* Special cases for two pointer arithmetic */
	if((type & POINTER) && (type1 & POINTER)) {
		if(oper == SUB)		/* Only allow subtract */
			ctype = 0;
		else if(!oflag)		/* assignment, or compares */
			oflag = 1; }

	/* Clean up processing and error checking */
	switch(oflag) {
		case 3 :	/* Reset compare output type to int */
			ctype = 0;
		case 2 :	/* Void pointers allowed */
			check_void(type);
			check_void(type1);
			break;
		case 1 :	/* No pointers allowed */
			if(type & POINTER)
				pointer_error();
		default:	/* Everything else cannot be void */
			check_void(ctype & ~POINTER); }

	push(token, value, ctype);
}

/*
 * Combine two types (For binary operations)
 */
combine(type1, type2)
	unsigned type1, type2;
{
/* If one type is pointer and other is not, return same pointer type */
	if(type1 & POINTER) {
		if(type2 & POINTER)
			return (type1|type2)&(BYTE|UNSIGNED|POINTER|TVOID|CONSTANT|REGISTER);
		return type1; }
	if(type2 & POINTER)
		return type2;

/* preserve byte contents if both operands are 8 bit */
/* preserve unsigned indication if either is unsigned */
	return ((type1 & type2) & BYTE) | ((type1 | type2) & (UNSIGNED|TVOID));
}

/*
 * Generate the correct token for partial stack operations
 */
partial_stack(token)
	unsigned token;
{
	return (token == ION_STACK) ? ISTACK_TOP : token;
}

/*
 * Examine the expression stack & produce code to place any
 * value in the indicated registers on the processor stack.
 */
stack_register(rflags)
	char rflags;
{
	int i, aflag;

	aflag = -1;
	for(i=0; i < expr_ptr; ++i) {
		if(expr_token[i] == IN_ACC) {				/* Test accumulator */
			if(rflags & STACK_ACC) {
				do_pending();
				accop(_STACK, expr_type[i]);
				expr_token[i] = ON_STACK; }
			else
				aflag = i; }
		if((rflags & STACK_IDX) && (expr_token[i] == INDIRECT)) {	/* Test index */
			if(aflag != -1) {
				do_pending();
				accop(_STACK, expr_type[aflag]);
				expr_token[aflag] = ON_STACK; }
			accop(_ISTACK, expr_type[i]);
			expr_token[i] = ION_STACK; } }
}

/*
 * Test for a pending operation, and perform it
 */
do_pending()
{
	if(pend_opr) {
		accop(pend_opr, pend_type);
		pend_opr = 0; }
}

/*
 * Test for a NOT condition, required to invert a jump
 */
test_not()
{
	if(pend_opr == _NOT) {
		pend_opr = 0;
		return TRUE; }
	do_pending();
	return FALSE;
}

/*
 * Load the accumulator with a value if it is not already in it.
 * If a partial result is already in the accumulator, stack it
 * and load the new value.
 */
load(atype, token, value, type)
	unsigned atype, token, value, type;
{

	if(type & SYMTYPE)
		type_error();

	if(token != IN_ACC) {		/* loading new value */
		stack_register(STACK_ACC);
		pend_opr = 0;
		accval(_LOAD, atype, token, value, type); }
	else						/* using existing value */
		do_pending();
}

/*
 * Store accumulator value
 */
store(atype, token, value, type)
	unsigned atype, token, value, type;
{
	switch(token) {
		case SYMBOL:
		case INDIRECT:
		case ION_STACK:
		case ISTACK_TOP:
			if((type & (CONSTANT|POINTER)) != CONSTANT) {
				accval(_STORE, atype, token, value, type);
				break; }
		default:
			line_error("Non-assignable"); }
}
