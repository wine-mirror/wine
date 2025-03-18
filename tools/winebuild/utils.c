/*
 * Small utility functions for winebuild
 *
 * Copyright 2000 Alexandre Julliard
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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build.h"

const char *temp_dir = NULL;
struct strarray temp_files = { 0 };
static const char *output_file_source_name;

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
    if (input_file_name)
    {
        fprintf( stderr, "%s:", input_file_name );
        if (current_line)
            fprintf( stderr, "%d:", current_line );
        fputc( ' ', stderr );
    }
    else fprintf( stderr, "winebuild: " );
    vfprintf( stderr, msg, valist );
    va_end( valist );
    exit(1);
}

void fatal_perror( const char *msg, ... )
{
    va_list valist;
    va_start( valist, msg );
    if (input_file_name)
    {
        fprintf( stderr, "%s:", input_file_name );
        if (current_line)
            fprintf( stderr, "%d:", current_line );
        fputc( ' ', stderr );
    }
    vfprintf( stderr, msg, valist );
    perror( " " );
    va_end( valist );
    exit(1);
}

void error( const char *msg, ... )
{
    va_list valist;
    va_start( valist, msg );
    if (input_file_name)
    {
        fprintf( stderr, "%s:", input_file_name );
        if (current_line)
            fprintf( stderr, "%d:", current_line );
        fputc( ' ', stderr );
    }
    vfprintf( stderr, msg, valist );
    va_end( valist );
    nb_errors++;
}

void warning( const char *msg, ... )
{
    va_list valist;

    if (!display_warnings) return;
    va_start( valist, msg );
    if (input_file_name)
    {
        fprintf( stderr, "%s:", input_file_name );
        if (current_line)
            fprintf( stderr, "%d:", current_line );
        fputc( ' ', stderr );
    }
    fprintf( stderr, "warning: " );
    vfprintf( stderr, msg, valist );
    va_end( valist );
}

int output( const char *format, ... )
{
    int ret;
    va_list valist;

    va_start( valist, format );
    ret = vfprintf( output_file, format, valist );
    va_end( valist );
    if (ret < 0) fatal_perror( "Output error" );
    return ret;
}

static struct strarray get_tools_path(void)
{
    static int done;
    static struct strarray dirs;

    if (!done)
    {
        strarray_addall( &dirs, tools_path );
        strarray_addall( &dirs, strarray_frompath( getenv( "PATH" )));
        done = 1;
    }
    return dirs;
}

/* find a binary in the path */
static const char *find_binary( const char *prefix, const char *name )
{
    struct strarray dirs = get_tools_path();
    unsigned int i, maxlen = 0;
    struct stat st;
    char *p, *file;

    if (strchr( name, '/' )) return name;
    if (!prefix) prefix = "";
    for (i = 0; i < dirs.count; i++) maxlen = max( maxlen, strlen(dirs.str[i]) + 2 );
    file = xmalloc( maxlen + strlen(prefix) + strlen(name) + sizeof(EXEEXT) + 1 );

    for (i = 0; i < dirs.count; i++)
    {
        strcpy( file, dirs.str[i] );
        p = file + strlen(file);
        if (p == file) *p++ = '.';
        if (p[-1] != '/') *p++ = '/';
        if (*prefix)
        {
            strcpy( p, prefix );
            p += strlen(p);
            *p++ = '-';
        }
        strcpy( p, name );
        strcat( p, EXEEXT );
        if (!stat( file, &st ) && S_ISREG(st.st_mode) && (st.st_mode & 0111)) return file;
    }
    free( file );
    return NULL;
}

void spawn( struct strarray args )
{
    int status;
    const char *argv0 = find_binary( NULL, args.str[0] );

    if (argv0) args.str[0] = argv0;
    if (verbose) strarray_trace( args );

    if ((status = strarray_spawn( args )))
    {
	if (status > 0) fatal_error( "%s failed with status %u\n", args.str[0], status );
	else fatal_perror( "winebuild" );
	exit( 1 );
    }
}

static const char *find_clang_tool( struct strarray clang, const char *tool )
{
    const char *out = make_temp_file( "print_tool", ".out" );
    struct strarray args = empty_strarray;
    int sout = -1;
    char *path, *p;
    struct stat st;
    size_t cnt;

    strarray_addall( &args, clang );
    if (!args.count) strarray_add( &args, "clang" );
    strarray_add( &args, strmake( "-print-prog-name=%s", tool ));
    if (verbose) strarray_add( &args, "-v" );

    sout = dup( fileno(stdout) );
    freopen( out, "w", stdout );
    spawn( args );
    if (sout >= 0)
    {
        dup2( sout, fileno(stdout) );
        close( sout );
    }

    if (stat(out, &st) || !st.st_size) return NULL;

    path = xmalloc(st.st_size + 1);
    sout = open(out, O_RDONLY);
    if (sout == -1) return NULL;
    cnt = read(sout, path, st.st_size);
    close(sout);
    path[cnt] = 0;
    if ((p = strchr(path, '\n'))) *p = 0;
    /* clang returns passed command instead of full path if the tool could not be found */
    if (!strcmp(path, tool))
    {
        free( path );
        return NULL;
    }
    return path;
}

/* find a build tool in the path, trying the various names */
struct strarray find_tool( const char *name, const char * const *names )
{
    struct strarray ret = empty_strarray;
    const char *file;
    const char *alt_names[2];

    if (!names)
    {
        alt_names[0] = name;
        alt_names[1] = NULL;
        names = alt_names;
    }

    while (*names)
    {
        if ((file = find_binary( target_alias, *names ))) break;
        names++;
    }

    if (!file && cc_command.count) file = find_clang_tool( cc_command, name );
    if (!file) file = find_binary( "llvm", name );
    if (!file) file = find_clang_tool( empty_strarray, strmake( "llvm-%s", name ));
    if (!file) file = find_clang_tool( empty_strarray, name );

    if (!file) fatal_error( "cannot find the '%s' tool\n", name );

    strarray_add( &ret, file );
    return ret;
}

/* find a link tool in the path */
struct strarray find_link_tool(void)
{
    struct strarray ret = empty_strarray;
    const char *file = NULL;

    if (cc_command.count) file = find_clang_tool( cc_command, "lld-link" );
    if (!file) file = find_binary( NULL, "lld-link" );
    if (!file) file = find_clang_tool( empty_strarray, "lld-link" );

    if (!file) fatal_error( "cannot find the 'lld-link' tool\n" );
    strarray_add( &ret, file );
    return ret;
}

struct strarray get_as_command(void)
{
    struct strarray args = empty_strarray;
    const char *file;
    unsigned int i;
    int using_cc = 0;

    if (cc_command.count)
    {
        strarray_addall( &args, cc_command );
        using_cc = 1;
    }
    else if (as_command.count)
    {
        strarray_addall( &args, as_command );
    }
    else if ((file = find_binary( target_alias, "as" )) || (file = find_binary( target_alias, "gas ")))
    {
        strarray_add( &args, file );
    }
    else if ((file = find_binary( NULL, "clang" )))
    {
        strarray_add( &args, file );
        if (target_alias)
        {
            strarray_add( &args, "-target" );
            strarray_add( &args, target_alias );
        }
        using_cc = 1;
    }

    if (using_cc)
    {
        strarray_add( &args, "-xassembler" );
        strarray_add( &args, "-c" );
        if (force_pointer_size)
            strarray_add( &args, (force_pointer_size == 8) ? "-m64" : "-m32" );
        if (cpu_option) strarray_add( &args, strmake("-mcpu=%s", cpu_option) );
        if (fpu_option) strarray_add( &args, strmake("-mfpu=%s", fpu_option) );
        if (arch_option) strarray_add( &args, strmake("-march=%s", arch_option) );
        for (i = 0; i < tools_path.count; i++)
            strarray_add( &args, strmake("-B%s", tools_path.str[i] ));
        return args;
    }

    if (force_pointer_size)
    {
        switch (target.platform)
        {
        case PLATFORM_APPLE:
            strarray_add( &args, "-arch" );
            strarray_add( &args, (force_pointer_size == 8) ? "x86_64" : "i386" );
            break;
        default:
            strarray_add( &args, (force_pointer_size == 8) ? "--64" : "--32" );
            break;
        }
    }

    if (cpu_option) strarray_add( &args, strmake("-mcpu=%s", cpu_option) );
    if (fpu_option) strarray_add( &args, strmake("-mfpu=%s", fpu_option) );
    return args;
}

struct strarray get_ld_command(void)
{
    struct strarray args = empty_strarray;

    if (!ld_command.count)
    {
        static const char * const commands[] = { "ld", "gld", NULL };
        ld_command = find_tool( "ld", commands );
    }

    strarray_addall( &args, ld_command );

    if (force_pointer_size)
    {
        switch (target.platform)
        {
        case PLATFORM_APPLE:
            strarray_add( &args, "-arch" );
            strarray_add( &args, (force_pointer_size == 8) ? "x86_64" : "i386" );
            break;
        case PLATFORM_FREEBSD:
            strarray_add( &args, "-m" );
            strarray_add( &args, (force_pointer_size == 8) ? "elf_x86_64_fbsd" : "elf_i386_fbsd" );
            break;
        case PLATFORM_MINGW:
        case PLATFORM_WINDOWS:
            strarray_add( &args, "-m" );
            strarray_add( &args, (force_pointer_size == 8) ? "i386pep" : "i386pe" );
            break;
        default:
            strarray_add( &args, "-m" );
            strarray_add( &args, (force_pointer_size == 8) ? "elf_x86_64" : "elf_i386" );
            break;
        }
    }

    if (target.cpu == CPU_ARM && !is_pe())
        strarray_add( &args, "--no-wchar-size-warning" );

    return args;
}

const char *get_nm_command(void)
{
    if (!nm_command.count)
    {
        static const char * const commands[] = { "nm", "gnm", NULL };
        nm_command = find_tool( "nm", commands );
    }
    if (nm_command.count > 1)
        fatal_error( "multiple arguments in nm command not supported yet\n" );
    return nm_command.str[0];
}


/*******************************************************************
 *         buffer management
 *
 * Function for reading from/writing to a memory buffer.
 */

int byte_swapped = 0;
const char *input_buffer_filename;
const unsigned char *input_buffer;
size_t input_buffer_pos;
size_t input_buffer_size;
unsigned char *output_buffer;
size_t output_buffer_pos;
size_t output_buffer_size;

void init_input_buffer( const char *file )
{
    if (!(input_buffer = read_file( file, &input_buffer_size ))) fatal_perror( "Cannot read %s", file );
    if (!input_buffer_size) fatal_error( "%s is an empty file\n", file );
    input_buffer_filename = xstrdup( file );
    input_buffer_pos = 0;
    byte_swapped = 0;
}

unsigned char get_byte(void)
{
    if (input_buffer_pos >= input_buffer_size)
        fatal_error( "%s is a truncated file\n", input_buffer_filename );
    return input_buffer[input_buffer_pos++];
}

unsigned short get_word(void)
{
    unsigned short ret;

    if (input_buffer_pos + sizeof(ret) > input_buffer_size)
        fatal_error( "%s is a truncated file\n", input_buffer_filename );
    memcpy( &ret, input_buffer + input_buffer_pos, sizeof(ret) );
    if (byte_swapped) ret = (ret << 8) | (ret >> 8);
    input_buffer_pos += sizeof(ret);
    return ret;
}

unsigned int get_dword(void)
{
    unsigned int ret;

    if (input_buffer_pos + sizeof(ret) > input_buffer_size)
        fatal_error( "%s is a truncated file\n", input_buffer_filename );
    memcpy( &ret, input_buffer + input_buffer_pos, sizeof(ret) );
    if (byte_swapped)
        ret = ((ret << 24) | ((ret << 8) & 0x00ff0000) | ((ret >> 8) & 0x0000ff00) | (ret >> 24));
    input_buffer_pos += sizeof(ret);
    return ret;
}

/* pointer-sized word */
void put_pword( unsigned int val )
{
    if (get_ptr_size() == 8) put_qword( val );
    else put_dword( val );
}

/* output a standard header for generated files */
void output_standard_file_header(void)
{
    if (spec_file_name)
        output( "/* File generated automatically from %s; do not edit! */\n", spec_file_name );
    else
        output( "/* File generated automatically; do not edit! */\n" );
    output( "/* This file can be copied, modified and distributed without restriction. */\n\n" );
    if (safe_seh)
    {
        output( "\t.def    @feat.00\n\t.scl 3\n\t.type 0\n\t.endef\n" );
        output( "\t.globl  @feat.00\n" );
        output( ".set @feat.00, 1\n" );
    }
}

/* dump a byte stream into the assembly code */
void dump_bytes( const void *buffer, unsigned int size )
{
    unsigned int i;
    const unsigned char *ptr = buffer;

    if (!size) return;
    output( "\t.byte " );
    for (i = 0; i < size - 1; i++, ptr++)
    {
        if ((i % 16) == 15) output( "0x%02x\n\t.byte ", *ptr );
        else output( "0x%02x,", *ptr );
    }
    output( "0x%02x\n", *ptr );
}


/*******************************************************************
 *         open_input_file
 *
 * Open a file in the given srcdir and set the input_file_name global variable.
 */
FILE *open_input_file( const char *srcdir, const char *name )
{
    char *fullname;
    FILE *file = fopen( name, "r" );

    if (!file && srcdir)
    {
        fullname = strmake( "%s/%s", srcdir, name );
        file = fopen( fullname, "r" );
    }
    else fullname = xstrdup( name );

    if (!file) fatal_error( "Cannot open file '%s'\n", fullname );
    input_file_name = fullname;
    current_line = 1;
    return file;
}


/*******************************************************************
 *         close_input_file
 *
 * Close the current input file (must have been opened with open_input_file).
 */
void close_input_file( FILE *file )
{
    fclose( file );
    free( input_file_name );
    input_file_name = NULL;
    current_line = 0;
}


/*******************************************************************
 *         open_output_file
 */
void open_output_file(void)
{
    if (output_file_name)
    {
        if (strendswith( output_file_name, ".o" ))
            output_file_source_name = open_temp_output_file( ".s" );
        else
            if (!(output_file = fopen( output_file_name, "w" )))
                fatal_error( "Unable to create output file '%s'\n", output_file_name );
    }
    else output_file = stdout;
}


/*******************************************************************
 *         close_output_file
 */
void close_output_file(void)
{
    if (!output_file || !output_file_name) return;
    if (fclose( output_file ) < 0) fatal_perror( "fclose" );
    if (output_file_source_name) assemble_file( output_file_source_name, output_file_name );
    output_file = NULL;
}


/*******************************************************************
 *         open_temp_output_file
 */
char *open_temp_output_file( const char *suffix )
{
    char *tmp_file = make_temp_file( output_file_name, suffix );
    if (!(output_file = fopen( tmp_file, "w" )))
        fatal_error( "Unable to create output file '%s'\n", tmp_file );
    return tmp_file;
}


/*******************************************************************
 *         remove_stdcall_decoration
 *
 * Remove a possible @xx suffix from a function name.
 * Return the numerical value of the suffix, or -1 if none.
 */
int remove_stdcall_decoration( char *name )
{
    char *p, *end = strrchr( name, '@' );
    if (!end || !end[1] || end == name) return -1;
    if (target.cpu != CPU_i386) return -1;
    /* make sure all the rest is digits */
    for (p = end + 1; *p; p++) if (!isdigit(*p)) return -1;
    *end = 0;
    return atoi( end + 1 );
}


/*******************************************************************
 *         assemble_file
 *
 * Run a file through the assembler.
 */
void assemble_file( const char *src_file, const char *obj_file )
{
    struct strarray args = get_as_command();
    strarray_add( &args, "-o" );
    strarray_add( &args, obj_file );
    strarray_add( &args, src_file );
    spawn( args );
}


/*******************************************************************
 *         alloc_dll_spec
 *
 * Create a new dll spec file descriptor
 */
DLLSPEC *alloc_dll_spec(void)
{
    DLLSPEC *spec;

    spec = xmalloc( sizeof(*spec) );
    memset( spec, 0, sizeof(*spec) );
    spec->type               = SPEC_WIN32;
    spec->characteristics    = IMAGE_FILE_EXECUTABLE_IMAGE;
    spec->subsystem          = IMAGE_SUBSYSTEM_WINDOWS_CUI;
    spec->subsystem_major    = 4;
    spec->subsystem_minor    = 0;
    spec->dll_characteristics = IMAGE_DLLCHARACTERISTICS_NX_COMPAT | IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE;
    spec->exports.base        = MAX_ORDINALS;
    spec->native_exports.base = MAX_ORDINALS;
    return spec;
}


static void free_exports( struct exports *entries )
{
    free( entries->entry_points );
    free( entries->names );
    free( entries->ordinals );
}


/*******************************************************************
 *         free_dll_spec
 *
 * Free dll spec file descriptor
 */
void free_dll_spec( DLLSPEC *spec )
{
    int i;

    for (i = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];
        free( odp->name );
        free( odp->export_name );
        free( odp->link_name );
    }
    free_exports( &spec->exports );
    free_exports( &spec->native_exports );
    free( spec->file_name );
    free( spec->dll_name );
    free( spec->c_name );
    free( spec->init_func );
    free( spec->entry_points );
    free( spec->resources );
    free( spec );
}


/*******************************************************************
 *         make_c_identifier
 *
 * Map a string to a valid C identifier.
 */
char *make_c_identifier( const char *str )
{
    char *p, buffer[256];

    for (p = buffer; *str && p < buffer+sizeof(buffer)-1; p++, str++)
    {
        if (isalnum(*str)) *p = *str;
        else *p = '_';
    }
    *p = 0;
    return xstrdup( buffer );
}


/*******************************************************************
 *         get_stub_name
 *
 * Generate an internal name for a stub entry point.
 */
const char *get_stub_name( const ORDDEF *odp, const DLLSPEC *spec )
{
    static char *buffer;

    free( buffer );
    if (odp->name || odp->export_name)
    {
        char *p;
        buffer = strmake( "__wine_stub_%s", odp->name ? odp->name : odp->export_name );
        /* make sure name is a legal C identifier */
        for (p = buffer; *p; p++) if (!isalnum(*p) && *p != '_') break;
        if (!*p) return buffer;
        free( buffer );
    }
    buffer = strmake( "__wine_stub_%s_%d", make_c_identifier(spec->file_name), odp->ordinal );
    return buffer;
}

/* return the stdcall-decorated name for an entry point */
const char *get_abi_name( const ORDDEF *odp, const char *name )
{
    static char *buffer;
    char *ret;

    if (target.cpu != CPU_i386) return name;

    switch (odp->type)
    {
    case TYPE_STDCALL:
        if (is_pe())
        {
            if (odp->flags & FLAG_THISCALL) return name;
            if (odp->flags & FLAG_FASTCALL) ret = strmake( "@%s@%u", name, get_args_size( odp ));
            else if (!kill_at) ret = strmake( "%s@%u", name, get_args_size( odp ));
            else return name;
        }
        else
        {
            if (odp->flags & FLAG_THISCALL) ret = strmake( "__thiscall_%s", name );
            else if (odp->flags & FLAG_FASTCALL) ret = strmake( "__fastcall_%s", name );
            else return name;
        }
        break;

    case TYPE_PASCAL:
        if (is_pe() && !kill_at)
        {
            int args = get_args_size( odp );
            if (odp->flags & FLAG_REGISTER) args += get_ptr_size();  /* context argument */
            ret = strmake( "%s@%u", name, args );
        }
        else return name;
        break;

    default:
        return name;
    }

    free( buffer );
    buffer = ret;
    return ret;
}

const char *get_link_name( const ORDDEF *odp )
{
    return get_abi_name( odp, odp->link_name );
}

/*******************************************************************
 *         sort_func_list
 *
 * Sort a list of functions, removing duplicates.
 */
int sort_func_list( ORDDEF **list, int count, int (*compare)(const void *, const void *) )
{
    int i, j;

    if (!count) return 0;
    qsort( list, count, sizeof(*list), compare );
    for (i = j = 0; i < count; i++) if (compare( &list[j], &list[i] )) list[++j] = list[i];
    return j + 1;
}


/* return the page size for the target CPU */
unsigned int get_page_size(void)
{
    return 0x1000;  /* same on all platforms */
}

/* return the total size in bytes of the arguments on the stack */
unsigned int get_args_size( const ORDDEF *odp )
{
    int i, size;

    for (i = size = 0; i < odp->u.func.nb_args; i++)
    {
        switch (odp->u.func.args[i])
        {
        case ARG_INT64:
        case ARG_DOUBLE:
            if (target.cpu == CPU_ARM) size = (size + 7) & ~7;
            size += 8;
            break;
        case ARG_INT128:
            /* int128 is passed as pointer on x86_64 */
            if (target.cpu != CPU_x86_64)
            {
                size += 16;
                break;
            }
            /* fall through */
        default:
            size += get_ptr_size();
            break;
        }
    }
    return size;
}

/* return the assembly name for a C symbol */
const char *asm_name( const char *sym )
{
    static char *buffer;

    switch (target.platform)
    {
    case PLATFORM_MINGW:
    case PLATFORM_WINDOWS:
        if (target.cpu != CPU_i386) return sym;
        if (sym[0] == '@') return sym;  /* fastcall */
        /* fall through */
    case PLATFORM_APPLE:
        if (sym[0] == '.' && sym[1] == 'L') return sym;
        free( buffer );
        buffer = strmake( "_%s", sym );
        return buffer;
    default:
        return sym;
    }
}

/* return the assembly name for an ARM64/ARM64EC function */
const char *arm64_name( const char *sym )
{
    if (target.cpu == CPU_ARM64EC) return strmake( "\"#%s\"", sym );
    return asm_name( sym );
}

/* return an assembly function declaration for a C function name */
void output_function_header( const char *func, int global )
{
    const char *name = arm64_name( func );

    output( "\t.text\n" );

    switch (target.platform)
    {
    case PLATFORM_APPLE:
        if (global) output( "\t.globl %s\n\t.private_extern %s\n", name, name );
        break;
    case PLATFORM_MINGW:
    case PLATFORM_WINDOWS:
        if (target.cpu == CPU_ARM64EC) output( ".section .text,\"xr\",discard,%s\n\t", name );
        output( "\t.def %s\n\t.scl 2\n\t.type 32\n\t.endef\n", name );
        if (global) output( "\t.globl %s\n", name );
        break;
    default:
        output( "\t.type %s,@function\n", name );
        if (global) output( "\t.globl %s\n\t.hidden %s\n", name, name );
        break;
    }
    output( "\t.balign 4\n" );
    output( "%s:\n", name );
}

/* output a size declaration for an assembly function */
void output_function_size( const char *name )
{
    switch (target.platform)
    {
    case PLATFORM_APPLE:
    case PLATFORM_MINGW:
    case PLATFORM_WINDOWS:
        break;
    default:
        output( "\t.size %s, .-%s\n", name, name );
        break;
    }
}

/* output a .cfi directive */
void output_cfi( const char *format, ... )
{
    va_list valist;

    if (!unwind_tables) return;
    va_start( valist, format );
    fputc( '\t', output_file );
    vfprintf( output_file, format, valist );
    fputc( '\n', output_file );
    va_end( valist );
}

/* output a .seh directive */
void output_seh( const char *format, ... )
{
    va_list valist;

    if (!is_pe()) return;
    va_start( valist, format );
    fputc( '\t', output_file );
    vfprintf( output_file, format, valist );
    fputc( '\n', output_file );
    va_end( valist );
}

/* output an RVA pointer */
void output_rva( const char *format, ... )
{
    va_list valist;

    va_start( valist, format );
    if (is_pe())
    {
        output( "\t.rva " );
        vfprintf( output_file, format, valist );
        fputc( '\n', output_file );
    }
    else
    {
        output( "\t.long " );
        vfprintf( output_file, format, valist );
        output( " - .L__wine_spec_rva_base\n" );
    }
    va_end( valist );
}

/* output an RVA pointer or ordinal for a function thunk */
void output_thunk_rva( int ordinal, const char *format, ... )
{
    if (ordinal == -1)
    {
        va_list valist;

        va_start( valist, format );
        if (is_pe())
        {
            output( "\t.rva " );
            vfprintf( output_file, format, valist );
            fputc( '\n', output_file );
            if (get_ptr_size() == 8) output( "\t.long 0\n" );
        }
        else
        {
            output( "\t%s ", get_asm_ptr_keyword() );
            vfprintf( output_file, format, valist );
            output( " - .L__wine_spec_rva_base\n" );
        }
        va_end( valist );
    }
    else
    {
        if (get_ptr_size() == 4) output( "\t.long 0x8000%04x\n", ordinal );
        else output( "\t.quad 0x800000000000%04x\n", ordinal );
    }
}

/* output the GNU note for non-exec stack */
void output_gnu_stack_note(void)
{
    switch (target.platform)
    {
    case PLATFORM_MINGW:
    case PLATFORM_WINDOWS:
    case PLATFORM_APPLE:
        break;
    default:
        output( "\t.section .note.GNU-stack,\"\",@progbits\n" );
        break;
    }
}

/* return a global symbol declaration for an assembly symbol */
const char *asm_globl( const char *func )
{
    static char *buffer;

    free( buffer );
    switch (target.platform)
    {
    case PLATFORM_APPLE:
        buffer = strmake( "\t.globl _%s\n\t.private_extern _%s\n_%s:", func, func, func );
        break;
    case PLATFORM_MINGW:
    case PLATFORM_WINDOWS:
    {
        const char *name = asm_name( func );
        buffer = strmake( "\t.globl %s\n%s:", name, name );
        break;
    }
    default:
        buffer = strmake( "\t.globl %s\n\t.hidden %s\n%s:", func, func, func );
        break;
    }
    return buffer;
}

const char *get_asm_ptr_keyword(void)
{
    switch(get_ptr_size())
    {
    case 4: return ".long";
    case 8: return ".quad";
    }
    assert(0);
    return NULL;
}

const char *get_asm_string_keyword(void)
{
    switch (target.platform)
    {
    case PLATFORM_APPLE:
        return ".asciz";
    default:
        return ".string";
    }
}

const char *get_asm_export_section(void)
{
    switch (target.platform)
    {
    case PLATFORM_APPLE:   return ".data";
    case PLATFORM_MINGW:
    case PLATFORM_WINDOWS: return ".section .edata";
    default:               return ".section .data";
    }
}

const char *get_asm_rodata_section(void)
{
    switch (target.platform)
    {
    case PLATFORM_APPLE: return ".const";
    default:             return ".section .rodata";
    }
}

const char *get_asm_rsrc_section(void)
{
    switch (target.platform)
    {
    case PLATFORM_APPLE:   return ".data";
    case PLATFORM_MINGW:
    case PLATFORM_WINDOWS: return ".section .rsrc";
    default:               return ".section .data";
    }
}

const char *get_asm_string_section(void)
{
    switch (target.platform)
    {
    case PLATFORM_APPLE: return ".cstring";
    default:             return ".section .rodata";
    }
}
