/*
 * Functions to read and clear a file stream error flag
 *
 * Contributed by Steffen Kaiser
 * ?COPY.TXT 1995-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#include \mc\file.h

/*
 * Return error flag associated with file stream
 */
int ferror(struct FILE_structure *fp)
{
    return fp->FILE_options & F_FERROR;
}

/*
 * Clear the error flag associated with a file stream
 */
clearerr(struct FILE_structure *fp)
{
    fp->FILE_options &= ~F_FERROR;
}
