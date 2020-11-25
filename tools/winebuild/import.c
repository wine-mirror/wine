/*
 * DLL imports support
 *
 * Copyright 2000, 2004 Alexandre Julliard
 * Copyright 2000 Eric Pouech
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "wine/list.h"
#include "build.h"

/* standard C functions that are also exported from ntdll */
static const char *stdc_names[] =
{
    "abs",
    "atan",
    "atoi",
    "atol",
    "bsearch",
    "ceil",
    "cos",
    "fabs",
    "floor",
    "isalnum",
    "isalpha",
    "iscntrl",
    "isdigit",
    "isgraph",
    "islower",
    "isprint",
    "ispunct",
    "isspace",
    "isupper",
    "iswalpha",
    "iswctype",
    "iswdigit",
    "iswlower",
    "iswspace",
    "iswxdigit",
    "isxdigit",
    "labs",
    "log",
    "mbstowcs",
    "memchr",
    "memcmp",
    "memcpy",
    "memmove",
    "memset",
    "pow",
    "qsort",
    "sin",
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
    "strnlen",
    "strpbrk",
    "strrchr",
    "strspn",
    "strstr",
    "strtol",
    "strtoul",
    "swprintf",
    "tan",
    "tolower",
    "toupper",
    "towlower",
    "towupper",
    "vsprintf",
    "wcscat",
    "wcschr",
    "wcscmp",
    "wcscpy",
    "wcscspn",
    "wcslen",
    "wcsncat",
    "wcsncmp",
    "wcsncpy",
    "wcspbrk",
    "wcsrchr",
    "wcsspn",
    "wcsstr",
    "wcstok",
    "wcstol",
    "wcstombs",
    "wcstoul"
};

static struct strarray stdc_functions = { stdc_names, ARRAY_SIZE(stdc_names), ARRAY_SIZE(stdc_names) };

struct import_func
{
    const char *name;
    const char *export_name;
    int         ordinal;
    int         hint;
};

struct import
{
    struct list         entry;       /* entry in global dll list */
    char               *dll_name;    /* exported file name of the dll */
    char               *c_name;      /* dll name as a C-compatible identifier */
    char               *full_name;   /* full name of the input file */
    dev_t               dev;         /* device/inode of the input file */
    ino_t               ino;
    ORDDEF            **exports;     /* functions exported from this dll */
    int                 nb_exports;  /* number of exported functions */
    struct import_func *imports;     /* functions we want to import from this dll */
    int                 nb_imports;  /* number of imported functions */
    int                 max_imports; /* size of imports array */
};

static struct strarray undef_symbols;    /* list of undefined symbols */
static struct strarray extra_ld_symbols; /* list of extra symbols that ld should resolve */
static struct strarray delayed_imports;  /* list of delayed import dlls */
static struct strarray ext_link_imports; /* list of external symbols to link to */

static struct list dll_imports = LIST_INIT( dll_imports );
static struct list dll_delayed = LIST_INIT( dll_delayed );

static struct strarray as_files;

static const char import_func_prefix[] = "__wine$func$";
static const char import_ord_prefix[]  = "__wine$ord$";

static inline const char *ppc_reg( int reg )
{
    static const char * const ppc_regs[32] = { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
                                               "r8", "r9", "r10","r11","r12","r13","r14","r15",
                                               "r16","r17","r18","r19","r20","r21","r22","r23",
                                               "r24","r25","r26","r27","r28","r29","r30","r31" };
    if (target_platform == PLATFORM_APPLE) return ppc_regs[reg];
    return ppc_regs[reg] + 1;  /* skip the 'r' */
}

/* compare function names; helper for resolve_imports */
static int name_cmp( const void *name, const void *entry )
{
    return strcmp( *(const char* const *)name, *(const char* const *)entry );
}

/* compare function names; helper for resolve_imports */
static int func_cmp( const void *func1, const void *func2 )
{
    const ORDDEF *odp1 = *(const ORDDEF * const *)func1;
    const ORDDEF *odp2 = *(const ORDDEF * const *)func2;
    return strcmp( odp1->name ? odp1->name : odp1->export_name,
                   odp2->name ? odp2->name : odp2->export_name );
}

/* remove a name from a name table */
static inline void remove_name( struct strarray *table, unsigned int idx )
{
    assert( idx < table->count );
    memmove( table->str + idx, table->str + idx + 1,
             (table->count - idx - 1) * sizeof(*table->str) );
    table->count--;
}

/* locate a name in a (sorted) list */
static inline const char *find_name( const char *name, const struct strarray *table )
{
    char **res = NULL;

    if (table->count) res = bsearch( &name, table->str, table->count, sizeof(*table->str), name_cmp );
    return res ? *res : NULL;
}

/* sort a name table */
static inline void sort_names( struct strarray *table )
{
    if (table->count) qsort( table->str, table->count, sizeof(*table->str), name_cmp );
}

/* locate an export in a (sorted) export list */
static inline ORDDEF *find_export( const char *name, ORDDEF **table, int size )
{
    ORDDEF func, *odp, **res = NULL;

    func.name = xstrdup(name);
    func.ordinal = -1;
    odp = &func;
    if (table) res = bsearch( &odp, table, size, sizeof(*table), func_cmp );
    free( func.name );
    return res ? *res : NULL;
}

/* free an import structure */
static void free_imports( struct import *imp )
{
    free( imp->exports );
    free( imp->imports );
    free( imp->dll_name );
    free( imp->c_name );
    free( imp->full_name );
    free( imp );
}

/* check whether a given dll is imported in delayed mode */
static int is_delayed_import( const char *name )
{
    unsigned int i;

    for (i = 0; i < delayed_imports.count; i++)
    {
        if (!strcmp( delayed_imports.str[i], name )) return 1;
    }
    return 0;
}

/* find an imported dll from its name */
static struct import *find_import_dll( const char *name )
{
    struct import *import;

    LIST_FOR_EACH_ENTRY( import, &dll_imports, struct import, entry )
        if (!strcasecmp( import->dll_name, name )) return import;
    LIST_FOR_EACH_ENTRY( import, &dll_delayed, struct import, entry )
        if (!strcasecmp( import->dll_name, name )) return import;
    return NULL;
}

/* open the .so library for a given dll in a specified path */
static char *try_library_path( const char *path, const char *name )
{
    char *buffer;
    int fd;

    buffer = strmake( "%s/lib%s.def", path, name );

    /* check if the file exists */
    if ((fd = open( buffer, O_RDONLY )) != -1)
    {
        close( fd );
        return buffer;
    }
    free( buffer );
    return NULL;
}

/* find the .def import library for a given dll */
static char *find_library( const char *name )
{
    char *fullname;
    unsigned int i;

    for (i = 0; i < lib_path.count; i++)
    {
        if ((fullname = try_library_path( lib_path.str[i], name ))) return fullname;
    }
    fatal_error( "could not open .def file for %s\n", name );
    return NULL;
}

/* read in the list of exported symbols of an import library */
static DLLSPEC *read_import_lib( struct import *imp )
{
    FILE *f;
    int i;
    struct stat stat;
    struct import *prev_imp;
    DLLSPEC *spec = alloc_dll_spec();

    f = open_input_file( NULL, imp->full_name );
    fstat( fileno(f), &stat );
    imp->dev = stat.st_dev;
    imp->ino = stat.st_ino;
    if (!parse_def_file( f, spec )) exit( 1 );
    close_input_file( f );

    /* check if we already imported that library from a different file */
    if ((prev_imp = find_import_dll( spec->file_name )))
    {
        if (prev_imp->dev != imp->dev || prev_imp->ino != imp->ino)
            fatal_error( "%s and %s have the same export name '%s'\n",
                         prev_imp->full_name, imp->full_name, spec->file_name );
        free_dll_spec( spec );
        return NULL;  /* the same file was already loaded, ignore this one */
    }

    if (spec->nb_entry_points)
    {
        imp->exports = xmalloc( spec->nb_entry_points * sizeof(*imp->exports) );
        for (i = 0; i < spec->nb_entry_points; i++)
            imp->exports[imp->nb_exports++] = &spec->entry_points[i];
        qsort( imp->exports, imp->nb_exports, sizeof(*imp->exports), func_cmp );
    }
    return spec;
}

/* build the dll exported name from the import lib name or path */
static char *get_dll_name( const char *name, const char *filename )
{
    char *ret;

    if (filename)
    {
        const char *basename = strrchr( filename, '/' );
        if (!basename) basename = filename;
        else basename++;
        if (!strncmp( basename, "lib", 3 )) basename += 3;
        ret = xmalloc( strlen(basename) + 5 );
        strcpy( ret, basename );
        if (strendswith( ret, ".def" )) ret[strlen(ret)-4] = 0;
    }
    else
    {
        ret = xmalloc( strlen(name) + 5 );
        strcpy( ret, name );
    }
    if (!strchr( ret, '.' )) strcat( ret, ".dll" );
    return ret;
}

/* add a dll to the list of imports */
void add_import_dll( const char *name, const char *filename )
{
    DLLSPEC *spec;
    char *dll_name = get_dll_name( name, filename );
    struct import *imp = xmalloc( sizeof(*imp) );

    memset( imp, 0, sizeof(*imp) );

    if (filename) imp->full_name = xstrdup( filename );
    else imp->full_name = find_library( name );

    if (!(spec = read_import_lib( imp )))
    {
        free_imports( imp );
        return;
    }

    imp->dll_name = spec->file_name ? spec->file_name : dll_name;
    imp->c_name = make_c_identifier( imp->dll_name );

    if (is_delayed_import( imp->dll_name ))
        list_add_tail( &dll_delayed, &imp->entry );
    else
        list_add_tail( &dll_imports, &imp->entry );
}

/* add a library to the list of delayed imports */
void add_delayed_import( const char *name )
{
    struct import *imp;
    char *fullname = get_dll_name( name, NULL );

    strarray_add( &delayed_imports, fullname, NULL );
    if ((imp = find_import_dll( fullname )))
    {
        list_remove( &imp->entry );
        list_add_tail( &dll_delayed, &imp->entry );
    }
}

/* add a symbol to the list of extra symbols that ld must resolve */
void add_extra_ld_symbol( const char *name )
{
    strarray_add( &extra_ld_symbols, name, NULL );
}

/* retrieve an imported dll, adding one if necessary */
struct import *add_static_import_dll( const char *name )
{
    struct import *import;
    char *dll_name = get_dll_name( name, NULL );

    if ((import = find_import_dll( dll_name ))) return import;

    import = xmalloc( sizeof(*import) );
    memset( import, 0, sizeof(*import) );

    import->dll_name = dll_name;
    import->full_name = xstrdup( dll_name );
    import->c_name = make_c_identifier( dll_name );

    if (is_delayed_import( dll_name ))
        list_add_tail( &dll_delayed, &import->entry );
    else
        list_add_tail( &dll_imports, &import->entry );
    return import;
}

/* add a function to the list of imports from a given dll */
static void add_import_func( struct import *imp, const char *name, const char *export_name,
                             int ordinal, int hint )
{
    if (imp->nb_imports == imp->max_imports)
    {
        imp->max_imports *= 2;
        if (imp->max_imports < 32) imp->max_imports = 32;
        imp->imports = xrealloc( imp->imports, imp->max_imports * sizeof(*imp->imports) );
    }
    imp->imports[imp->nb_imports].name = name;
    imp->imports[imp->nb_imports].export_name = export_name;
    imp->imports[imp->nb_imports].ordinal = ordinal;
    imp->imports[imp->nb_imports].hint = hint;
    imp->nb_imports++;
}

/* add an import for an undefined function of the form __wine$func$ */
static void add_undef_import( const char *name, int is_ordinal )
{
    char *p, *dll_name = xstrdup( name );
    int ordinal = 0;
    struct import *import;

    if (!(p = strchr( dll_name, '$' ))) return;
    *p++ = 0;
    while (*p >= '0' && *p <= '9') ordinal = 10 * ordinal + *p++ - '0';
    if (*p != '$') return;
    p++;

    import = add_static_import_dll( dll_name );
    if (is_ordinal)
        add_import_func( import, NULL, xstrdup( p ), ordinal, 0 );
    else
        add_import_func( import, xstrdup( p ), NULL, ordinal, 0 );
}

/* check if the spec file exports any stubs */
static int has_stubs( const DLLSPEC *spec )
{
    int i;

    if (unix_lib) return 0;

    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];
        if (odp->type == TYPE_STUB) return 1;
    }
    return 0;
}

/* add the extra undefined symbols that will be contained in the generated spec file itself */
static void add_extra_undef_symbols( DLLSPEC *spec )
{
    add_extra_ld_symbol( spec->init_func );
    if (spec->type == SPEC_WIN16) add_extra_ld_symbol( "DllMain" );
    if (has_stubs( spec )) add_extra_ld_symbol( "__wine_spec_unimplemented_stub" );
    if (delayed_imports.count) add_extra_ld_symbol( "__wine_spec_delay_load" );
}

/* check if a given imported dll is not needed, taking forwards into account */
static int check_unused( const struct import* imp, const DLLSPEC *spec )
{
    int i;
    const char *file_name = imp->dll_name;
    size_t len = strlen( file_name );
    const char *p = strchr( file_name, '.' );
    if (p && !strcasecmp( p, ".dll" )) len = p - file_name;

    for (i = spec->base; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp || !(odp->flags & FLAG_FORWARD)) continue;
        if (!strncasecmp( odp->link_name, file_name, len ) &&
            odp->link_name[len] == '.')
            return 0;  /* found a forward, it is used */
    }
    return 1;
}

/* check if a given forward does exist in one of the imported dlls */
static void check_undefined_forwards( DLLSPEC *spec )
{
    struct import *imp;
    char *link_name, *api_name, *dll_name, *p;
    int i;

    if (unix_lib) return;

    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];

        if (!(odp->flags & FLAG_FORWARD)) continue;

        link_name = xstrdup( odp->link_name );
        p = strrchr( link_name, '.' );
        *p = 0;
        api_name = p + 1;
        dll_name = get_dll_name( link_name, NULL );

        if ((imp = find_import_dll( dll_name )))
        {
            if (!find_export( api_name, imp->exports, imp->nb_exports ))
                warning( "%s:%d: forward '%s' not found in %s\n",
                         spec->src_name, odp->lineno, odp->link_name, imp->dll_name );
        }
        else warning( "%s:%d: forward '%s' not found in the imported dll list\n",
                      spec->src_name, odp->lineno, odp->link_name );
        free( link_name );
        free( dll_name );
    }
}

/* flag the dll exports that link to an undefined symbol */
static void check_undefined_exports( DLLSPEC *spec )
{
    int i;

    if (unix_lib) return;

    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];
        if (odp->type == TYPE_STUB || odp->type == TYPE_ABS || odp->type == TYPE_VARIABLE) continue;
        if (odp->flags & FLAG_FORWARD) continue;
        if (odp->flags & FLAG_SYSCALL) continue;
        if (find_name( odp->link_name, &undef_symbols ))
        {
            switch(odp->type)
            {
            case TYPE_PASCAL:
            case TYPE_STDCALL:
            case TYPE_CDECL:
            case TYPE_VARARGS:
                if (link_ext_symbols)
                {
                    odp->flags |= FLAG_EXT_LINK;
                    strarray_add( &ext_link_imports, odp->link_name, NULL );
                }
                else error( "%s:%d: function '%s' not defined\n",
                            spec->src_name, odp->lineno, odp->link_name );
                break;
            default:
                if (!strcmp( odp->link_name, "__wine_syscall_dispatcher" )) break;
                error( "%s:%d: external symbol '%s' is not a function\n",
                       spec->src_name, odp->lineno, odp->link_name );
                break;
            }
        }
    }
}

/* create a .o file that references all the undefined symbols we want to resolve */
static char *create_undef_symbols_file( DLLSPEC *spec )
{
    char *as_file, *obj_file;
    int i;
    unsigned int j;

    if (unix_lib) return NULL;

    as_file = open_temp_output_file( ".s" );
    output( "\t.data\n" );

    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];
        if (odp->type == TYPE_STUB || odp->type == TYPE_ABS || odp->type == TYPE_VARIABLE) continue;
        if (odp->flags & FLAG_FORWARD) continue;
        if (odp->flags & FLAG_SYSCALL) continue;
        output( "\t%s %s\n", get_asm_ptr_keyword(), asm_name( get_link_name( odp )));
    }
    for (j = 0; j < extra_ld_symbols.count; j++)
        output( "\t%s %s\n", get_asm_ptr_keyword(), asm_name(extra_ld_symbols.str[j]) );
    fclose( output_file );

    obj_file = get_temp_file_name( output_file_name, ".o" );
    assemble_file( as_file, obj_file );
    return obj_file;
}

/* combine a list of object files with ld into a single object file */
/* returns the name of the combined file */
static const char *ldcombine_files( DLLSPEC *spec, char **argv )
{
    char *ld_tmp_file, *undef_file;
    struct strarray args = get_ld_command();

    undef_file = create_undef_symbols_file( spec );
    ld_tmp_file = get_temp_file_name( output_file_name, ".o" );

    strarray_add( &args, "-r", "-o", ld_tmp_file, undef_file, NULL );
    strarray_addv( &args, argv );
    spawn( args );
    return ld_tmp_file;
}

/* read in the list of undefined symbols */
void read_undef_symbols( DLLSPEC *spec, char **argv )
{
    size_t prefix_len;
    FILE *f;
    const char *prog = get_nm_command();
    char *cmd, buffer[1024], name_prefix[16];
    int err;
    const char *name;

    if (!argv[0]) return;

    add_extra_undef_symbols( spec );

    strcpy( name_prefix, asm_name("") );
    prefix_len = strlen( name_prefix );

    name = ldcombine_files( spec, argv );

    cmd = strmake( "%s -u %s", prog, name );
    if (verbose)
        fprintf( stderr, "%s\n", cmd );
    if (!(f = popen( cmd, "r" )))
        fatal_error( "Cannot execute '%s'\n", cmd );

    while (fgets( buffer, sizeof(buffer), f ))
    {
        char *p = buffer + strlen(buffer) - 1;
        if (p < buffer) continue;
        if (*p == '\n') *p-- = 0;
        p = buffer;
        while (*p == ' ') p++;
        if (p[0] == 'U' && p[1] == ' ' && p[2]) p += 2;
        if (prefix_len && !strncmp( p, name_prefix, prefix_len )) p += prefix_len;
        if (!strncmp( p, import_func_prefix, strlen(import_func_prefix) ))
            add_undef_import( p + strlen( import_func_prefix ), 0 );
        else if (!strncmp( p, import_ord_prefix, strlen(import_ord_prefix) ))
            add_undef_import( p + strlen( import_ord_prefix ), 1 );
        else if (use_msvcrt || !find_name( p, &stdc_functions ))
            strarray_add( &undef_symbols, xstrdup( p ), NULL );
    }
    if ((err = pclose( f ))) warning( "%s failed with status %d\n", cmd, err );
    free( cmd );
}

void resolve_dll_imports( DLLSPEC *spec, struct list *list )
{
    unsigned int j;
    struct import *imp, *next;
    ORDDEF *odp;

    LIST_FOR_EACH_ENTRY_SAFE( imp, next, list, struct import, entry )
    {
        for (j = 0; j < undef_symbols.count; j++)
        {
            odp = find_export( undef_symbols.str[j], imp->exports, imp->nb_exports );
            if (odp)
            {
                if (odp->flags & FLAG_PRIVATE) continue;
                if (odp->type != TYPE_STDCALL && odp->type != TYPE_CDECL)
                    warning( "winebuild: Data export '%s' cannot be imported from %s\n",
                             odp->link_name, imp->dll_name );
                else
                {
                    add_import_func( imp, (odp->flags & FLAG_NONAME) ? NULL : odp->name,
                                     odp->export_name, odp->ordinal, odp->hint );
                    remove_name( &undef_symbols, j-- );
                }
            }
        }
        if (!imp->nb_imports)
        {
            /* the dll is not used, get rid of it */
            if (check_unused( imp, spec ))
                warning( "winebuild: %s imported but no symbols used\n", imp->dll_name );
            list_remove( &imp->entry );
            free_imports( imp );
        }
    }
}

/* resolve the imports for a Win32 module */
void resolve_imports( DLLSPEC *spec )
{
    check_undefined_forwards( spec );
    resolve_dll_imports( spec, &dll_imports );
    resolve_dll_imports( spec, &dll_delayed );
    sort_names( &undef_symbols );
    check_undefined_exports( spec );
}

/* check if symbol is still undefined */
int is_undefined( const char *name )
{
    return find_name( name, &undef_symbols ) != NULL;
}

/* output the get_pc thunk if needed */
void output_get_pc_thunk(void)
{
    assert( target_cpu == CPU_x86 );
    output( "\n\t.text\n" );
    output( "\t.align %d\n", get_alignment(4) );
    output( "\t%s\n", func_declaration("__wine_spec_get_pc_thunk_eax") );
    output( "%s:\n", asm_name("__wine_spec_get_pc_thunk_eax") );
    output_cfi( ".cfi_startproc" );
    output( "\tmovl (%%esp),%%eax\n" );
    output( "\tret\n" );
    output_cfi( ".cfi_endproc" );
    output_function_size( "__wine_spec_get_pc_thunk_eax" );
}

/* output a single import thunk */
static void output_import_thunk( const char *name, const char *table, int pos )
{
    output( "\n\t.align %d\n", get_alignment(4) );
    output( "\t%s\n", func_declaration(name) );
    output( "%s\n", asm_globl(name) );
    output_cfi( ".cfi_startproc" );

    switch(target_cpu)
    {
    case CPU_x86:
        if (!UsePIC)
        {
            output( "\tjmp *(%s+%d)\n", table, pos );
        }
        else
        {
            output( "\tcall %s\n", asm_name("__wine_spec_get_pc_thunk_eax") );
            output( "1:\tjmp *%s+%d-1b(%%eax)\n", table, pos );
            needs_get_pc_thunk = 1;
        }
        break;
    case CPU_x86_64:
        output( "\tjmpq *%s+%d(%%rip)\n", table, pos );
        break;
    case CPU_ARM:
        output( "\tldr IP,1f\n");
        output( "\tldr PC,[PC,IP]\n" );
        output( "1:\t.long %s+%u-(1b+4)\n", table, pos );
        break;
    case CPU_ARM64:
        output( "\tadrp x9, %s\n", arm64_page( table ) );
        output( "\tadd x9, x9, #%s\n", arm64_pageoff( table ) );
        if (pos & 0xf000) output( "\tadd x9, x9, #%u\n", pos & 0xf000 );
        if (pos & 0x0f00) output( "\tadd x9, x9, #%u\n", pos & 0x0f00 );
        if (pos & 0x00f0) output( "\tadd x9, x9, #%u\n", pos & 0x00f0 );
        if (pos & 0x000f) output( "\tadd x9, x9, #%u\n", pos & 0x000f );
        output( "\tldur x9, [x9, #0]\n" );
        output( "\tbr x9\n" );
        break;
    case CPU_POWERPC:
        output( "\tmr %s, %s\n", ppc_reg(0), ppc_reg(31) );
        if (target_platform == PLATFORM_APPLE)
        {
            output( "\tlis %s, ha16(%s+%d+32768)\n", ppc_reg(31), table, pos );
            output( "\tla  %s, lo16(%s+%d)(%s)\n", ppc_reg(31), table, pos, ppc_reg(31) );
        }
        else
        {
            output( "\tlis %s, (%s+%d+32768)@h\n", ppc_reg(31), table, pos );
            output( "\tla  %s, (%s+%d)@l(%s)\n", ppc_reg(31), table, pos, ppc_reg(31) );
        }
        output( "\tlwz   %s, 0(%s)\n", ppc_reg(31), ppc_reg(31) );
        output( "\tmtctr %s\n", ppc_reg(31) );
        output( "\tmr    %s, %s\n", ppc_reg(31), ppc_reg(0) );
        output( "\tbctr\n" );
        break;
    }
    output_cfi( ".cfi_endproc" );
    output_function_size( name );
}

/* check if we need an import directory */
int has_imports(void)
{
    return !list_empty( &dll_imports );
}

/* output the import table of a Win32 module */
static void output_immediate_imports(void)
{
    int i, j;
    struct import *import;

    if (list_empty( &dll_imports )) return;  /* no immediate imports */

    /* main import header */

    output( "\n/* import table */\n" );
    output( "\n\t.data\n" );
    output( "\t.align %d\n", get_alignment(4) );
    output( ".L__wine_spec_imports:\n" );

    /* list of dlls */

    j = 0;
    LIST_FOR_EACH_ENTRY( import, &dll_imports, struct import, entry )
    {
        output_rva( ".L__wine_spec_import_data_names + %d", j * get_ptr_size() ); /* OriginalFirstThunk */
        output( "\t.long 0\n" );     /* TimeDateStamp */
        output( "\t.long 0\n" );     /* ForwarderChain */
        output_rva( ".L__wine_spec_import_name_%s", import->c_name ); /* Name */
        output_rva( ".L__wine_spec_import_data_ptrs + %d", j * get_ptr_size() );  /* FirstThunk */
        j += import->nb_imports + 1;
    }
    output( "\t.long 0\n" );     /* OriginalFirstThunk */
    output( "\t.long 0\n" );     /* TimeDateStamp */
    output( "\t.long 0\n" );     /* ForwarderChain */
    output( "\t.long 0\n" );     /* Name */
    output( "\t.long 0\n" );     /* FirstThunk */

    output( "\n\t.align %d\n", get_alignment(get_ptr_size()) );
    /* output the names twice, once for OriginalFirstThunk and once for FirstThunk */
    for (i = 0; i < 2; i++)
    {
        output( ".L__wine_spec_import_data_%s:\n", i ? "ptrs" : "names" );
        LIST_FOR_EACH_ENTRY( import, &dll_imports, struct import, entry )
        {
            for (j = 0; j < import->nb_imports; j++)
            {
                struct import_func *func = &import->imports[j];
                if (i)
                {
                    if (func->name) output( "__imp_%s:\n", asm_name( func->name ));
                    else if (func->export_name) output( "__imp_%s:\n", asm_name( func->export_name ));
                }
                if (func->name)
                    output( "\t%s .L__wine_spec_import_data_%s_%s-.L__wine_spec_rva_base\n",
                            get_asm_ptr_keyword(), import->c_name, func->name );
                else
                {
                    if (get_ptr_size() == 8)
                        output( "\t.quad 0x800000000000%04x\n", func->ordinal );
                    else
                        output( "\t.long 0x8000%04x\n", func->ordinal );
                }
            }
            output( "\t%s 0\n", get_asm_ptr_keyword() );
        }
    }
    output( ".L__wine_spec_imports_end:\n" );

    LIST_FOR_EACH_ENTRY( import, &dll_imports, struct import, entry )
    {
        for (j = 0; j < import->nb_imports; j++)
        {
            struct import_func *func = &import->imports[j];
            if (!func->name) continue;
            output( "\t.align %d\n", get_alignment(2) );
            output( ".L__wine_spec_import_data_%s_%s:\n", import->c_name, func->name );
            output( "\t.short %d\n", func->hint );
            output( "\t%s \"%s\"\n", get_asm_string_keyword(), func->name );
        }
    }

    LIST_FOR_EACH_ENTRY( import, &dll_imports, struct import, entry )
    {
        output( ".L__wine_spec_import_name_%s:\n\t%s \"%s\"\n",
                import->c_name, get_asm_string_keyword(), import->dll_name );
    }
}

/* output the import thunks of a Win32 module */
static void output_immediate_import_thunks(void)
{
    int j, pos;
    struct import *import;
    static const char import_thunks[] = "__wine_spec_import_thunks";

    if (list_empty( &dll_imports )) return;

    output( "\n/* immediate import thunks */\n\n" );
    output( "\t.text\n" );
    output( "\t.align %d\n", get_alignment(8) );
    output( "%s:\n", asm_name(import_thunks));

    pos = 0;
    LIST_FOR_EACH_ENTRY( import, &dll_imports, struct import, entry )
    {
        for (j = 0; j < import->nb_imports; j++, pos += get_ptr_size())
        {
            struct import_func *func = &import->imports[j];
            output_import_thunk( func->name ? func->name : func->export_name,
                                 ".L__wine_spec_import_data_ptrs", pos );
        }
        pos += get_ptr_size();
    }
    output_function_size( import_thunks );
}

/* output the delayed import table of a Win32 module */
static void output_delayed_imports( const DLLSPEC *spec )
{
    int j, mod;
    struct import *import;

    if (list_empty( &dll_delayed )) return;

    output( "\n/* delayed imports */\n\n" );
    output( "\t.data\n" );
    output( "\t.align %d\n", get_alignment(get_ptr_size()) );
    output( "%s\n", asm_globl("__wine_spec_delay_imports") );

    /* list of dlls */

    j = mod = 0;
    LIST_FOR_EACH_ENTRY( import, &dll_delayed, struct import, entry )
    {
        output( "\t%s 0\n", get_asm_ptr_keyword() );   /* grAttrs */
        output( "\t%s .L__wine_delay_name_%s\n",       /* szName */
                 get_asm_ptr_keyword(), import->c_name );
        output( "\t%s .L__wine_delay_modules+%d\n",    /* phmod */
                 get_asm_ptr_keyword(), mod * get_ptr_size() );
        output( "\t%s .L__wine_delay_IAT+%d\n",        /* pIAT */
                 get_asm_ptr_keyword(), j * get_ptr_size() );
        output( "\t%s .L__wine_delay_INT+%d\n",        /* pINT */
                 get_asm_ptr_keyword(), j * get_ptr_size() );
        output( "\t%s 0\n", get_asm_ptr_keyword() );   /* pBoundIAT */
        output( "\t%s 0\n", get_asm_ptr_keyword() );   /* pUnloadIAT */
        output( "\t%s 0\n", get_asm_ptr_keyword() );   /* dwTimeStamp */
        j += import->nb_imports;
        mod++;
    }
    output( "\t%s 0\n", get_asm_ptr_keyword() );   /* grAttrs */
    output( "\t%s 0\n", get_asm_ptr_keyword() );   /* szName */
    output( "\t%s 0\n", get_asm_ptr_keyword() );   /* phmod */
    output( "\t%s 0\n", get_asm_ptr_keyword() );   /* pIAT */
    output( "\t%s 0\n", get_asm_ptr_keyword() );   /* pINT */
    output( "\t%s 0\n", get_asm_ptr_keyword() );   /* pBoundIAT */
    output( "\t%s 0\n", get_asm_ptr_keyword() );   /* pUnloadIAT */
    output( "\t%s 0\n", get_asm_ptr_keyword() );   /* dwTimeStamp */

    output( "\n.L__wine_delay_IAT:\n" );
    LIST_FOR_EACH_ENTRY( import, &dll_delayed, struct import, entry )
    {
        for (j = 0; j < import->nb_imports; j++)
        {
            struct import_func *func = &import->imports[j];
            const char *name = func->name ? func->name : func->export_name;
            output( "__imp_%s:\n", asm_name( name ));
            output( "\t%s .L__wine_delay_imp_%s_%s\n",
                    get_asm_ptr_keyword(), import->c_name, name );
        }
    }

    output( "\n.L__wine_delay_INT:\n" );
    LIST_FOR_EACH_ENTRY( import, &dll_delayed, struct import, entry )
    {
        for (j = 0; j < import->nb_imports; j++)
        {
            struct import_func *func = &import->imports[j];
            if (!func->name)
                output( "\t%s %d\n", get_asm_ptr_keyword(), func->ordinal );
            else
                output( "\t%s .L__wine_delay_data_%s_%s\n",
                        get_asm_ptr_keyword(), import->c_name, func->name );
        }
    }

    output( "\n.L__wine_delay_modules:\n" );
    LIST_FOR_EACH_ENTRY( import, &dll_delayed, struct import, entry )
    {
        output( "\t%s 0\n", get_asm_ptr_keyword() );
    }

    LIST_FOR_EACH_ENTRY( import, &dll_delayed, struct import, entry )
    {
        output( ".L__wine_delay_name_%s:\n", import->c_name );
        output( "\t%s \"%s\"\n", get_asm_string_keyword(), import->dll_name );
    }

    LIST_FOR_EACH_ENTRY( import, &dll_delayed, struct import, entry )
    {
        for (j = 0; j < import->nb_imports; j++)
        {
            struct import_func *func = &import->imports[j];
            if (!func->name) continue;
            output( ".L__wine_delay_data_%s_%s:\n", import->c_name, func->name );
            output( "\t%s \"%s\"\n", get_asm_string_keyword(), func->name );
        }
    }
    output_function_size( "__wine_spec_delay_imports" );
}

/* output the delayed import thunks of a Win32 module */
static void output_delayed_import_thunks( const DLLSPEC *spec )
{
    int idx, j, pos, extra_stack_storage = 0;
    struct import *import;
    static const char delayed_import_loaders[] = "__wine_spec_delayed_import_loaders";
    static const char delayed_import_thunks[] = "__wine_spec_delayed_import_thunks";

    if (list_empty( &dll_delayed )) return;

    output( "\n/* delayed import thunks */\n\n" );
    output( "\t.text\n" );
    output( "\t.align %d\n", get_alignment(8) );
    output( "%s:\n", asm_name(delayed_import_loaders));
    output( "\t%s\n", func_declaration("__wine_delay_load_asm") );
    output( "%s:\n", asm_name("__wine_delay_load_asm") );
    output_cfi( ".cfi_startproc" );
    switch(target_cpu)
    {
    case CPU_x86:
        output( "\tpushl %%ecx\n" );
        output_cfi( ".cfi_adjust_cfa_offset 4" );
        output( "\tpushl %%edx\n" );
        output_cfi( ".cfi_adjust_cfa_offset 4" );
        output( "\tpushl %%eax\n" );
        output_cfi( ".cfi_adjust_cfa_offset 4" );
        output( "\tcall %s\n", asm_name("__wine_spec_delay_load") );
        output_cfi( ".cfi_adjust_cfa_offset -4" );
        output( "\tpopl %%edx\n" );
        output_cfi( ".cfi_adjust_cfa_offset -4" );
        output( "\tpopl %%ecx\n" );
        output_cfi( ".cfi_adjust_cfa_offset -4" );
        output( "\tjmp *%%eax\n" );
        break;
    case CPU_x86_64:
        output( "\tsubq $0x98,%%rsp\n" );
        output_cfi( ".cfi_adjust_cfa_offset 0x98" );
        output( "\tmovq %%rdx,0x88(%%rsp)\n" );
        output( "\tmovq %%rcx,0x80(%%rsp)\n" );
        output( "\tmovq %%r8,0x78(%%rsp)\n" );
        output( "\tmovq %%r9,0x70(%%rsp)\n" );
        output( "\tmovq %%r10,0x68(%%rsp)\n" );
        output( "\tmovq %%r11,0x60(%%rsp)\n" );
        output( "\tmovups %%xmm0,0x50(%%rsp)\n" );
        output( "\tmovups %%xmm1,0x40(%%rsp)\n" );
        output( "\tmovups %%xmm2,0x30(%%rsp)\n" );
        output( "\tmovups %%xmm3,0x20(%%rsp)\n" );
        output( "\tmovq %%rax,%%rcx\n" );
        output( "\tcall %s\n", asm_name("__wine_spec_delay_load") );
        output( "\tmovups 0x20(%%rsp),%%xmm3\n" );
        output( "\tmovups 0x30(%%rsp),%%xmm2\n" );
        output( "\tmovups 0x40(%%rsp),%%xmm1\n" );
        output( "\tmovups 0x50(%%rsp),%%xmm0\n" );
        output( "\tmovq 0x60(%%rsp),%%r11\n" );
        output( "\tmovq 0x68(%%rsp),%%r10\n" );
        output( "\tmovq 0x70(%%rsp),%%r9\n" );
        output( "\tmovq 0x78(%%rsp),%%r8\n" );
        output( "\tmovq 0x80(%%rsp),%%rcx\n" );
        output( "\tmovq 0x88(%%rsp),%%rdx\n" );
        output( "\taddq $0x98,%%rsp\n" );
        output_cfi( ".cfi_adjust_cfa_offset -0x98" );
        output( "\tjmp *%%rax\n" );
        break;
    case CPU_ARM:
        output( "\tpush {r0-r3,FP,LR}\n" );
        output( "\tmov r0,IP\n" );
        output( "\tldr IP,2f\n");
        output( "\tadd IP,PC\n");
        output( "\tblx IP\n");
        output( "1:\tmov IP,r0\n");
        output( "\tpop {r0-r3,FP,LR}\n" );
        output( "\tbx IP\n");
        output( "2:\t.long %s-1b\n", asm_name("__wine_spec_delay_load") );
        break;
    case CPU_ARM64:
        output( "\tstp x29, x30, [sp,#-16]!\n" );
        output( "\tmov x29, sp\n" );
        output( "\tadrp x9, %s\n", arm64_page( asm_name("__wine_spec_delay_load") ) );
        output( "\tadd x9, x9, #%s\n", arm64_pageoff( asm_name("__wine_spec_delay_load") ) );
        output( "\tblr x9\n" );
        output( "\tmov x9, x0\n" );
        output( "\tldp x29, x30, [sp],#16\n" );
        output( "\tldp x0, x1, [sp,#16]\n" );
        output( "\tldp x2, x3, [sp,#32]\n" );
        output( "\tldp x4, x5, [sp,#48]\n" );
        output( "\tldp x6, x7, [sp],#80\n" );
        output( "\tbr x9\n" ); /* or "ret x9" */
        break;
    case CPU_POWERPC:
        if (target_platform == PLATFORM_APPLE) extra_stack_storage = 56;

        /* Save all callee saved registers into a stackframe. */
        output( "\tstwu %s, -%d(%s)\n",ppc_reg(1), 48+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(3),  4+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(4),  8+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(5), 12+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(6), 16+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(7), 20+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(8), 24+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(9), 28+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(10),32+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(11),36+extra_stack_storage, ppc_reg(1));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(12),40+extra_stack_storage, ppc_reg(1));

        /* r0 -> r3 (arg1) */
        output( "\tmr %s, %s\n", ppc_reg(3), ppc_reg(0));

        /* save return address */
        output( "\tmflr %s\n", ppc_reg(0));
        output( "\tstw  %s, %d(%s)\n", ppc_reg(0), 44+extra_stack_storage, ppc_reg(1));

        /* Call the __wine_delay_load function, arg1 is arg1. */
        output( "\tbl %s\n", asm_name("__wine_spec_delay_load") );

        /* Load return value from call into ctr register */
        output( "\tmtctr %s\n", ppc_reg(3));

        /* restore all saved registers and drop stackframe. */
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(3),  4+extra_stack_storage, ppc_reg(1));
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(4),  8+extra_stack_storage, ppc_reg(1));
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(5), 12+extra_stack_storage, ppc_reg(1));
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(6), 16+extra_stack_storage, ppc_reg(1));
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(7), 20+extra_stack_storage, ppc_reg(1));
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(8), 24+extra_stack_storage, ppc_reg(1));
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(9), 28+extra_stack_storage, ppc_reg(1));
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(10),32+extra_stack_storage, ppc_reg(1));
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(11),36+extra_stack_storage, ppc_reg(1));
        output( "\tlwz  %s, %d(%s)\n", ppc_reg(12),40+extra_stack_storage, ppc_reg(1));

        /* Load return value from call into return register */
        output( "\tlwz  %s,  %d(%s)\n", ppc_reg(0), 44+extra_stack_storage, ppc_reg(1));
        output( "\tmtlr %s\n", ppc_reg(0));
        output( "\taddi %s, %s, %d\n", ppc_reg(1), ppc_reg(1),  48+extra_stack_storage);

        /* branch to ctr register. */
        output( "\tbctr\n");
        break;
    }
    output_cfi( ".cfi_endproc" );
    output_function_size( "__wine_delay_load_asm" );
    output( "\n" );

    idx = 0;
    LIST_FOR_EACH_ENTRY( import, &dll_delayed, struct import, entry )
    {
        for (j = 0; j < import->nb_imports; j++)
        {
            struct import_func *func = &import->imports[j];
            const char *name = func->name ? func->name : func->export_name;

            output( ".L__wine_delay_imp_%s_%s:\n", import->c_name, name );
            output_cfi( ".cfi_startproc" );
            switch(target_cpu)
            {
            case CPU_x86:
                output( "\tmovl $%d, %%eax\n", (idx << 16) | j );
                output( "\tjmp %s\n", asm_name("__wine_delay_load_asm") );
                break;
            case CPU_x86_64:
                output( "\tmovq $%d,%%rax\n", (idx << 16) | j );
                output( "\tjmp %s\n", asm_name("__wine_delay_load_asm") );
                break;
            case CPU_ARM:
            {
                unsigned int mask, count = 0, val = (idx << 16) | j;

                for (mask = 0xff; mask; mask <<= 8)
                    if (val & mask) output( "\t%s IP,#%u\n", count++ ? "add" : "mov", val & mask );
                if (!count) output( "\tmov IP,#0\n" );
                output( "\tb %s\n", asm_name("__wine_delay_load_asm") );
                break;
            }
            case CPU_ARM64:
                output( "\tstp x6, x7, [sp,#-80]!\n" );
                output( "\tstp x4, x5, [sp,#48]\n" );
                output( "\tstp x2, x3, [sp,#32]\n" );
                output( "\tstp x0, x1, [sp,#16]\n" );
                output( "\tmov x0, #%d\n", idx );
                output( "\tmov x1, #16384\n" );
                output( "\tmul x1, x0, x1\n" );
                output( "\tmov x0, x1\n" );
                output( "\tmov x1, #4\n" );
                output( "\tmul x1, x0, x1\n" );
                output( "\tmov x0, x1\n" );
                output( "\tadd x0, x0, #%d\n", j );
                output( "\tadr x9, %s\n", asm_name("__wine_delay_load_asm") );
                output( "\tbr x9\n" );
                break;
            case CPU_POWERPC:
                switch(target_platform)
                {
                case PLATFORM_APPLE:
                    /* On Darwin we can use r0 and r2 */
                    /* Upper part in r2 */
                    output( "\tlis %s, %d\n", ppc_reg(2), idx);
                    /* Lower part + r2 -> r0, Note we can't use r0 directly */
                    output( "\taddi %s, %s, %d\n", ppc_reg(0), ppc_reg(2), j);
                    output( "\tb %s\n", asm_name("__wine_delay_load_asm") );
                    break;
                default:
                    /* On linux we can't use r2 since r2 is not a scratch register (hold the TOC) */
                    /* Save r13 on the stack */
                    output( "\taddi %s, %s, -0x4\n", ppc_reg(1), ppc_reg(1));
                    output( "\tstw  %s, 0(%s)\n",    ppc_reg(13), ppc_reg(1));
                    /* Upper part in r13 */
                    output( "\tlis %s, %d\n", ppc_reg(13), idx);
                    /* Lower part + r13 -> r0, Note we can't use r0 directly */
                    output( "\taddi %s, %s, %d\n", ppc_reg(0), ppc_reg(13), j);
                    /* Restore r13 */
                    output( "\tstw  %s, 0(%s)\n",    ppc_reg(13), ppc_reg(1));
                    output( "\taddic %s, %s, 0x4\n", ppc_reg(1), ppc_reg(1));
                    output( "\tb %s\n", asm_name("__wine_delay_load_asm") );
                    break;
                }
                break;
            }
            output_cfi( ".cfi_endproc" );
        }
        idx++;
    }
    output_function_size( delayed_import_loaders );

    output( "\n\t.align %d\n", get_alignment(get_ptr_size()) );
    output( "%s:\n", asm_name(delayed_import_thunks));
    pos = 0;
    LIST_FOR_EACH_ENTRY( import, &dll_delayed, struct import, entry )
    {
        for (j = 0; j < import->nb_imports; j++, pos += get_ptr_size())
        {
            struct import_func *func = &import->imports[j];
            output_import_thunk( func->name ? func->name : func->export_name,
                                 ".L__wine_delay_IAT", pos );
        }
    }
    output_function_size( delayed_import_thunks );
}

/* output import stubs for exported entry points that link to external symbols */
static void output_external_link_imports( DLLSPEC *spec )
{
    unsigned int i, pos;

    if (!ext_link_imports.count) return;  /* nothing to do */

    sort_names( &ext_link_imports );

    /* get rid of duplicate names */
    for (i = 1; i < ext_link_imports.count; i++)
    {
        if (!strcmp( ext_link_imports.str[i-1], ext_link_imports.str[i] ))
            remove_name( &ext_link_imports, i-- );
    }

    output( "\n/* external link thunks */\n\n" );
    output( "\t.data\n" );
    output( "\t.align %d\n", get_alignment(get_ptr_size()) );
    output( ".L__wine_spec_external_links:\n" );
    for (i = 0; i < ext_link_imports.count; i++)
        output( "\t%s %s\n", get_asm_ptr_keyword(), asm_name(ext_link_imports.str[i]) );

    output( "\n\t.text\n" );
    output( "\t.align %d\n", get_alignment(get_ptr_size()) );
    output( "%s:\n", asm_name("__wine_spec_external_link_thunks") );

    for (i = pos = 0; i < ext_link_imports.count; i++)
    {
        char *buffer = strmake( "__wine_spec_ext_link_%s", ext_link_imports.str[i] );
        output_import_thunk( buffer, ".L__wine_spec_external_links", pos );
        free( buffer );
        pos += get_ptr_size();
    }
    output_function_size( "__wine_spec_external_link_thunks" );
}

/*******************************************************************
 *         output_stubs
 *
 * Output the functions for stub entry points
 */
void output_stubs( DLLSPEC *spec )
{
    const char *name, *exp_name;
    int i;

    if (!has_stubs( spec )) return;

    output( "\n/* stub functions */\n\n" );
    output( "\t.text\n" );

    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];
        if (odp->type != TYPE_STUB) continue;

        name = get_stub_name( odp, spec );
        exp_name = odp->name ? odp->name : odp->export_name;
        output( "\t.align %d\n", get_alignment(4) );
        output( "\t%s\n", func_declaration(name) );
        output( "%s:\n", asm_name(name) );
        output_cfi( ".cfi_startproc" );

        switch (target_cpu)
        {
        case CPU_x86:
            /* flesh out the stub a bit to make safedisc happy */
            output(" \tnop\n" );
            output(" \tnop\n" );
            output(" \tnop\n" );
            output(" \tnop\n" );
            output(" \tnop\n" );
            output(" \tnop\n" );
            output(" \tnop\n" );
            output(" \tnop\n" );
            output(" \tnop\n" );

            output( "\tsubl $12,%%esp\n" );
            output_cfi( ".cfi_adjust_cfa_offset 12" );
            if (UsePIC)
            {
                output( "\tcall %s\n", asm_name("__wine_spec_get_pc_thunk_eax") );
                output( "1:" );
                needs_get_pc_thunk = 1;
                if (exp_name)
                {
                    output( "\tleal .L%s_string-1b(%%eax),%%ecx\n", name );
                    output( "\tmovl %%ecx,4(%%esp)\n" );
                }
                else
                    output( "\tmovl $%d,4(%%esp)\n", odp->ordinal );
                output( "\tleal .L__wine_spec_file_name-1b(%%eax),%%ecx\n" );
                output( "\tmovl %%ecx,(%%esp)\n" );
            }
            else
            {
                if (exp_name)
                    output( "\tmovl $.L%s_string,4(%%esp)\n", name );
                else
                    output( "\tmovl $%d,4(%%esp)\n", odp->ordinal );
                output( "\tmovl $.L__wine_spec_file_name,(%%esp)\n" );
            }
            output( "\tcall %s\n", asm_name("__wine_spec_unimplemented_stub") );
            break;
        case CPU_x86_64:
            output( "\tsubq $0x28,%%rsp\n" );
            output_cfi( ".cfi_adjust_cfa_offset 8" );
            output( "\tleaq .L__wine_spec_file_name(%%rip),%%rcx\n" );
            if (exp_name)
                output( "leaq .L%s_string(%%rip),%%rdx\n", name );
            else
                output( "\tmovq $%d,%%rdx\n", odp->ordinal );
            output( "\tcall %s\n", asm_name("__wine_spec_unimplemented_stub") );
            break;
        case CPU_ARM:
            output( "\tldr r0,2f\n");
            output( "\tadd r0,PC\n");
            output( "\tldr r1,2f+4\n");
            output( "1:" );
            if (exp_name) output( "\tadd r1,PC\n");
            output( "\tbl %s\n", asm_name("__wine_spec_unimplemented_stub") );
            output( "2:\t.long .L__wine_spec_file_name-1b\n" );
            if (exp_name) output( "\t.long .L%s_string-2b\n", name );
            else output( "\t.long %u\n", odp->ordinal );
            break;
        case CPU_ARM64:
            output( "\tadrp x0, %s\n", arm64_page(".L__wine_spec_file_name") );
            output( "\tadd x0, x0, #%s\n", arm64_pageoff(".L__wine_spec_file_name") );
            if (exp_name)
            {
                char *sym = strmake( ".L%s_string", name );
                output( "\tadrp x1, %s\n", arm64_page( sym ) );
                output( "\tadd x1, x1, #%s\n", arm64_pageoff( sym ) );
                free( sym );
            }
            else
                output( "\tmov x1, %u\n", odp->ordinal );
            output( "\tadrp x2, %s\n", arm64_page( asm_name("__wine_spec_unimplemented_stub") ) );
            output( "\tadd x2, x2, #%s\n", arm64_pageoff( asm_name("__wine_spec_unimplemented_stub") ) );
            output( "\tblr x2\n" );
            break;
        default:
            assert(0);
        }
        output_cfi( ".cfi_endproc" );
        output_function_size( name );
    }

    output( "\t%s\n", get_asm_string_section() );
    output( ".L__wine_spec_file_name:\n" );
    output( "\t%s \"%s\"\n", get_asm_string_keyword(), spec->file_name );
    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];
        if (odp->type != TYPE_STUB) continue;
        exp_name = odp->name ? odp->name : odp->export_name;
        if (exp_name)
        {
            name = get_stub_name( odp, spec );
            output( ".L%s_string:\n", name );
            output( "\t%s \"%s\"\n", get_asm_string_keyword(), exp_name );
        }
    }
}

static int cmp_link_name( const void *e1, const void *e2 )
{
    const ORDDEF *odp1 = *(const ORDDEF * const *)e1;
    const ORDDEF *odp2 = *(const ORDDEF * const *)e2;

    return strcmp( odp1->link_name, odp2->link_name );
}


/* output the functions for system calls */
void output_syscalls( DLLSPEC *spec )
{
    const unsigned int invalid_param = 0xc000000d; /* STATUS_INVALID_PARAMETER */
    int i, count;
    ORDDEF **syscalls = NULL;

    for (i = count = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];
        if (!(odp->flags & FLAG_SYSCALL)) continue;
        if (!syscalls) syscalls = xmalloc( (spec->nb_entry_points - i) * sizeof(*syscalls) );
        syscalls[count++] = odp;
    }
    if (!count) return;
    count = sort_func_list( syscalls, count, cmp_link_name );

    output( "\n/* system calls */\n\n" );
    output( "\t.text\n" );

    if (unix_lib)
    {
        output( "\t.align %d\n", get_alignment(4) );
        output( "\t%s\n", func_declaration("__wine_syscall_dispatcher") );
        output( "%s\n", asm_globl("__wine_syscall_dispatcher") );
        output_cfi( ".cfi_startproc" );
        switch (target_cpu)
        {
        case CPU_x86:
            output( "\tpushl %%ebp\n" );
            output_cfi( ".cfi_adjust_cfa_offset 4\n" );
            output_cfi( ".cfi_rel_offset %%ebp,0\n" );
            output( "\tmovl %%esp,%%ebp\n" );
            output_cfi( ".cfi_def_cfa_register %%ebp\n" );
            output( "\tpushl %%ebx\n" );
            output_cfi( ".cfi_rel_offset %%ebx,-4\n" );
            output( "\tpushl %%esi\n" );
            output_cfi( ".cfi_rel_offset %%esi,-8\n" );
            output( "\tpushl %%edi\n" );
            output_cfi( ".cfi_rel_offset %%edi,-12\n" );
            output( "\tmovl %%esp,%%fs:0x1f8\n" );  /* x86_thread_data()->syscall_frame */
            output( "\tcmpl $%u,%%eax\n", count );
            output( "\tjae 3f\n" );
            if (UsePIC)
            {
                output( "\tmovl %%eax,%%edx\n" );
                output( "\tcall %s\n", asm_name("__wine_spec_get_pc_thunk_eax") );
                output( "1:\tmovzbl .Lsyscall_args-1b(%%eax,%%edx,1),%%ecx\n" );
                needs_get_pc_thunk = 1;
            }
            else output( "\tmovzbl .Lsyscall_args(%%eax),%%ecx\n" );
            output( "\tsubl %%ecx,%%esp\n" );
            output( "\tshrl $2,%%ecx\n" );
            output( "\tleal 12(%%ebp),%%esi\n" );
            output( "\tandl $~15,%%esp\n" );
            output( "\tmovl %%esp,%%edi\n" );
            output( "\tcld\n" );
            output( "\trep; movsl\n" );
            if (UsePIC)
                output( "\tcall *.Lsyscall_table-1b(%%eax,%%edx,4)\n" );
            else
                output( "\tcall *.Lsyscall_table(,%%eax,4)\n" );
            output( "\tleal -12(%%ebp),%%esp\n" );
            output( "2:\tmovl $0,%%fs:0x1f8\n" );
            output( "\tpopl %%edi\n" );
            output_cfi( ".cfi_same_value %%edi\n" );
            output( "\tpopl %%esi\n" );
            output_cfi( ".cfi_same_value %%esi\n" );
            output( "\tpopl %%ebx\n" );
            output_cfi( ".cfi_same_value %%ebx\n" );
            output( "\tpopl %%ebp\n" );
            output_cfi( ".cfi_def_cfa %%esp,4\n" );
            output_cfi( ".cfi_same_value %%ebp\n" );
            output( "\tret\n" );
            output( "3:\tmovl $0x%x,%%eax\n", invalid_param );
            output( "\tjmp 2b\n" );
            break;
        case CPU_x86_64:
            output( "\tpushq %%rbp\n" );
            output_cfi( ".cfi_adjust_cfa_offset 8" );
            output_cfi( ".cfi_rel_offset %%rbp,0" );
            output( "\tmovq %%rsp,%%rbp\n" );
            output_cfi( ".cfi_def_cfa_register %%rbp" );
            output( "\tleaq -0xe0(%%rbp),%%rsp\n" );
            output( "\tmovq %%gs:0x30,%%rcx\n" );
            output( "\tmovdqu %%xmm6,-0xe0(%%rbp)\n" );
            output( "\tmovdqu %%xmm7,-0xd0(%%rbp)\n" );
            output( "\tmovdqu %%xmm8,-0xc0(%%rbp)\n" );
            output( "\tmovdqu %%xmm9,-0xb0(%%rbp)\n" );
            output( "\tmovdqu %%xmm10,-0xa0(%%rbp)\n" );
            output( "\tmovdqu %%xmm11,-0x90(%%rbp)\n" );
            output( "\tmovdqu %%xmm12,-0x80(%%rbp)\n" );
            output( "\tmovdqu %%xmm13,-0x70(%%rbp)\n" );
            output( "\tmovdqu %%xmm14,-0x60(%%rbp)\n" );
            output( "\tmovdqu %%xmm15,-0x50(%%rbp)\n" );
            output( "\tstmxcsr -0x40(%%rbp)\n" );
            output( "\tmovq %%r12,-0x38(%%rbp)\n" );
            output( "\tmovq %%r13,-0x30(%%rbp)\n" );
            output( "\tmovq %%r14,-0x28(%%rbp)\n" );
            output( "\tmovq %%r15,-0x20(%%rbp)\n" );
            output( "\tmovq %%rdi,-0x18(%%rbp)\n" );
            output_cfi( ".cfi_rel_offset %%rdi,-24" );
            output( "\tmovq %%rsi,-0x10(%%rbp)\n" );
            output_cfi( ".cfi_rel_offset %%rsi,-16" );
            output( "\tmovq %%rbx,-0x08(%%rbp)\n" );
            output_cfi( ".cfi_rel_offset %%rbx,-8" );
            /* Legends of Runeterra hooks the first system call return instruction, and
             * depends on us returning to it. Adjust the return address accordingly. */
            output( "\tsubq $0xb,0x8(%%rbp)\n" );
            output( "\tmovq %%rsp,0x328(%%rcx)\n" );  /* amd64_thread_data()->syscall_frame */
            output( "\tcmpq $%u,%%rax\n", count );
            output( "\tjae 4f\n" );
            output( "\tleaq .Lsyscall_args(%%rip),%%rcx\n" );
            output( "\tmovzbl (%%rcx,%%rax),%%ecx\n" );
            output( "\tsubq $0x20,%%rcx\n" );
            output( "\tja 1f\n" );
            output( "\tandq $~15,%%rsp\n\t" );
            output( "\tjmp 2f\n" );
            output( "1:\tsubq %%rcx,%%rsp\n" );
            output( "\tshrq $3,%%rcx\n" );
            output( "\tleaq 0x38(%%rbp),%%rsi\n" );
            output( "\tandq $~15,%%rsp\n\t" );
            output( "\tmovq %%rsp,%%rdi\n" );
            output( "\tcld\n" );
            output( "\trep; movsq\n" );
            output( "2:\tmovq %%r10,%%rcx\n" );
            output( "\tsubq $0x20,%%rsp\n" );
            output( "\tleaq .Lsyscall_table(%%rip),%%r10\n" );
            output( "\tcallq *(%%r10,%%rax,8)\n" );
            output( "3:\tmovq %%gs:0x30,%%rcx\n" );
            output( "\tmovq $0,0x328(%%rcx)\n" );
            output( "\tmovq -0x18(%%rbp),%%rdi\n" );
            output_cfi( ".cfi_same_value %%rdi" );
            output( "\tmovq -0x10(%%rbp),%%rsi\n" );
            output_cfi( ".cfi_same_value %%rsi" );
            output( "\tmovq -0x8(%%rbp),%%rbx\n" );
            output_cfi( ".cfi_same_value %%rbx" );
            output_cfi( ".cfi_def_cfa_register %%rsp" );
            output( "\tleave\n" );
            output_cfi( ".cfi_adjust_cfa_offset -8" );
            output_cfi( ".cfi_same_value %%rbp" );
            output( "\tret\n" );
            output( "4:\tmovl $0x%x,%%eax\n", invalid_param );
            output( "\tjmp 3b\n" );
            break;
        case CPU_ARM:
            output( "\tpush {r5-r11,lr}\n" );
            output( "\tadd r6, sp, #40\n" );  /* stack parameters */
            output( "\tldr r5, 8f\n" );
            output( "\tcmp r4, r5\n" );
            output( "\tbcs 5f\n" );
            output( "\tsub sp, sp, #8\n" );
            output( "\tpush {r0-r3}\n" );
            output( "\tbl %s\n", asm_name("NtCurrentTeb") );
            output( "\tadd r7, r0, #0x1d8\n" );  /* arm_thread_data()->syscall_frame */
            output( "\tpop {r0-r3}\n" );
            output( "\tmrs ip, CPSR\n" );
            output( "\tstr ip, [sp, #4]\n" );
            output( "\tstr sp, [r7]\n" );  /* syscall frame */
            output( "\tldr r5, 7f\n");
            output( "\tadd r5, pc\n");
            output( "\tldrb r5, [r5, r4]\n" );  /* syscall args */
            output( "1:\tsubs r5, #16\n" );   /* first 4 args are in registers */
            output( "\tble 3f\n" );
            output( "\tsub sp, r5\n" );
            output( "\tand sp, #~7\n" );
            output( "2:\tsubs r5, r5, #4\n" );
            output( "\tldr ip, [r6, r5]\n" );
            output( "\tstr ip, [sp, r5]\n" );
            output( "\tbgt 2b\n" );
            output( "3:\tldr r5, 6f\n");
            output( "\tadd r5, pc\n");
            output( "\tldr ip, [r5, r4, lsl #2]\n");  /* syscall table */
            output( "4:\tblx ip\n");
            output( "\tmov ip, #0\n" );
            output( "\tstr ip, [r7]\n" );
            output( "\tsub sp, r6, #40\n" );
            output( "\tpop {r5-r11,pc}\n" );
            output( "5:\tldr r0, 9f\n" );
            output( "\tpop {r5-r11,pc}\n" );
            output( "6:\t.long .Lsyscall_table-4b\n" );
            output( "7:\t.long .Lsyscall_args-1b\n" );
            output( "8:\t.long %u\n", count );
            output( "9:\t.long 0x%x\n", invalid_param );
            break;
        case CPU_ARM64:
            output( "\tcmp x8, %u\n", count );
            output( "\tbcs 3f\n" );
            output( "\tstp x29, x30, [sp,#-160]!\n" );
            output_cfi( "\t.cfi_def_cfa_offset 160\n" );
            output_cfi( "\t.cfi_offset 29, -160\n" );
            output_cfi( "\t.cfi_offset 30, -152\n" );
            output( "\tmov x29, sp\n" );
            output_cfi( "\t.cfi_def_cfa_register 29\n" );
            output( "\tstp x27, x28, [sp, #144]\n" );
            output_cfi( "\t.cfi_offset 27, -16\n" );
            output_cfi( "\t.cfi_offset 28, -8\n" );
            output( "\tstp x25, x26, [sp, #128]\n" );
            output_cfi( "\t.cfi_offset 25, -32\n" );
            output_cfi( "\t.cfi_offset 26, -24\n" );
            output( "\tstp x23, x24, [sp, #112]\n" );
            output_cfi( "\t.cfi_offset 23, -48\n" );
            output_cfi( "\t.cfi_offset 24, -40\n" );
            output( "\tstp x21, x22, [sp, #96]\n" );
            output_cfi( "\t.cfi_offset 21, -64\n" );
            output_cfi( "\t.cfi_offset 22, -56\n" );
            output( "\tstp x19, x20, [sp, #80]\n" );
            output_cfi( "\t.cfi_offset 19, -80\n" );
            output_cfi( "\t.cfi_offset 20, -72\n" );
            output( "\tstp x6, x7, [sp, #64]\n" );
            output( "\tstp x4, x5, [sp, #48]\n" );
            output( "\tstp x2, x3, [sp, #32]\n" );
            output( "\tstp x0, x1, [sp, #16]\n" );
            output( "\tmov x20, x8\n" );
            output( "\tbl %s\n", asm_name("NtCurrentTeb") );
            output( "\tadd x19, x0, #0x2f8\n" );  /* arm64_thread_data()->syscall_frame */
            output( "\tstr x29, [x19]\n" );
            output( "\tldp x0, x1, [sp, #16]\n" );
            output( "\tldp x2, x3, [sp, #32]\n" );
            output( "\tldp x4, x5, [sp, #48]\n" );
            output( "\tldp x6, x7, [sp, #64]\n" );
            output( "\tadrp x16, %s\n", arm64_page(".Lsyscall_args") );
            output( "\tadd x16, x16, #%s\n", arm64_pageoff(".Lsyscall_args") );
            output( "\tldrb w9, [x16, x20]\n" );
            output( "\tsubs x9, x9, #64\n" );
            output( "\tbls 2f\n" );
            output( "\tadd x11, x29, #176\n" );
            output( "\tsub sp, sp, x9\n" );
            output( "\ttbz x9, #3, 1f\n" );
            output( "\tsub sp, sp, #8\n" );
            output( "1:\tsub x9, x9, #8\n" );
            output( "\tldr x10, [x11, x9]\n" );
            output( "\tstr x10, [sp, x9]\n" );
            output( "\tcbnz x9, 1b\n" );
            output( "2:\tadrp x16, %s\n", arm64_page(".Lsyscall_table") );
            output( "\tadd x16, x16, #%s\n", arm64_pageoff(".Lsyscall_table") );
            output( "\tldr x16, [x16, x20, lsl 3]\n" );
            output( "\tblr x16\n" );
            output( "\tmov sp, x29\n" );
            output( "\tstr xzr, [x19]\n" );
            output( "\tldp x19, x20, [sp, #80]\n" );
            output( "\tldp x21, x22, [sp, #96]\n" );
            output( "\tldp x23, x24, [sp, #112]\n" );
            output( "\tldp x25, x26, [sp, #128]\n" );
            output( "\tldp x27, x28, [sp, #144]\n" );
            output( "\tldp x29, x30, [sp], #160\n" );
            output( "\tret\n" );
            output( "3:\tmov x0, #0x%x\n", invalid_param & 0xffff0000 );
            output( "\tmovk x0, #0x%x\n", invalid_param & 0x0000ffff );
            output( "\tret\n" );
            break;
        default:
            assert(0);
        }
        output_cfi( ".cfi_endproc" );
        output_function_size( "__wine_syscall_dispatcher" );

        output( "\t.data\n" );
        output( "\t.align %d\n", get_alignment( get_ptr_size() ) );
        output( ".Lsyscall_table:\n" );
        for (i = 0; i < count; i++)
            output( "\t%s %s\n", get_asm_ptr_keyword(), asm_name( get_link_name( syscalls[i] )));
        output( ".Lsyscall_args:\n" );
        for (i = 0; i < count; i++)
            output( "\t.byte %u\n", get_args_size( syscalls[i] ));
        return;
    }

    for (i = 0; i < count; i++)
    {
        ORDDEF *odp = syscalls[i];
        const char *name = get_link_name(odp);
        output( "\t.align %d\n", get_alignment(16) );
        output( "\t%s\n", func_declaration(name) );
        output( "%s\n", asm_globl(name) );
        output_cfi( ".cfi_startproc" );
        switch (target_cpu)
        {
        case CPU_x86:
            if (UsePIC)
            {
                output( "\tcall %s\n", asm_name("__wine_spec_get_pc_thunk_eax") );
                output( "1:\tmovl %s-1b(%%eax),%%edx\n", asm_name("__wine_syscall_dispatcher") );
                output( "\tmovl $%u,%%eax\n", i );
                needs_get_pc_thunk = 1;
            }
            else
            {
                output( "\tmovl $%u,%%eax\n", i );
                output( "\tmovl $%s,%%edx\n", asm_name("__wine_syscall") );
            }
            output( "\tcall *%%edx\n" );
            output( "\tret $%u\n", odp->type == TYPE_STDCALL ? get_args_size( odp ) : 0 );
            break;
        case CPU_x86_64:
            /* Chromium depends on syscall thunks having the same form as on
             * Windows. For 64-bit systems the only viable form we can emulate is
             * having an int $0x2e fallback. Since actually using an interrupt is
             * expensive, and since for some reason Chromium doesn't actually
             * validate that instruction, we can just put a jmp there instead. */
            output( "\t.byte 0x4c,0x8b,0xd1\n" ); /* movq %rcx,%r10 */
            output( "\t.byte 0xb8\n" );           /* movl $i,%eax */
            output( "\t.long %u\n", i );
            output( "\t.byte 0xf6,0x04,0x25,0x08,0x03,0xfe,0x7f,0x01\n" ); /* testb $1,0x7ffe0308 */
            output( "\t.byte 0x75,0x03\n" );      /* jne 1f */
            output( "\t.byte 0x0f,0x05\n" );      /* syscall */
            output( "\t.byte 0xc3\n" );           /* ret */
            output( "\tjmp 1f\n" );
            output( "\t.byte 0xc3\n" );           /* ret */
            if (target_platform == PLATFORM_WINDOWS)
            {
                output( "1:\t.byte 0xff,0x14,0x25\n" ); /* 1: callq *(0x7ffe1000) */
                output( "\t.long 0x7ffe1000\n" );
            }
            else
            {
                output( "\tnop\n" );
                output( "1:\tcallq *%s(%%rip)\n", asm_name("__wine_syscall_dispatcher") );
            }
            output( "\tret\n" );
            break;
        case CPU_ARM:
            output( "\tpush {r4,lr}\n" );
            output( "\tldr r4, 3f\n");
            output( "\tldr ip, 2f\n");
            output( "\tadd ip, pc\n");
            output( "\tldr ip, [ip]\n");
            output( "1:\tblx ip\n");
            output( "\tpop {r4,pc}\n" );
            output( "2:\t.long %s-1b\n", asm_name("__wine_syscall_dispatcher") );
            output( "3:\t.long %u\n", i );
            break;
        case CPU_ARM64:
            output( "\tstp x29, x30, [sp,#-16]!\n" );
            output_cfi( "\t.cfi_def_cfa_offset 16\n" );
            output_cfi( "\t.cfi_offset 29, -16\n" );
            output_cfi( "\t.cfi_offset 30, -8\n" );
            output( "\tmov x8, #%u\n", i );
            output( "\tadrp x16, %s\n", arm64_page( asm_name("__wine_syscall_dispatcher") ) );
            output( "\tldr x16, [x16, #%s]\n", arm64_pageoff( asm_name("__wine_syscall_dispatcher") ) );
            output( "\tblr x16\n");
            output( "\tldp x29, x30, [sp], #16\n" );
            output( "\tret\n" );
            break;
        default:
            assert(0);
        }
        output_cfi( ".cfi_endproc" );
        output_function_size( name );
    }

    if (target_cpu == CPU_x86 && !UsePIC)
    {
        output( "\t.align %d\n", get_alignment(16) );
        output( "\t%s\n", func_declaration("__wine_syscall") );
        output( "%s:\n", asm_name("__wine_syscall") );
        output( "\tjmp *(%s)\n", asm_name("__wine_syscall_dispatcher") );
        output_function_size( "__wine_syscall" );
    }
    output( "\t.data\n" );
    output( "\t.align %d\n", get_alignment( get_ptr_size() ) );
    output( "%s\n", asm_globl("__wine_syscall_dispatcher") );
    output( "\t%s 0\n", get_asm_ptr_keyword() );
}


/* output the import and delayed import tables of a Win32 module */
void output_imports( DLLSPEC *spec )
{
    if (target_platform == PLATFORM_WINDOWS) return;
    output_immediate_imports();
    output_delayed_imports( spec );
    output_immediate_import_thunks();
    output_delayed_import_thunks( spec );
    output_external_link_imports( spec );
}

/* create a new asm temp file */
static void new_output_as_file(void)
{
    char *name;

    if (output_file) fclose( output_file );
    name = open_temp_output_file( ".s" );
    strarray_add( &as_files, name, NULL );
}

/* assemble all the asm files */
static void assemble_files( const char *prefix )
{
    unsigned int i;

    if (output_file) fclose( output_file );
    output_file = NULL;

    for (i = 0; i < as_files.count; i++)
    {
        char *obj = get_temp_file_name( prefix, ".o" );
        assemble_file( as_files.str[i], obj );
        as_files.str[i] = obj;
    }
}

/* build a library from the current asm files and any additional object files in argv */
static void build_library( const char *output_name, char **argv, int create )
{
    struct strarray args = find_tool( "ar", NULL );
    struct strarray ranlib = find_tool( "ranlib", NULL );

    strarray_add( &args, create ? "rc" : "r", output_name, NULL );
    strarray_addall( &args, as_files );
    strarray_addv( &args, argv );
    if (create) unlink( output_name );
    spawn( args );

    strarray_add( &ranlib, output_name, NULL );
    spawn( ranlib );
}

/* create a Windows-style import library */
static void build_windows_import_lib( DLLSPEC *spec )
{
    struct strarray args;
    char *def_file;
    const char *as_flags, *m_flag;

    def_file = open_temp_output_file( ".def" );
    output_def_file( spec, 1 );
    fclose( output_file );

    args = find_tool( "dlltool", NULL );
    switch (target_cpu)
    {
        case CPU_x86:
            m_flag = "i386";
            as_flags = "--as-flags=--32";
            break;
        case CPU_x86_64:
            m_flag = "i386:x86-64";
            as_flags = "--as-flags=--64";
            break;
        default:
            m_flag = NULL;
            break;
    }
    strarray_add( &args, "-k", strendswith( output_file_name, ".delay.a" ) ? "-y" : "-l",
                  output_file_name, "-d", def_file, NULL );
    if (m_flag)
        strarray_add( &args, "-m", m_flag, as_flags, NULL );
    spawn( args );
}

/* create a Unix-style import library */
static void build_unix_import_lib( DLLSPEC *spec )
{
    static const char valid_chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._@";
    int i, total;
    const char *name, *prefix;
    char *dll_name = xstrdup( spec->file_name );

    if (strendswith( dll_name, ".dll" )) dll_name[strlen(dll_name) - 4] = 0;
    if (strspn( dll_name, valid_chars ) < strlen( dll_name ))
        fatal_error( "%s contains invalid characters\n", spec->file_name );

    /* entry points */

    for (i = total = 0; i < spec->nb_entry_points; i++)
    {
        const ORDDEF *odp = &spec->entry_points[i];

        if (odp->name) name = odp->name;
        else if (odp->export_name) name = odp->export_name;
        else continue;

        if (odp->flags & FLAG_PRIVATE) continue;
        total++;

        /* C++ mangled names cannot be imported */
        if (strpbrk( name, "?@" )) continue;

        switch(odp->type)
        {
        case TYPE_VARARGS:
        case TYPE_CDECL:
        case TYPE_STDCALL:
            prefix = (!odp->name || (odp->flags & FLAG_ORDINAL)) ? import_ord_prefix : import_func_prefix;
            new_output_as_file();
            output( "\t.text\n" );
            output( "\n\t.align %d\n", get_alignment( get_ptr_size() ));
            output( "\t%s\n", func_declaration( name ) );
            output( "%s\n", asm_globl( name ) );
            output( "\t%s %s%s$%u$%s\n", get_asm_ptr_keyword(),
                    asm_name( prefix ), dll_name, odp->ordinal, name );
            output_function_size( name );
            output_gnu_stack_note();
            break;

        default:
            break;
        }
    }
    if (!total) warning( "%s: Import library doesn't export anything\n", spec->file_name );

    if (!as_files.count)  /* create a dummy file to avoid empty import libraries */
    {
        new_output_as_file();
        output( "\t.text\n" );
    }

    assemble_files( spec->file_name );
    free( dll_name );
}

/* output an import library for a Win32 module and additional object files */
void output_static_lib( DLLSPEC *spec, char **argv )
{
    if (target_platform == PLATFORM_WINDOWS)
    {
        if (spec) build_windows_import_lib( spec );
        if (argv[0] || !spec) build_library( output_file_name, argv, !spec );
    }
    else
    {
        if (spec) build_unix_import_lib( spec );
        build_library( output_file_name, argv, 1 );
    }
}
