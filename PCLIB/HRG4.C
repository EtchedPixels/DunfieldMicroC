/*
 * High Resolution Graphics (HRG) library
 *
 * High level functions: Circle drawing
 *
 * ?COPY.TXT 1998-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

/*
 * Draw a circle about point (x, y) of radius (r), in color (c)
 */
hrg_circle(int x, int y, int r, int c)
{
    int i, j, k, rs, lj;

    rs = (lj = r)*r;
    for(i=0; i <= r; ++i) {
        j = k = sqrt(rs - (i*i));
        do {
            hrg_plot(x+i, y+j, c);
            hrg_plot(x+i, y-j, c);
            hrg_plot(x-i, y+j, c);
            hrg_plot(x-i, y-j, c); }
        while(++j < lj);
        lj = k; }
}

/*
 * Draw a circle about point (x, y) of radius (r), in color (c)
 */
hrg_fcircle(int x, int y, int r, int c)
{
    int i, j, k, rs, lj;

    rs = (lj = r)*r;
    for(i=1; i <= r; ++i) {
        j = k = sqrt(rs - (i*i));
        do {
            hrg_hline(x-i, y-j, i+i, c);
            hrg_hline(x-i, y+j, i+i, c); }
        while(++j < lj);
        lj = k; }
}
