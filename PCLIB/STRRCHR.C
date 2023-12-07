/*
 * Search a string for the LAST occurance of a character
 *
 * ?COPY.TXT 1994-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
char *strrchr(char *string, char c)
{
    char *ptr;
    ptr = 0;
    do {
        if(*string == c)
            ptr = string; }
    while(*string++);
    return ptr;
}
