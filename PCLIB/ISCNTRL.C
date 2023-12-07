/*
 * Determine if character is a control code
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
iscntrl(c)
    unsigned c;
{
    return c < ' ' || c == 0x7f;
}
