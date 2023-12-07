/*
 * Allocate a block of memory & clear it to zero
 *
 * ?COPY.TXT 1994-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
char *calloc(unsigned items, unsigned size)
{
    char *ptr;
    if(ptr = malloc(size *= items))
        memset(ptr, 0, size);
    return ptr;
}
