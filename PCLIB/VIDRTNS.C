/*
 * DDS MICRO-C IBM/PC Supplementary video functions
 *
 * ?COPY.TXT 1989-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#include \mc\video.h
#define SCREEN_WIDTH 80     /* width of IBM-PC screen */

/*
 * Draw a BOX on the screen using PC graphics characters
 */
vdraw_box(x, y, w, h)
    int x, y, w, h;
{
    int i;

    vgotoxy(x, y);
    vputc(0xDA);            /* top left corner */
    for(i=1; i < w; ++i)
        vputc(0xC4);        /* horizontal line */
    vputc(0xBF);            /* top right corner */
    vgotoxy(x, y+h);
    vputc(0xC0);            /* bottom left corner */
    for(i=1; i < w; ++i)
        vputc(0xC4);        /* horizontal line */
    vputc(0xD9);            /* bottom right corner */
    for(i=1; i<h; ++i) {    /* draw box sides */
        vgotoxy(x, i+y);
        vputc(0xB3);
        vgotoxy(x+w, i+y);
        vputc(0xB3); }
}

/*
 * Clear a box on the screen
 */
vclear_box(x, y, w, h)
    int x, y, w, h;
{
    int i, j;

    for(i = 0; i <= h; ++i) {
        vgotoxy(x, y+i);
        for(j = 0; j <= w; ++j)
            vputc(' '); }
}

/*
 * Display a message in a box on the PC screen
 */
vmessage(x, y, text)
    int x, y;
    char *text;
{
    vdraw_box(x, y, strlen(text)+1, 2);
    vgotoxy(x+1, y+1);
    vputs(text);
}

/*
 * Put a string on the screen
 */
vputs(text)
    char *text;
{
    while(*text)
        vputc(*text++);
}

/*
 * Put a string on the screen in a certain width field
 */
vputf(text, width)
    char *text;
    int width;
{
    while(width--)
        vputc(*text ? *text++ : ' ');
}

/*
 * Select an entry from a multiple choice menu
 */
vmenu(x, y, names, erase, initial)
    char *names[], erase;
    int x, y, *initial;
{
    int i, max_len, max_entry;
    char chr, *ptr;

/* First, draw all selections on screen, keeping track of longest */
    for(max_entry=max_len=0; ptr = names[max_entry]; ++max_entry) {
        vgotoxy(x+1, max_entry+y+1);
        for(i = 0; *ptr; ++i)
            vputc(*ptr++);
        if(i > max_len)
            max_len = i; }

/* Then enclose them in a box */
    vdraw_box(x, y, max_len+1, --max_entry + 2);

/* Print selection cursor, and wait for keys */
    i = *initial;
    do {
        vgotoxy(x+1, y+i+1);
        V_ATTR = REVERSE;
        vputf(names[i], max_len);
        V_ATTR = NORMAL;
        chr = vgetc();
        vgotoxy(x+1, y+i+1);
        vputf(names[i], max_len);
        switch(chr) {       /* handle special cases */
            case _KUA:          /* UP arrow */
                i = i ? i-1 : max_entry;
                break;
            case _KDA:          /* DOWN arrow */
                i = (i < max_entry) ? i+1 : 0;
                break;
            case _KHO:          /* HOME key */
                i = 0;
                break;
            case _KEN:          /* END key */
                i = max_entry;
                break;
            case 0x1B:          /* ESCAPE key (abort) */
                erase = chr = '\n';
                initial = -1;
                break;
            case '\n':          /* RETURN key (select) */
                *initial = i;
                initial = 0;
                break;
            default:
                vputc(7); } }
    while(chr != '\n');

    if(erase)
        vclear_box(x, y, max_len+1, max_entry + 2);
    else {
        vgotoxy(x, y+i+1);
        vputc('>');
        vgotoxy(x+max_len+1, y+i+1);
        vputc('<'); }

    return initial;
}

/*
 * Prompt for & get a string
 */
vgets(x, y, prompt, field, width)
    int x, y;
    char char *prompt, *field;
    int width;
{
    int w, p, q, r;
    char c, d;

    vdraw_box(x, y, (w = strlen(prompt)) + width + 2, 2);
    vgotoxy(++x, ++y);
    vputs(prompt);
    vputf(field, width);
    p = 0;
    r = x + w;
    vcursor_line();
    for(;;) {
        vgotoxy(r+p, y);
        switch(c = vgetc()) {
            case _KRA:      /* Right arrow */
                if(field[p])
                    ++p;
                break;
            case _KLA:      /* Left arrow */
                if(p)
                    --p;
                break;
            case _KHO:      /* HOME key */
                p = 0;
                break;
            case _KEN:      /* END right */
                for(p=0; field[p]; ++p);
                break;
            case _KBS:      /* Backspace */
                if(!p)
                    break;
                vgotoxy(r + --p, y);
            case _KDL:      /* Delete */
                if(field[q=p]) {
                    while(c = field[q] = field[++q])
                        vputc(c);
                    vputc(' '); }
                break;
            case _KPU:      /* Page up */
                vgotoxy(r, y);
                p = 0;
            case _KPD:      /* Page down */
                for(q = p; field[q]; ++q)
                    vputc(' ');
                field[p] = 0;
                break;
            case '\n' :     /* Newline, exit */
            case 0x1B :     /* Escape, abort */
                vcursor_off();
                vclear_box(x-1, y-1, w+width+2, 2);
                return c == 0x1B;
            default:
                if((c > 0) && (p < width)) {
                    q = p++;
                    do {
                        vputc(c);
                        d = field[q];
                        field[q++] = c; }
                    while((c = d) && (q < width));
                    field[q] = 0; } } }
}

/*
 * Formatted print to video display
 */
register vprintf(args)
    unsigned args;
{
    char buffer[SCREEN_WIDTH + 1], *cptr;
    unsigned l;

    l = _format_(nargs() * 2 + &args, cptr = buffer);
    while(*cptr)
        vputc(*cptr++);
    return l;
}
