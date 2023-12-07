/*
 * Returns a pointer to the first occurence of 'chr' in the
 * passed 'string'.
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
char *strchr(string, chr)
    char *string, chr;
{
    do
        if(*string == chr)
            return string;
    while(*string++);
    return 0;
}
