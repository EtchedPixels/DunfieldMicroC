/*
 * Calculate absolute value of passed number
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
int abs(value)
    int value;
{
    return (value < 0) ? -value : value;
}
