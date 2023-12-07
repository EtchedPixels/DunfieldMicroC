/*
 * Functions to READ/WRITE multiple items to/from a file
 *
 * ?COPY.TXT 1995-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#include \mc\stdio.h

/*
 * Read multiple data items from a file
 */
int fread(char *ptr, unsigned size, unsigned count, FILE *fp)
{
    int i;
    for(i=0; i < count; ++i) {
        if(fget(ptr, size, fp) < size)
            break;
        ptr += size; }
    return i;
}

/*
 * Write multiple data items to a file
 */
int fwrite(char *ptr, unsigned size, unsigned count, FILE *fp)
{
    int i;
    for(i=0; i < count; ++i) {
        if(fput(ptr, size, fp) < size)
            break;
        ptr += size; }
    return i;
}
