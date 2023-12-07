/*
 * Compare portions of two strings
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
int strncmp(str1, str2, length)
    char *str1, *str2;
    unsigned length;
{
    do {
        if(*str1 > *str2)       /* String1 > String2 */
            return 1;
        if(*str1 < *str2++)     /* String1 < String2 */
            return -1; }
    while(*str1++ && --length);
    return 0;                   /* String 1 == String 2 */
}
