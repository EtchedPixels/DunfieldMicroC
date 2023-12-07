/*
 * Duplicate a string with allocated memory
 *
 * ?COPY.TXT 1994-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
char *strdup(char *string)
{
    char *dest;

    if(dest = malloc(strlen(string)+1))
        strcpy(dest, string);

    return dest;
}
