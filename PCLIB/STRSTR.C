/*
 * Search for occurance of string2 in string1
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
char *strstr(str1, str2)
    char *str1, *str2;
{
    while(*str1) {
        if(strbeg(str1, str2))
            return str1;
        ++str1; }
    return 0;
}
