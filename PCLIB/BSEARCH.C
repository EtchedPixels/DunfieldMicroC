/*
 * Perform a binary search of a structure in memory
 *
 * ?COPY.TXT 1994-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
char *bsearch(key, table, entries, esize, compare)
    unsigned char *key;     /* Key to search for */
    unsigned char *table;   /* Table to search */
    unsigned entries;       /* Number of entries in table */
    unsigned esize;         /* Size of each entry */
    int *compare;           /* Function to perform comparison */
{
    int result;
    unsigned current, bot;
    char *ptr;

    if(entries--) {
        bot = 0;
        current = entries/2;
        for(;;) {
            if(!(result = (*compare)(key, ptr = current * esize + table)))
                return ptr;
            if(result < 0) {        /* Key is less than current entry */
                if(current <= bot)
                    break;
                entries = current-1;
                current -= (current+1 - bot) / 2; }
            else {                  /* Key is greater than current entry */
                if(current >= entries)
                    break;
                bot = current+1;
                current += (entries+1 - current) / 2; } } }
    return 0;
}
