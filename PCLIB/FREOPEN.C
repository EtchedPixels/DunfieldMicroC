/*
 * Function to REOPEN a file for buffered I/O.
 *
 * ?COPY.TXT 1995-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#include \mc\stdio.h
#include \mc\file.h

FILE *freopen(char *filename, char *mode, FILE *fp)
{
    FILE *fpnew;
    if(fpnew = fopen(filename, mode)) {
        fflush(fp);
        if(!dup2(fpnew->FILE_handle, fp->FILE_handle)) {
            fp->FILE_options = fpnew->FILE_options;
            fclose(fpnew);
            return fp; }
        fclose(fpnew); }
    return 0;
}
