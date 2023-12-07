/*
 * Set all characters in a string
 *
 * ?COPY.TXT 1994-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
strnset(char *string, char c, unsigned length)
{
    while(*string && length--)
        *string++ = c;
}
