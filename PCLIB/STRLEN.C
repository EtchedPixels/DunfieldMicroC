/*
 * Find length of string
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
strlen(str)
    char *str;
{
    unsigned length;

    length = 0;
    while(*str++)
        ++length;
    return length;
}
