/*
 * Convert a number to ASCII representation
 *
 * ?COPY.TXT 1994-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
int itoa(unsigned value, char *string, int base)
{
    unsigned sp, c;
    char stack[16];

    if(base < 0) {      /* Allow negative output */
        base = -base;
        if((int)value < 0) {
            value = -value;
            *string++ = '-'; } }

    sp = 0;
    do
        stack[sp++] = value % base;
    while(value /= base);

    value = sp;

    while(sp)
        *string++ = ((c = stack[--sp]) > 9) ? c + '7' : c + '0';

    *string = 0;

    return value;
}
