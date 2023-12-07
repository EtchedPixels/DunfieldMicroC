/*
 * Convert an ASCII-HEX string into unsigned 16 bit number.
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
int atox(ptr)
    char *ptr;
{
    unsigned value, c;

    value = 0;
    for(;;) {
        if(isdigit(c = toupper(*ptr++)))
            c -= '0';
        else if((c >= 'A') && (c <= 'F'))
            c -= ('A' - 10);
        else
            return value;
        value = (value << 4) + c; }
}
