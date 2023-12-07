/*
 * Search a block of memory for a character
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
char *memchr(char *source, char c, unsigned size)
{
    while(size--) {
        if(*source == c)
            return source;
        ++source; }
    return 0;
}
