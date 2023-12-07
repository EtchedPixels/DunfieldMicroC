/*
 * Determine if a character is punctuation (non-alphanumeric printable)
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
ispunct(c)
    unsigned c;
{
    return isgraph(c) && !isalnum(c);
}
