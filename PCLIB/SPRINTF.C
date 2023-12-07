/*
 * Formatted output routines for DDS MICRO-C: sprintf
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

/*
 * Formatted print to memory string
 */
register sprintf(args)
    unsigned args;
{
    unsigned *ptr;

    return _format_(ptr = (nargs() - 1) * 2 + &args, *ptr);
}
