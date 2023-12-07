/*
 * Determine if passed char is an ASCII numeric digit
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
isdigit(c)
    unsigned c;
{
    return c >= '0' && c <= '9';
}
