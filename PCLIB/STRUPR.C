/*
 * Convert a string to UPPER case
 *
 * ?COPY.TXT 1994-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
strupr(char *string)
{
    while(*string) {
        *string = toupper(*string);
        ++string; }
}
