/*
 * Random number routine for DDS MICRO-C
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 */

unsigned RAND_SEED;     /* Seed value */

/*
 * Generate a pseudo-random number via liner-congruential sequence
 */
rand()
{
    return RAND_SEED = (RAND_SEED * 13709) + 13849;
}

/*
 * Get pseudo-random number and impose limit
 */
random(unsigned range)
{
    return rand() % range;
}
