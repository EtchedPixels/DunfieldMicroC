/*
 * High Resolution Graphics (HRG) library
 *
 * High level functions: Box drawing
 *
 * ?COPY.TXT 1998-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

/*
 * Draw a box on the screen
 */
hrg_box(unsigned x, unsigned y, unsigned xl, unsigned yl, unsigned color)
{
    hrg_hline(x, y, xl, color);
    hrg_hline(x, y+yl, xl+1, color);
    hrg_vline(x, y, yl, color);
    hrg_vline(x + xl, y, yl, color);
}

/*
 * Draw a filled box on the screen
 */
hrg_fbox(unsigned x, unsigned y, unsigned xl, unsigned yl, unsigned color)
{
    while(yl) {
        hrg_hline(x, y++, xl, color);
        --yl; }
}
