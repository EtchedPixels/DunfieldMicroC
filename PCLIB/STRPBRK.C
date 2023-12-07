/*
 * Search string for any characters from a given set
 *
 * ?COPY.TXT 1994-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
char *strpbrk(char *string, char *set)
{
    while(*string) {
        if(strchr(set, *string))
            return string;
        ++string; }
    return 0;
}
