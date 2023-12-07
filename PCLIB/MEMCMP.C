/*
 * Compare two blocks of memory
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
int memcmp(unsigned char *s1, unsigned char *s2, unsigned size)
{
    while(size--) {
        if(*s1 > *s2)
            return 1;
        if(*s1++ < *s2++)
            return -1; }
    return 0;
}
