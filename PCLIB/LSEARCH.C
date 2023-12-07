/*
 * Perform a linear search of a structure in memory
 *
 * ?COPY.TXT 1994-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
char *lsearch(key, table, entries, esize, compare)
    unsigned char *key;     /* Key to search for */
    unsigned char *table;   /* Table to search */
    unsigned entries;       /* Number of entries in table */
    unsigned esize;         /* Size of each entry */
    int *compare;           /* Function to perform comparison */
{
    while(entries--) {
        if(!(*compare)(key, table))
            return table;
        table += esize; }
    return 0;
}
