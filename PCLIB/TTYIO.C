/*
 * Basic console TTY I/O functions
 *
 * ?COPY.TXT 1994-2005 Dave Dunfield
 * **See COPY.TXT**.
 */
#include \mc\stdio.h

/*
 * Get character from standard input stream
 */
int getchar()
{
    return getc(stdin);
}

/*
 * Get a string from the standard input stream
 */
char *gets(char *string)
{
    return fgets(string, -1, stdin);
}

/*
 * Write character to standard output stream
 */
putchar(int c)
{
    return putc(c, stdout);
}

/*
 * Write a string to the standard output stream
 */
puts(char *string)
{
    fputs(string, stdout);
    return putchar('\n');
}
