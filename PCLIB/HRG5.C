/*
 * High Resolution Graphics (HRG) library
 *
 * High level functions: ARC drawing
 *
 * ?COPY.TXT 1998-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#define ARC_RES     64          /* Resolution of each ARC quadrant */

/*
 * Draw an arc centered at (x, y), radius r at vectors v1, v2
 */
hrg_arc(int x, int y, int r, unsigned char v1, unsigned char v2, int c)
{
    int rs, i, j, ax, x1, y1, x2, y2;
    static unsigned sine[] = {      /* Sine table */
            0,  1608,  3216,  4821,  6424,  8022,  9616, 11204,
        12785, 14359, 15924, 17479, 19024, 20557, 22078, 23586,
        25079, 26557, 28020, 29465, 30893, 32302, 33692, 35062,
        36410, 37736, 39040, 40320, 41575, 42806, 44011, 45190,
        46341, 47464, 48559, 49624, 50660, 51665, 52639, 53581,
        54491, 55368, 56212, 57022, 57797, 58538, 59243, 59913,
        60547, 61144, 61705, 62228, 62714, 63162, 63571, 63943,
        64276, 64571, 64826, 65043, 65220, 65358, 65457, 65516 };

    x2 = -1;
    rs = r*r;

    do {
        j = (ARC_RES-1) - (i = v1 & (ARC_RES-1));
        switch(v1 & (ARC_RES*3)) {
            case ARC_RES*0 :    /* Quadrant one */
                x1 = x + (ax = hrg_scale(r, sine[i], -1));
                y1 = y - sqrt(rs - (ax*ax));
                break;
            case ARC_RES*1 :    /* Quadrant two */
                x1 = x + (ax = hrg_scale(r, sine[j], -1));
                y1 = y + sqrt(rs - (ax*ax));
                break;
            case ARC_RES*2 :    /* Quadrant three */
                x1 = x - (ax = hrg_scale(r, sine[i], -1));
                y1 = y + sqrt(rs - (ax*ax));
                break;
            case ARC_RES*3 :    /* Quadrant four */
                x1 = x - (ax = hrg_scale(r, sine[j], -1));
                y1 = y - sqrt(rs - (ax*ax)); }
        if(x2 != -1)
            hrg_line(x2, y2, x1, y1, c);
        x2 = x1;
        y2 = y1; }
    while(v1++ != v2);
}
