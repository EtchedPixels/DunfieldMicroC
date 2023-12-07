/*
 * Determine if character is lower case letter
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
islower(c)
    unsigned c;
{
    return c >= 'a' && c <= 'z';
}
