/*
 * Set the I/O buffer size for use with an open file
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#include \mc\stdio.h
#include \mc\file.h

FILE *setbuf(fp, size)
    struct FILE_structure *fp;
    unsigned size;
{
    struct FILE_structure *fp1;

    fflush(fp);
    if(fp1 = malloc(size+sizeof(struct FILE_structure))) {
        memcpy(fp1, fp, sizeof(struct FILE_structure));
        free(fp);
        (fp = fp1)->FILE_iob_size = size; }

    return fp;
}
