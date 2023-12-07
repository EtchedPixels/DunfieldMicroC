/*
 * Formatted output routines for DDS MICRO-C: lprintf
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#include \mc\stdio.h

#define LINE_SIZE   132     /* Size of output file lines */

/*
 * Formatted print to file
 */
register lprintf(args)
    unsigned args;
{
    char buffer[LINE_SIZE+1];
    unsigned *ptr, l;
    int handle;

    handle = *(ptr = (nargs() - 1) * 2 + &args);
    l = _format_(ptr, buffer);
    lputs(buffer, handle);
    return l;
}
