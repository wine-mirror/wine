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
    char       **exports;     /* functions exported from this dll */
    int          nb_exports;  /* number of exported functions */
    char       **imports;     /* functions we want to import from this dll */
    int          nb_imports;  /* number of imported functions */
};

static char **undef_symbols;  /* list of undefined symbols */
static int nb_undef_symbols, undef_size;

static struct import **dll_imports = NULL;
static int nb_imports = 0;  /* number of imported dlls */
static int total_imports = 0;  /* total number of imported functions */


/* compare function names; helper for resolve_imports */
static int name_cmp( const void *name, const void *entry )
{
    return strcmp( *(char **)name, *(char **)entry );
}

/* locate a symbol in a (sorted) list */
inline static const char *find_symbol( const char *name, char **table, int size )
{
    char **res = bsearch( &name, table, size, sizeof(*table), name_cmp );
    return res ? *res : NULL;
}

/* sort a symbol table */
inline static void sort_symbols( char **table, int size )
{
    qsort( table, size, sizeof(*table), name_cmp );
}

/* open the .so library for a given dll in a specified path */
static char *try_library_path( const char *path, const char *name )
{
    char *buffer, *p;
    int fd;

    buffer = xmalloc( strlen(path) + strlen(name) + 8 );
    sprintf( buffer, "%s/lib%s", path, name );
    p = buffer + strlen(buffer) - 4;
    if (!strcmp( p, ".dll" )) *p = 0;
    strcat( buffer, ".so" );
    /* check if the file exists */
    if ((fd = open( buffer, O_RDONLY )) == -1) return NULL;
    close( fd );
    return buffer;
}

/* open the .so library for a given dll */
static char *open_library( const char *name )
{
    char *fullname;
    int i;

    for (i = 0; i < nb_lib_paths; i++)
    {
        if ((fullname = try_library_path( lib_path[i], name ))) return fullname;
    }
    if (!(fullname = try_library_path( ".", name )))
        fatal_error( "could not open .so file for %s\n", name );
    return fullname;
}

/* read in the list of exported symbols of a .so */
static void read_exported_symbols( const char *name, struct import *imp )
{
    FILE *f;
    char buffer[1024];
    char *fullname, *cmdline;
    const char *ext;
    int size, err;

    imp->exports    = NULL;
    imp->nb_exports = size = 0;

    if (!(ext = strrchr( name, '.' ))) ext = name + strlen(name);

    if (!(fullname = open_library( name ))) return;
    cmdline = xmalloc( strlen(fullname) + 4 );
    sprintf( cmdline, "nm %s", fullname );
    free( fullname );

    if (!(f = popen( cmdline, "r" )))
        fatal_error( "Cannot execute '%s'\n", cmdline );

    while (fgets( buffer, sizeof(buffer), f ))
    {
        char *p = buffer + strlen(buffer) - 1;
        if (p < buffer) continue;
        if (*p == '\n') *p-- = 0;
        if (!(p = strstr( buffer, "__wine_dllexport_" ))) continue;
        p += 17;
        if (strncmp( p, name, ext - name )) continue;
        p += ext - name;
        if (*p++ != '_') continue;

        if (imp->nb_exports == size)
        {
            size += 128;
            imp->exports = xrealloc( imp->exports, size * sizeof(*imp->exports) );
        }
        imp->exports[imp->nb_exports++] = xstrdup( p );
    }
    if ((err = pclose( f ))) fatal_error( "%s error %d\n", cmdline, err );
    free( cmdline );
    sort_symbols( imp->exports, imp->nb_exports );
}

/* add a dll to the list of imports */
void add_import_dll( const char *name )
{
    struct import *imp = xmalloc( sizeof(*imp) );
    imp->dll        = xstrdup( name );
    imp->imports    = NULL;
    imp->nb_imports = 0;

    read_exported_symbols( name, imp );

    dll_imports = xrealloc( dll_imports, (nb_imports+1) * sizeof(*dll_imports) );
    dll_imports[nb_imports++] = imp;
}

/* add a function to the list of imports from a given dll */
static void add_import_func( struct import *imp, const char *name )
{
    imp->imports = xrealloc( imp->imports, (imp->nb_imports+1) * sizeof(*imp->imports) );
    imp->imports[imp->nb_imports++] = xstrdup( name );
    total_imports++;
}

/* add a symbol to the undef list */
inline static void add_undef_symbol( const char *name )
{
    if (nb_undef_symbols == undef_size)
    {
        undef_size += 128;
        undef_symbols = xrealloc( undef_symbols, undef_size * sizeof(*undef_symbols) );
    }
    undef_symbols[nb_undef_symbols++] = xstrdup( name );
}

/* add the extra undefined symbols that will be contained in the generated spec file itself */
static void add_extra_undef_symbols(void)
{
    const char *extras[8];
    int i, count = 0;

#define ADD_SYM(name) \
    do { if (!find_symbol( extras[count] = (name), undef_symbols, \
                           nb_undef_symbols )) count++; } while(0)

    sort_symbols( undef_symbols, nb_undef_symbols );

    /* add symbols that will be contained in the spec file itself */
    switch (SpecMode)
    {
    case SPEC_MODE_GUIEXE:
        ADD_SYM( "GetCommandLineA" );
        ADD_SYM( "GetStartupInfoA" );
        ADD_SYM( "GetModuleHandleA" );
        /* fall through */
    case SPEC_MODE_CUIEXE:
        ADD_SYM( "__wine_get_main_args" );
        ADD_SYM( "ExitProcess" );
        /* fall through */
    case SPEC_MODE_DLL:
    case SPEC_MODE_GUIEXE_NO_MAIN:
    case SPEC_MODE_CUIEXE_NO_MAIN:
        ADD_SYM( "RtlRaiseException" );
        break;
    }
    if (count)
    {
        for (i = 0; i < count; i++) add_undef_symbol( extras[i] );
        sort_symbols( undef_symbols, nb_undef_symbols );
    }
}

/* read in the list of undefined symbols */
void read_undef_symbols( const char *name )
{
    FILE *f;
    char buffer[1024];
    int err;

    undef_size = nb_undef_symbols = 0;

    sprintf( buffer, "nm -u %s", name );
    if (!(f = popen( buffer, "r" )))
        fatal_error( "Cannot execute '%s'\n", buffer );

    while (fgets( buffer, sizeof(buffer), f ))
    {
        char *p = buffer + strlen(buffer) - 1;
        if (p < buffer) continue;
        if (*p == '\n') *p-- = 0;
        add_undef_symbol( buffer );
    }
    if ((err = pclose( f ))) fatal_error( "nm -u %s error %d\n", name, err );
}

/* resolve the imports for a Win32 module */
int resolve_imports( FILE *outfile )
{
    int i, j, off;
    char **p;

    if (!nb_undef_symbols) return 0; /* no symbol file specified */

    add_extra_undef_symbols();

    for (i = 0; i < nb_imports; i++)
    {
        struct import *imp = dll_imports[i];

        for (j = 0; j < nb_undef_symbols; j++)
        {
            const char *res = find_symbol( undef_symbols[j], imp->exports, imp->nb_exports );
            if (res)
            {
                add_import_func( imp, res );
                undef_symbols[j] = NULL;
            }
        }
        /* remove all the holes in the undef symbols list */
        p = undef_symbols;
        for (j = off = 0; j < nb_undef_symbols; j++)
        {
            if (!undef_symbols[j]) off++;
            else undef_symbols[j - off] = undef_symbols[j];
        }
        nb_undef_symbols -= off;
        if (!off) warning( "%s imported but no symbols used\n", imp->dll );
    }
    return 1;
}

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
    fprintf( outfile, "asm(\".align 4\\n\"\n" );
    for (i = 0; i < nb_imports; i++, pos += 4)
    {
        for (j = 0; j < dll_imports[i]->nb_imports; j++, pos += 4)
        {
            fprintf( outfile, "    \"\\t.type " PREFIX "%s,@function\\n\"\n",
                     dll_imports[i]->imports[j] );
            fprintf( outfile, "    \"\\t.globl " PREFIX "%s\\n\"\n",
                     dll_imports[i]->imports[j] );
            fprintf( outfile, "    \"" PREFIX "%s:\\tjmp *(imports+%d)\\n\"\n",
                     dll_imports[i]->imports[j], pos );
            fprintf( outfile, "    \"\\tmovl %%esi,%%esi\\n\"\n" );
        }
    }
    fprintf( outfile, ");\n#ifndef __GNUC__\n}\n#endif\n\n" );

 done:
    return nb_imports;
}
