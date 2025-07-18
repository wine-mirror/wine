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

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "build.h"
#include "wine/list.h"

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
    "longjmp",
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

static const struct strarray stdc_functions = { ARRAY_SIZE(stdc_names), ARRAY_SIZE(stdc_names), stdc_names };

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

/* compare function names; helper for resolve_imports */
static int name_cmp( const char **name, const char **entry )
{
    return strcmp( *name, *entry );
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
static inline const char *find_name( const char *name, struct strarray table )
{
    return strarray_bsearch( table, name, name_cmp );
}

/* sort a name table */
static inline void sort_names( struct strarray *table )
{
    strarray_qsort( table, name_cmp );
}

/* locate an export in a (sorted) export list */
static inline ORDDEF *find_export( const char *name, ORDDEF **table, int size )
{
    ORDDEF func, *odp, **res = NULL;

    func.name = func.export_name = xstrdup(name);
    odp = &func;
    if (table) res = bsearch( &odp, table, size, sizeof(*table), func_cmp );
    free( func.name );
    return res ? *res : NULL;
}

static const char valid_chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.";

/* encode a dll name into a linker-compatible name */
static char *encode_dll_name( const char *name )
{
    char *p, *ret, *ret_end;
    int len = strlen(name);

    if (strendswith( name, ".dll" )) len -= 4;
    if (strspn( name, valid_chars ) >= len) return strmake( "%.*s", len, name );

    ret = p = xmalloc( len * 4 + 1 );
    ret_end = ret + (len * 4 + 1);
    for ( ; len > 0; len--, name++)
    {
        if (!strchr( valid_chars, *name ))
            p += snprintf( p, ret_end - p, "$x%02x", *name );
        else *p++ = *name;
    }
    *p = 0;
    return ret;
}

/* decode a linker-compatible dll name */
static char *decode_dll_name( const char **name )
{
    const char *src = *name;
    char *p, *ret;

    ret = p = xmalloc( strlen( src ) + 5 );
    for ( ; *src; src++, p++)
    {
        if (*src != '$')
        {
            *p = *src;
        }
        else if (src[1] == 'x')  /* hex escape */
        {
            int val = 0;
            src += 2;
            if (*src >= '0' && *src <= '9') val += *src - '0';
            else if (*src >= 'A' && *src <= 'F') val += *src - 'A' + 10;
            else if (*src >= 'a' && *src <= 'f') val += *src - 'a' + 10;
            else return NULL;
            val *= 16;
            src++;
            if (*src >= '0' && *src <= '9') val += *src - '0';
            else if (*src >= 'A' && *src <= 'F') val += *src - 'A' + 10;
            else if (*src >= 'a' && *src <= 'f') val += *src - 'a' + 10;
            else return NULL;
            *p = val;
        }
        else break;  /* end of dll name */
    }
    *p = 0;
    if (!strchr( ret, '.' )) strcpy( p, ".dll" );
    *name = src;
    return ret;
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

/* build the dll exported name from the import lib name */
static char *get_dll_name( const char *name )
{
    char *ret = xmalloc( strlen(name) + 5 );

    strcpy( ret, name );
    if (!strchr( ret, '.' )) strcat( ret, ".dll" );
    return ret;
}

/* add a library to the list of delayed imports */
void add_delayed_import( const char *name )
{
    struct import *imp;
    char *fullname = get_dll_name( name );

    strarray_add( &delayed_imports, fullname );
    if ((imp = find_import_dll( fullname )))
    {
        list_remove( &imp->entry );
        list_add_tail( &dll_delayed, &imp->entry );
    }
}

/* add a symbol to the list of extra symbols that ld must resolve */
void add_extra_ld_symbol( const char *name )
{
    strarray_add( &extra_ld_symbols, name );
}

/* retrieve an imported dll, adding one if necessary */
static struct import *add_static_import_dll( const char *name )
{
    struct import *import;

    if ((import = find_import_dll( name ))) return import;

    import = xmalloc( sizeof(*import) );
    memset( import, 0, sizeof(*import) );

    import->dll_name = xstrdup( name );
    import->full_name = xstrdup( name );
    import->c_name = make_c_identifier( name );

    if (is_delayed_import( name ))
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
    char *dll_name = decode_dll_name( &name );
    int ordinal = 0;
    struct import *import;

    if (!dll_name) return;
    if (*name++ != '$') return;
    while (*name >= '0' && *name <= '9') ordinal = 10 * ordinal + *name++ - '0';
    if (*name++ != '$') return;

    if (!use_msvcrt && find_name( name, stdc_functions )) return;

    import = add_static_import_dll( dll_name );
    if (is_ordinal)
        add_import_func( import, NULL, xstrdup( name ), ordinal, 0 );
    else
        add_import_func( import, xstrdup( name ), NULL, ordinal, 0 );
    free( dll_name );
}

/* check if the spec file exports any stubs */
static int has_stubs( const struct exports *exports )
{
    int i;

    for (i = 0; i < exports->nb_entry_points; i++)
    {
        ORDDEF *odp = exports->entry_points[i];
        if (odp->type == TYPE_STUB) return 1;
    }
    return 0;
}

/* add the extra undefined symbols that will be contained in the generated spec file itself */
static void add_extra_undef_symbols( DLLSPEC *spec )
{
    add_extra_ld_symbol( spec->init_func );
    if (spec->type == SPEC_WIN16) add_extra_ld_symbol( "DllMain" );
    if (has_stubs( &spec->exports )) add_extra_ld_symbol( "__wine_spec_unimplemented_stub" );
    if (delayed_imports.count) add_extra_ld_symbol( "__delayLoadHelper2" );
}

/* check if a given imported dll is not needed, taking forwards into account */
static int check_unused( const struct import* imp, const struct exports *exports )
{
    int i;
    const char *file_name = imp->dll_name;
    size_t len = strlen( file_name );
    const char *p = strchr( file_name, '.' );
    if (p && !strcasecmp( p, ".dll" )) len = p - file_name;

    for (i = exports->base; i <= exports->limit; i++)
    {
        ORDDEF *odp = exports->ordinals[i];
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

    for (i = 0; i < spec->exports.nb_entry_points; i++)
    {
        ORDDEF *odp = spec->exports.entry_points[i];

        if (!(odp->flags & FLAG_FORWARD)) continue;

        link_name = xstrdup( odp->link_name );
        p = strrchr( link_name, '.' );
        *p = 0;
        api_name = p + 1;
        dll_name = get_dll_name( link_name );

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

    for (i = 0; i < spec->exports.nb_entry_points; i++)
    {
        ORDDEF *odp = spec->exports.entry_points[i];
        if (odp->type == TYPE_STUB || odp->type == TYPE_ABS || odp->type == TYPE_VARIABLE) continue;
        if (odp->flags & FLAG_FORWARD) continue;
        if (find_name( odp->link_name, undef_symbols ))
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
                    strarray_add( &ext_link_imports, odp->link_name );
                }
                else error( "%s:%d: function '%s' not defined\n",
                            spec->src_name, odp->lineno, odp->link_name );
                break;
            default:
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

    as_file = open_temp_output_file( ".s" );
    output( "\t.data\n" );

    for (i = 0; i < spec->exports.nb_entry_points; i++)
    {
        ORDDEF *odp = spec->exports.entry_points[i];
        if (odp->type == TYPE_STUB || odp->type == TYPE_ABS || odp->type == TYPE_VARIABLE) continue;
        if (odp->flags & FLAG_FORWARD) continue;
        output( "\t%s %s\n", get_asm_ptr_keyword(), asm_name( get_link_name( odp )));
    }
    for (j = 0; j < extra_ld_symbols.count; j++)
        output( "\t%s %s\n", get_asm_ptr_keyword(), asm_name(extra_ld_symbols.str[j]) );

    output_gnu_stack_note();
    fclose( output_file );

    obj_file = make_temp_file( output_file_name, ".o" );
    assemble_file( as_file, obj_file );
    return obj_file;
}

/* combine a list of object files with ld into a single object file */
/* returns the name of the combined file */
static const char *ldcombine_files( DLLSPEC *spec, struct strarray files )
{
    char *ld_tmp_file, *undef_file;
    struct strarray args = get_ld_command();

    undef_file = create_undef_symbols_file( spec );
    ld_tmp_file = make_temp_file( output_file_name, ".o" );

    strarray_add( &args, "-r" );
    strarray_add( &args, "-o" );
    strarray_add( &args, ld_tmp_file );
    if (undef_file) strarray_add( &args, undef_file );
    strarray_addall( &args, files );
    spawn( args );
    return ld_tmp_file;
}

/* read in the list of undefined symbols */
void read_undef_symbols( DLLSPEC *spec, struct strarray files )
{
    size_t prefix_len;
    FILE *f;
    const char *prog = get_nm_command();
    char *cmd, buffer[1024], name_prefix[16];
    int err;
    const char *name;

    if (!files.count) return;

    add_extra_undef_symbols( spec );

    strcpy( name_prefix, asm_name("") );
    prefix_len = strlen( name_prefix );

    name = ldcombine_files( spec, files );

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
        else if (use_msvcrt || !find_name( p, stdc_functions ))
            strarray_add( &undef_symbols, xstrdup( p ));
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
            if (check_unused( imp, &spec->exports ))
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
    return find_name( name, undef_symbols ) != NULL;
}

/* output the get_pc thunk if needed */
void output_get_pc_thunk(void)
{
    assert( target.cpu == CPU_i386 );
    output_function_header( "__wine_spec_get_pc_thunk_eax", 0 );
    output( "\tmovl (%%esp),%%eax\n" );
    output( "\tret\n" );
    output_function_size( "__wine_spec_get_pc_thunk_eax" );
}

/* output a single import thunk */
static void output_import_thunk( const char *name, const char *table, int pos )
{
    output_function_header( name, 1 );

    switch (target.cpu)
    {
    case CPU_i386:
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
    default:
        assert( 0 );
        break;
    }
    output_function_size( name );
}

/* check if we need an import directory */
int has_imports(void)
{
    return !list_empty( &dll_imports );
}

/* check if we need a delayed import directory */
int has_delay_imports(void)
{
    return !list_empty( &dll_delayed );
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
    output( "\t.balign 4\n" );
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

    output( "\n\t.balign %u\n", get_ptr_size() );
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
                output_thunk_rva( func->name ? -1 : func->ordinal,
                                  ".L__wine_spec_import_data_%s_%s", import->c_name, func->name );
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
            output( "\t.balign 2\n" );
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
    output( "\t.balign 8\n" );
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
    int j, iat_pos, int_pos, mod_pos;
    struct import *import;

    if (list_empty( &dll_delayed )) return;

    output( "\n/* delayed imports */\n\n" );
    output( "\t.data\n" );
    output( "\t.balign %u\n", get_ptr_size() );
    output( ".L__wine_spec_delay_imports:\n" );

    /* list of dlls */

    iat_pos = int_pos = mod_pos = 0;
    LIST_FOR_EACH_ENTRY( import, &dll_delayed, struct import, entry )
    {
        output( "\t.long 1\n" );                                /* Attributes */
        output_rva( ".L__wine_delay_name_%s", import->c_name ); /* DllNameRVA */
        output_rva( ".L__wine_delay_modules+%d", mod_pos );     /* ModuleHandleRVA */
        output_rva( ".L__wine_delay_IAT+%d", iat_pos );         /* ImportAddressTableRVA */
        output_rva( ".L__wine_delay_INT+%d", int_pos );         /* ImportNameTableRVA */
        output( "\t.long 0\n" );                                /* BoundImportAddressTableRVA */
        output( "\t.long 0\n" );                                /* UnloadInformationTableRVA */
        output( "\t.long 0\n" );                                /* TimeDateStamp */
        iat_pos += import->nb_imports * get_ptr_size();
        int_pos += (import->nb_imports + 1) * get_ptr_size();
        mod_pos += get_ptr_size();
    }
    output( "\t.long 0,0,0,0,0,0,0,0\n" );
    output( ".L__wine_spec_delay_imports_end:\n" );

    output( "\n.L__wine_delay_IAT:\n" );
    LIST_FOR_EACH_ENTRY( import, &dll_delayed, struct import, entry )
    {
        for (j = 0; j < import->nb_imports; j++)
        {
            struct import_func *func = &import->imports[j];
            const char *name = func->name ? func->name : func->export_name;
            output( "__imp_%s:\n", asm_name( name ));
            output( "\t%s __wine_delay_imp_%s_%s\n",
                    get_asm_ptr_keyword(), import->c_name, name );
        }
    }

    output( "\n.L__wine_delay_INT:\n" );
    LIST_FOR_EACH_ENTRY( import, &dll_delayed, struct import, entry )
    {
        for (j = 0; j < import->nb_imports; j++)
        {
            struct import_func *func = &import->imports[j];
            output_thunk_rva( func->name ? -1 : func->ordinal,
                                ".L__wine_delay_data_%s_%s", import->c_name, func->name );
        }
        output( "\t%s 0\n", get_asm_ptr_keyword() );
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
            output( "\t.balign 2\n" );
            output( ".L__wine_delay_data_%s_%s:\n", import->c_name, func->name );
            output( "\t.short %d\n", func->hint );
            output( "\t%s \"%s\"\n", get_asm_string_keyword(), func->name );
        }
    }
}

/* output the delayed import thunks of a Win32 module */
static void output_delayed_import_thunks( const DLLSPEC *spec )
{
    int j, pos, iat_pos;
    struct import *import;
    static const char delayed_import_loaders[] = "__wine_spec_delayed_import_loaders";
    static const char delayed_import_thunks[] = "__wine_spec_delayed_import_thunks";

    if (list_empty( &dll_delayed )) return;

    output( "\n/* delayed import thunks */\n\n" );
    output( "\t.text\n" );
    output( "\t.balign 8\n" );
    output( "%s:\n", asm_name(delayed_import_loaders));

    pos = iat_pos = 0;
    LIST_FOR_EACH_ENTRY( import, &dll_delayed, struct import, entry )
    {
        char *module_func = strmake( "__wine_delay_load_asm_%s", import->c_name );
        output_function_header( module_func, 0 );
        output_cfi( ".cfi_startproc" );
        switch (target.cpu)
        {
        case CPU_i386:
            output( "\tpushl %%ecx\n" );
            output_cfi( ".cfi_adjust_cfa_offset 4" );
            output( "\tpushl %%edx\n" );
            output_cfi( ".cfi_adjust_cfa_offset 4" );
            output( "\tpushl %%eax\n" );
            output_cfi( ".cfi_adjust_cfa_offset 4" );
            if (UsePIC)
            {
                output( "\tcall %s\n", asm_name("__wine_spec_get_pc_thunk_eax") );
                output( "1:\tleal .L__wine_spec_delay_imports+%d-1b(%%eax),%%eax\n", pos );
                output( "\tpushl %%eax\n" );
                output_cfi( ".cfi_adjust_cfa_offset 4" );
                needs_get_pc_thunk = 1;
            }
            else
            {
                output( "\tpushl $.L__wine_spec_delay_imports+%d\n", pos );
                output_cfi( ".cfi_adjust_cfa_offset 4" );
            }
            output( "\tcall %s\n", asm_name("__delayLoadHelper2") );
            output_cfi( ".cfi_adjust_cfa_offset -8" );
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
            output( "\tleaq .L__wine_spec_delay_imports+%d(%%rip),%%rcx\n", pos );
            output( "\tmovq %%rax,%%rdx\n" );
            output( "\tcall %s\n", asm_name("__delayLoadHelper2") );
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
        default:
            assert( 0 );
            break;
        }
        output_cfi( ".cfi_endproc" );
        output_function_size( module_func );
        output( "\n" );

        for (j = 0; j < import->nb_imports; j++)
        {
            struct import_func *func = &import->imports[j];
            const char *name = func->name ? func->name : func->export_name;

            output( "__wine_delay_imp_%s_%s:\n", import->c_name, name );
            switch (target.cpu)
            {
            case CPU_i386:
                if (UsePIC)
                {
                    output( "\tcall %s\n", asm_name("__wine_spec_get_pc_thunk_eax") );
                    output( "1:\tleal .L__wine_delay_IAT+%d-1b(%%eax),%%eax\n", iat_pos );
                    needs_get_pc_thunk = 1;
                }
                else output( "\tmovl $.L__wine_delay_IAT+%d,%%eax\n", iat_pos );
                output( "\tjmp %s\n", asm_name(module_func) );
                break;
            case CPU_x86_64:
                output( "\tleaq .L__wine_delay_IAT+%d(%%rip),%%rax\n", iat_pos );
                output( "\tjmp %s\n", asm_name(module_func) );
                break;
            default:
                assert( 0 );
                break;
            }
            iat_pos += get_ptr_size();
        }
        pos += 8 * 4;  /* IMAGE_DELAYLOAD_DESCRIPTOR is 8 DWORDs */
    }
    output_function_size( delayed_import_loaders );

    output( "\n\t.balign %u\n", get_ptr_size() );
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
    output( "\t.balign %u\n", get_ptr_size() );
    output( ".L__wine_spec_external_links:\n" );
    for (i = 0; i < ext_link_imports.count; i++)
        output( "\t%s %s\n", get_asm_ptr_keyword(), asm_name(ext_link_imports.str[i]) );

    output( "\n\t.text\n" );
    output( "\t.balign %u\n", get_ptr_size() );
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
    struct exports *exports = &spec->exports;
    const char *name, *exp_name;
    int i;

    if (!has_stubs( exports )) return;

    output( "\n/* stub functions */\n\n" );

    for (i = 0; i < exports->nb_entry_points; i++)
    {
        ORDDEF *odp = exports->entry_points[i];
        if (odp->type != TYPE_STUB) continue;
        if (odp->flags & FLAG_SYSCALL) continue;

        name = get_link_name( odp );
        exp_name = odp->name ? odp->name : odp->export_name;
        output_function_header( name, 0 );

        switch (target.cpu)
        {
        case CPU_i386:
            output_cfi( ".cfi_startproc" );
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
            output_cfi( ".cfi_endproc" );
            break;
        case CPU_x86_64:
            output_cfi( ".cfi_startproc" );
            output_seh( ".seh_proc %s", asm_name(name) );
            output( "\tsubq $0x28,%%rsp\n" );
            output_cfi( ".cfi_adjust_cfa_offset 0x28" );
            output_seh( ".seh_stackalloc 0x28" );
            output_seh( ".seh_endprologue" );
            output( "\tleaq .L__wine_spec_file_name(%%rip),%%rcx\n" );
            if (exp_name)
                output( "leaq .L%s_string(%%rip),%%rdx\n", name );
            else
                output( "\tmovq $%d,%%rdx\n", odp->ordinal );
            output( "\tcall %s\n", asm_name("__wine_spec_unimplemented_stub") );
            output_cfi( ".cfi_endproc" );
            output_seh( ".seh_endproc" );
            break;
        case CPU_ARM:
            output( "\t.seh_proc %s\n", asm_name(name) );
            output( "\t.seh_endprologue\n" );
            output( "\tmovw r0,:lower16:.L__wine_spec_file_name\n");
            output( "\tmovt r0,:upper16:.L__wine_spec_file_name\n");
            if (exp_name)
            {
                output( "\tmovw r1,:lower16:.L%s_string\n", name );
                output( "\tmovt r1,:upper16:.L%s_string\n", name );
            }
            else output( "\tmov r1,#%u\n", odp->ordinal );
            output( "\tb %s\n", asm_name("__wine_spec_unimplemented_stub") );
            output( "\t.seh_endproc\n" );
            break;
        case CPU_ARM64:
        case CPU_ARM64EC:
            output( "\t.seh_proc %s\n", arm64_name(name) );
            output( "\t.seh_endprologue\n" );
            output( "\tadrp x0, .L__wine_spec_file_name\n" );
            output( "\tadd x0, x0, #:lo12:.L__wine_spec_file_name\n" );
            if (exp_name)
            {
                output( "\tadrp x1, .L%s_string\n", name );
                output( "\tadd x1, x1, #:lo12:.L%s_string\n", name );
            }
            else
                output( "\tmov x1, %u\n", odp->ordinal );
            output( "\tb %s\n", arm64_name("__wine_spec_unimplemented_stub") );
            output( "\t.seh_endproc\n" );
            break;
        }
        output_function_size( name );
    }

    output( "\t%s\n", get_asm_string_section() );
    output( ".L__wine_spec_file_name:\n" );
    output( "\t%s \"%s\"\n", get_asm_string_keyword(), spec->file_name );
    for (i = 0; i < exports->nb_entry_points; i++)
    {
        ORDDEF *odp = exports->entry_points[i];
        if (odp->type != TYPE_STUB) continue;
        if (odp->flags & FLAG_SYSCALL) continue;
        exp_name = odp->name ? odp->name : odp->export_name;
        if (exp_name)
        {
            name = get_link_name( odp );
            output( ".L%s_string:\n", name );
            output( "\t%s \"%s\"\n", get_asm_string_keyword(), exp_name );
        }
    }
}

/* output the import and delayed import tables of a Win32 module */
void output_imports( DLLSPEC *spec )
{
    if (is_pe()) return;
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
    strarray_add( &as_files, name );
}

/* assemble all the asm files */
static void assemble_files( const char *prefix )
{
    unsigned int i;

    if (output_file) fclose( output_file );
    output_file = NULL;

    for (i = 0; i < as_files.count; i++)
    {
        char *obj = make_temp_file( prefix, ".o" );
        assemble_file( as_files.str[i], obj );
        as_files.str[i] = obj;
    }
}

static const char *get_target_machine(void)
{
    static const char *machine_names[] =
    {
        [CPU_i386]    = "x86",
        [CPU_x86_64]  = "x64",
        [CPU_ARM]     = "arm",
        [CPU_ARM64]   = "arm64",
        [CPU_ARM64EC] = "arm64ec",
    };

    return machine_names[target.cpu];
}

/* build a library from the current asm files and any additional object files in argv */
void output_static_lib( const char *output_name, struct strarray files, int create )
{
    struct strarray args;

    if (!create || target.platform != PLATFORM_WINDOWS)
    {
        args = find_tool( "ar", NULL );
        strarray_add( &args, create ? "rc" : "r" );
        strarray_add( &args, output_name );
    }
    else
    {
        args = find_link_tool();
        strarray_add( &args, "/lib" );
        strarray_add( &args, strmake( "-machine:%s", get_target_machine() ));
        strarray_add( &args, strmake( "-out:%s", output_name ));
    }
    strarray_addall( &args, as_files );
    strarray_addall( &args, files );
    if (create) unlink( output_name );
    spawn( args );

    if (target.platform != PLATFORM_WINDOWS)
    {
        struct strarray ranlib = find_tool( "ranlib", NULL );
        strarray_add( &ranlib, output_name );
        spawn( ranlib );
    }
}

/* create a Windows-style import library using dlltool */
static void build_dlltool_import_lib( const char *lib_name, DLLSPEC *spec, struct strarray files )
{
    const char *def_file, *native_def_file = NULL;
    struct strarray args;

    def_file = open_temp_output_file( ".def" );
    output_def_file( spec, &spec->exports, 1 );
    fclose( output_file );

    if (native_arch != -1)
    {
        native_def_file = open_temp_output_file( ".def" );
        output_def_file( spec, &spec->native_exports, 1 );
        fclose( output_file );
    }

    args = find_tool( "dlltool", NULL );
    strarray_add( &args, "-k" );
    strarray_add( &args, strendswith( lib_name, ".delay.a" ) ? "-y" : "-l" );
    strarray_add( &args, lib_name );
    strarray_add( &args, "-d" );
    strarray_add( &args, def_file );
    if (native_def_file)
    {
        strarray_add( &args, "-N" );
        strarray_add( &args, native_def_file );
    }

    switch (target.cpu)
    {
        case CPU_i386:
            strarray_add( &args, "-m" );
            strarray_add( &args, "i386" );
            strarray_add( &args, "--as-flags=--32" );
            break;
        case CPU_x86_64:
            strarray_add( &args, "-m" );
            strarray_add( &args, "i386:x86-64" );
            strarray_add( &args, "--as-flags=--64" );
            break;
        case CPU_ARM:
            strarray_add( &args, "-m" );
            strarray_add( &args, "arm" );
            break;
        case CPU_ARM64:
            strarray_add( &args, "-m" );
            strarray_add( &args, "arm64" );
            break;
        case CPU_ARM64EC:
            strarray_add( &args, "-m" );
            strarray_add( &args, "arm64ec" );
            break;
        default:
            break;
    }

    spawn( args );

    if (files.count) output_static_lib( output_file_name, files, 0 );
}

static void output_import_section( int index, int is_delay )
{
    if (!is_delay)
        output( "\n\t.section .idata$%d\n", index );
    else if (index == 5)
        output( "\n\t.section .data$didat%d\n", index );
    else
        output( "\n\t.section .rdata$didat%d\n", index );
}

/* create a Windows-style import library */
static void build_windows_import_lib( const char *lib_name, DLLSPEC *spec, struct strarray files )
{
    char *dll_name, *import_desc, *import_name, *delay_load;
    struct strarray objs = empty_strarray;
    int i, total, by_name;
    int is_delay = strendswith( lib_name, ".delay.a" );
    const char *name;

    /* make sure assemble_files doesn't strip suffixes */
    dll_name = encode_dll_name( spec->file_name );
    for (i = 0; i < strlen( dll_name ); ++i) if (dll_name[i] == '.') dll_name[i] = '_';

    import_desc = strmake( "__wine_import_%s_desc", dll_name );
    import_name = strmake( "__wine_import_%s_name", dll_name );
    delay_load = strmake( "__wine_delay_load_%s", dll_name );

    new_output_as_file();

    if (is_delay)
    {
        output_function_header( delay_load, 1 );

        switch (target.cpu)
        {
        case CPU_i386:
            output_cfi( ".cfi_startproc" );
            output( "\tpushl %%ecx\n" );
            output_cfi( ".cfi_adjust_cfa_offset 4" );
            output( "\tpushl %%edx\n" );
            output_cfi( ".cfi_adjust_cfa_offset 4" );
            output( "\tpushl %%eax\n" );
            output_cfi( ".cfi_adjust_cfa_offset 4" );
            output( "\tpushl $%s\n", asm_name( import_desc ) );
            output( "\tcalll ___delayLoadHelper2@8\n" );
            output_cfi( ".cfi_adjust_cfa_offset -8" );
            output( "\tpopl %%edx\n" );
            output_cfi( ".cfi_adjust_cfa_offset -4" );
            output( "\tpopl %%ecx\n" );
            output_cfi( ".cfi_adjust_cfa_offset -4" );
            output( "\tjmp *%%eax\n" );
            output_cfi( ".cfi_endproc" );
            break;
        case CPU_x86_64:
            output_seh( ".seh_proc %s", asm_name( delay_load ) );
            output( "\tsubq $0x48, %%rsp\n" );
            output_seh( ".seh_stackalloc 0x48" );
            output_seh( ".seh_endprologue" );
            output( "\tmovq %%rcx, 0x40(%%rsp)\n" );
            output( "\tmovq %%rdx, 0x38(%%rsp)\n" );
            output( "\tmovq %%r8, 0x30(%%rsp)\n" );
            output( "\tmovq %%r9, 0x28(%%rsp)\n" );
            output( "\tmovq %%rax, %%rdx\n" );
            output( "\tleaq %s(%%rip), %%rcx\n", asm_name( import_desc ) );
            output( "\tcall __delayLoadHelper2\n" );
            output( "\tmovq 0x28(%%rsp), %%r9\n" );
            output( "\tmovq 0x30(%%rsp), %%r8\n" );
            output( "\tmovq 0x38(%%rsp), %%rdx\n" );
            output( "\tmovq 0x40(%%rsp), %%rcx\n" );
            output( "\taddq $0x48, %%rsp\n" );
            output( "\tjmp *%%rax\n" );
            output_seh( ".seh_endproc" );
            break;
        case CPU_ARM:
            output( "\t.seh_proc %s\n", asm_name( delay_load ) );
            output( "\tpush {r0-r3, FP, LR}\n" );
            output( "\t.seh_save_regs {r0-r3,fp,lr}\n" );
            output( "\t.seh_endprologue\n" );
            output( "\tmov r1, IP\n" );
            output( "\tldr r0, 1f\n" );
            output( "\tldr r0, [r0]\n" );
            output( "\tbl __delayLoadHelper2\n" );
            output( "\tmov IP, r0\n" );
            output( "\tpop {r0-r3, FP, LR}\n" );
            output( "\tbx IP\n" );
            output( "1:\t.long %s\n", asm_name( import_desc ) );
            output( "\t.seh_endproc\n" );
            break;
        case CPU_ARM64:
            output( "\t.seh_proc %s\n", asm_name( delay_load ) );
            output( "\tstp x29, x30, [sp, #-80]!\n" );
            output( "\t.seh_save_fplr_x 80\n" );
            output( "\tmov x29, sp\n" );
            output( "\t.seh_set_fp\n" );
            output( "\t.seh_endprologue\n" );
            output( "\tstp x0, x1, [sp, #16]\n" );
            output( "\tstp x2, x3, [sp, #32]\n" );
            output( "\tstp x4, x5, [sp, #48]\n" );
            output( "\tstp x6, x7, [sp, #64]\n" );
            output( "\tmov x1, x16\n" );
            output( "\tadrp x0, %s\n", asm_name( import_desc ) );
            output( "\tadd x0, x0, #:lo12:%s\n", asm_name( import_desc ) );
            output( "\tbl __delayLoadHelper2\n" );
            output( "\tmov x16, x0\n" );
            output( "\tldp x0, x1, [sp, #16]\n" );
            output( "\tldp x2, x3, [sp, #32]\n" );
            output( "\tldp x4, x5, [sp, #48]\n" );
            output( "\tldp x6, x7, [sp, #64]\n" );
            output( "\tldp x29, x30, [sp], #80\n" );
            output( "\tbr x16\n" );
            output( "\t.seh_endproc\n" );
            break;
        case CPU_ARM64EC:
            assert( 0 );
            break;
        }
        output_function_size( delay_load );
        output_gnu_stack_note();

        output( "\n\t.data\n" );
        output( ".L__wine_delay_import_handle:\n" );
        output( "\t%s 0\n", get_asm_ptr_keyword() );

        output( "%s\n", asm_globl( import_desc ) );
        output( "\t.long 1\n" );                         /* DllAttributes */
        output_rva( "%s", asm_name( import_name ) );     /* DllNameRVA */
        output_rva( ".L__wine_delay_import_handle" );    /* ModuleHandleRVA */
        output_rva( ".L__wine_import_addrs" );           /* ImportAddressTableRVA */
        output_rva( ".L__wine_import_names" );           /* ImportNameTableRVA */
        output( "\t.long 0\n" );                         /* BoundImportAddressTableRVA */
        output( "\t.long 0\n" );                         /* UnloadInformationTableRVA */
        output( "\t.long 0\n" );                         /* TimeDateStamp */

        output_import_section( 5, is_delay );
        output( "\t%s 0\n", get_asm_ptr_keyword() );     /* FirstThunk tail */
        output( ".L__wine_import_addrs:\n" );

        output_import_section( 4, is_delay );
        output( "\t%s 0\n", get_asm_ptr_keyword() );     /* OriginalFirstThunk tail */
        output( ".L__wine_import_names:\n" );

        /* required to avoid internal linker errors with some binutils versions */
        output_import_section( 2, is_delay );
    }
    else
    {
        output_import_section( 2, is_delay );
        output( "%s\n", asm_globl( import_desc ) );
        output_rva( ".L__wine_import_names" );           /* OriginalFirstThunk */
        output( "\t.long 0\n" );                         /* TimeDateStamp */
        output( "\t.long 0\n" );                         /* ForwarderChain */
        output_rva( "%s", asm_name( import_name ) );     /* Name */
        output_rva( ".L__wine_import_addrs" );           /* FirstThunk */

        output_import_section( 4, is_delay );
        output( ".L__wine_import_names:\n" );            /* OriginalFirstThunk head */

        output_import_section( 5, is_delay );
        output( ".L__wine_import_addrs:\n" );            /* FirstThunk head */
    }

    /* _head suffix to keep this object sections first */
    assemble_files( strmake( "%s_head", dll_name ) );
    strarray_addall( &objs, as_files );
    as_files = empty_strarray;

    new_output_as_file();

    output_import_section( 4, is_delay );
    output( "\t%s 0\n", get_asm_ptr_keyword() );         /* OriginalFirstThunk tail */
    output_import_section( 5, is_delay );
    output( "\t%s 0\n", get_asm_ptr_keyword() );         /* FirstThunk tail */
    output_import_section( 7, is_delay );
    output( "%s\n", asm_globl( import_name ) );
    output( "\t%s \"%s\"\n", get_asm_string_keyword(), spec->file_name );

    /* _tail suffix to keep this object sections last */
    assemble_files( strmake( "%s_tail", dll_name ) );
    strarray_addall( &objs, as_files );
    as_files = empty_strarray;

    for (i = total = 0; i < spec->exports.nb_entry_points; i++)
    {
        const ORDDEF *odp = spec->exports.entry_points[i];
        const char *abi_name;
        char *imp_name;

        if (odp->name) name = odp->name;
        else if (odp->export_name) name = odp->export_name;
        else continue;

        if (odp->flags & FLAG_PRIVATE) continue;
        total++;

        /* C++ mangled names cannot be imported */
        if (strpbrk( name, "?@" )) continue;

        switch (odp->type)
        {
        case TYPE_VARARGS:
        case TYPE_CDECL:
        case TYPE_STDCALL:
            by_name = odp->name && !(odp->flags & FLAG_ORDINAL);
            abi_name = get_abi_name( odp, name );
            imp_name = strmake( "%s_imp_%s", target.cpu != CPU_i386 ? "_" : "",
                                asm_name( abi_name ) );

            new_output_as_file();
            output_function_header( abi_name, 1 );

            switch (target.cpu)
            {
            case CPU_i386:
                output( "\tjmp *%s\n", asm_name( imp_name ) );
                if (is_delay)
                {
                    output( "\n\t.section .text$1\n" );
                    output( ".L__wine_delay_import:\n" );
                    output( "\tmov $%s,%%eax\n", asm_name( imp_name ) );
                    output( "\tjmp %s\n", asm_name( delay_load ) );
                }
                break;
            case CPU_x86_64:
                output( "\tjmp *%s(%%rip)\n", asm_name( imp_name ) );
                if (is_delay)
                {
                    output( "\n\t.section .text$1\n" );
                    output( ".L__wine_delay_import:\n" );
                    output( "\tlea %s(%%rip),%%rax\n", asm_name( imp_name ) );
                    output( "\tjmp %s\n", asm_name( delay_load ) );
                }
                break;
            case CPU_ARM:
                output( "\tldr IP, 1f\n" );
                output( "\tldr PC, [IP]\n" );
                if (is_delay)
                {
                    output( "\n\t.section .text$1\n" );
                    output( ".L__wine_delay_import:\n" );
                    output( "\tldr IP, 1f\n" );
                    output( "\tldr IP, [IP]\n" );
                    output( "\tb %s\n", asm_name( delay_load ) );
                }
                output( "1:\t.long %s\n", asm_name( imp_name ) );
                break;
            case CPU_ARM64:
                output( "\tadrp x16, %s\n", asm_name( imp_name ) );
                output( "\tadd x16, x16, #:lo12:%s\n", asm_name( imp_name ) );
                output( "\tbr x16\n" );
                if (is_delay)
                {
                    output( "\n\t.section .text$1\n" );
                    output( ".L__wine_delay_import:\n" );
                    output( "\tadrp x16, %s\n", asm_name( imp_name ) );
                    output( "\tadd x16, x16, #:lo12:%s\n", asm_name( imp_name ) );
                    output( "\tb %s\n", asm_name( delay_load ) );
                }
                break;
            case CPU_ARM64EC:
                assert( 0 );
                break;
            }

            output_import_section( 4, is_delay );
            output_thunk_rva( by_name ? -1 : odp->ordinal, ".L__wine_import_name" );

            output_import_section( 5, is_delay );
            output( "%s\n", asm_globl( imp_name ) );
            if (is_delay)
                output( "\t%s .L__wine_delay_import\n", get_asm_ptr_keyword() );
            else
                output_thunk_rva( by_name ? -1 : odp->ordinal, ".L__wine_import_name" );

            if (by_name)
            {
                output_import_section( 6, is_delay );
                output( ".L__wine_import_name:\n" );
                output( "\t.short %d\n", odp->hint );
                output( "\t%s \"%s\"\n", get_asm_string_keyword(), name );
            }

            /* reference head object to always pull its sections */
            output_import_section( 7, is_delay );
            output_rva( "%s", asm_name( import_desc ) );

            free( imp_name );
            break;

        default:
            break;
        }
    }

    /* _syms suffix to keep these objects sections in between _head and _tail */
    assemble_files( strmake( "%s_syms", dll_name ) );
    strarray_addall( &objs, as_files );
    as_files = objs;

    free( import_desc );
    free( import_name );
    free( delay_load );
    free( dll_name );

    output_static_lib( output_file_name, files, 1 );
}

/* create a Unix-style import library */
static void build_unix_import_lib( DLLSPEC *spec, struct strarray files )
{
    int i, total;
    const char *name, *prefix;
    char *dll_name = encode_dll_name( spec->file_name );

    /* entry points */

    for (i = total = 0; i < spec->exports.nb_entry_points; i++)
    {
        const ORDDEF *odp = spec->exports.entry_points[i];

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
            output_function_header( name, 1 );
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

    output_static_lib( output_file_name, files, 1 );
}

/* output an import library for a Win32 module and additional object files */
void output_import_lib( DLLSPEC *spec, struct strarray files )
{
    if (!is_pe()) build_unix_import_lib( spec, files );
    else if (use_dlltool) build_dlltool_import_lib( output_file_name, spec, files );
    else build_windows_import_lib( output_file_name, spec, files );
}
