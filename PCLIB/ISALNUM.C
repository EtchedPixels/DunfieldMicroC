/*
 * Determine if a character is alphabetic or numeric
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
isalnum(c)
    unsigned c;
{
    return isalpha(c) || isdigit(c);
}
