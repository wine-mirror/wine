/* small utility functions for winebuild */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "build.h"

void *xmalloc (size_t size)
{
    void *res;

    res = malloc (size ? size : 1);
    if (res == NULL)
    {
        fprintf (stderr, "Virtual memory exhausted.\n");
        exit (1);
    }
    return res;
}

void *xrealloc (void *ptr, size_t size)
{
    void *res = realloc (ptr, size);
    if (res == NULL)
    {
        fprintf (stderr, "Virtual memory exhausted.\n");
        exit (1);
    }
    return res;
}

char *xstrdup( const char *str )
{
    char *res = strdup( str );
    if (!res)
    {
        fprintf (stderr, "Virtual memory exhausted.\n");
        exit (1);
    }
    return res;
}

char *strupper(char *s)
{
    char *p;
    for (p = s; *p; p++) *p = toupper(*p);
    return s;
}

void fatal_error( const char *msg, ... )
{
    va_list valist;
    va_start( valist, msg );
    if (input_file_name && current_line)
        fprintf( stderr, "%s:%d: ", input_file_name, current_line );
    vfprintf( stderr, msg, valist );
    va_end( valist );
    exit(1);
}

/* dump a byte stream into the assembly code */
void dump_bytes( FILE *outfile, const unsigned char *data, int len, const char *label )
{
    int i;

    fprintf( outfile, "\nstatic unsigned char %s[] = \n{", label );

    for (i = 0; i < len; i++)
    {
        if (!(i & 0x0f)) fprintf( outfile, "\n    " );
        fprintf( outfile, "%d", *data++ );
        if (i < len - 1) fprintf( outfile, ", " );
    }
    fprintf( outfile, "\n};\n" );
}
