/*
 * Calculate the integer square root of a value
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#define PRECESSION  (16>>1)
unsigned sqrt(unsigned value)
{
    unsigned root, rootsquared;
    unsigned mask, masksquared;
    unsigned power, t;
    root = rootsquared = 0;
    mask        = 1 <<  (PRECESSION-1);
    masksquared = 1 << ((PRECESSION-1) << 1);
    power = PRECESSION;
    do {
        if((t = (root<<power)+rootsquared+masksquared) <= value) {
            rootsquared = t;
            root |= mask; }
        mask >>= 1;
        masksquared >>= 2; }
    while(--power);

    rootsquared = root+1;
    if((value - (root*root)) < ((rootsquared*rootsquared)-value))
        return root;
    return rootsquared;
}
