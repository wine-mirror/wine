/*
 * DLL imports support
 *
 * Copyright 2000 Alexandre Julliard
 *           2000 Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "build.h"

struct func
{
    char       *name;         /* function name */
    int         ordinal;      /* function ordinal */
    int         ord_only;     /* non-zero if function is imported by ordinal */
};

struct import
{
    char        *dll;         /* dll name */
    int          delay;       /* delay or not dll loading ? */
    struct func *exports;     /* functions exported from this dll */
    int          nb_exports;  /* number of exported functions */
    struct func *imports;     /* functions we want to import from this dll */
    int          nb_imports;  /* number of imported functions */
};

static char **undef_symbols;  /* list of undefined symbols */
static int nb_undef_symbols = -1;
static int undef_size;

static char **ignore_symbols; /* list of symbols to ignore */
static int nb_ignore_symbols;
static int ignore_size;

static char *ld_tmp_file;  /* ld temp file name */

static struct import **dll_imports = NULL;
static int nb_imports = 0;      /* number of imported dlls (delayed or not) */
static int nb_delayed = 0;      /* number of delayed dlls */
static int total_imports = 0;   /* total number of imported functions */
static int total_delayed = 0;   /* total number of imported functions in delayed DLLs */

/* list of symbols that are ignored by default */
static const char * const default_ignored_symbols[] =
{
    "abs",
    "acos",
    "asin",
    "atan",
    "atan2",
    "atof",
    "atoi",
    "atol",
    "bsearch",
    "ceil",
    "cos",
    "cosh",
    "div",
    "exp",
    "fabs",
    "floor",
    "fmod",
    "frexp",
    "labs",
    "ldiv",
    "log",
    "log10",
    "memchr",
    "memcmp",
    "memcpy",
    "memmove",
    "memset",
    "modf",
    "pow",
    "qsort",
    "sin",
    "sinh",
    "sprintf",
    "sqrt",
    "sscanf",
    "strcat",
    "strchr",
    "strcmp",
    "strcpy",
    "strcspn",
    "strlen",
    "strncat",
    "strncmp",
    "strncpy",
    "strpbrk",
    "strrchr",
    "strspn",
    "strstr",
    "tan",
    "tanh",
    "vsprintf"
};

/* compare function names; helper for resolve_imports */
static int name_cmp( const void *name, const void *entry )
{
    return strcmp( *(char **)name, *(char **)entry );
}

/* compare function names; helper for resolve_imports */
static int func_cmp( const void *func1, const void *func2 )
{
    return strcmp( ((struct func *)func1)->name, ((struct func *)func2)->name );
}

/* locate a symbol in a (sorted) list */
inline static const char *find_symbol( const char *name, char **table, int size )
{
    char **res = NULL;

    if (table) {
        res = bsearch( &name, table, size, sizeof(*table), name_cmp );
    }

    return res ? *res : NULL;
}

/* locate an export in a (sorted) export list */
inline static struct func *find_export( const char *name, struct func *table, int size )
{
    struct func func, *res = NULL;

    func.name = (char *)name;
    func.ordinal = -1;
    if (table) res = bsearch( &func, table, size, sizeof(*table), func_cmp );
    return res;
}

/* sort a symbol table */
inline static void sort_symbols( char **table, int size )
{
    if (table )
        qsort( table, size, sizeof(*table), name_cmp );
}

/* free an import structure */
static void free_imports( struct import *imp )
{
    int i;

    for (i = 0; i < imp->nb_exports; i++) free( imp->exports[i].name );
    for (i = 0; i < imp->nb_imports; i++) free( imp->imports[i].name );
    free( imp->exports );
    free( imp->imports );
    free( imp->dll );
    free( imp );
}

/* remove the temp file at exit */
static void remove_ld_tmp_file(void)
{
    if (ld_tmp_file) unlink( ld_tmp_file );
}

/* check whether a given dll has already been imported */
static int is_already_imported( const char *name )
{
    int i;

    for (i = 0; i < nb_imports; i++)
    {
        if (!strcmp( dll_imports[i]->dll, name )) return 1;
    }
    return 0;
}

/* open the .so library for a given dll in a specified path */
static char *try_library_path( const char *path, const char *name )
{
    char *buffer;
    int fd;

    buffer = xmalloc( strlen(path) + strlen(name) + 9 );
    sprintf( buffer, "%s/lib%s.def", path, name );

    /* check if the file exists */
    if ((fd = open( buffer, O_RDONLY )) != -1)
    {
        close( fd );
        return buffer;
    }
    free( buffer );
    return NULL;
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
        fatal_error( "could not open .def file for %s\n", name );
    return fullname;
}

/* skip whitespace until the next token */
static char *skip_whitespace( char *p )
{
    while (*p && isspace(*p)) p++;
    if (!*p || *p == ';') p = NULL;
    return p;
}

/* skip to the start of the next token, null terminating the current one */
static char *next_token( char *p )
{
    while (*p && !isspace(*p)) p++;
    if (*p) *p++ = 0;
    return skip_whitespace( p );
}

/* remove the @nn suffix from stdcall names */
static char *remove_stdcall_decoration( char *buffer )
{
    char *p = buffer + strlen(buffer) - 1;
    while (p > buffer && isdigit(*p)) p--;
    if (p > buffer && *p == '@') *p = 0;
    return buffer;
}

/* read in the list of exported symbols of an import library */
static int read_import_lib( const char *name, struct import *imp )
{
    FILE *f;
    char buffer[1024];
    char *fullname;
    int size;

    imp->exports    = NULL;
    imp->nb_exports = size = 0;

    fullname = open_library( name );
    f = open_input_file( NULL, fullname );
    free( fullname );

    while (fgets( buffer, sizeof(buffer), f ))
    {
        char *name, *flags;
        int ordinal = 0, ord_only = 0;

        char *p = buffer + strlen(buffer) - 1;
        if (p < buffer) goto next;
        if (*p == '\n') *p-- = 0;

        p = buffer;
        if (!(p = skip_whitespace(p))) goto next;
        name = p;
        p = next_token( name );

        if (!strcmp( name, "LIBRARY" ))
        {
            if (!p) fatal_error( "Expected name after LIBRARY\n" );
            name = p;
            p = next_token( name );
            if (p) fatal_error( "Garbage after LIBRARY statement\n" );
            if (is_already_imported( name ))
            {
                close_input_file( f );
                return 0;  /* ignore this dll */
            }
            free( imp->dll );
            imp->dll = xstrdup( name );
            goto next;
        }
        if (!strcmp( name, "EXPORTS" )) goto next;

        /* check for ordinal */
        if (!p) fatal_error( "Expected ordinal after function name\n" );
        if (*p != '@' || !isdigit(p[1]))
            fatal_error( "Expected ordinal after function name '%s'\n", name );
        ordinal = strtol( p+1, &p, 10 );
        if (ordinal >= MAX_ORDINALS) fatal_error( "Invalid ordinal number %d\n", ordinal );

        /* check for optional flags */
        while (p && (p = skip_whitespace(p)))
        {
            flags = p;
            p = next_token( flags );
            if (!strcmp( flags, "NONAME" ))
            {
                ord_only = 1;
                if (!ordinal) fatal_error( "Invalid ordinal number %d\n", ordinal );
            }
            else if (!strcmp( flags, "CONSTANT" ) || !strcmp( flags, "DATA" ))
            {
                /* we don't support importing non-function entry points */
                goto next;
            }
            else fatal_error( "Garbage after ordinal declaration\n" );
        }

        if (imp->nb_exports == size)
        {
            size += 128;
            imp->exports = xrealloc( imp->exports, size * sizeof(*imp->exports) );
        }
        if ((p = strchr( name, '=' ))) *p = 0;
        remove_stdcall_decoration( name );
        imp->exports[imp->nb_exports].name     = xstrdup( name );
        imp->exports[imp->nb_exports].ordinal  = ordinal;
        imp->exports[imp->nb_exports].ord_only = ord_only;
        imp->nb_exports++;
    next:
        current_line++;
    }
    close_input_file( f );
    if (imp->nb_exports)
        qsort( imp->exports, imp->nb_exports, sizeof(*imp->exports), func_cmp );
    return 1;
}

/* add a dll to the list of imports */
void add_import_dll( const char *name, int delay )
{
    struct import *imp;
    char *fullname;

    fullname = xmalloc( strlen(name) + 5 );
    strcpy( fullname, name );
    if (!strchr( fullname, '.' )) strcat( fullname, ".dll" );

    /* check if we already imported it */
    if (is_already_imported( fullname ))
    {
        free( fullname );
        return;
    }

    imp = xmalloc( sizeof(*imp) );
    imp->dll        = fullname;
    imp->delay      = delay;
    imp->imports    = NULL;
    imp->nb_imports = 0;

    if (delay) nb_delayed++;

    if (read_import_lib( name, imp ))
    {
        dll_imports = xrealloc( dll_imports, (nb_imports+1) * sizeof(*dll_imports) );
        dll_imports[nb_imports++] = imp;
    }
    else free_imports( imp );
}

/* initialize the list of ignored symbols */
static void init_ignored_symbols(void)
{
    int i;

    nb_ignore_symbols = sizeof(default_ignored_symbols)/sizeof(default_ignored_symbols[0]);
    ignore_size = nb_ignore_symbols + 32;
    ignore_symbols = xmalloc( ignore_size * sizeof(*ignore_symbols) );
    for (i = 0; i < nb_ignore_symbols; i++)
        ignore_symbols[i] = xstrdup( default_ignored_symbols[i] );
}

/* add a symbol to the ignored symbol list */
/* if the name starts with '-' the symbol is removed instead */
void add_ignore_symbol( const char *name )
{
    int i;

    if (!ignore_symbols) init_ignored_symbols();  /* first time around, fill list with defaults */

    if (name[0] == '-')  /* remove it */
    {
        if (!name[1])  /* remove everything */
        {
            for (i = 0; i < nb_ignore_symbols; i++) free( ignore_symbols[i] );
            nb_ignore_symbols = 0;
        }
        else
        {
            for (i = 0; i < nb_ignore_symbols; i++)
            {
                if (!strcmp( ignore_symbols[i], name+1 ))
                {
                    free( ignore_symbols[i] );
                    memmove( &ignore_symbols[i], &ignore_symbols[i+1], nb_ignore_symbols - i - 1 );
                    nb_ignore_symbols--;
                }
            }
        }
    }
    else
    {
        if (nb_ignore_symbols == ignore_size)
        {
            ignore_size += 32;
            ignore_symbols = xrealloc( ignore_symbols, ignore_size * sizeof(*ignore_symbols) );
        }
        ignore_symbols[nb_ignore_symbols++] = xstrdup( name );
    }
}

/* add a function to the list of imports from a given dll */
static void add_import_func( struct import *imp, const struct func *func )
{
    imp->imports = xrealloc( imp->imports, (imp->nb_imports+1) * sizeof(*imp->imports) );
    imp->imports[imp->nb_imports].name     = xstrdup( func->name );
    imp->imports[imp->nb_imports].ordinal  = func->ordinal;
    imp->imports[imp->nb_imports].ord_only = func->ord_only;
    imp->nb_imports++;
    total_imports++;
    if (imp->delay) total_delayed++;
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

/* remove all the holes in the undefined symbol list; return the number of removed symbols */
static int remove_symbol_holes(void)
{
    int i, off;
    for (i = off = 0; i < nb_undef_symbols; i++)
    {
        if (!undef_symbols[i]) off++;
        else undef_symbols[i - off] = undef_symbols[i];
    }
    nb_undef_symbols -= off;
    return off;
}

/* add a symbol to the extra list, but only if needed */
static int add_extra_symbol( const char **extras, int *count, const char *name )
{
    int i;

    if (!find_symbol( name, undef_symbols, nb_undef_symbols ))
    {
        /* check if the symbol is being exported by this dll */
        for (i = 0; i < nb_entry_points; i++)
        {
            ORDDEF *odp = EntryPoints[i];
            if (odp->type == TYPE_STDCALL ||
                odp->type == TYPE_CDECL ||
                odp->type == TYPE_VARARGS ||
                odp->type == TYPE_EXTERN)
            {
                if (odp->name && !strcmp( odp->name, name )) return 0;
            }
        }
        extras[*count] = name;
        (*count)++;
    }
    return 1;
}

/* add the extra undefined symbols that will be contained in the generated spec file itself */
static void add_extra_undef_symbols(void)
{
    const char *extras[10];
    int i, count = 0, nb_stubs = 0, nb_regs = 0;
    int kernel_imports = 0, ntdll_imports = 0;

    sort_symbols( undef_symbols, nb_undef_symbols );

    for (i = 0; i < nb_entry_points; i++)
    {
        ORDDEF *odp = EntryPoints[i];
        if (odp->type == TYPE_STUB) nb_stubs++;
        if (odp->flags & FLAG_REGISTER) nb_regs++;
    }

    /* add symbols that will be contained in the spec file itself */
    switch (SpecMode)
    {
    case SPEC_MODE_DLL:
        break;
    case SPEC_MODE_GUIEXE:
        kernel_imports += add_extra_symbol( extras, &count, "GetCommandLineA" );
        kernel_imports += add_extra_symbol( extras, &count, "GetStartupInfoA" );
        kernel_imports += add_extra_symbol( extras, &count, "GetModuleHandleA" );
        /* fall through */
    case SPEC_MODE_CUIEXE:
        kernel_imports += add_extra_symbol( extras, &count, "ExitProcess" );
        break;
    case SPEC_MODE_GUIEXE_UNICODE:
        kernel_imports += add_extra_symbol( extras, &count, "GetCommandLineA" );
        kernel_imports += add_extra_symbol( extras, &count, "GetStartupInfoA" );
        kernel_imports += add_extra_symbol( extras, &count, "GetModuleHandleA" );
        /* fall through */
    case SPEC_MODE_CUIEXE_UNICODE:
        kernel_imports += add_extra_symbol( extras, &count, "ExitProcess" );
        break;
    }
    if (nb_delayed)
    {
        kernel_imports += add_extra_symbol( extras, &count, "LoadLibraryA" );
        kernel_imports += add_extra_symbol( extras, &count, "GetProcAddress" );
    }
    if (nb_regs)
        ntdll_imports += add_extra_symbol( extras, &count, "__wine_call_from_32_regs" );
    if (nb_delayed || nb_stubs)
        ntdll_imports += add_extra_symbol( extras, &count, "RtlRaiseException" );

    /* make sure we import the dlls that contain these functions */
    if (kernel_imports) add_import_dll( "kernel32", 0 );
    if (ntdll_imports) add_import_dll( "ntdll", 0 );

    if (count)
    {
        for (i = 0; i < count; i++) add_undef_symbol( extras[i] );
        sort_symbols( undef_symbols, nb_undef_symbols );
    }
}

/* warn if a given dll is not used, but check forwards first */
static void warn_unused( const struct import* imp )
{
    int i;
    size_t len = strlen(imp->dll);
    const char *p = strchr( imp->dll, '.' );
    if (p && !strcasecmp( p, ".dll" )) len = p - imp->dll;

    for (i = Base; i <= Limit; i++)
    {
        ORDDEF *odp = Ordinals[i];
        if (!odp || odp->type != TYPE_FORWARD) continue;
        if (!strncasecmp( odp->link_name, imp->dll, len ) &&
            odp->link_name[len] == '.')
            return;  /* found an import, do not warn */
    }
    warning( "%s imported but no symbols used\n", imp->dll );
}

/* combine a list of object files with ld into a single object file */
/* returns the name of the combined file */
static const char *ldcombine_files( char **argv )
{
    int i, len = 0;
    char *cmd;
    int fd, err;

    if (output_file_name && output_file_name[0])
    {
        ld_tmp_file = xmalloc( strlen(output_file_name) + 8 );
        strcpy( ld_tmp_file, output_file_name );
        strcat( ld_tmp_file, ".XXXXXX" );
    }
    else ld_tmp_file = xstrdup( "/tmp/winebuild.tmp.XXXXXX" );

    if ((fd = mkstemp( ld_tmp_file ) == -1)) fatal_error( "could not generate a temp file\n" );
    close( fd );
    atexit( remove_ld_tmp_file );

    for (i = 0; argv[i]; i++) len += strlen(argv[i]) + 1;
    cmd = xmalloc( len + strlen(ld_tmp_file) + 10 );
    sprintf( cmd, "ld -r -o %s", ld_tmp_file );
    for (i = 0; argv[i]; i++) sprintf( cmd + strlen(cmd), " %s", argv[i] );
    err = system( cmd );
    if (err) fatal_error( "ld -r failed with status %d\n", err );
    free( cmd );
    return ld_tmp_file;
}

/* read in the list of undefined symbols */
void read_undef_symbols( char **argv )
{
    FILE *f;
    char buffer[1024];
    int err;
    const char *name;

    if (!argv[0]) return;

    undef_size = nb_undef_symbols = 0;

    /* if we have multiple object files, link them together */
    if (argv[1]) name = ldcombine_files( argv );
    else name = argv[0];

    sprintf( buffer, "nm -u %s", name );
    if (!(f = popen( buffer, "r" )))
        fatal_error( "Cannot execute '%s'\n", buffer );

    while (fgets( buffer, sizeof(buffer), f ))
    {
        char *p = buffer + strlen(buffer) - 1;
        if (p < buffer) continue;
        if (*p == '\n') *p-- = 0;
        p = buffer; while (*p == ' ') p++;
        add_undef_symbol( p );
    }
    if ((err = pclose( f ))) fatal_error( "nm -u %s error %d\n", name, err );
}

static void remove_ignored_symbols(void)
{
    int i;

    if (!ignore_symbols) init_ignored_symbols();
    sort_symbols( ignore_symbols, nb_ignore_symbols );
    for (i = 0; i < nb_undef_symbols; i++)
    {
        if (find_symbol( undef_symbols[i], ignore_symbols, nb_ignore_symbols ))
        {
            free( undef_symbols[i] );
            undef_symbols[i] = NULL;
        }
    }
    remove_symbol_holes();
}

/* resolve the imports for a Win32 module */
int resolve_imports( void )
{
    int i, j;

    if (nb_undef_symbols == -1) return 0; /* no symbol file specified */

    add_extra_undef_symbols();
    remove_ignored_symbols();

    for (i = 0; i < nb_imports; i++)
    {
        struct import *imp = dll_imports[i];

        for (j = 0; j < nb_undef_symbols; j++)
        {
            struct func *func = find_export( undef_symbols[j], imp->exports, imp->nb_exports );
            if (func)
            {
                add_import_func( imp, func );
                free( undef_symbols[j] );
                undef_symbols[j] = NULL;
            }
        }
        /* remove all the holes in the undef symbols list */
        if (!remove_symbol_holes()) warn_unused( imp );
    }
    return 1;
}

/* output the import table of a Win32 module */
static int output_immediate_imports( FILE *outfile )
{
    int i, j, pos;
    int nb_imm = nb_imports - nb_delayed;

    if (!nb_imm) goto done;

    /* main import header */

    fprintf( outfile, "\nstatic struct {\n" );
    fprintf( outfile, "  struct {\n" );
    fprintf( outfile, "    void        *OriginalFirstThunk;\n" );
    fprintf( outfile, "    unsigned int TimeDateStamp;\n" );
    fprintf( outfile, "    unsigned int ForwarderChain;\n" );
    fprintf( outfile, "    const char  *Name;\n" );
    fprintf( outfile, "    void        *FirstThunk;\n" );
    fprintf( outfile, "  } imp[%d];\n", nb_imm+1 );
    fprintf( outfile, "  const char *data[%d];\n",
             total_imports - total_delayed + nb_imm );
    fprintf( outfile, "} imports = {\n  {\n" );

    /* list of dlls */

    for (i = j = 0; i < nb_imports; i++)
    {
        if (dll_imports[i]->delay) continue;
        fprintf( outfile, "    { 0, 0, 0, \"%s\", &imports.data[%d] },\n",
                 dll_imports[i]->dll, j );
        j += dll_imports[i]->nb_imports + 1;
    }

    fprintf( outfile, "    { 0, 0, 0, 0, 0 },\n" );
    fprintf( outfile, "  },\n  {\n" );

    /* list of imported functions */

    for (i = 0; i < nb_imports; i++)
    {
        if (dll_imports[i]->delay) continue;
        fprintf( outfile, "    /* %s */\n", dll_imports[i]->dll );
        for (j = 0; j < dll_imports[i]->nb_imports; j++)
        {
            struct func *import = &dll_imports[i]->imports[j];
            if (!import->ord_only)
            {
                unsigned short ord = import->ordinal;
                fprintf( outfile, "    \"\\%03o\\%03o%s\",\n",
                         *(unsigned char *)&ord, *((unsigned char *)&ord + 1), import->name );
            }
            else
                fprintf( outfile, "    (char *)%d,\n", import->ordinal );
        }
        fprintf( outfile, "    0,\n" );
    }
    fprintf( outfile, "  }\n};\n\n" );

    /* thunks for imported functions */

    fprintf( outfile, "#ifndef __GNUC__\nstatic void __asm__dummy_import(void) {\n#endif\n\n" );
    pos = 20 * (nb_imm + 1);  /* offset of imports.data from start of imports */
    fprintf( outfile, "asm(\".data\\n\\t.align %d\\n\"\n", get_alignment(8) );
    for (i = 0; i < nb_imports; i++)
    {
        if (dll_imports[i]->delay) continue;
        for (j = 0; j < dll_imports[i]->nb_imports; j++, pos += 4)
        {
            struct func *import = &dll_imports[i]->imports[j];
            fprintf( outfile, "    \"\\t" __ASM_FUNC("%s") "\\n\"\n", import->name );
            fprintf( outfile, "    \"\\t.globl " __ASM_NAME("%s") "\\n\"\n", import->name );
            fprintf( outfile, "    \"" __ASM_NAME("%s") ":\\n\\t", import->name);

#if defined(__i386__)
            if (strstr( import->name, "__wine_call_from_16" ))
                fprintf( outfile, ".byte 0x2e\\n\\tjmp *(imports+%d)\\n\\tnop\\n", pos );
            else
                fprintf( outfile, "jmp *(imports+%d)\\n\\tmovl %%esi,%%esi\\n", pos );
#elif defined(__sparc__)
            if ( !UsePIC )
            {
                fprintf( outfile, "sethi %%hi(imports+%d), %%g1\\n\\t", pos );
                fprintf( outfile, "ld [%%g1+%%lo(imports+%d)], %%g1\\n\\t", pos );
                fprintf( outfile, "jmp %%g1\\n\\tnop\\n" );
            }
            else
            {
                /* Hmpf.  Stupid sparc assembler always interprets global variable
                   names as GOT offsets, so we have to do it the long way ... */
                fprintf( outfile, "save %%sp, -96, %%sp\\n" );
                fprintf( outfile, "0:\\tcall 1f\\n\\tnop\\n" );
                fprintf( outfile, "1:\\tsethi %%hi(imports+%d-0b), %%g1\\n\\t", pos );
                fprintf( outfile, "or %%g1, %%lo(imports+%d-0b), %%g1\\n\\t", pos );
                fprintf( outfile, "ld [%%g1+%%o7], %%g1\\n\\t" );
                fprintf( outfile, "jmp %%g1\\n\\trestore\\n" );
            }

#elif defined(__PPC__)
            fprintf(outfile, "\taddi 1, 1, -0x4\\n\"\n");
            fprintf(outfile, "\t\"\\tstw 9, 0(1)\\n\"\n");
            fprintf(outfile, "\t\"\\taddi 1, 1, -0x4\\n\"\n");
            fprintf(outfile, "\t\"\\tstw 8, 0(1)\\n\"\n");
            fprintf(outfile, "\t\"\\taddi 1, 1, -0x4\\n\"\n");
            fprintf(outfile, "\t\"\\tstw 7, 0(1)\\n\"\n");

            fprintf(outfile, "\t\"\\tlis 9,imports+%d@ha\\n\"\n", pos);
            fprintf(outfile, "\t\"\\tla 8,imports+%d@l(9)\\n\"\n", pos);
            fprintf(outfile, "\t\"\\tlwz 7, 0(8)\\n\"\n");
            fprintf(outfile, "\t\"\\tmtctr 7\\n\"\n");

            fprintf(outfile, "\t\"\\tlwz 7, 0(1)\\n\"\n");
            fprintf(outfile, "\t\"\\taddi 1, 1, 0x4\\n\"\n");
            fprintf(outfile, "\t\"\\tlwz 8, 0(1)\\n\"\n");
            fprintf(outfile, "\t\"\\taddi 1, 1, 0x4\\n\"\n");
            fprintf(outfile, "\t\"\\tlwz 9, 0(1)\\n\"\n");
            fprintf(outfile, "\t\"\\taddi 1, 1, 0x4\\n\"\n");
            fprintf(outfile, "\t\"\\tbctr\\n");
#else
#error You need to define import thunks for your architecture!
#endif
            fprintf( outfile, "\"\n" );
        }
        pos += 4;
    }
    fprintf( outfile, "\".section\\t\\\".text\\\"\");\n#ifndef __GNUC__\n}\n#endif\n\n" );

 done:
    return nb_imm;
}

/* output the delayed import table of a Win32 module */
static int output_delayed_imports( FILE *outfile )
{
    int i, idx, j, pos;

    if (!nb_delayed) goto done;

    for (i = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        fprintf( outfile, "static void *__wine_delay_imp_%d_hmod;\n", i);
        for (j = 0; j < dll_imports[i]->nb_imports; j++)
        {
            fprintf( outfile, "void __wine_delay_imp_%d_%s();\n",
                     i, dll_imports[i]->imports[j].name );
        }
    }
    fprintf( outfile, "\n" );
    fprintf( outfile, "static struct {\n" );
    fprintf( outfile, "  struct ImgDelayDescr {\n" );
    fprintf( outfile, "    unsigned int  grAttrs;\n" );
    fprintf( outfile, "    const char   *szName;\n" );
    fprintf( outfile, "    void        **phmod;\n" );
    fprintf( outfile, "    void        **pIAT;\n" );
    fprintf( outfile, "    const char  **pINT;\n" );
    fprintf( outfile, "    void*         pBoundIAT;\n" );
    fprintf( outfile, "    void*         pUnloadIAT;\n" );
    fprintf( outfile, "    unsigned long dwTimeStamp;\n" );
    fprintf( outfile, "  } imp[%d];\n", nb_delayed );
    fprintf( outfile, "  void         *IAT[%d];\n", total_delayed );
    fprintf( outfile, "  const char   *INT[%d];\n", total_delayed );
    fprintf( outfile, "} delay_imports = {\n" );
    fprintf( outfile, "  {\n" );
    for (i = j = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        fprintf( outfile, "    { 0, \"%s\", &__wine_delay_imp_%d_hmod, &delay_imports.IAT[%d], &delay_imports.INT[%d], 0, 0, 0 },\n",
                 dll_imports[i]->dll, i, j, j );
        j += dll_imports[i]->nb_imports;
    }
    fprintf( outfile, "  },\n  {\n" );
    for (i = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        fprintf( outfile, "    /* %s */\n", dll_imports[i]->dll );
        for (j = 0; j < dll_imports[i]->nb_imports; j++)
        {
            fprintf( outfile, "    &__wine_delay_imp_%d_%s,\n", i, dll_imports[i]->imports[j].name);
        }
    }
    fprintf( outfile, "  },\n  {\n" );
    for (i = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        fprintf( outfile, "    /* %s */\n", dll_imports[i]->dll );
        for (j = 0; j < dll_imports[i]->nb_imports; j++)
        {
            struct func *import = &dll_imports[i]->imports[j];
            if (import->ord_only)
                fprintf( outfile, "    (char *)%d,\n", import->ordinal );
            else
                fprintf( outfile, "    \"%s\",\n", import->name );
        }
    }
    fprintf( outfile, "  }\n};\n\n" );

    /* check if there's some stub defined. if so, exception struct
     *  is already defined, so don't emit it twice
     */
    for (i = 0; i < nb_entry_points; i++) if (EntryPoints[i]->type == TYPE_STUB) break;

    if (i == nb_entry_points) {
       fprintf( outfile, "struct exc_record {\n" );
       fprintf( outfile, "  unsigned int code, flags;\n" );
       fprintf( outfile, "  void *rec, *addr;\n" );
       fprintf( outfile, "  unsigned int params;\n" );
       fprintf( outfile, "  const void *info[15];\n" );
       fprintf( outfile, "};\n\n" );
       fprintf( outfile, "extern void __stdcall RtlRaiseException( struct exc_record * );\n" );
    }

    fprintf( outfile, "extern void * __stdcall LoadLibraryA(const char*);\n");
    fprintf( outfile, "extern void * __stdcall GetProcAddress(void *, const char*);\n");
    fprintf( outfile, "\n" );

    fprintf( outfile, "void *__stdcall __wine_delay_load( int idx_nr )\n" );
    fprintf( outfile, "{\n" );
    fprintf( outfile, "  int idx = idx_nr >> 16, nr = idx_nr & 0xffff;\n" );
    fprintf( outfile, "  struct ImgDelayDescr *imd = delay_imports.imp + idx;\n" );
    fprintf( outfile, "  void **pIAT = imd->pIAT + nr;\n" );
    fprintf( outfile, "  const char** pINT = imd->pINT + nr;\n" );
    fprintf( outfile, "  void *fn;\n\n" );

    fprintf( outfile, "  if (!*imd->phmod) *imd->phmod = LoadLibraryA(imd->szName);\n" );
    fprintf( outfile, "  if (*imd->phmod && (fn = GetProcAddress(*imd->phmod, *pINT)))\n");
    fprintf( outfile, "    /* patch IAT with final value */\n" );
    fprintf( outfile, "    return *pIAT = fn;\n" );
    fprintf( outfile, "  else {\n");
    fprintf( outfile, "    struct exc_record rec;\n" );
    fprintf( outfile, "    rec.code    = 0x80000100;\n" );
    fprintf( outfile, "    rec.flags   = 1;\n" );
    fprintf( outfile, "    rec.rec     = 0;\n" );
    fprintf( outfile, "    rec.params  = 2;\n" );
    fprintf( outfile, "    rec.info[0] = imd->szName;\n" );
    fprintf( outfile, "    rec.info[1] = *pINT + 2;\n" );
    fprintf( outfile, "#ifdef __GNUC__\n" );
    fprintf( outfile, "    rec.addr = __builtin_return_address(1);\n" );
    fprintf( outfile, "#else\n" );
    fprintf( outfile, "    rec.addr = 0;\n" );
    fprintf( outfile, "#endif\n" );
    fprintf( outfile, "    for (;;) RtlRaiseException( &rec );\n" );
    fprintf( outfile, "    return 0; /* shouldn't go here */\n" );
    fprintf( outfile, "  }\n}\n\n" );

    fprintf( outfile, "#ifndef __GNUC__\n" );
    fprintf( outfile, "static void __asm__dummy_delay_import(void) {\n" );
    fprintf( outfile, "#endif\n" );

    fprintf( outfile, "asm(\".align %d\\n\"\n", get_alignment(8) );
    fprintf( outfile, "    \"\\t" __ASM_FUNC("__wine_delay_load_asm") "\\n\"\n" );
    fprintf( outfile, "    \"" __ASM_NAME("__wine_delay_load_asm") ":\\n\"\n" );
#if defined(__i386__)
    fprintf( outfile, "    \"\\tpushl %%ecx\\n\\tpushl %%edx\\n\\tpushl %%eax\\n\"\n" );
    fprintf( outfile, "    \"\\tcall __wine_delay_load\\n\"\n" );
    fprintf( outfile, "    \"\\tpopl %%edx\\n\\tpopl %%ecx\\n\\tjmp *%%eax\\n\"\n" );
#elif defined(__sparc__)
    fprintf( outfile, "    \"\\tsave %%sp, -96, %%sp\\n\"\n" );
    fprintf( outfile, "    \"\\tcall __wine_delay_load\\n\"\n" );
    fprintf( outfile, "    \"\\tmov %%g1, %%o0\\n\"\n" );
    fprintf( outfile, "    \"\\tjmp %%o0\\n\\trestore\\n\"\n" );
#elif defined(__PPC__)
    /* Save all callee saved registers into a stackframe. */
    fprintf( outfile, "    \"\\tstwu %%r1, -48(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tstw  %%r3, 4(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tstw  %%r4, 8(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tstw  %%r5, 12(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tstw  %%r6, 16(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tstw  %%r7, 20(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tstw  %%r8, 24(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tstw  %%r9, 28(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tstw  %%r10, 32(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tstw  %%r11, 36(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tstw  %%r12, 40(%%r1)\\n\"\n" );

    /* r0 -> r3 (arg1) */
    fprintf( outfile, "    \"\\tmr  %%r3, %%r0\\n\"\n" );

    /* save return address */
    fprintf( outfile, "    \"\\tmflr  %%r0\\n\"\n" );
    fprintf( outfile, "    \"\\tstw  %%r0, 44(%%r1)\\n\"\n" );

    /* Call the __wine_delay_load function, arg1 is arg1. */
    fprintf( outfile, "    \"\\tbl __wine_delay_load\\n\"\n" );

    /* Load return value from call into ctr register */
    fprintf( outfile, "    \"\\tmtctr %%r3\\n\"\n" );

    /* restore all saved registers and drop stackframe. */
    fprintf( outfile, "    \"\\tlwz  %%r3, 4(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tlwz  %%r4, 8(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tlwz  %%r5, 12(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tlwz  %%r6, 16(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tlwz  %%r7, 20(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tlwz  %%r8, 24(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tlwz  %%r9, 28(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tlwz  %%r10, 32(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tlwz  %%r11, 36(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tlwz  %%r12, 40(%%r1)\\n\"\n" );
    /* Load return value from call into return register */
    fprintf( outfile, "    \"\\tlwz  %%r0, 44(%%r1)\\n\"\n" );
    fprintf( outfile, "    \"\\tmtlr %%r0\\n\"\n" );
    fprintf( outfile, "    \"\\taddi %%r1, %%r1, 48\\n\"\n" );
    /* branch to ctr register. */
    fprintf( outfile, "    \"\\tbctr\\n\"\n" );

#else
#error You need to defined delayed import thunks for your architecture!
#endif

    for (i = idx = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        for (j = 0; j < dll_imports[i]->nb_imports; j++)
        {
            char buffer[128];
            sprintf( buffer, "__wine_delay_imp_%d_%s", i, dll_imports[i]->imports[j].name );
            fprintf( outfile, "    \"\\t" __ASM_FUNC("%s") "\\n\"\n", buffer );
            fprintf( outfile, "    \"" __ASM_NAME("%s") ":\\n\"\n", buffer );
#if defined(__i386__)
            fprintf( outfile, "    \"\\tmovl $%d, %%eax\\n\"\n", (idx << 16) | j );
            fprintf( outfile, "    \"\\tjmp __wine_delay_load_asm\\n\"\n" );
#elif defined(__sparc__)
            fprintf( outfile, "    \"\\tset %d, %%g1\\n\"\n", (idx << 16) | j );
            fprintf( outfile, "    \"\\tb,a __wine_delay_load_asm\\n\"\n" );
#elif defined(__PPC__)
	    /* g0 is a function scratch register or so I understand. */
	    fprintf( outfile, "    \"\\tli  %%r0, %d\\n\"\n", (idx << 16) | j  );
	    fprintf( outfile, "    \"\\tb  __wine_delay_load_asm\\n\"\n" );
#else
#error You need to defined delayed import thunks for your architecture!
#endif
        }
        idx++;
    }

    fprintf( outfile, "\n    \".data\\n\\t.align %d\\n\"\n", get_alignment(8) );
    pos = nb_delayed * 32;
    for (i = 0; i < nb_imports; i++)
    {
        if (!dll_imports[i]->delay) continue;
        for (j = 0; j < dll_imports[i]->nb_imports; j++, pos += 4)
        {
            struct func *import = &dll_imports[i]->imports[j];
            fprintf( outfile, "    \"\\t" __ASM_FUNC("%s") "\\n\"\n", import->name );
            fprintf( outfile, "    \"\\t.globl " __ASM_NAME("%s") "\\n\"\n", import->name );
            fprintf( outfile, "    \"" __ASM_NAME("%s") ":\\n\\t\"", import->name);
#if defined(__i386__)
            if (strstr( import->name, "__wine_call_from_16" ))
                fprintf( outfile, "\".byte 0x2e\\n\\tjmp *(delay_imports+%d)\\n\\tnop\\n\"", pos );
            else
                fprintf( outfile, "\"jmp *(delay_imports+%d)\\n\\tmovl %%esi,%%esi\\n\"", pos );
#elif defined(__sparc__)
            if ( !UsePIC )
            {
                fprintf( outfile, "\"sethi %%hi(delay_imports+%d), %%g1\\n\\t\"", pos );
                fprintf( outfile, "\"ld [%%g1+%%lo(delay_imports+%d)], %%g1\\n\\t\"", pos );
                fprintf( outfile, "\"jmp %%g1\\n\\tnop\\n\"" );
            }
            else
            {
                /* Hmpf.  Stupid sparc assembler always interprets global variable
                   names as GOT offsets, so we have to do it the long way ... */
                fprintf( outfile, "\"save %%sp, -96, %%sp\\n\"" );
                fprintf( outfile, "\"0:\\tcall 1f\\n\\tnop\\n\"" );
                fprintf( outfile, "\"1:\\tsethi %%hi(delay_imports+%d-0b), %%g1\\n\\t\"", pos );
                fprintf( outfile, "\"or %%g1, %%lo(delay_imports+%d-0b), %%g1\\n\\t\"", pos );
                fprintf( outfile, "\"ld [%%g1+%%o7], %%g1\\n\\t\"" );
                fprintf( outfile, "\"jmp %%g1\\n\\trestore\\n\"" );
            }

#elif defined(__PPC__)
            fprintf(outfile, "\t\"addi 1, 1, -0x4\\n\"\n");
            fprintf(outfile, "\t\"\\tstw 9, 0(1)\\n\"\n");
            fprintf(outfile, "\t\"\\taddi 1, 1, -0x4\\n\"\n");
            fprintf(outfile, "\t\"\\tstw 8, 0(1)\\n\"\n");
            fprintf(outfile, "\t\"\\taddi 1, 1, -0x4\\n\"\n");
            fprintf(outfile, "\t\"\\tstw 7, 0(1)\\n\"\n");

            fprintf(outfile, "\t\"\\tlis 9,delay_imports+%d@ha\\n\"\n", pos);
            fprintf(outfile, "\t\"\\tla 8,delay_imports+%d@l(9)\\n\"\n", pos);
            fprintf(outfile, "\t\"\\tlwz 7, 0(8)\\n\"\n");
            fprintf(outfile, "\t\"\\tmtctr 7\\n\"\n");

            fprintf(outfile, "\t\"\\tlwz 7, 0(1)\\n\"\n");
            fprintf(outfile, "\t\"\\taddi 1, 1, 0x4\\n\"\n");
            fprintf(outfile, "\t\"\\tlwz 8, 0(1)\\n\"\n");
            fprintf(outfile, "\t\"\\taddi 1, 1, 0x4\\n\"\n");
            fprintf(outfile, "\t\"\\tlwz 9, 0(1)\\n\"\n");
            fprintf(outfile, "\t\"\\taddi 1, 1, 0x4\\n\"\n");
            fprintf(outfile, "\t\"\\tbctr\\n\"");

            /*fprintf(outfile, "\t\"li r0,delay_imports\\n\\t\"\n" );
            fprintf(outfile, "\t\"lwz r0, %d(r0)\\n\\t\"\n", pos);
            fprintf(outfile, "\t\"mtctr r0\\n\\t\"\n");
            fprintf(outfile, "\t\"bctr\\n\"");*/
#else
#error You need to define delayed import thunks for your architecture!
#endif
            fprintf( outfile, "\n" );
        }
    }
    fprintf( outfile, "\".section \\\".text\\\"\");\n" );
    fprintf( outfile, "#ifndef __GNUC__\n" );
    fprintf( outfile, "}\n" );
    fprintf( outfile, "#endif\n" );
    fprintf( outfile, "\n" );

 done:
    return nb_delayed;
}

/* output the import and delayed import tables of a Win32 module
 * returns number of DLLs exported in 'immediate' mode
 */
int output_imports( FILE *outfile )
{
   output_delayed_imports( outfile );
   return output_immediate_imports( outfile );
}
