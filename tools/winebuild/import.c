/*
 * DLL imports support
 *
 * Copyright 2000 Alexandre Julliard
 */

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "winnt.h"
#include "build.h"

struct import
{
    char        *dll;         /* dll name */
    char       **imports;     /* functions we want to import from this dll */
    int          nb_imports;  /* number of imported functions */
};


static struct import **dll_imports = NULL;
static int nb_imports = 0;  /* number of imported dlls */
static int total_imports = 0;  /* total number of imported functions */


/* add a dll to the list of imports */
void add_import_dll( const char *name )
{
    struct import *imp = xmalloc( sizeof(*imp) );
    imp->dll        = xstrdup( name );
    imp->imports    = NULL;
    imp->nb_imports = 0;

    dll_imports = xrealloc( dll_imports, (nb_imports+1) * sizeof(*dll_imports) );
    dll_imports[nb_imports++] = imp;
}

#ifdef notyet
/* add a function to the list of imports from a given dll */
static void add_import_func( struct import *imp, const char *name )
{
    imp->imports = xrealloc( imp->imports, (imp->nb_imports+1) * sizeof(*imp->imports) );
    imp->imports[imp->nb_imports++] = xstrdup( name );
    total_imports++;
}
#endif

/* output the import table of a Win32 module */
int output_imports( FILE *outfile )
{
    int i, j, pos;

    if (!nb_imports) goto done;

    /* main import header */

    fprintf( outfile, "\n\n/* imports */\n\n" );
    fprintf( outfile, "static struct {\n" );
    fprintf( outfile, "  struct {\n" );
    fprintf( outfile, "    void        *OriginalFirstThunk;\n" );
    fprintf( outfile, "    unsigned int TimeDateStamp;\n" );
    fprintf( outfile, "    unsigned int ForwarderChain;\n" );
    fprintf( outfile, "    const char  *Name;\n" );
    fprintf( outfile, "    void        *FirstThunk;\n" );
    fprintf( outfile, "  } imp[%d];\n", nb_imports+1 );
    fprintf( outfile, "  const char *data[%d];\n", total_imports + nb_imports );
    fprintf( outfile, "} imports = {\n  {\n" );

    /* list of dlls */

    for (i = j = 0; i < nb_imports; i++)
    {
        fprintf( outfile, "    { 0, 0, 0, \"%s\", &imports.data[%d] },\n",
                 dll_imports[i]->dll, j );
        j += dll_imports[i]->nb_imports + 1;
    }
    fprintf( outfile, "    { 0, 0, 0, 0, 0 },\n" );
    fprintf( outfile, "  },\n  {\n" );

    /* list of imported functions */

    for (i = 0; i < nb_imports; i++)
    {
        fprintf( outfile, "    /* %s */\n", dll_imports[i]->dll );
        for (j = 0; j < dll_imports[i]->nb_imports; j++)
            fprintf( outfile, "    \"\\0\\0%s\",\n", dll_imports[i]->imports[j] );
        fprintf( outfile, "    0,\n" );
    }
    fprintf( outfile, "  }\n};\n\n" );

    /* thunks for imported functions */

    fprintf( outfile, "#ifndef __GNUC__\nstatic void __asm__dummy_import(void) {\n#endif\n\n" );
    pos = 20 * (nb_imports + 1);  /* offset of imports.data from start of imports */
    fprintf( outfile, "asm(\".align 4\");\n" );
    for (i = 0; i < nb_imports; i++, pos += 4)
    {
        for (j = 0; j < dll_imports[i]->nb_imports; j++, pos += 4)
        {
            fprintf( outfile,
                     "asm(\".type " PREFIX "%s,@function\\n\\t"
                     ".globl " PREFIX "%s\\n"
                     PREFIX "%s:\\tjmp *(imports+%d)\\n\\t"
                     "movl %%esi,%%esi\");\n",
                     dll_imports[i]->imports[j], dll_imports[i]->imports[j],
                     dll_imports[i]->imports[j], pos );
        }
    }
    fprintf( outfile, "#ifndef __GNUC__\n}\n#endif\n\n" );

 done:
    return nb_imports;
}
