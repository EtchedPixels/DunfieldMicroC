/*
 * Copy one string to another - return pointer to terminating null
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
char *stpcpy(dest, source)
    char *dest, *source;
{
    while(*dest = *source++)
        ++dest;
    return dest;
}
