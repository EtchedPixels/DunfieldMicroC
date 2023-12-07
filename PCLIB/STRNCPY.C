/*
 * Copies a variable length string into a fixed length field.
 * The destination field will be padded with zero (0) chars
 * if it is longer than the source string.
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
strncpy(dest, source, length)
    char *dest, *source;
    unsigned length;
{
    char *ptr;
    ptr = dest;
    while(length--)
        if(*ptr++ = *source)
            ++source;
    return dest;
}
