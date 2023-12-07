/*
 * Abort the 'C' program with message
 *
 * ?COPY.TXT 1988-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#include \mc\stdio.h

abort(message)
    char *message;
{
    fputs(message, stderr);
    exit(-1);
}
