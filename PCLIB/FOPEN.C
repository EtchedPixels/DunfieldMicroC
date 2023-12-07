/*
 * Functions to OPEN and CLOSE files for buffered I/O.
 *
 * ?COPY.TXT 1990-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#include \mc\stdio.h
#include \mc\file.h

unsigned IOB_size = 256;

/*
 * Open a file for buffered I/O
 */
FILE *fopen(name, options)
    char *name, *options;
{
    int i, c, d, fopt;
    struct FILE_structure *fp;

    fopt = 0;
    while(c = *options++) {
        i = 0;
        while(d = "qv?b?awr"[i]) {
            if(c == d)
                fopt |= (0x80>>i);
            ++i; } }
    if(fp = malloc(IOB_size + sizeof(struct FILE_structure))) {
        if(i = open(name, fopt)) {
            fp->FILE_options    = fopt;             /* Attributes */
            fp->FILE_handle     = i;                /* File handle */
            fp->FILE_iob_size   = IOB_size;         /* Buffer size */
            fp->FILE_io_ptr = fp->FILE_io_top = 0;  /* Access & top */
            return fp; }
        free(fp); }
    if(fopt & F_VERB) {
        fputs(name, stderr);
        fputs(": unable to access\n", stderr); }
    if(fopt & F_QUIT)
        exit(-1);
    return 0;
}

/*
 * Close a file
 */
fclose(fp)
    struct FILE_structure *fp;
{
    int rc;

    rc = fflush(fp) || close(fp->FILE_handle);
    free(fp);
    return rc;
}
