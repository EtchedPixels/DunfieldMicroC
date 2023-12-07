/*
 * Generate a timed beep of a specific frequency on the PC speaker
 *
 * ?COPY.TXT 1994-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
beep(unsigned freq, unsigned duration)
{
    sound_off();
    sound(freq);
    delay(duration);
    sound_off();
}
