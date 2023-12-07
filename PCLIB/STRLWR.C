/*
 * Convert a string to LOWER case
 *
 * ?COPY.TXT 1994-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
strlwr(char *string)
{
    while(*string) {
        *string = tolower(*string);
        ++string; }
}
