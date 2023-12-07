/*
 * Determine length of initial segment of "string" which does not
 * contains any characters from specified set.
 *
 * ?COPY.TXT 1994-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
int strcspn(char *string, char *set)
{
    int l;
    for(l=0; *string; ++l)
        if(strchr(set, *string++))
            break;
    return l;
}
