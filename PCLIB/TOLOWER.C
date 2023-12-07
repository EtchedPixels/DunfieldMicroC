/*
 * Convert character to lower case
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
tolower(chr)
    unsigned chr;
{
    return(isupper(chr) ? chr + ('a'-'A') : chr);
}
