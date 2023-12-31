/*
 * DDS MICRO-C IBM/PC Supplementary window functions
 *
 * ?COPY.TXT 1989-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#include \mc\stdio.h
#include \mc\window.h

#define SCREEN_WIDTH 132        /* Maximum width of IBM-PC screen */

static char wgi = 0;

/*
 * Put a string on the screen in a window
 */
w_puts(text, window)
    char *text, *window;
{
    while(*text)
        w_putc(*text++, window);
}

/*
 * Put a string on the screen in the current window
 */
wputs(text)
    char *text;
{
    while(*text)
        wputc(*text++);
}

/*
 * Put a string on the screen in a certain width field
 */
wputf(text, width)
    char *text;
    int width;
{
    while(width--)
        wputc(*text ? *text++ : ' ');
}

/*
 * Select an entry from a multiple choice menu
 */
wmenu(x, y, attrs, names, initial)
    int x, y, attrs, *initial;
    char *names[];
{
    int i, j, mawlen, mawentry;
    char c, *ptr, n, r;

/* First, determine longest selection, and open the window */
    mawentry = mawlen = i = 0;
    while(ptr = names[mawentry]) {
        if((j = strlen(ptr)) > mawlen)
            mawlen = j;
        ++mawentry; }
    wopen(x, y, (attrs & WBOX3) ? mawlen+2 : mawlen,
                (attrs & WBOX3) ? mawentry+2 : mawentry, attrs);
    --mawentry;
    wcursor_off();

    n = *W_OPEN;
    r = ((unsigned)n>>4) | ((unsigned)n<<4 & 0xf0);
    while(i <= mawentry) {
        wgotoxy(0, i);
        wputs(names[i++]); }

/* Print selection cursor, and wait for keys */
    i = *initial;
    do {
        wgotoxy(0, i);
        *W_OPEN = r;
        wputf(names[i], mawlen);
        *W_OPEN = n;
        c = toupper(wgetc());
        wgotoxy(0, i);
        wputf(names[i], mawlen);
        switch(c) {         /* handle special cases */
            case _KUA:          /* UP arrow */
                i = i ? i-1 : mawentry;
                break;
            case _KDA:          /* DOWN arrow */
                i = (i < mawentry) ? i+1 : 0;
                break;
            case _KHO:          /* HOME key */
                i = 0;
                break;
            case _KEN:          /* END key */
                i = mawentry;
                break;
            case 0x1B:          /* ESCAPE key (abort) */
                W_OPEN[1] |= 0x10;  /* Insure window clears */
                c = '\n';           /* Force exit */
                initial = -1;       /* Indicate abortion */
                break;
            case '\n':          /* RETURN key (select) */
                wgotoxy(0, i);
                *W_OPEN = r;
                wputc(*names[i]);
                *W_OPEN = n;
                *initial = i;
                initial = 0;        /* Indicate enter */
                break;
            default:
                if(c >= 0) {    /* First character lookup */
                    j=0;
                    while(j <= mawentry) {
                        if(*names[j] == c) {
                            i = j;
                            break; }
                        ++j; }
                    break; }
                wputc(7); } }
    while(c != '\n');
    wclose();
    return initial;
}

/*
 * Get a string from a field in the window
 */
wgets(x, y, string, length)
    int x, y, length;
    char string[];
{
    int c;
    unsigned i, j, len;

    len = length & 0x7f;
    i = 0;
    if(x & 0x100) {
        while(string[i])
            ++i; }
    x &= 255;
    for(;;) {
        if(i > len) i = len;
        wgotoxy(x, y);
        wputf(string, len);
        wgotoxy(x+i, y);
        wgi ? wcursor_block() : wcursor_line();
        switch(c = wgetc()) {
            case _KLA :     /* Backup */
                if(i)
                    --i;
                continue;
            case _KRA :     /* Forward */
                if(string[i])
                    ++i;
                continue;
            case _KBS :     /* Backspace & delete */
                if(!i)
                    continue;
                --i;
            case _KDL :     /* Delete character */
                j=i;
                while((j < len) && string[j]) {
                    string[j] = string[j+1];
                    ++j; }
                continue;
            case _KIN:      /* Insert key */
                wgi = !wgi;
                continue;
            case _KHO:      /* Home key */
                i = 0;
                continue;
            case _KEN:      /* End key */
                i=0; while(string[i]) ++i;
                continue;
            default:        /* Normal (data) character */
                if(c >= 0) {
                    if(i >= len)
                        continue;
                    if((length & 0x80) && ((c < '0') || (c > '9')))
                        continue;
                    if(wgi) {
                        j = len;
                        while(j > i) {
                            string[j] = string[j-1];
                            --j; }
                        string[len] = 0; }
                    if(!string[i])
                        string[i+1] = 0;
                    string[i++] = c;
                    continue; }
            case '\n':      /* Enter */
            case 0x1B:      /* Escape */
                return c;
            case _KPU:      /* Page up (clear field) */
                i = 0;
            case _KPD:      /* Page down (Clear to end) */
                string[i] = 0; } }
}

/*
 * Handle a screen form input
 */
register wform(args)
    unsigned args;
{
    unsigned *argptr, attrs, x, y, length, pos, *nptr, *nptr1;
    char **form, *ptr, buffer[25];

    /* Extract the fixed arguments */
    argptr = nargs()*2 + &args;
    x = *--argptr;
    y = *--argptr;
    attrs = *--argptr;
    form = *--argptr;   /* Get form address */

    /* Draw the initial values */
    wopen(x, y, (int)form[0] >> 8, (int)form[0] & 255, attrs);
//  for(pos=1; ptr = form[pos]; ++pos) {
    pos = 1;
    while(ptr = form[pos]) {
        nptr = nptr1 = *(argptr - pos*2);
        wgotoxy(*ptr++, *ptr++);
        if((length = *ptr++) & 0x80)
            sprintf(nptr = buffer,"%u", *nptr1);
        wputs(ptr);
        wputc(' ');
        wputf(nptr, length & 0x7f);
        ++pos; }

    /* Now, allow editing of the forms */
    pos = 1;
    for(;;) {
        ptr = form[pos];
        nptr1 = nptr = *(argptr - pos*2);
        x = *ptr++;
        y = *ptr++;
        if((length = *ptr++) & 0x80)
            sprintf(nptr = buffer,"%u", *nptr1);
        attrs = wgets(x+strlen(ptr)+1, y, nptr, length);
        if(length & 0x80)
            *nptr1 = atoi(buffer);
        switch(attrs) {
            case _KUA :     /* Up arrow */
                if(pos > 1)
                    --pos;
                else {
                    length = 1; while(form[length]) ++length;
                    pos = length-1; }
                break;
            case _KDA :     /* Down arrow */
                if(!form[++pos])
                    pos = 1;
                break;
            case '\n' :     /* Enter key */
                if(form[++pos])
                    break;
                wclose();
                return 0;
            case 0x1B :     /* Escape */
                wclose();
                return -1;
            default:
                wputc(7); } }
}

/*
 * Formatted print to active window
 */
register wprintf(args)
    unsigned args;
{
    char buffer[SCREEN_WIDTH + 2];
    unsigned l;

    l = _format_(nargs() * 2 + &args, buffer);
    wputs(buffer);
    return l;
}

/*
 * Formatted print to any window
 */
register w_printf(args)
    unsigned args;
{
    char buffer[SCREEN_WIDTH + 2];
    unsigned *ptr, l;

    l = _format_(ptr = (nargs() - 1) * 2 + &args, buffer);
    w_puts(buffer, *ptr);
    return l;
}
