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
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#include "build.h"

#if defined(_WIN32) && !defined(__CYGWIN__)
# define PATH_SEPARATOR ';'
#else
# define PATH_SEPARATOR ':'
#endif

static struct strarray tmp_files;
static struct strarray empty_strarray;
static const char *output_file_source_name;

static const struct
{
    const char *name;
    enum target_cpu cpu;
} cpu_names[] =
{
    { "i386",    CPU_x86 },
    { "i486",    CPU_x86 },
    { "i586",    CPU_x86 },
    { "i686",    CPU_x86 },
    { "i786",    CPU_x86 },
    { "amd64",   CPU_x86_64 },
    { "x86_64",  CPU_x86_64 },
    { "powerpc", CPU_POWERPC },
    { "arm",     CPU_ARM },
    { "armv5",   CPU_ARM },
    { "armv6",   CPU_ARM },
    { "armv7",   CPU_ARM },
    { "armv7a",  CPU_ARM },
    { "arm64",   CPU_ARM64 },
    { "aarch64", CPU_ARM64 },
};

/* atexit handler to clean tmp files */
void cleanup_tmp_files(void)
{
    unsigned int i;
    for (i = 0; i < tmp_files.count; i++) if (tmp_files.str[i]) unlink( tmp_files.str[i] );
}


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
    if (size && res == NULL)
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

int strendswith(const char* str, const char* end)
{
    int l = strlen(str);
    int m = strlen(end);
    return l >= m && strcmp(str + l - m, end) == 0;
}

char *strmake( const char* fmt, ... )
{
    int n;
    size_t size = 100;
    va_list ap;

    for (;;)
    {
        char *p = xmalloc( size );
        va_start( ap, fmt );
	n = vsnprintf( p, size, fmt, ap );
	va_end( ap );
        if (n == -1) size *= 2;
        else if ((size_t)n >= size) size = n + 1;
        else return p;
        free( p );
    }
}

static struct strarray strarray_copy( struct strarray src )
{
    struct strarray array;
    array.count = src.count;
    array.max = src.max;
    array.str = xmalloc( array.max * sizeof(*array.str) );
    memcpy( array.str, src.str, array.count * sizeof(*array.str) );
    return array;
}

static void strarray_add_one( struct strarray *array, const char *str )
{
    if (array->count == array->max)
    {
        array->max *= 2;
        if (array->max < 16) array->max = 16;
        array->str = xrealloc( array->str, array->max * sizeof(*array->str) );
    }
    array->str[array->count++] = str;
}

void strarray_add( struct strarray *array, ... )
{
    va_list valist;
    const char *str;

    va_start( valist, array );
    while ((str = va_arg( valist, const char *))) strarray_add_one( array, str );
    va_end( valist );
}

void strarray_addv( struct strarray *array, char * const *argv )
{
    while (*argv) strarray_add_one( array, *argv++ );
}

void strarray_addall( struct strarray *array, struct strarray args )
{
    unsigned int i;

    for (i = 0; i < args.count; i++) strarray_add_one( array, args.str[i] );
}

struct strarray strarray_fromstring( const char *str, const char *delim )
{
    const char *tok;
    struct strarray array = empty_strarray;
    char *buf = xstrdup( str );

    for (tok = strtok( buf, delim ); tok; tok = strtok( NULL, delim ))
	strarray_add_one( &array, strdup( tok ));

    free( buf );
    return array;
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

void spawn( struct strarray args )
{
    unsigned int i;
    int status;

    strarray_add_one( &args, NULL );
    if (verbose)
        for (i = 0; args.str[i]; i++)
            fprintf( stderr, "%s%c", args.str[i], args.str[i+1] ? ' ' : '\n' );

    if ((status = _spawnvp( _P_WAIT, args.str[0], args.str )))
    {
	if (status > 0) fatal_error( "%s failed with status %u\n", args.str[0], status );
	else fatal_perror( "winebuild" );
	exit( 1 );
    }
}

/* find a build tool in the path, trying the various names */
struct strarray find_tool( const char *name, const char * const *names )
{
    static char **dirs;
    static unsigned int count, maxlen;

    char *p, *file;
    const char *alt_names[2];
    unsigned int i, len;
    struct stat st;

    if (!dirs)
    {
        char *path;

        /* split the path in directories */

        if (!getenv( "PATH" )) fatal_error( "PATH not set, cannot find required tools\n" );
        path = xstrdup( getenv( "PATH" ));
        for (p = path, count = 2; *p; p++) if (*p == PATH_SEPARATOR) count++;
        dirs = xmalloc( count * sizeof(*dirs) );
        count = 0;
        dirs[count++] = p = path;
        while (*p)
        {
            while (*p && *p != PATH_SEPARATOR) p++;
            if (!*p) break;
            *p++ = 0;
            dirs[count++] = p;
        }
        for (i = 0; i < count; i++) maxlen = max( maxlen, strlen(dirs[i])+2 );
    }

    if (!names)
    {
        alt_names[0] = name;
        alt_names[1] = NULL;
        names = alt_names;
    }

    while (*names)
    {
        len = strlen(*names) + sizeof(EXEEXT) + 1;
        if (target_alias)
            len += strlen(target_alias) + 1;
        file = xmalloc( maxlen + len );

        for (i = 0; i < count; i++)
        {
            strcpy( file, dirs[i] );
            p = file + strlen(file);
            if (p == file) *p++ = '.';
            if (p[-1] != '/') *p++ = '/';
            if (target_alias)
            {
                strcpy( p, target_alias );
                p += strlen(p);
                *p++ = '-';
            }
            strcpy( p, *names );
            strcat( p, EXEEXT );

            if (!stat( file, &st ) && S_ISREG(st.st_mode) && (st.st_mode & 0111))
            {
                struct strarray ret = empty_strarray;
                strarray_add_one( &ret, file );
                return ret;
            }
        }
        free( file );
        names++;
    }
    fatal_error( "cannot find the '%s' tool\n", name );
}

struct strarray get_as_command(void)
{
    struct strarray args;

    if (cc_command.count)
    {
        args = strarray_copy( cc_command );
        strarray_add( &args, "-xassembler", "-c", NULL );
        if (force_pointer_size)
            strarray_add_one( &args, (force_pointer_size == 8) ? "-m64" : "-m32" );
        if (cpu_option) strarray_add_one( &args, strmake("-mcpu=%s", cpu_option) );
        if (fpu_option) strarray_add_one( &args, strmake("-mfpu=%s", fpu_option) );
        if (arch_option) strarray_add_one( &args, strmake("-march=%s", arch_option) );
        return args;
    }

    if (!as_command.count)
    {
        static const char * const commands[] = { "gas", "as", NULL };
        as_command = find_tool( "as", commands );
    }

    args = strarray_copy( as_command );

    if (force_pointer_size)
    {
        switch (target_platform)
        {
        case PLATFORM_APPLE:
            strarray_add( &args, "-arch", (force_pointer_size == 8) ? "x86_64" : "i386", NULL );
            break;
        default:
            switch(target_cpu)
            {
            case CPU_POWERPC:
                strarray_add_one( &args, (force_pointer_size == 8) ? "-a64" : "-a32" );
                break;
            default:
                strarray_add_one( &args, (force_pointer_size == 8) ? "--64" : "--32" );
                break;
            }
            break;
        }
    }

    if (cpu_option) strarray_add_one( &args, strmake("-mcpu=%s", cpu_option) );
    if (fpu_option) strarray_add_one( &args, strmake("-mfpu=%s", fpu_option) );
    return args;
}

struct strarray get_ld_command(void)
{
    struct strarray args;

    if (!ld_command.count)
    {
        static const char * const commands[] = { "ld", "gld", NULL };
        ld_command = find_tool( "ld", commands );
    }

    args = strarray_copy( ld_command );

    if (force_pointer_size)
    {
        switch (target_platform)
        {
        case PLATFORM_APPLE:
            strarray_add( &args, "-arch", (force_pointer_size == 8) ? "x86_64" : "i386", NULL );
            break;
        case PLATFORM_FREEBSD:
            strarray_add( &args, "-m", (force_pointer_size == 8) ? "elf_x86_64_fbsd" : "elf_i386_fbsd", NULL );
            break;
        case PLATFORM_WINDOWS:
            strarray_add( &args, "-m", (force_pointer_size == 8) ? "i386pep" : "i386pe", NULL );
            break;
        default:
            switch(target_cpu)
            {
            case CPU_POWERPC:
                strarray_add( &args, "-m", (force_pointer_size == 8) ? "elf64ppc" : "elf32ppc", NULL );
                break;
            default:
                strarray_add( &args, "-m", (force_pointer_size == 8) ? "elf_x86_64" : "elf_i386", NULL );
                break;
            }
            break;
        }
    }

    if (target_cpu == CPU_ARM && target_platform != PLATFORM_WINDOWS)
        strarray_add( &args, "--no-wchar-size-warning", NULL );

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

/* get a name for a temp file, automatically cleaned up on exit */
char *get_temp_file_name( const char *prefix, const char *suffix )
{
    char *name;
    const char *ext, *basename;
    int fd;

    if (!prefix || !prefix[0]) prefix = "winebuild";
    if (!suffix) suffix = "";
    if ((basename = strrchr( prefix, '/' ))) basename++;
    else basename = prefix;
    if (!(ext = strchr( basename, '.' ))) ext = prefix + strlen(prefix);
    name = xmalloc( sizeof("/tmp/") + (ext - prefix) + sizeof(".XXXXXX") + strlen(suffix) );
    memcpy( name, prefix, ext - prefix );
    strcpy( name + (ext - prefix), ".XXXXXX" );
    strcat( name, suffix );

    if ((fd = mkstemps( name, strlen(suffix) )) == -1)
    {
        strcpy( name, "/tmp/" );
        memcpy( name + 5, basename, ext - basename );
        strcpy( name + 5 + (ext - basename), ".XXXXXX" );
        strcat( name, suffix );
        if ((fd = mkstemps( name, strlen(suffix) )) == -1)
            fatal_error( "could not generate a temp file\n" );
    }

    close( fd );
    strarray_add_one( &tmp_files, name );
    return name;
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

static void check_output_buffer_space( size_t size )
{
    if (output_buffer_pos + size >= output_buffer_size)
    {
        output_buffer_size = max( output_buffer_size * 2, output_buffer_pos + size );
        output_buffer = xrealloc( output_buffer, output_buffer_size );
    }
}

void init_input_buffer( const char *file )
{
    int fd;
    struct stat st;
    unsigned char *buffer;

    if ((fd = open( file, O_RDONLY | O_BINARY )) == -1) fatal_perror( "Cannot open %s", file );
    if ((fstat( fd, &st ) == -1)) fatal_perror( "Cannot stat %s", file );
    if (!st.st_size) fatal_error( "%s is an empty file\n", file );
    input_buffer = buffer = xmalloc( st.st_size );
    if (read( fd, buffer, st.st_size ) != st.st_size) fatal_error( "Cannot read %s\n", file );
    close( fd );
    input_buffer_filename = xstrdup( file );
    input_buffer_size = st.st_size;
    input_buffer_pos = 0;
    byte_swapped = 0;
}

void init_output_buffer(void)
{
    output_buffer_size = 1024;
    output_buffer_pos = 0;
    output_buffer = xmalloc( output_buffer_size );
}

void flush_output_buffer(void)
{
    open_output_file();
    if (fwrite( output_buffer, 1, output_buffer_pos, output_file ) != output_buffer_pos)
        fatal_error( "Error writing to %s\n", output_file_name );
    close_output_file();
    free( output_buffer );
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

void put_data( const void *data, size_t size )
{
    check_output_buffer_space( size );
    memcpy( output_buffer + output_buffer_pos, data, size );
    output_buffer_pos += size;
}

void put_byte( unsigned char val )
{
    check_output_buffer_space( 1 );
    output_buffer[output_buffer_pos++] = val;
}

void put_word( unsigned short val )
{
    if (byte_swapped) val = (val << 8) | (val >> 8);
    put_data( &val, sizeof(val) );
}

void put_dword( unsigned int val )
{
    if (byte_swapped)
        val = ((val << 24) | ((val << 8) & 0x00ff0000) | ((val >> 8) & 0x0000ff00) | (val >> 24));
    put_data( &val, sizeof(val) );
}

void put_qword( unsigned int val )
{
    if (byte_swapped)
    {
        put_dword( 0 );
        put_dword( val );
    }
    else
    {
        put_dword( val );
        put_dword( 0 );
    }
}

/* pointer-sized word */
void put_pword( unsigned int val )
{
    if (get_ptr_size() == 8) put_qword( val );
    else put_dword( val );
}

void align_output( unsigned int align )
{
    size_t size = align - (output_buffer_pos % align);

    if (size == align) return;
    check_output_buffer_space( size );
    memset( output_buffer + output_buffer_pos, 0, size );
    output_buffer_pos += size;
}

/* output a standard header for generated files */
void output_standard_file_header(void)
{
    if (spec_file_name)
        output( "/* File generated automatically from %s; do not edit! */\n", spec_file_name );
    else
        output( "/* File generated automatically; do not edit! */\n" );
    output( "/* This file can be copied, modified and distributed without restriction. */\n\n" );
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
    char *tmp_file = get_temp_file_name( output_file_name, suffix );
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
    if (target_cpu != CPU_x86) return -1;
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
    strarray_add( &args, "-o", obj_file, src_file, NULL );
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
    spec->base               = MAX_ORDINALS;
    spec->characteristics    = IMAGE_FILE_EXECUTABLE_IMAGE;
    spec->subsystem          = 0;
    spec->subsystem_major    = 4;
    spec->subsystem_minor    = 0;
    if (get_ptr_size() > 4)
        spec->characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
    else
        spec->characteristics |= IMAGE_FILE_32BIT_MACHINE;
    spec->dll_characteristics = IMAGE_DLLCHARACTERISTICS_NX_COMPAT;
    return spec;
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
    free( spec->file_name );
    free( spec->dll_name );
    free( spec->c_name );
    free( spec->init_func );
    free( spec->entry_points );
    free( spec->names );
    free( spec->ordinals );
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
const char *get_link_name( const ORDDEF *odp )
{
    static char *buffer;
    char *ret;

    if (target_cpu != CPU_x86) return odp->link_name;

    switch (odp->type)
    {
    case TYPE_STDCALL:
        if (target_platform == PLATFORM_WINDOWS)
        {
            if (odp->flags & FLAG_THISCALL) return odp->link_name;
            if (odp->flags & FLAG_FASTCALL) ret = strmake( "@%s@%u", odp->link_name, get_args_size( odp ));
            else if (!kill_at) ret = strmake( "%s@%u", odp->link_name, get_args_size( odp ));
            else return odp->link_name;
        }
        else
        {
            if (odp->flags & FLAG_THISCALL) ret = strmake( "__thiscall_%s", odp->link_name );
            else if (odp->flags & FLAG_FASTCALL) ret = strmake( "__fastcall_%s", odp->link_name );
            else return odp->link_name;
        }
        break;

    case TYPE_PASCAL:
        if (target_platform == PLATFORM_WINDOWS && !kill_at)
        {
            int args = get_args_size( odp );
            if (odp->flags & FLAG_REGISTER) args += get_ptr_size();  /* context argument */
            ret = strmake( "%s@%u", odp->link_name, args );
        }
        else return odp->link_name;
        break;

    default:
        return odp->link_name;
    }

    free( buffer );
    buffer = ret;
    return ret;
}

/* parse a cpu name and return the corresponding value */
int get_cpu_from_name( const char *name )
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(cpu_names); i++)
        if (!strcmp( cpu_names[i].name, name )) return cpu_names[i].cpu;
    return -1;
}

/*****************************************************************
 *  Function:    get_alignment
 *
 *  Description:
 *    According to the info page for gas, the .align directive behaves
 * differently on different systems.  On some architectures, the
 * argument of a .align directive is the number of bytes to pad to, so
 * to align on an 8-byte boundary you'd say
 *     .align 8
 * On other systems, the argument is "the number of low-order zero bits
 * that the location counter must have after advancement."  So to
 * align on an 8-byte boundary you'd say
 *     .align 3
 *
 * The reason gas is written this way is that it's trying to mimic
 * native assemblers for the various architectures it runs on.  gas
 * provides other directives that work consistently across
 * architectures, but of course we want to work on all arches with or
 * without gas.  Hence this function.
 *
 *
 *  Parameters:
 *    align  --  the number of bytes to align to. Must be a power of 2.
 */
unsigned int get_alignment(unsigned int align)
{
    unsigned int n;

    assert( !(align & (align - 1)) );

    switch(target_cpu)
    {
    case CPU_x86:
    case CPU_x86_64:
        if (target_platform != PLATFORM_APPLE) return align;
        /* fall through */
    case CPU_POWERPC:
    case CPU_ARM:
    case CPU_ARM64:
        n = 0;
        while ((1u << n) != align) n++;
        return n;
    }
    /* unreached */
    assert(0);
    return 0;
}

/* return the page size for the target CPU */
unsigned int get_page_size(void)
{
    switch(target_cpu)
    {
    case CPU_x86:
    case CPU_x86_64:
    case CPU_POWERPC:
    case CPU_ARM:
        return 0x1000;
    case CPU_ARM64:
        return 0x10000;
    }
    /* unreached */
    assert(0);
    return 0;
}

/* return the size of a pointer on the target CPU */
unsigned int get_ptr_size(void)
{
    switch(target_cpu)
    {
    case CPU_x86:
    case CPU_POWERPC:
    case CPU_ARM:
        return 4;
    case CPU_x86_64:
    case CPU_ARM64:
        return 8;
    }
    /* unreached */
    assert(0);
    return 0;
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
            size += 8;
            break;
        case ARG_INT128:
            /* int128 is passed as pointer on x86_64 */
            if (target_cpu != CPU_x86_64)
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

    switch (target_platform)
    {
    case PLATFORM_WINDOWS:
        if (target_cpu != CPU_x86) return sym;
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

/* return an assembly function declaration for a C function name */
const char *func_declaration( const char *func )
{
    static char *buffer;

    switch (target_platform)
    {
    case PLATFORM_APPLE:
        return "";
    case PLATFORM_WINDOWS:
        free( buffer );
        buffer = strmake( ".def %s%s; .scl 2; .type 32; .endef", target_cpu == CPU_x86 ? "_" : "", func );
        break;
    default:
        free( buffer );
        switch(target_cpu)
        {
        case CPU_ARM:
        case CPU_ARM64:
            buffer = strmake( ".type %s,%%function", func );
            break;
        default:
            buffer = strmake( ".type %s,@function", func );
            break;
        }
        break;
    }
    return buffer;
}

/* output a size declaration for an assembly function */
void output_function_size( const char *name )
{
    switch (target_platform)
    {
    case PLATFORM_APPLE:
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

/* output an RVA pointer */
void output_rva( const char *format, ... )
{
    va_list valist;

    va_start( valist, format );
    switch (target_platform)
    {
    case PLATFORM_WINDOWS:
        output( "\t.rva " );
        vfprintf( output_file, format, valist );
        fputc( '\n', output_file );
        break;
    default:
        output( "\t.long " );
        vfprintf( output_file, format, valist );
        output( " - .L__wine_spec_rva_base\n" );
        break;
    }
    va_end( valist );
}

/* output the GNU note for non-exec stack */
void output_gnu_stack_note(void)
{
    switch (target_platform)
    {
    case PLATFORM_WINDOWS:
    case PLATFORM_APPLE:
        break;
    default:
        switch(target_cpu)
        {
        case CPU_ARM:
        case CPU_ARM64:
            output( "\t.section .note.GNU-stack,\"\",%%progbits\n" );
            break;
        default:
            output( "\t.section .note.GNU-stack,\"\",@progbits\n" );
            break;
        }
        break;
    }
}

/* return a global symbol declaration for an assembly symbol */
const char *asm_globl( const char *func )
{
    static char *buffer;

    free( buffer );
    switch (target_platform)
    {
    case PLATFORM_APPLE:
        buffer = strmake( "\t.globl _%s\n\t.private_extern _%s\n_%s:", func, func, func );
        break;
    case PLATFORM_WINDOWS:
        buffer = strmake( "\t.globl %s%s\n%s%s:", target_cpu == CPU_x86 ? "_" : "", func,
                          target_cpu == CPU_x86 ? "_" : "", func );
        break;
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
    switch (target_platform)
    {
    case PLATFORM_APPLE:
        return ".asciz";
    default:
        return ".string";
    }
}

const char *get_asm_export_section(void)
{
    switch (target_platform)
    {
    case PLATFORM_APPLE:   return ".data";
    case PLATFORM_WINDOWS: return ".section .edata";
    default:               return ".section .data";
    }
}

const char *get_asm_rodata_section(void)
{
    switch (target_platform)
    {
    case PLATFORM_APPLE: return ".const";
    default:             return ".section .rodata";
    }
}

const char *get_asm_rsrc_section(void)
{
    switch (target_platform)
    {
    case PLATFORM_APPLE:   return ".data";
    case PLATFORM_WINDOWS: return ".section .rsrc";
    default:               return ".section .data";
    }
}

const char *get_asm_string_section(void)
{
    switch (target_platform)
    {
    case PLATFORM_APPLE: return ".cstring";
    default:             return ".section .rodata";
    }
}
