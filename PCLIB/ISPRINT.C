/*
 * Determine if a character is printing character
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
isprint(c)
    unsigned c;
{
    return c >= ' ' && c <= 0x7E;
}
