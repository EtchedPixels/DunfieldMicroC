/*
 * Shared scan routine used by the various versions of "scanf".
 * Format spec. and operands are passed as a pointer to the calling
 * functions argument list.
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
int _scan_(optr, input)
    unsigned *optr;
    char *input;
{
    unsigned count, value, value1, base;
    char *format, *ptr, *savinp, chr, mflag, cflag;

    format = *--optr;
    count = 0;

    while(chr = *format++) {
        if(isspace(chr))        /* whitespace */
            continue;
        savinp = input;
        while(isspace(*input))
            ++input;
        if(chr != '%') {        /* Non-format character */
            if(*input == chr)
                ++input;
            continue; }
        ptr = *--optr;
        cflag = mflag = base = value = value1 = 0;
        while(isdigit(chr = *format++))     /* get width if any */
            value = (value * 10) + (chr - '0');
        switch(chr) {
            case 'c' :              /* character input */
                input = savinp;
                value1 = value ? value : 1;
                while(value1-- && (*ptr = *input)) {
                    ++ptr;
                    ++input;
                    cflag = 1; }
                if(value)
                    *ptr = 0;
                break;
            case 's' :              /* string input */
                do {
                    if((!(chr = *input)) || isspace(chr))
                        break;
                    *ptr++ = chr;
                    ++input;
                    cflag = 1; }
                while(--value);
                *ptr = 0;
                break;
            case 'd' :              /* signed number */
                if(*input == '-') {
                    ++input;
                    mflag = -1; }
            case 'u' :              /* unsigned number */
                base = 10;
                break;
            case 'b' :              /* Binary number */
                base = 2;
                break;
            case 'o' :              /* Octal number */
                base = 8;
                break;
            case 'x' :              /* Hexidecimal number */
                base = 16;
                break;
            case '%' :              /* Doubled percent sign */
                if(*input == '%')
                    ++input;
                break;
            default:                /* Illegal type character */
                return 0; }

        if(base) {              /* Number conversion required */
            do {
                if(isdigit(chr = *input))
                    chr -= '0';
                else if(chr >= 'a')
                    chr -= ('a' - 10);
                else if(chr >= 'A')
                    chr -= ('A' - 10);
                else
                    break;
                if(chr >= base)
                    break;
                value1 = (value1 * base) + chr;
                cflag = 1;
                ++input; }
            while(--value);
            *(int *)ptr = (mflag) ? -value1 : value1; }
        count += cflag; }
    return count;
}
