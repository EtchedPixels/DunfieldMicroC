/*
 * Determine if a value is a ASCII character
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
isascii(value)
    unsigned value;
{
    return value < 0x80;
}
