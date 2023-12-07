/*
 * Low Resolution Graphics (LRG) library
 *
 * High level functions: Circle drawing
 *
 * ?COPY.TXT 1998-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

/*
 * Draw a circle about point (x, y) of radius (r), in color (c)
 */
lrg_circle(int x, int y, int r, int c)
{
    int i, j, k, l, rs, lj;

    rs = (lj = r)*r;
    for(i=0; i <= r; ++i) {
        j = k = sqrt(rs - (i*i));
        do {
            l = (j << 2) / 5;
            lrg_plot(x+i, y+l, c);
            lrg_plot(x+i, y-l, c);
            lrg_plot(x-i, y+l, c);
            lrg_plot(x-i, y-l, c); }
        while(++j < lj);
        lj = k; }
}

/*
 * Draw a circle about point (x, y) of radius (r), in color (c)
 */
lrg_fcircle(int x, int y, int r, int c)
{
    int i, j, k, l, rs, lj;

    rs = (lj = r)*r;
    for(i=0; i <= r; ++i) {
        j = k = sqrt(rs - (i*i));
        do {
            l = (j << 2) / 5;
            lrg_hline(x-i, y-l, i+i, c);
            lrg_hline(x-i, y+l, i+i, c); }
        while(++j < lj);
        lj = k; }
}
