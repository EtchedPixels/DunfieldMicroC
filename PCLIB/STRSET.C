/*
 * Set all characters in a string
 *
 * ?COPY.TXT 1994-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
strset(char *string, char c)
{
    while(*string)
        *string++ = c;
}
