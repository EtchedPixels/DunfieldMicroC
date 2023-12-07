/*
 * Determine if a character is graphic printing character
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
isgraph(c)
    unsigned c;
{
    return c > ' ' && c < 0x7f;
}
