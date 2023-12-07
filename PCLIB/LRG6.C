/*
 * Low Resolution Graphics (LRG) library
 *
 * High level functions: Polygon drawing
 *
 * ?COPY.TXT 1998-2005 Dave Dunfield
 * **See COPY.TXT**.
 *
 * Draws a polygon from a series of relative points.
 *
 * NOTE: Micro-C always evaluates its parameters in sequential order from
 * the leftmost to the rightmost, and has an implied sequence point between
 * each argument. This means that is it OK To modify a variable to the right
 * of it's occurance in the argument list. As there is no official sequence
 * point between arguments in a standard C argument list, another compiler
 * may do this differently, and this operation would be undefined and unsafe.
 */
lrg_polygon(int x, int  y, int color, char *data)
{
    unsigned char c;
    c = *data;  /* First byte is total# points */
    do
        lrg_line(x, y, x += (int)*++data, y += (int)*++data, color);
    while(--c);
}
