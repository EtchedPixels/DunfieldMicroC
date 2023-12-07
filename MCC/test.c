/*
 * Test program for verifying the DDS MICRO-C code generator.
 *
 * This program makes use of the characteristics of MICRO-C internal
 * allocation, and may not execute properly if compiled with another
 * compiler (Or even some MICRO-C code generators).
 *
 * NOTE: requires a 'putstr' functions to display a string.
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

int i, j;
unsigned u;
int agi[10], gi;
char agc[10], gc;

main()
{
	putstr("Global variables:\n");
	test_global();
	putstr("\nLocal variables:\n");
	test_local();
	putstr("\nExpressions:\n");
	test_expr();
	putstr("\nSigned compares +:\n");
	i = 128; test_scomp();
	putstr("\nSigned compares -:\n");
	i = -128; test_scomp();
	putstr("\nUnsigned compares (<32768):\n");
	u = 32767; test_ucomp();
	putstr("\nUnsigned compares (>32767):\n");
	u = 32768; test_ucomp();
	putstr("\nPointers:\n");
	test_ptr();
	putstr("\nFunction calls:\n");
	gi = 5678;
	gc = 123;
	test_args(4321, gi, gc, agi, agc, agi, agc);
	test_switch();
}

/*
 * Test global variable allocation
 */
test_global()
{
	putstr("INT Array Allocation....");
	for(i=0; i < 10; ++i)
		agi[i] = 10 - i;
	for(i = 0; i < 10; ++i) {
		if(agi[i] != 10 - i)
			break; }
	if(i == 10)
		pass();
	else
		fail();

	putstr("CHAR Array Allocation...");
	for(i=0; i < 10; ++i)
		agc[i] = 10 - i;
	for(i = 0; i < 10; ++i) {
		if(agc[i] != 10 - i)
			break; }
	if(i == 10)
		pass();
	else
		fail();

	putstr("INT Array bounds........");
	gi = 0;
	agi[10] = 1234;
	if(gi != 1234)
		fail();
	else
		pass();

	putstr("CHAR Array bounds.......");
	gc = 0;
	agc[10] = 99;
	if(gc != 99)
		fail();
	else
		pass();
}

/*
 * Test local variable allocation
 */
test_local()
{
	int ali[10], li;
	char alc[10], lc;

	putstr("INT Array Allocation....");
	for(i=0; i < 10; ++i)
		ali[i] = 10 - i;
	for(i = 0; i < 10; ++i) {
		if(ali[i] != 10 - i)
			break; }
	if(i == 10)
		pass();
	else
		fail();

	putstr("CHAR Array Allocation...");
	for(i=0; i < 10; ++i)
		alc[i] = 10 - i;
	for(i = 0; i < 10; ++i) {
		if(alc[i] != 10 - i)
			break; }
	if(i == 10)
		pass();
	else
		fail();

	putstr("INT Array bounds........");
	li = 0;
	ali[10] = 1234;
	if(li != 1234)
		fail();
	else
		pass();

	putstr("CHAR Array bounds.......");
	lc = 0;
	alc[10] = 99;
	if(lc != 99)
		fail();
	else
		pass();
}

/*
 * Test expression handling
 */
test_expr()
{
	i = 5;
	j = 9;
	putstr("Addition................");
	if((i + j) == (5 + 9))
		pass();
	else
		fail();

	putstr("Subtraction.............");
	if((j - i) == (9 - 5))
		pass();
	else
		fail();

	putstr("Multiplication..........");
	if((i * j) == (5 * 9))
		pass();
	else
		fail();

	putstr("Division................");
	j = 25;
	if((j / i) == (25 / 5))
		pass();
	else
		fail();

	putstr("Modulus.................");
	j = 54;
	if((j % i) == (54 % 5))
		pass();
	else
		fail();

	putstr("Shift Left..............");
	j = 170;
	if((j << i) == (170 << 5))
		pass();
	else
		fail();

	putstr("Shift Right.............");
	if((j >> i) == (170 >> 5))
		pass();
	else
		fail();

	putstr("Bitwise AND.............");
	i = 15;
	if((j & i) == (170 & 15))
		pass();
	else
		fail();

	putstr("Bitwise OR..............");
	if((j | i) == (170 | 15))
		pass();
	else
		fail();

	putstr("Bitwise XOR.............");
	if((j ^ i) == (170 ^ 15))
		pass();
	else
		fail();
}

/*
 * Test signed compare operators
 */
test_scomp()
{
	int e, l, g;
	e = i;
	l = i-1;
	g = i+1;
	putstr("== .....................");
	if(i == e) pass(); else fail();

	putstr("!= .....................");
	if(i != e) fail(); else pass();

	putstr("< ......................");
	if((i < g) && !(i < e))
		pass(); else fail();

	putstr("<= .....................");
	if((i <= g) && (i <= e) && !(i <= l))
		pass(); else fail();

	putstr("> ......................");
	if((i > l) && !(i > e))
		pass(); else fail();

	putstr(">= .....................");
	if((i >= l) && (i >= e) && ! (i >= g))
		pass(); else fail();
}

/*
 * Test unsigned compare operators
 */
test_ucomp()
{
	unsigned e, l, g;
	e = i;
	l = i-1;
	g = i+1;
	putstr("== .....................");
	if(i == e) pass(); else fail();

	putstr("!= .....................");
	if(i != e) fail(); else pass();

	putstr("< ......................");
	if((i < g) && !(i < e))
		pass(); else fail();

	putstr("<= .....................");
	if((i <= g) && (i <= e) && !(i <= l))
		pass(); else fail();

	putstr("> ......................");
	if((i > l) && !(i > e))
		pass(); else fail();

	putstr(">= .....................");
	if((i >= l) && (i >= e) && ! (i >= g))
		pass(); else fail();
}

/*
 * Test pointer operations
 */
test_ptr()
{
	int *ptr;
	char *ptr1;

	putstr("Pointer to INT..........");
	for(i = 0; i < 10; ++i)
		agi[i] = i;
	ptr = agi;
	for(i = 0; i < 10; ++i)
		if(agi[i] != *ptr++)
			break;
	if(i == 10)
		pass();
	else
		fail();

	putstr("Pointer to CHAR.........");
	for(i = 0; i < 10; ++i)
		agc[i] = i;
	ptr1 = agc;
	for(i = 0; i < 10; ++i)
		if(agc[i] != *ptr1++)
			break;
	if(i == 10)
		pass();
	else
		fail();
}

/*
 * Test arguments to function
 */
test_args(a, b, c, ptr, ptr1, ali, alc)
	unsigned a;
	int b;
	char c;
	int *ptr;
	char *ptr1;
	int ali[];
	char alc[];
{
	putstr("Argument constant.......");
	if(a == 4321)
		pass();
	else
		fail();

	putstr("Argument INT............");
	if(b == 5678)
		pass();
	else
		fail();

	putstr("Argument CHAR...........");
	if(c == 123)
		pass();
	else
		fail();

	putstr("Argument INT pointer....");
	for(i=0; i < 10; ++i)
		if(*ptr++ != i)
			break;
	if(i == 10)
		pass();
	else
		fail();

	putstr("Argument CHAR pointer...");
	for(i=0; i < 10; ++i)
		if(*ptr1++ != i)
			break;
	if(i == 10)
		pass();
	else
		fail();

	putstr("Argument INT array......");
	for(i=0; i < 10; ++i)
		if(ali[i] != i)
			break;
	if(i == 10)
		pass();
	else
		fail();

	putstr("Argument CHAR array.....");
	for(i=0; i < 10; ++i)
		if(alc[i] != i)
			break;
	if(i == 10)
		pass();
	else
		fail();
}

/*
 * Test switch statement
 */
test_switch()
{
	int i;
	putstr("\nSwitch Statements.......");
	for(i=0; i < 10; ++i) switch(i) {
		case 0 :
			if(i == 0)
				break;
			fail();
			return;
		case 1 :
			if(i == 1)
				break;
			fail();
			return;
		case 5 :
			if(i == 5)
				break;
			fail();
			return;
		case 8 :
			if(i == 8)
				break;
			fail();
			return;
		default:
			if((i == 0) || (i == 1) || (i == 5) || (i == 8)) {
				fail();
				return; } }
	if(i == 10)
		pass();
	else
		fail();
}

/*
 * Indicate operation passed
 */
pass()
{
	putstr("Passed\n");
}

/*
 * Indicate operation failed
 */
fail()
{
	putstr("Failed\n");
	getchr();
}
