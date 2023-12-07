/*
 * When a function declared as "register" is called, MICRO-C loads
 * the accumulator with the number of arguments which were passed,
 * just prior to performing the actual subroutine call.
 *
 * This is a "null" function which simply performs a return, passing
 * back the contents of the accumulator. When called as the FIRST
 * THING EXECUTED within a "register" function, the value returned
 * will therefore be the number of arguments which were passed to
 * that function.
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
nargs() { }     /* smallest MICRO-C function you can write */
