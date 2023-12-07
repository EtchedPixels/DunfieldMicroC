/*
 * DDS MICRO-C IBM/PC: Function to execute system command
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#include \mc\stdio.h

system(command)
    char *command;
{
    char comspec[65], tail[128];

    if(!getenv("COMSPEC", comspec))
        return -1;

    concat(tail, "/C ", command);
    return exec(comspec, tail);
}
