/*
 * Formatted input routines for DDS MICRO-C: sscanf
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

/*
 * Scan input from memory string
 */
register sscanf(args)
    unsigned args;
{
    unsigned *ptr;

    return _scan_(ptr = (nargs() - 1) * 2 + &args, *ptr);
}
