/*
 * Concatenates a string to another string, insuring that at most
 * "length" characters are copied.
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
char *strncat(dest, source, length)
    char *dest, *source;
    unsigned length;
{
    char *ptr;
    ptr = dest;
    while(*ptr)                 /* Find end of source */
        ++ptr;
    while(*source && length--)
        *ptr++ = *source++;
    *ptr = 0;
    return dest;
}
