/*
 * Formatted input routines for DDS MICRO-C: fscanf
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#include \mc\stdio.h

#define LINE_SIZE   100     /* Size of input file lines */

/*
 * Scan input from file
 */
register fscanf(args)
    unsigned args;
{
    unsigned *ptr;
    char buffer[LINE_SIZE];

    ptr = nargs() * 2 + &args;
    if(!fgets(buffer, LINE_SIZE, *--ptr))
        return EOF;
    return _scan_(ptr, buffer);
}
