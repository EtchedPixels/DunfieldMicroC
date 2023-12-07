/*
 * Convert character to upper case
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
toupper(chr)
    unsigned chr;
{
    return(islower(chr) ? chr - ('a'-'A') : chr);
}
