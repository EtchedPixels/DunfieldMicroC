/*
 * High Resolution Graphics (HRG) library
 *
 * High level functions: Line drawing
 *
 * ?COPY.TXT 1998-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

/*
 * Draw a line from point (x1, y1) to (x2, y2)
 */
hrg_line(int x1, int y1, int x2, int y2, int c)
{
    int i, w, h;
    /* If 'X' is greater, increment through 'X' coordinate */
    if((w = abs(x1 - x2)) >= (h = abs(y1 - y2))) {
        if(x1 > x2) {
            i = x1;
            x1 = x2;
            x2 = i;
            i = y1;
            y1 = y2;
            y2 = i; }
        if(y1 < y2) {
            for(i=0; i < w; ++i)
                hrg_plot(x1+i, y1+hrg_scale(i, h, w), c); }
        else {
            for(i=0; i < w; ++i)
                hrg_plot(x1+i, y1-hrg_scale(i, h, w), c); } }
    /* If 'Y' is greater, increment through 'Y' coordinate */
    else {
        if(y1 > y2) {
            i = x1;
            x1 = x2;
            x2 = i;
            i = y1;
            y1 = y2;
            y2 = i; }
        if(x1 < x2) {
            for(i=0; i < h; ++i)
                hrg_plot(x1+hrg_scale(i, w, h), y1+i, c); }
        else {
            for(i=0; i < h; ++i)
                hrg_plot(x1-hrg_scale(i, w, h), y1+i, c); } }

    hrg_plot(x2, y2, c);
}
