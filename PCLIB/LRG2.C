/*
 * Low Resolution Graphics (LRG) library
 *
 * High level functions: Box drawing
 *
 * ?COPY.TXT 1998-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

/*
 * Draw a box on the screen
 */
lrg_box(unsigned x, unsigned y, unsigned xl, unsigned yl, unsigned color)
{
    lrg_hline(x, y, xl, color);
    lrg_hline(x, y+yl, xl+1, color);
    lrg_vline(x, y, yl, color);
    lrg_vline(x + xl, y, yl, color);
}

/*
 * Draw a filled box on the screen
 */
lrg_fbox(unsigned x, unsigned y, unsigned xl, unsigned yl, unsigned color)
{
    while(yl) {
        lrg_hline(x, y++, xl, color);
        --yl; }
}
