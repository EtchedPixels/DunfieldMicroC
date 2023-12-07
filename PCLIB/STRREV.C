/*
 * Reverse a string in place
 *
 * ?COPY.TXT 1994-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
strrev(char *string)
{
    char *ptr, c;

    ptr = string;
    while(*ptr)
        ++ptr;
    while(string < ptr) {
        c = *string;
        *string++ = *--ptr;
        *ptr = c; }
}
