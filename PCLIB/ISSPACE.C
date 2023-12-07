/*
 * Determine if character is a space character
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
isspace(c)
    unsigned c;
{
    return c == ' ' || c == '\n' || c == '\t';
}
