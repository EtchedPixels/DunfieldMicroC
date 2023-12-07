/*
 * Compare two strings without case sensitivity
 *
 * ?COPY.TXT 1994-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
int stricmp(char *str1, char *str2)
{
    char c1, c2;
    do {
        if((c1=toupper(*str1++)) > (c2=toupper(*str2++)))
            return 1;           /* String1 > String2 */
        if(c1 < c2)
            return -1; }        /* String1 < String2 */
    while(c1);
    return 0;                   /* String 1 == String 2 */
}
