/*
 * Parse a string into tokens
 *
 * ?COPY.TXT 1994-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
char *strtok(char *string, char *delim)
{
    static char *ptr;
    if(string)
        ptr = string;
    while(strchr(delim, *ptr))
        if(!*ptr++)
            return 0;
    string = ptr;
    while(*ptr) {
        if(strchr(delim, *ptr)) {
            *ptr++ = 0;
            break; }
        ++ptr; }
    return string;
}
