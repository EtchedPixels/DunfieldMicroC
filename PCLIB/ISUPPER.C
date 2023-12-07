/*
 * Determine if character is upper case letter
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
isupper(c)
    unsigned c;
{
    return c >= 'A' && c <= 'Z';
}
