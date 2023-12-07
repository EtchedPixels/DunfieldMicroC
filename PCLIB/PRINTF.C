/*
 * Formatted output routines for DDS MICRO-C: printf
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#include \mc\stdio.h

#define LINE_SIZE   132     /* Size of output file lines */

/*
 * Formatted print to console device
 */
register printf(args)
    unsigned args;
{
    char buffer[LINE_SIZE+1];
    unsigned l;

    l = _format_(nargs() * 2 + &args, buffer);
    fputs(buffer, stdout);
    return l;
}
