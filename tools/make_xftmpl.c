/*
 * Binary encode X templates from text format.
 *
 * Copyright 2011 Dylan Smith
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

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "windef.h"
#include "guiddef.h"

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))

#define TOKEN_NAME         1
#define TOKEN_STRING       2
#define TOKEN_INTEGER      3
#define TOKEN_GUID         5
#define TOKEN_INTEGER_LIST 6
#define TOKEN_FLOAT_LIST   7
#define TOKEN_OBRACE      10
#define TOKEN_CBRACE      11
#define TOKEN_OPAREN      12
#define TOKEN_CPAREN      13
#define TOKEN_OBRACKET    14
#define TOKEN_CBRACKET    15
#define TOKEN_OANGLE      16
#define TOKEN_CANGLE      17
#define TOKEN_DOT         18
#define TOKEN_COMMA       19
#define TOKEN_SEMICOLON   20
#define TOKEN_TEMPLATE    31
#define TOKEN_WORD        40
#define TOKEN_DWORD       41
#define TOKEN_FLOAT       42
#define TOKEN_DOUBLE      43
#define TOKEN_CHAR        44
#define TOKEN_UCHAR       45
#define TOKEN_SWORD       46
#define TOKEN_SDWORD      47
#define TOKEN_VOID        48
#define TOKEN_LPSTR       49
#define TOKEN_UNICODE     50
#define TOKEN_CSTRING     51
#define TOKEN_ARRAY       52

struct keyword
{
    const char *word;
    WORD token;
};

static const struct keyword reserved_words[] = {
    {"ARRAY", TOKEN_ARRAY},
    {"CHAR", TOKEN_CHAR},
    {"CSTRING", TOKEN_CSTRING},
    {"DOUBLE", TOKEN_DOUBLE},
    {"DWORD", TOKEN_DWORD},
    {"FLOAT", TOKEN_FLOAT},
    {"SDWORD", TOKEN_SDWORD},
    {"STRING", TOKEN_LPSTR},
    {"SWORD", TOKEN_SWORD},
    {"TEMPLATE", TOKEN_TEMPLATE},
    {"UCHAR", TOKEN_UCHAR},
    {"UNICODE", TOKEN_UNICODE},
    {"VOID", TOKEN_VOID},
    {"WORD", TOKEN_WORD}
};

extern int getopt(int argc, char *const *argv, const char *optstring);

static BOOL option_header;
static char *option_inc_var_name = NULL;
static char *option_inc_size_name = NULL;
static const char *option_outfile_name = "-";
static char *program_name;
static FILE *infile;
static int line_no;
static const char *infile_name;
static FILE *outfile;
static BYTE *output_data;
static UINT output_pos, output_size;

#ifndef __GNUC__
#define __attribute__(x)
#endif

static void fatal_error( const char *msg, ... ) __attribute__ ((__format__ (__printf__, 1, 2)));

static void fatal_error( const char *msg, ... )
{
    va_list valist;
    va_start( valist, msg );
    if (infile_name)
    {
        fprintf( stderr, "%s:%d:", infile_name, line_no );
        fprintf( stderr, " error: " );
    }
    else fprintf( stderr, "%s: error: ", program_name );
    vfprintf( stderr, msg, valist );
    va_end( valist );
    exit( 1 );
}


static inline BOOL read_byte( char *byte )
{
    int c = fgetc(infile);
    *byte = c;
    if (c == '\n') line_no++;
    return c != EOF;
}

static inline BOOL unread_byte( char last_byte )
{
    if (last_byte == '\n') line_no--;
    return ungetc(last_byte, infile) != EOF;
}

static inline BOOL read_bytes( void *data, DWORD size )
{
    return fread(data, size, 1, infile) > 0;
}

static BOOL write_c_hex_bytes(void)
{
    UINT i;
    for (i = 0; i < output_pos; i++)
    {
        if (i % 12 == 0)
            fprintf(outfile, "\n ");
        fprintf(outfile, " 0x%02x,", output_data[i]);
    }
    return TRUE;
}

static BOOL write_raw_bytes(void)
{
    return fwrite(output_data, output_pos, 1, outfile) > 0;
}

static inline BOOL write_bytes(const void *data, DWORD size)
{
    if (output_pos + size > output_size)
    {
        output_size = max( output_size * 2, size );
        output_data = realloc( output_data, output_size );
        if (!output_data) return FALSE;
    }
    memcpy( output_data + output_pos, data, size );
    output_pos += size;
    return TRUE;
}

static inline BOOL write_byte(BYTE value)
{
    return write_bytes( &value, sizeof(value) );
}

static inline BOOL write_word(WORD value)
{
    return write_byte( value ) &&
           write_byte( value >> 8 );
}

static inline BOOL write_dword(DWORD value)
{
    return write_word( value ) &&
           write_word( value >> 16 );
}

static inline BOOL write_float(float value)
{
    DWORD val;
    memcpy( &val, &value, sizeof(value) );
    return write_dword( val );
}

static inline BOOL write_guid(const GUID *guid)
{
    return write_dword( guid->Data1 ) &&
           write_word( guid->Data2 ) &&
           write_word( guid->Data3 ) &&
           write_bytes( guid->Data4, sizeof(guid->Data4) );
}

static int compare_names(const void *a, const void *b)
{
    return strcasecmp(*(const char **)a, *(const char **)b);
}

static BOOL parse_keyword( const char *name )
{
    const struct keyword *keyword;

    keyword = bsearch(&name, reserved_words, ARRAY_SIZE(reserved_words),
                      sizeof(reserved_words[0]), compare_names);
    if (!keyword)
        return FALSE;

    return write_word(keyword->token);
}

static BOOL parse_guid(void)
{
    char buf[39];
    GUID guid;
    DWORD tab[10];
    BOOL ret;
    static const char *guidfmt = "<%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X>";

    buf[0] = '<';
    if (!read_bytes(buf + 1, 37)) fatal_error( "truncated GUID\n" );
    buf[38] = 0;

    ret = sscanf(buf, guidfmt, &guid.Data1, tab, tab+1, tab+2, tab+3, tab+4, tab+5, tab+6, tab+7, tab+8, tab+9);
    if (ret != 11) fatal_error( "invalid GUID '%s'\n", buf );

    guid.Data2 = tab[0];
    guid.Data3 = tab[1];
    guid.Data4[0] = tab[2];
    guid.Data4[1] = tab[3];
    guid.Data4[2] = tab[4];
    guid.Data4[3] = tab[5];
    guid.Data4[4] = tab[6];
    guid.Data4[5] = tab[7];
    guid.Data4[6] = tab[8];
    guid.Data4[7] = tab[9];

    return write_word(TOKEN_GUID) &&
           write_guid(&guid);
}

static BOOL parse_name(void)
{
    char c;
    int len = 0;
    char name[512];

    while (read_byte(&c) && len < sizeof(name) &&
           (isalnum(c) || c == '_' || c == '-'))
    {
        if (len + 1 < sizeof(name))
            name[len++] = c;
    }
    unread_byte(c);
    name[len] = 0;

    if (parse_keyword(name)) {
        return TRUE;
    } else {
        return write_word(TOKEN_NAME) &&
               write_dword(len) &&
               write_bytes(name, len);
    }
}

static BOOL parse_number(void)
{
    int len = 0;
    char c;
    char buffer[512];
    BOOL dot = FALSE;
    BOOL ret;

    while (read_byte(&c) &&
           ((!len && c == '-') || (!dot && c == '.') || isdigit(c)))
    {
        if (len + 1 < sizeof(buffer))
            buffer[len++] = c;
        if (c == '.')
            dot = TRUE;
    }
    unread_byte(c);
    buffer[len] = 0;

    if (dot) {
        float value;
        ret = sscanf(buffer, "%f", &value);
        if (!ret) fatal_error( "invalid float token\n" );
        ret = write_word(TOKEN_FLOAT) &&
              write_float(value);
    } else {
        int value;
        ret = sscanf(buffer, "%d", &value);
        if (!ret) fatal_error( "invalid integer token\n" );
        ret = write_word(TOKEN_INTEGER) &&
              write_dword(value);
    }

    return ret;
}

static BOOL parse_token(void)
{
    char c;
    int len;
    char *tok, buffer[512];

    if (!read_byte(&c))
        return FALSE;

    switch (c)
    {
        case '\n':
        case '\r':
        case ' ':
        case '\t':
            return TRUE;

        case '{': return write_word(TOKEN_OBRACE);
        case '}': return write_word(TOKEN_CBRACE);
        case '[': return write_word(TOKEN_OBRACKET);
        case ']': return write_word(TOKEN_CBRACKET);
        case '(': return write_word(TOKEN_OPAREN);
        case ')': return write_word(TOKEN_CPAREN);
        case ',': return write_word(TOKEN_COMMA);
        case ';': return write_word(TOKEN_SEMICOLON);
        case '.': return write_word(TOKEN_DOT);

        case '/':
            if (!read_byte(&c) || c != '/')
                fatal_error( "invalid single '/' comment token\n" );
            while (read_byte(&c) && c != '\n');
            return c == '\n';

        case '#':
            len = 0;
            while (read_byte(&c) && c != '\n')
                if (len + 1 < sizeof(buffer)) buffer[len++] = c;
            if (c != '\n') fatal_error( "line too long\n" );
            buffer[len] = 0;
            tok = strtok( buffer, " \t" );
            if (!tok || strcmp( tok, "pragma" )) return TRUE;
            tok = strtok( NULL, " \t" );
            if (!tok || strcmp( tok, "xftmpl" )) return TRUE;
            tok = strtok( NULL, " \t" );
            if (!tok) return TRUE;
            if (!strcmp( tok, "name" ))
            {
                tok = strtok( NULL, " \t" );
                if (tok && !option_inc_var_name) option_inc_var_name = strdup( tok );
            }
            else if (!strcmp( tok, "size" ))
            {
                tok = strtok( NULL, " \t" );
                if (tok && !option_inc_size_name) option_inc_size_name = strdup( tok );
            }
            return TRUE;

        case '<':
            return parse_guid();

        case '"':
            len = 0;

            /* FIXME: Handle '\' (e.g. "valid\"string") */
            while (read_byte(&c) && c != '"') {
                if (len + 1 < sizeof(buffer))
                    buffer[len++] = c;
            }
            if (c != '"') fatal_error( "unterminated string\n" );
            return write_word(TOKEN_STRING) &&
                   write_dword(len) &&
                   write_bytes(buffer, len);

        default:
            unread_byte(c);
            if (isdigit(c) || c == '-')
                return parse_number();
            if (isalpha(c) || c == '_')
                return parse_name();
            fatal_error( "invalid character '%c' to start token\n", c );
    }

    return TRUE;
}

static const char *output_file;

static void cleanup_files(void)
{
    if (output_file) unlink(output_file);
}

static void exit_on_signal( int sig )
{
    exit(1);  /* this will call the atexit functions */
}

static void usage(void)
{
    fprintf(stderr, "Usage: %s [OPTIONS] INFILE\n"
                    "Options:\n"
                    "  -H        Output to a c header file instead of a binary file\n"
                    "  -i NAME   Output to a c header file, data in variable NAME\n"
                    "  -s NAME   In a c header file, define NAME to be the data size\n"
                    "  -o FILE   Write output to FILE\n",
                    program_name);
}

static char **parse_options(int argc, char **argv)
{
    int optc;

    while ((optc = getopt(argc, argv, "hHi:o:s:")) != -1)
    {
        switch (optc)
        {
            case 'h':
                usage();
                exit(0);
            case 'H':
                option_header = TRUE;
                break;
            case 'i':
                option_header = TRUE;
                option_inc_var_name = strdup(optarg);
                break;
            case 'o':
                option_outfile_name = strdup(optarg);
                break;
            case 's':
                option_inc_size_name = strdup(optarg);
                break;
        }
    }
    return &argv[optind];
}

int main(int argc, char **argv)
{
    char header[16];
    char **args;
    char *header_name = NULL;

    program_name = argv[0];

    args = parse_options(argc, argv);
    infile_name = *args++;
    if (!infile_name || *args)
    {
        usage();
        return 1;
    }

    infile = stdin;
    outfile = NULL;

    if (!strcmp(infile_name, "-")) {
        infile_name = "stdin";
    } else if (!(infile = fopen(infile_name, "rb"))) {
        perror(infile_name);
        goto error;
    }

    if (!read_bytes(header, sizeof(header))) {
        fprintf(stderr, "%s: Failed to read file header\n", program_name);
        goto error;
    }
    if (strncmp(header, "xof ", 4))
    {
        fprintf(stderr, "%s: Invalid magic value '%.4s'\n", program_name, header);
        goto error;
    }
    if (strncmp(header + 4, "0302", 4) && strncmp(header + 4, "0303", 4))
    {
        fprintf(stderr, "%s: Unsupported version '%.4s'\n", program_name, header + 4);
        goto error;
    }
    if (strncmp(header + 8, "txt ", 4))
    {
        fprintf(stderr, "%s: Only support conversion from text encoded X files.",
                program_name);
        goto error;
    }
    if (strncmp(header + 12, "0032", 4) && strncmp(header + 12, "0064", 4))
    {
        fprintf(stderr, "%s: Only 32-bit or 64-bit float format supported, not '%.4s'.\n",
                program_name, header + 12);
        goto error;
    }

    if (!strcmp(option_outfile_name, "-")) {
        option_outfile_name = "stdout";
        outfile = stdout;
    } else {
        output_file = option_outfile_name;
        atexit(cleanup_files);
        signal(SIGTERM, exit_on_signal);
        signal(SIGINT, exit_on_signal);
#ifdef SIGHUP
        signal(SIGHUP, exit_on_signal);
#endif
        if (!(outfile = fopen(output_file, "wb"))) {
            perror(option_outfile_name);
            goto error;
        }
    }

    if (!write_bytes("xof 0302bin 0064", 16))
        goto error;

    line_no = 1;
    while (parse_token());

    if (ferror(outfile) || ferror(infile))
        goto error;

    if (option_header)
    {
        char *str_ptr;

        if (!option_inc_var_name)
            fatal_error( "variable name must be specified with -i or #pragma name\n" );

        header_name = strrchr(option_outfile_name, '/');
        if (header_name)
            header_name = strdup(header_name + 1);
        else
            header_name = strdup(option_outfile_name);
        if (!header_name) {
            fprintf(stderr, "Out of memory\n");
            goto error;
        }

        str_ptr = header_name;
        while (*str_ptr) {
            if (*str_ptr == '.')
                *str_ptr = '_';
            else
                *str_ptr = toupper(*str_ptr);
            str_ptr++;
        }

        fprintf(outfile,
            "/* File generated automatically from %s; do not edit */\n"
            "\n"
            "#ifndef __WINE_%s\n"
            "#define __WINE_%s\n"
            "\n"
            "unsigned char %s[] = {",
            infile_name, header_name, header_name, option_inc_var_name);
        write_c_hex_bytes();
        fprintf(outfile, "\n};\n\n");
        if (option_inc_size_name)
            fprintf(outfile, "#define %s %u\n\n", option_inc_size_name, output_pos);
        fprintf(outfile, "#endif /* __WINE_%s */\n", header_name);
        if (ferror(outfile))
            goto error;
    }
    else write_raw_bytes();

    fclose(infile);
    fclose(outfile);
    output_file = NULL;

    return 0;
error:
    if (infile) {
        if (ferror(infile))
            perror(infile_name);
        fclose(infile);
    }
    if (outfile) {
        if (ferror(outfile))
            perror(option_outfile_name);
        fclose(outfile);
    }
    return 1;
}
