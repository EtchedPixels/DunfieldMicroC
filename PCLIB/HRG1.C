/*
 * High Resolution Graphics (HRG) library
 *
 * High level functions: Character drawing
 *
 * ?COPY.TXT 1998-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#define BIOS_ROM    0xF000      /* BIOS ROM segment */
#define BIOS_CHR    0xFA6E      /* BIOS character set table */

/*
 * Draw a character at the specified coordinates
 */
hrg_putc(unsigned x, unsigned y, unsigned cl, int c)
{
    int i, j, b, ci, bl;

    bl = cl >> 8;
    ci = (c << 3) + BIOS_CHR;
    for(i=0; i < 8; ++i) {
        b = peek(BIOS_ROM, ci++);
        for(j=0; j < 8; ++j) {
            hrg_plot(x+j, y+i, (b & 0x80) ? cl : bl);
            b <<= 1; } }
}

/*
 * Draw a string beginning at the specified co-ordinates
 */
hrg_puts(unsigned x, unsigned y, unsigned c, char *s)
{
    while(*s) {
        hrg_putc(x, y, c, *s++);
        x += 8; }
}

/*
 * Formatted print to string: hrg_printf(x, y, c, format, args)
 */
register hrg_printf(args)
    unsigned args;
{
    unsigned *ptr, x, y, c, l;
    char buffer[100];

    ptr = (nargs() * 2) + &args;
    x = *--ptr;
    y = *--ptr;
    c = *--ptr;
    l = _format_(ptr, buffer);
    hrg_puts(x, y, c, buffer);
    return l;
}
