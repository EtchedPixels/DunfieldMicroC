/*
 * Find length of initial segment of string that consists of characters
 * from the specified set.
 *
 * ?COPY.TXT 1994-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
int strspn(char *string, char *set)
{
    int l;
    for(l = 0; *string; ++l)
        if(!strchr(set, *string++))
            break;
    return l;
}
