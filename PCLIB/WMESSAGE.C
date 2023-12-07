/*
 * Display message on the screen in it's own window and wait for a key.
 * Optional features: Center box on screen, list of acceptable keys
 *
 * Format of OPTION word: CxxxxxxxKyyyyyyy
 *  C       - Center message box on screen (ignore xxxxxxx/yyyyyyy)
 *  xxxxxxx - X position if 'C' bit not set
 *  K       - Optional key list is supplied (as first function arg)
 *  yyyyyyy - Y position if 'C' bit not set
 *
 * Original contributed by Steffen Kaiser
 * ?COPY.TXT 1995-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#include \mc\window.h
int wmessage(char *keys, int option, int attr, char *msg)
{
    int xlen, ylen;
    char *p, *h;

/* determine the size of the message window */
    p = msg - 1;
    xlen = ylen = 1;
    while(p = strchr(h = p + 1, '\n')) {
        if(p - h > xlen)
            xlen = p - h;
        ++ylen; }
    if((p = strchr(h, '\0')) - h > xlen)    /* last line */
        xlen = p - h;

    if(attr & WBOX3)                /* the border uses 2 rows & cols */
        xlen += 2, ylen += 2;

    if(option & WMSG_CENTER) {      /* Center on screem */
        p = (xlen < W_COLS) && (W_COLS - xlen) >> 1;
        h = (ylen < W_ROWS) && (W_ROWS - ylen) >> 1; }
    else {                          /* Position at absolute address */
        p = option >> 8;
        h = option & 0x7F; }

    /* Open window, display message & wait for keys */
    if(wopen((int)p, (int)h, xlen, ylen, attr)) {
        wcursor_off();
        wputs(msg);
        if(option & WMSG_KEY)       /* selective wait */
            while(!strchr(keys, xlen = wgetc()));
        else        /* Anything goes */
            xlen = wgetc();
        wclose();
        return xlen; }
    return 0;                       /* indicates window open error */
}
