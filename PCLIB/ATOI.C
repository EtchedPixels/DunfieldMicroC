/*
 * Convert an ASCII string into a signed 16 bit number.
 * May be used on unsigned input if target is unsigned.
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
int atoi(ptr)
    char *ptr;
{
    unsigned value;
    char mflag;

    mflag = value = 0;
    if(*ptr == '-') {       /* Handle leading "negative" sign */
        ++ptr;
        mflag = -1; }
    while(isdigit(*ptr))    /* Assemble decimal value from digits */
        value = (value * 10) + (*ptr++ - '0');
    return mflag ? -value : value;
}
