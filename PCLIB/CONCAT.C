/*
 * Concatinate several strings into one
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
register concat(args)
    unsigned args;
{
    unsigned *sptr, sources;
    char *source, *dest;

    sptr = (sources = nargs()) * 2 + &args;
    dest = *--sptr;

    while(--sources) {      /* Do all sources */
        source = *--sptr;
        while(*source)      /* Append this string */
            *dest++ = *source++; }
    *dest = 0;              /* Null terminate */
}
