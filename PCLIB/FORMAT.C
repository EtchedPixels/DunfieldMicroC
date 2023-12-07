/*
 * Shared format routine used by the various versions of "printf".
 * Format spec. and operands are passed as a pointer to the calling
 * functions argument list.
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
_format_(inptr, outptr)
    unsigned *inptr;
    char *outptr;
{
    char outstk[17], *ptr, *optr, *format, justify, zero, minus, chr;
    unsigned width, value, i;

    optr = outptr;
    format = *--inptr;

    while(chr = *format++) {
        if(chr == '%') {                    /* format code */
            chr = *format++;
            *(ptr = &outstk[16]) = justify = minus = width = value = i = 0;
            zero = ' ';
            if(chr == '-') {                /* left justify */
                justify = -1;
                chr = *format++; }
            if(chr == '0')                  /* leading zeros */
                zero = '0';
            while(isdigit(chr)) {           /* field width specifier */
                width = (width * 10) + (chr - '0');
                chr = *format++; }

            value = *--inptr;               /* get parameter value */

            switch(chr) {
                case 'd' :                  /* decimal number */
                    if(value & 0x8000) {
                        value = -value;
                        minus = -1; }
                case 'u' :                  /* unsigned number */
                    i = 10;
                    break;
                case 'x' :                  /* hexidecimal number */
                    i = 16;
                    break;
                case 'o' :                  /* octal number */
                    i = 8;
                    break;
                case 'b' :                  /* binary number */
                    i = 2;
                    break;
                default:                    /* Unknown */
                    ++inptr;
                    value = chr;
                case 'c' :                  /* character data */
                    *--ptr = value;
                    break;
                case 's' :                  /* string */
                    ptr = value; }

            if(i)       /* for all numbers, generate the ASCII string */
                do {
                    if((chr = (value % i) + '0') > '9')
                        chr += 7;
                    *--ptr = chr; }
                while(value /= i);

/* output sign if any */
            if(minus) {
                *optr++ = '-';
                if(width)
                    --width; }

/* pad with 'zero' value if right justify enabled  */
            if(width && !justify) {
                for(i = strlen(ptr); i < width; ++i)
                    *optr++ = zero; }

/* move in data */
            i = 0;
            value = width - 1;
            while((*ptr) && (i <= value)) {
                *optr++ = *ptr++;
                ++i; }

/* pad with 'zero' value if left justify enabled */
            if(width && justify) {
                while(i < width) {
                    *optr++ = zero;
                    ++i; } } }
        else
/* not a format code, simply display the character */
            *optr++ = chr; }

    *optr = 0;
    return optr - outptr;
}
