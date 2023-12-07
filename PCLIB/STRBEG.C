/*
 * Test for string1 beginning with string2
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
strbeg(str1, str2)
    char *str1, *str2;
{
    while(*str2)
        if(*str1++ != *str2++)
            return 0;
    return 1;
}
