/*
 * Determine if a character is alphabetic
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
isalpha(c)
    unsigned c;
{
    return islower(c) || isupper(c);
}
