/*
 * DDS MICRO-C IBM/PC Supplementary long math functions
 *
 * ?COPY.TXT 1994-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#define LSIZE   4       /* 32 bit numbers */

extern char Longreg[];  /* Contains remainder after long division */

/*
 * Convert a LONG number into a printable string
 */
char *ltoa(unsigned char *n1, unsigned char *string, unsigned char base)
{
    unsigned sp;
    unsigned char c, stack[(LSIZE*8)+1];
    char temp1[LSIZE], temp2[LSIZE];

    longcpy(temp2, n1);
    longset(temp1, base);

    /* Stack up digits in reverse order */
    sp = 0;
    do {
        longdiv(temp2, temp1);
        stack[sp++] = ((c = *Longreg) > 9) ? c + '7' : c + '0'; }
    while(longtst(temp2));

    /* Unstack digits into output buffer */
    do
        *string++ = stack[--sp];
    while(sp);
    *string = 0;
    return string;
}

/*
 * Convert a string into a LONG number
 * Returns character terminating conversion.
 */
int atol(unsigned char *string, unsigned char *n1, unsigned char base)
{
    unsigned char c;
    char temp[LSIZE];

    longset(n1, 0);

    while(c = *string++) {
        if(isdigit(c))
            c -= '0';
        else if(c >= 'a')
            c -= ('a' - 10);
        else if(c >= 'A')
            c -= ('A' - 10);
        else
            break;
        if(c >= base)
            break;
        longset(temp, base);
        longmul(n1, temp);
        longset(temp, c);
        longadd(n1, temp); }
    return c;
}
