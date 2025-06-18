/*
 * INF file parsing
 *
 * Copyright 2002 Alexandre Julliard for CodeWeavers
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

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "winternl.h"
#include "winerror.h"
#include "setupapi.h"
#include "setupapi_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

#define CONTROL_Z  '\x1a'
#define MAX_SECTION_NAME_LEN  255
#define MAX_FIELD_LEN         511  /* larger fields get silently truncated */
/* actual string limit is MAX_INF_STRING_LENGTH+1 (plus terminating null) under Windows */
#define MAX_STRING_LEN        (MAX_INF_STRING_LENGTH+1)

/* inf file structure definitions */

struct field
{
    const WCHAR *text;         /* field text */
};

struct line
{
    int first_field;           /* index of first field in field array */
    int nb_fields;             /* number of fields in line */
    int key_field;             /* index of field for key or -1 if no key */
};

struct section
{
    const WCHAR *name;         /* section name */
    unsigned int nb_lines;     /* number of used lines */
    unsigned int alloc_lines;  /* total number of allocated lines in array below */
    struct line  lines[16];    /* lines information (grown dynamically, 16 is initial size) */
};

struct inf_file
{
    struct inf_file *next;            /* next appended file */
    WCHAR           *strings;         /* buffer for string data (section names and field values) */
    WCHAR           *string_pos;      /* position of next available string in buffer */
    unsigned int     nb_sections;     /* number of used sections */
    unsigned int     alloc_sections;  /* total number of allocated section pointers */
    struct section **sections;        /* section pointers array */
    unsigned int     nb_fields;
    unsigned int     alloc_fields;
    struct field    *fields;
    int              strings_section; /* index of [Strings] section or -1 if none */
    WCHAR           *filename;        /* filename of the INF */
};

/* parser definitions */

enum parser_state
{
    LINE_START,      /* at beginning of a line */
    SECTION_NAME,    /* parsing a section name */
    KEY_NAME,        /* parsing a key name */
    VALUE_NAME,      /* parsing a value name */
    EOL_BACKSLASH,   /* backslash at end of line */
    QUOTES,          /* inside quotes */
    LEADING_SPACES,  /* leading spaces */
    TRAILING_SPACES, /* trailing spaces */
    COMMENT,         /* inside a comment */
    NB_PARSER_STATES
};

struct parser
{
    const WCHAR      *start;        /* start position of item being parsed */
    const WCHAR      *end;          /* end of buffer */
    struct inf_file  *file;         /* file being built */
    enum parser_state state;        /* current parser state */
    enum parser_state stack[4];     /* state stack */
    int               stack_pos;    /* current pos in stack */

    int               cur_section;  /* index of section being parsed*/
    struct line      *line;         /* current line */
    unsigned int      line_pos;     /* current line position in file */
    unsigned int      broken_line;  /* first line containing invalid data (if any) */
    unsigned int      error;        /* error code */
    unsigned int      token_len;    /* current token len */
    WCHAR token[MAX_FIELD_LEN+1];   /* current token */
};

typedef const WCHAR * (*parser_state_func)( struct parser *parser, const WCHAR *pos );

/* parser state machine functions */
static const WCHAR *line_start_state( struct parser *parser, const WCHAR *pos );
static const WCHAR *section_name_state( struct parser *parser, const WCHAR *pos );
static const WCHAR *key_name_state( struct parser *parser, const WCHAR *pos );
static const WCHAR *value_name_state( struct parser *parser, const WCHAR *pos );
static const WCHAR *eol_backslash_state( struct parser *parser, const WCHAR *pos );
static const WCHAR *quotes_state( struct parser *parser, const WCHAR *pos );
static const WCHAR *leading_spaces_state( struct parser *parser, const WCHAR *pos );
static const WCHAR *trailing_spaces_state( struct parser *parser, const WCHAR *pos );
static const WCHAR *comment_state( struct parser *parser, const WCHAR *pos );

static const parser_state_func parser_funcs[NB_PARSER_STATES] =
{
    line_start_state,      /* LINE_START */
    section_name_state,    /* SECTION_NAME */
    key_name_state,        /* KEY_NAME */
    value_name_state,      /* VALUE_NAME */
    eol_backslash_state,   /* EOL_BACKSLASH */
    quotes_state,          /* QUOTES */
    leading_spaces_state,  /* LEADING_SPACES */
    trailing_spaces_state, /* TRAILING_SPACES */
    comment_state          /* COMMENT */
};


/* extend an array, allocating more memory if necessary */
static void *grow_array( void *array, unsigned int *count, size_t elem )
{
    void *new_array;
    unsigned int new_count = *count + *count / 2;
    if (new_count < 32) new_count = 32;

    new_array = _recalloc( array, new_count, elem );

    if (new_array)
        *count = new_count;
    else
        free( array );
    return new_array;
}


/* get the directory of the inf file (as counted string, not null-terminated) */
static const WCHAR *get_inf_dir( const struct inf_file *file, unsigned int *len )
{
    const WCHAR *p = wcsrchr( file->filename, '\\' );
    *len = p ? (p + 1 - file->filename) : 0;
    return file->filename;
}


/* find a section by name */
static int find_section( const struct inf_file *file, const WCHAR *name )
{
    unsigned int i;

    for (i = 0; i < file->nb_sections; i++)
        if (!wcsicmp( name, file->sections[i]->name )) return i;
    return -1;
}


/* find a line by name */
static struct line *find_line( struct inf_file *file, int section_index, const WCHAR *name )
{
    struct section *section;
    struct line *line;
    unsigned int i;

    if (section_index < 0 || section_index >= file->nb_sections) return NULL;
    section = file->sections[section_index];
    for (i = 0, line = section->lines; i < section->nb_lines; i++, line++)
    {
        if (line->key_field == -1) continue;
        if (!wcsicmp( name, file->fields[line->key_field].text )) return line;
    }
    return NULL;
}


/* add a section to the file and return the section index */
static int add_section( struct inf_file *file, const WCHAR *name )
{
    struct section *section;

    if (file->nb_sections >= file->alloc_sections)
    {
        if (!(file->sections = grow_array( file->sections, &file->alloc_sections,
                                           sizeof(file->sections[0]) ))) return -1;
    }
    if (!(section = malloc( sizeof(*section) ))) return -1;
    section->name        = name;
    section->nb_lines    = 0;
    section->alloc_lines = ARRAY_SIZE( section->lines );
    file->sections[file->nb_sections] = section;
    return file->nb_sections++;
}


/* add a line to a given section */
static struct line *add_line( struct inf_file *file, int section_index )
{
    struct section *section;
    struct line *line;

    assert( section_index >= 0 && section_index < file->nb_sections );

    section = file->sections[section_index];
    if (section->nb_lines == section->alloc_lines)  /* need to grow the section */
    {
        int size = sizeof(*section) - sizeof(section->lines) + 2*section->alloc_lines*sizeof(*line);
        if (!(section = realloc( section, size ))) return NULL;
        section->alloc_lines *= 2;
        file->sections[section_index] = section;
    }
    line = &section->lines[section->nb_lines++];
    line->first_field = file->nb_fields;
    line->nb_fields   = 0;
    line->key_field   = -1;
    return line;
}


/* retrieve a given line from section/line index */
static inline struct line *get_line( struct inf_file *file, unsigned int section_index,
                                     unsigned int line_index )
{
    struct section *section;

    if (section_index >= file->nb_sections) return NULL;
    section = file->sections[section_index];
    if (line_index >= section->nb_lines) return NULL;
    return &section->lines[line_index];
}


/* retrieve a given field from section/line/field index */
static struct field *get_field( struct inf_file *file, int section_index, int line_index,
                                int field_index )
{
    struct line *line = get_line( file, section_index, line_index );

    if (!line) return NULL;
    if (!field_index)  /* get the key */
    {
        if (line->key_field == -1) return NULL;
        return &file->fields[line->key_field];
    }
    field_index--;
    if (field_index >= line->nb_fields) return NULL;
    return &file->fields[line->first_field + field_index];
}


/* allocate a new field, growing the array if necessary */
static struct field *add_field( struct inf_file *file, const WCHAR *text )
{
    struct field *field;

    if (file->nb_fields >= file->alloc_fields)
    {
        if (!(file->fields = grow_array( file->fields, &file->alloc_fields,
                                         sizeof(file->fields[0]) ))) return NULL;
    }
    field = &file->fields[file->nb_fields++];
    field->text = text;
    return field;
}


/* retrieve the string substitution for a directory id */
static const WCHAR *get_dirid_subst( const struct inf_file *file, int dirid, unsigned int *len )
{
    const WCHAR *ret;

    if (dirid == DIRID_SRCPATH) return get_inf_dir( file, len );
    ret = DIRID_get_string( dirid );
    if (ret) *len = lstrlenW(ret);
    return ret;
}


/* retrieve the string substitution for a given string, or NULL if not found */
/* if found, len is set to the substitution length */
static const WCHAR *get_string_subst( const struct inf_file *file, const WCHAR *str, unsigned int *len,
                                      BOOL no_trailing_slash )
{
    static const WCHAR percent = '%';

    struct section *strings_section;
    struct line *line;
    struct field *field;
    unsigned int i;
    int dirid;
    WCHAR *dirid_str, *end;
    const WCHAR *ret = NULL;

    if (!*len)  /* empty string (%%) is replaced by single percent */
    {
        *len = 1;
        return &percent;
    }
    if (file->strings_section == -1) goto not_found;
    strings_section = file->sections[file->strings_section];
    for (i = 0, line = strings_section->lines; i < strings_section->nb_lines; i++, line++)
    {
        if (line->key_field == -1) continue;
        if (wcsnicmp( str, file->fields[line->key_field].text, *len )) continue;
        if (!file->fields[line->key_field].text[*len]) break;
    }
    if (i == strings_section->nb_lines || !line->nb_fields) goto not_found;
    field = &file->fields[line->first_field];
    *len = lstrlenW( field->text );
    return field->text;

 not_found:  /* check for integer id */
    if ((dirid_str = malloc( (*len + 1) * sizeof(WCHAR) )))
    {
        memcpy( dirid_str, str, *len * sizeof(WCHAR) );
        dirid_str[*len] = 0;
        dirid = wcstol( dirid_str, &end, 10 );
        if (!*end) ret = get_dirid_subst( file, dirid, len );
        if (no_trailing_slash && ret && *len && ret[*len - 1] == '\\') *len -= 1;
        free( dirid_str );
        return ret;
    }
    return NULL;
}


/* do string substitutions on the specified text */
/* the buffer is assumed to be large enough */
/* returns necessary length not including terminating null */
static unsigned int PARSER_string_substW( const struct inf_file *file, const WCHAR *text,
                                          WCHAR *buffer, unsigned int size )
{
    const WCHAR *start, *subst, *p;
    unsigned int len, total = 0;
    BOOL inside = FALSE;

    if (!buffer) size = MAX_STRING_LEN + 1;
    for (p = start = text; *p; p++)
    {
        if (*p != '%') continue;
        inside = !inside;
        if (inside)  /* start of a %xx% string */
        {
            len = p - start;
            if (len > size - 1) len = size - 1;
            if (buffer) memcpy( buffer + total, start, len * sizeof(WCHAR) );
            total += len;
            size -= len;
            start = p;
        }
        else /* end of the %xx% string, find substitution */
        {
            len = p - start - 1;
            subst = get_string_subst( file, start + 1, &len, p[1] == '\\' );
            if (!subst)
            {
                subst = start;
                len = p - start + 1;
            }
            if (len > size - 1) len = size - 1;
            if (buffer) memcpy( buffer + total, subst, len * sizeof(WCHAR) );
            total += len;
            size -= len;
            start = p + 1;
        }
    }

    if (start != p) /* unfinished string, copy it */
    {
        len = p - start;
        if (len > size - 1) len = size - 1;
        if (buffer) memcpy( buffer + total, start, len * sizeof(WCHAR) );
        total += len;
    }
    if (buffer && size) buffer[total] = 0;
    return total;
}


/* do string substitutions on the specified text */
/* the buffer is assumed to be large enough */
/* returns necessary length not including terminating null */
static unsigned int PARSER_string_substA( const struct inf_file *file, const WCHAR *text,
                                          char *buffer, unsigned int size )
{
    WCHAR buffW[MAX_STRING_LEN+1];
    DWORD ret;

    unsigned int len = PARSER_string_substW( file, text, buffW, ARRAY_SIZE( buffW ));
    if (!buffer) RtlUnicodeToMultiByteSize( &ret, buffW, len * sizeof(WCHAR) );
    else
    {
        RtlUnicodeToMultiByteN( buffer, size-1, &ret, buffW, len * sizeof(WCHAR) );
        buffer[ret] = 0;
    }
    return ret;
}


/* push some string data into the strings buffer */
static WCHAR *push_string( struct inf_file *file, const WCHAR *string )
{
    WCHAR *ret = file->string_pos;
    lstrcpyW( ret, string );
    file->string_pos += lstrlenW( ret ) + 1;
    return ret;
}


/* push the current state on the parser stack */
static inline void push_state( struct parser *parser, enum parser_state state )
{
    assert( parser->stack_pos < ARRAY_SIZE( parser->stack ));
    parser->stack[parser->stack_pos++] = state;
}


/* pop the current state */
static inline void pop_state( struct parser *parser )
{
    assert( parser->stack_pos );
    parser->state = parser->stack[--parser->stack_pos];
}


/* set the parser state and return the previous one */
static inline enum parser_state set_state( struct parser *parser, enum parser_state state )
{
    enum parser_state ret = parser->state;
    parser->state = state;
    return ret;
}


/* check if the pointer points to an end of file */
static inline BOOL is_eof( const struct parser *parser, const WCHAR *ptr )
{
    return (ptr >= parser->end || *ptr == CONTROL_Z);
}


/* check if the pointer points to an end of line */
static inline BOOL is_eol( const struct parser *parser, const WCHAR *ptr )
{
    return (ptr >= parser->end || *ptr == CONTROL_Z || *ptr == '\n');
}


/* push data from current token start up to pos into the current token */
static int push_token( struct parser *parser, const WCHAR *pos )
{
    int len = pos - parser->start;
    const WCHAR *src = parser->start;
    WCHAR *dst = parser->token + parser->token_len;

    if (len > MAX_FIELD_LEN - parser->token_len) len = MAX_FIELD_LEN - parser->token_len;

    parser->token_len += len;
    for ( ; len > 0; len--, dst++, src++) *dst = *src ? *src : ' ';
    *dst = 0;
    parser->start = pos;
    return 0;
}


/* add a section with the current token as name */
static int add_section_from_token( struct parser *parser )
{
    int section_index;

    if (parser->token_len > MAX_SECTION_NAME_LEN)
    {
        parser->error = ERROR_SECTION_NAME_TOO_LONG;
        return -1;
    }
    if ((section_index = find_section( parser->file, parser->token )) == -1)
    {
        /* need to create a new one */
        const WCHAR *name = push_string( parser->file, parser->token );
        if ((section_index = add_section( parser->file, name )) == -1)
        {
            parser->error = ERROR_NOT_ENOUGH_MEMORY;
            return -1;
        }
    }
    parser->token_len = 0;
    parser->cur_section = section_index;
    return section_index;
}


/* add a field containing the current token to the current line */
static struct field *add_field_from_token( struct parser *parser, BOOL is_key )
{
    struct field *field;
    WCHAR *text;

    if (!parser->line)  /* need to start a new line */
    {
        if (parser->cur_section == -1)  /* got a line before the first section */
        {
            parser->error = ERROR_EXPECTED_SECTION_NAME;
            return NULL;
        }
        if (!(parser->line = add_line( parser->file, parser->cur_section ))) goto error;
    }
    else assert(!is_key);

    text = push_string( parser->file, parser->token );
    if ((field = add_field( parser->file, text )))
    {
        if (!is_key) parser->line->nb_fields++;
        else
        {
            /* replace first field by key field */
            parser->line->key_field = parser->line->first_field;
            parser->line->first_field++;
        }
        parser->token_len = 0;
        return field;
    }
 error:
    parser->error = ERROR_NOT_ENOUGH_MEMORY;
    return NULL;
}


/* close the current line and prepare for parsing a new one */
static void close_current_line( struct parser *parser )
{
    struct line *cur_line = parser->line;

    if (cur_line)
    {
        /* if line has a single field and no key, the field is the key too */
        if (cur_line->nb_fields == 1 && cur_line->key_field == -1)
            cur_line->key_field = cur_line->first_field;
    }
    parser->line = NULL;
}


/* handler for parser LINE_START state */
static const WCHAR *line_start_state( struct parser *parser, const WCHAR *pos )
{
    const WCHAR *p;

    for (p = pos; !is_eof( parser, p ); p++)
    {
        switch(*p)
        {
        case '\n':
            parser->line_pos++;
            close_current_line( parser );
            break;
        case ';':
            push_state( parser, LINE_START );
            set_state( parser, COMMENT );
            return p + 1;
        case '[':
            parser->start = p + 1;
            set_state( parser, SECTION_NAME );
            return p + 1;
        default:
            if (iswspace(*p)) break;
            if (parser->cur_section != -1)
            {
                parser->start = p;
                set_state( parser, KEY_NAME );
                return p;
            }
            if (!parser->broken_line)
                parser->broken_line = parser->line_pos;
            break;
        }
    }
    close_current_line( parser );
    return NULL;
}


/* handler for parser SECTION_NAME state */
static const WCHAR *section_name_state( struct parser *parser, const WCHAR *pos )
{
    const WCHAR *p;

    for (p = pos; !is_eol( parser, p ); p++)
    {
        if (*p == ']')
        {
            push_token( parser, p );
            if (add_section_from_token( parser ) == -1) return NULL;
            push_state( parser, LINE_START );
            set_state( parser, COMMENT );  /* ignore everything else on the line */
            return p + 1;
        }
    }
    parser->error = ERROR_BAD_SECTION_NAME_LINE; /* unfinished section name */
    return NULL;
}


/* handler for parser KEY_NAME state */
static const WCHAR *key_name_state( struct parser *parser, const WCHAR *pos )
{
    const WCHAR *p, *token_end = parser->start;

    for (p = pos; !is_eol( parser, p ); p++)
    {
        if (*p == ',') break;
        switch(*p)
        {

         case '=':
            push_token( parser, token_end );
            if (!add_field_from_token( parser, TRUE )) return NULL;
            parser->start = p + 1;
            push_state( parser, VALUE_NAME );
            set_state( parser, LEADING_SPACES );
            return p + 1;
        case ';':
            push_token( parser, token_end );
            if (!add_field_from_token( parser, FALSE )) return NULL;
            push_state( parser, LINE_START );
            set_state( parser, COMMENT );
            return p + 1;
        case '"':
            push_token( parser, p );
            parser->start = p + 1;
            push_state( parser, KEY_NAME );
            set_state( parser, QUOTES );
            return p + 1;
        case '\\':
            push_token( parser, token_end );
            parser->start = p;
            push_state( parser, KEY_NAME );
            set_state( parser, EOL_BACKSLASH );
            return p;
        default:
            if (!iswspace(*p)) token_end = p + 1;
            else
            {
                push_token( parser, p );
                push_state( parser, KEY_NAME );
                set_state( parser, TRAILING_SPACES );
                return p;
            }
            break;
        }
    }
    push_token( parser, token_end );
    set_state( parser, VALUE_NAME );
    return p;
}


/* handler for parser VALUE_NAME state */
static const WCHAR *value_name_state( struct parser *parser, const WCHAR *pos )
{
    const WCHAR *p, *token_end = parser->start;

    for (p = pos; !is_eol( parser, p ); p++)
    {
        switch(*p)
        {
        case ';':
            push_token( parser, token_end );
            if (!add_field_from_token( parser, FALSE )) return NULL;
            push_state( parser, LINE_START );
            set_state( parser, COMMENT );
            return p + 1;
        case ',':
            push_token( parser, token_end );
            if (!add_field_from_token( parser, FALSE )) return NULL;
            parser->start = p + 1;
            push_state( parser, VALUE_NAME );
            set_state( parser, LEADING_SPACES );
            return p + 1;
        case '"':
            push_token( parser, p );
            parser->start = p + 1;
            push_state( parser, VALUE_NAME );
            set_state( parser, QUOTES );
            return p + 1;
        case '\\':
            push_token( parser, token_end );
            parser->start = p;
            push_state( parser, VALUE_NAME );
            set_state( parser, EOL_BACKSLASH );
            return p;
        default:
            if (*p && !iswspace(*p)) token_end = p + 1;
            else
            {
                push_token( parser, p );
                push_state( parser, VALUE_NAME );
                set_state( parser, TRAILING_SPACES );
                return p;
            }
            break;
        }
    }
    push_token( parser, token_end );
    if (!add_field_from_token( parser, FALSE )) return NULL;
    set_state( parser, LINE_START );
    return p;
}


/* handler for parser EOL_BACKSLASH state */
static const WCHAR *eol_backslash_state( struct parser *parser, const WCHAR *pos )
{
    const WCHAR *p;

    for (p = pos; !is_eof( parser, p ); p++)
    {
        switch(*p)
        {
        case '\n':
            parser->line_pos++;
            parser->start = p + 1;
            set_state( parser, LEADING_SPACES );
            return p + 1;
        case '\\':
            continue;
        case ';':
            push_state( parser, EOL_BACKSLASH );
            set_state( parser, COMMENT );
            return p + 1;
        default:
            if (iswspace(*p)) continue;
            push_token( parser, p );
            pop_state( parser );
            return p;
        }
    }
    parser->start = p;
    pop_state( parser );
    return p;
}


/* handler for parser QUOTES state */
static const WCHAR *quotes_state( struct parser *parser, const WCHAR *pos )
{
    const WCHAR *p;

    for (p = pos; !is_eol( parser, p ); p++)
    {
        if (*p == '"')
        {
            if (p+1 < parser->end && p[1] == '"')  /* double quotes */
            {
                push_token( parser, p + 1 );
                parser->start = p + 2;
                p++;
            }
            else  /* end of quotes */
            {
                push_token( parser, p );
                parser->start = p + 1;
                pop_state( parser );
                return p + 1;
            }
        }
    }
    push_token( parser, p );
    pop_state( parser );
    return p;
}


/* handler for parser LEADING_SPACES state */
static const WCHAR *leading_spaces_state( struct parser *parser, const WCHAR *pos )
{
    const WCHAR *p;

    for (p = pos; !is_eol( parser, p ); p++)
    {
        if (*p == '\\')
        {
            parser->start = p;
            set_state( parser, EOL_BACKSLASH );
            return p;
        }
        if (!iswspace(*p)) break;
    }
    parser->start = p;
    pop_state( parser );
    return p;
}


/* handler for parser TRAILING_SPACES state */
static const WCHAR *trailing_spaces_state( struct parser *parser, const WCHAR *pos )
{
    const WCHAR *p;

    for (p = pos; !is_eol( parser, p ); p++)
    {
        if (*p == '\\')
        {
            set_state( parser, EOL_BACKSLASH );
            return p;
        }
        if (*p && !iswspace(*p)) break;
    }
    pop_state( parser );
    return p;
}


/* handler for parser COMMENT state */
static const WCHAR *comment_state( struct parser *parser, const WCHAR *pos )
{
    const WCHAR *p = pos;

    while (!is_eol( parser, p )) p++;
    pop_state( parser );
    return p;
}


static void free_inf_file( struct inf_file *file )
{
    unsigned int i;

    for (i = 0; i < file->nb_sections; i++) free( file->sections[i] );
    free( file->filename );
    free( file->sections );
    free( file->fields );
    HeapFree( GetProcessHeap(), 0, file->strings );
    free( file );
}


/* parse a complete buffer */
static DWORD parse_buffer( struct inf_file *file, const WCHAR *buffer, const WCHAR *end,
                           UINT *error_line )
{
    struct parser parser;
    const WCHAR *pos = buffer;

    parser.start       = buffer;
    parser.end         = end;
    parser.file        = file;
    parser.line        = NULL;
    parser.state       = LINE_START;
    parser.stack_pos   = 0;
    parser.cur_section = -1;
    parser.line_pos    = 1;
    parser.broken_line = 0;
    parser.error       = 0;
    parser.token_len   = 0;

    /* parser main loop */
    while (pos) pos = (parser_funcs[parser.state])( &parser, pos );

    /* trim excess buffer space */
    if (file->alloc_sections > file->nb_sections)
    {
        file->sections = realloc( file->sections, file->nb_sections * sizeof(file->sections[0]) );
        file->alloc_sections = file->nb_sections;
    }
    if (file->alloc_fields > file->nb_fields)
    {
        file->fields = realloc( file->fields, file->nb_fields * sizeof(file->fields[0]) );
        file->alloc_fields = file->nb_fields;
    }
    file->strings = HeapReAlloc( GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY, file->strings,
                                 (file->string_pos - file->strings) * sizeof(WCHAR) );

    if (parser.error)
    {
        if (error_line) *error_line = parser.line_pos;
        return parser.error;
    }

    /* find the [strings] section */
    file->strings_section = find_section( file, L"Strings" );

    if (file->strings_section == -1 && parser.broken_line)
    {
        if (error_line) *error_line = parser.broken_line;
        return ERROR_EXPECTED_SECTION_NAME;
    }

    return 0;
}


/* append a child INF file to its parent list, in a thread-safe manner */
static void append_inf_file( struct inf_file *parent, struct inf_file *child )
{
    struct inf_file **ppnext = &parent->next;
    child->next = NULL;

    for (;;)
    {
        struct inf_file *next = InterlockedCompareExchangePointer( (void **)ppnext, child, NULL );
        if (!next) return;
        ppnext = &next->next;
    }
}


/***********************************************************************
 *            parse_file
 *
 * parse an INF file.
 */
static struct inf_file *parse_file( HANDLE handle, const WCHAR *class, DWORD style, UINT *error_line )
{
    void *buffer;
    DWORD err = 0;
    struct inf_file *file;

    DWORD size = GetFileSize( handle, NULL );
    HANDLE mapping = CreateFileMappingW( handle, NULL, PAGE_READONLY, 0, size, NULL );
    if (!mapping) return NULL;
    buffer = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, size );
    NtClose( mapping );
    if (!buffer) return NULL;

    if (class) FIXME( "class %s not supported yet\n", debugstr_w(class) );

    if (!(file = calloc( 1, sizeof(*file) )))
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    /* we won't need more strings space than the size of the file,
     * so we can preallocate it here
     */
    if (!(file->strings = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) )))
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }
    file->string_pos = file->strings;
    file->strings_section = -1;

    if (!RtlIsTextUnicode( buffer, size, NULL ))
    {
        static const BYTE utf8_bom[3] = { 0xef, 0xbb, 0xbf };
        WCHAR *new_buff;
        UINT codepage = CP_ACP;
        UINT offset = 0;

        if (size > sizeof(utf8_bom) && !memcmp( buffer, utf8_bom, sizeof(utf8_bom) ))
        {
            codepage = CP_UTF8;
            offset = sizeof(utf8_bom);
        }

        if ((new_buff = malloc( size * sizeof(WCHAR) )))
        {
            DWORD len = MultiByteToWideChar( codepage, 0, (char *)buffer + offset,
                                             size - offset, new_buff, size );
            err = parse_buffer( file, new_buff, new_buff + len, error_line );
            free( new_buff );
        }
    }
    else
    {
        WCHAR *new_buff = buffer;
        /* UCS-16 files should start with the Unicode BOM; we should skip it */
        if (*new_buff == 0xfeff)
            new_buff++;
        err = parse_buffer( file, new_buff, (WCHAR *)((char *)buffer + size), error_line );
    }

    if (!err)  /* now check signature */
    {
        int version_index = find_section( file, L"Version" );
        if (version_index != -1)
        {
            struct line *line = find_line( file, version_index, L"Signature" );
            if (line && line->nb_fields > 0)
            {
                struct field *field = file->fields + line->first_field;
                if (!wcsicmp( field->text, L"$Chicago$" )) goto done;
                if (!wcsicmp( field->text, L"$Windows NT$" )) goto done;
                if (!wcsicmp( field->text, L"$Windows 95$" )) goto done;
            }
        }
        if (error_line) *error_line = 0;
        if (style & INF_STYLE_WIN4) err = ERROR_WRONG_INF_STYLE;
    }

 done:
    UnmapViewOfFile( buffer );
    if (err)
    {
        if (file) free_inf_file( file );
        SetLastError( err );
        file = NULL;
    }
    return file;
}


/***********************************************************************
 *            PARSER_get_inf_filename
 *
 * Retrieve the filename of an inf file.
 */
const WCHAR *PARSER_get_inf_filename( HINF hinf )
{
    struct inf_file *file = hinf;
    return file->filename;
}

/***********************************************************************
 *            PARSER_get_dest_dir
 *
 * retrieve a destination dir of the form "dirid,relative_path" in the given entry.
 * returned buffer must be freed by caller.
 */
WCHAR *PARSER_get_dest_dir( INFCONTEXT *context )
{
    const WCHAR *dir;
    WCHAR *ptr, *ret;
    INT dirid;
    unsigned int len1;
    DWORD len2;

    if (!SetupGetIntField( context, 1, &dirid )) return NULL;
    if (!(dir = get_dirid_subst( context->Inf, dirid, &len1 ))) return NULL;
    if (!SetupGetStringFieldW( context, 2, NULL, 0, &len2 )) len2 = 0;
    if (!(ret = malloc( (len1 + len2 + 1) * sizeof(WCHAR) ))) return NULL;
    memcpy( ret, dir, len1 * sizeof(WCHAR) );
    ptr = ret + len1;
    if (len2 && ptr > ret && ptr[-1] != '\\') *ptr++ = '\\';
    if (!SetupGetStringFieldW( context, 2, ptr, len2, NULL )) *ptr = 0;
    return ret;
}


/***********************************************************************
 *            SetupOpenInfFileA   (SETUPAPI.@)
 */
HINF WINAPI SetupOpenInfFileA( PCSTR name, PCSTR class, DWORD style, UINT *error )
{
    UNICODE_STRING nameW, classW;
    HINF ret = INVALID_HANDLE_VALUE;

    classW.Buffer = NULL;
    if (class && !RtlCreateUnicodeStringFromAsciiz( &classW, class ))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return ret;
    }
    if (RtlCreateUnicodeStringFromAsciiz( &nameW, name ))
    {
        ret = SetupOpenInfFileW( nameW.Buffer, classW.Buffer, style, error );
        RtlFreeUnicodeString( &nameW );
    }
    RtlFreeUnicodeString( &classW );
    return ret;
}


/***********************************************************************
 *            SetupOpenInfFileW   (SETUPAPI.@)
 */
HINF WINAPI SetupOpenInfFileW( PCWSTR name, PCWSTR class, DWORD style, UINT *error )
{
    struct inf_file *file = NULL;
    HANDLE handle;
    WCHAR *path, *p;
    UINT len;

    if (wcschr( name, '\\' ) || wcschr( name, '/' ))
    {
        if (!(len = GetFullPathNameW( name, 0, NULL, NULL ))) return INVALID_HANDLE_VALUE;
        if (!(path = malloc( len * sizeof(WCHAR) )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return INVALID_HANDLE_VALUE;
        }
        GetFullPathNameW( name, len, path, NULL );
        handle = CreateFileW( path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    }
    else  /* try Windows directory */
    {
        len = GetWindowsDirectoryW( NULL, 0 ) + lstrlenW(name) + 12;
        if (!(path = malloc( len * sizeof(WCHAR) )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return INVALID_HANDLE_VALUE;
        }
        GetWindowsDirectoryW( path, len );
        p = path + lstrlenW(path);
        lstrcpyW( p, L"\\inf\\" );
        lstrcatW( p, name );
        handle = CreateFileW( path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
        if (handle == INVALID_HANDLE_VALUE)
        {
            lstrcpyW( p, L"\\system32\\" );
            lstrcatW( p, name );
            handle = CreateFileW( path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
        }
    }

    if (handle != INVALID_HANDLE_VALUE)
    {
        file = parse_file( handle, class, style, error );
        CloseHandle( handle );
    }
    if (!file)
    {
        free( path );
        return INVALID_HANDLE_VALUE;
    }
    TRACE( "%s -> %p\n", debugstr_w(path), file );
    file->filename = path;
    SetLastError( 0 );
    return file;
}


/***********************************************************************
 *            SetupOpenAppendInfFileA    (SETUPAPI.@)
 */
BOOL WINAPI SetupOpenAppendInfFileA( PCSTR name, HINF parent_hinf, UINT *error )
{
    HINF child_hinf;

    if (!name) return SetupOpenAppendInfFileW( NULL, parent_hinf, error );
    child_hinf = SetupOpenInfFileA( name, NULL, INF_STYLE_WIN4, error );
    if (child_hinf == INVALID_HANDLE_VALUE) return FALSE;
    append_inf_file( parent_hinf, child_hinf );
    TRACE( "%p: appended %s (%p)\n", parent_hinf, debugstr_a(name), child_hinf );
    return TRUE;
}


/***********************************************************************
 *            SetupOpenAppendInfFileW    (SETUPAPI.@)
 */
BOOL WINAPI SetupOpenAppendInfFileW( PCWSTR name, HINF parent_hinf, UINT *error )
{
    HINF child_hinf;

    if (!name)
    {
        INFCONTEXT context;
        WCHAR filename[MAX_PATH];
        int idx = 1;

        if (!SetupFindFirstLineW( parent_hinf, L"Version", L"LayoutFile", &context )) return FALSE;
        while (SetupGetStringFieldW( &context, idx++, filename, ARRAY_SIZE( filename ), NULL ))
        {
            child_hinf = SetupOpenInfFileW( filename, NULL, INF_STYLE_WIN4, error );
            if (child_hinf == INVALID_HANDLE_VALUE) return FALSE;
            append_inf_file( parent_hinf, child_hinf );
            TRACE( "%p: appended %s (%p)\n", parent_hinf, debugstr_w(filename), child_hinf );
        }
        return TRUE;
    }
    child_hinf = SetupOpenInfFileW( name, NULL, INF_STYLE_WIN4, error );
    if (child_hinf == INVALID_HANDLE_VALUE) return FALSE;
    append_inf_file( parent_hinf, child_hinf );
    TRACE( "%p: appended %s (%p)\n", parent_hinf, debugstr_w(name), child_hinf );
    return TRUE;
}


/***********************************************************************
 *            SetupOpenMasterInf   (SETUPAPI.@)
 */
HINF WINAPI SetupOpenMasterInf( VOID )
{
    WCHAR Buffer[MAX_PATH];

    GetWindowsDirectoryW( Buffer, MAX_PATH );
    lstrcatW( Buffer, L"\\inf\\layout.inf" );
    return SetupOpenInfFileW( Buffer, NULL, INF_STYLE_WIN4, NULL);
}



/***********************************************************************
 *            SetupCloseInfFile   (SETUPAPI.@)
 */
void WINAPI SetupCloseInfFile( HINF hinf )
{
    struct inf_file *file = hinf;

    if (!hinf || (hinf == INVALID_HANDLE_VALUE)) return;

    free_inf_file( file );
}


/***********************************************************************
 *            SetupEnumInfSectionsA   (SETUPAPI.@)
 */
BOOL WINAPI SetupEnumInfSectionsA( HINF hinf, UINT index, PSTR buffer, DWORD size, DWORD *need )
{
    struct inf_file *file = hinf;

    for (file = hinf; file; file = file->next)
    {
        if (index < file->nb_sections)
        {
            DWORD len = WideCharToMultiByte( CP_ACP, 0, file->sections[index]->name, -1,
                                             NULL, 0, NULL, NULL );
            if (need) *need = len;
            if (!buffer)
            {
                if (!size) return TRUE;
                SetLastError( ERROR_INVALID_USER_BUFFER );
                return FALSE;
            }
            if (len > size)
            {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                return FALSE;
            }
            WideCharToMultiByte( CP_ACP, 0, file->sections[index]->name, -1, buffer, size, NULL, NULL );
            return TRUE;
        }
        index -= file->nb_sections;
    }
    SetLastError( ERROR_NO_MORE_ITEMS );
    return FALSE;
}


/***********************************************************************
 *            SetupEnumInfSectionsW   (SETUPAPI.@)
 */
BOOL WINAPI SetupEnumInfSectionsW( HINF hinf, UINT index, PWSTR buffer, DWORD size, DWORD *need )
{
    struct inf_file *file = hinf;

    for (file = hinf; file; file = file->next)
    {
        if (index < file->nb_sections)
        {
            DWORD len = lstrlenW( file->sections[index]->name ) + 1;
            if (need) *need = len;
            if (!buffer)
            {
                if (!size) return TRUE;
                SetLastError( ERROR_INVALID_USER_BUFFER );
                return FALSE;
            }
            if (len > size)
            {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                return FALSE;
            }
            memcpy( buffer, file->sections[index]->name, len * sizeof(WCHAR) );
            return TRUE;
        }
        index -= file->nb_sections;
    }
    SetLastError( ERROR_NO_MORE_ITEMS );
    return FALSE;
}


/***********************************************************************
 *            SetupGetLineCountA   (SETUPAPI.@)
 */
LONG WINAPI SetupGetLineCountA( HINF hinf, PCSTR name )
{
    UNICODE_STRING sectionW;
    LONG ret = -1;

    if (!RtlCreateUnicodeStringFromAsciiz( &sectionW, name ))
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    else
    {
        ret = SetupGetLineCountW( hinf, sectionW.Buffer );
        RtlFreeUnicodeString( &sectionW );
    }
    return ret;
}


/***********************************************************************
 *            SetupGetLineCountW   (SETUPAPI.@)
 */
LONG WINAPI SetupGetLineCountW( HINF hinf, PCWSTR section )
{
    struct inf_file *file = hinf;
    int section_index;
    LONG ret = -1;

    for (file = hinf; file; file = file->next)
    {
        if ((section_index = find_section( file, section )) == -1) continue;
        if (ret == -1) ret = 0;
        ret += file->sections[section_index]->nb_lines;
    }
    TRACE( "(%p,%s) returning %ld\n", hinf, debugstr_w(section), ret );
    SetLastError( (ret == -1) ? ERROR_SECTION_NOT_FOUND : 0 );
    return ret;
}


/***********************************************************************
 *            SetupGetLineByIndexA   (SETUPAPI.@)
 */
BOOL WINAPI SetupGetLineByIndexA( HINF hinf, PCSTR section, DWORD index, INFCONTEXT *context )
{
    UNICODE_STRING sectionW;
    BOOL ret = FALSE;

    if (!RtlCreateUnicodeStringFromAsciiz( &sectionW, section ))
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    else
    {
        ret = SetupGetLineByIndexW( hinf, sectionW.Buffer, index, context );
        RtlFreeUnicodeString( &sectionW );
    }
    return ret;
}


/***********************************************************************
 *            SetupGetLineByIndexW   (SETUPAPI.@)
 */
BOOL WINAPI SetupGetLineByIndexW( HINF hinf, PCWSTR section, DWORD index, INFCONTEXT *context )
{
    struct inf_file *file = hinf;
    int section_index;

    for (file = hinf; file; file = file->next)
    {
        if ((section_index = find_section( file, section )) == -1) continue;
        if (index < file->sections[section_index]->nb_lines)
        {
            context->Inf        = hinf;
            context->CurrentInf = file;
            context->Section    = section_index;
            context->Line       = index;
            SetLastError( 0 );
            TRACE( "(%p,%s): returning %d/%ld\n",
                   hinf, debugstr_w(section), section_index, index );
            return TRUE;
        }
        index -= file->sections[section_index]->nb_lines;
    }
    TRACE( "(%p,%s) not found\n", hinf, debugstr_w(section) );
    SetLastError( ERROR_LINE_NOT_FOUND );
    return FALSE;
}


/***********************************************************************
 *            SetupFindFirstLineA   (SETUPAPI.@)
 */
BOOL WINAPI SetupFindFirstLineA( HINF hinf, PCSTR section, PCSTR key, INFCONTEXT *context )
{
    UNICODE_STRING sectionW, keyW;
    BOOL ret = FALSE;

    if (!RtlCreateUnicodeStringFromAsciiz( &sectionW, section ))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    if (!key) ret = SetupFindFirstLineW( hinf, sectionW.Buffer, NULL, context );
    else
    {
        if (RtlCreateUnicodeStringFromAsciiz( &keyW, key ))
        {
            ret = SetupFindFirstLineW( hinf, sectionW.Buffer, keyW.Buffer, context );
            RtlFreeUnicodeString( &keyW );
        }
        else SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    }
    RtlFreeUnicodeString( &sectionW );
    return ret;
}


/***********************************************************************
 *            SetupFindFirstLineW   (SETUPAPI.@)
 */
BOOL WINAPI SetupFindFirstLineW( HINF hinf, PCWSTR section, PCWSTR key, INFCONTEXT *context )
{
    struct inf_file *file;
    int section_index;

    for (file = hinf; file; file = file->next)
    {
        if ((section_index = find_section( file, section )) == -1) continue;
        if (key)
        {
            INFCONTEXT ctx;
            ctx.Inf        = hinf;
            ctx.CurrentInf = file;
            ctx.Section    = section_index;
            ctx.Line       = -1;
            return SetupFindNextMatchLineW( &ctx, key, context );
        }
        if (file->sections[section_index]->nb_lines)
        {
            context->Inf        = hinf;
            context->CurrentInf = file;
            context->Section    = section_index;
            context->Line       = 0;
            SetLastError( 0 );
            TRACE( "(%p,%s,%s): returning %d/0\n",
                   hinf, debugstr_w(section), debugstr_w(key), section_index );
            return TRUE;
        }
    }
    TRACE( "(%p,%s,%s): not found\n", hinf, debugstr_w(section), debugstr_w(key) );
    SetLastError( ERROR_LINE_NOT_FOUND );
    return FALSE;
}


/***********************************************************************
 *            SetupFindNextLine   (SETUPAPI.@)
 */
BOOL WINAPI SetupFindNextLine( PINFCONTEXT context_in, PINFCONTEXT context_out )
{
    struct inf_file *file = context_in->CurrentInf;
    struct section *section;

    if (context_in->Section >= file->nb_sections) goto error;

    section = file->sections[context_in->Section];
    if (context_in->Line+1 < section->nb_lines)
    {
        if (context_out != context_in) *context_out = *context_in;
        context_out->Line++;
        SetLastError( 0 );
        return TRUE;
    }

    /* now search the appended files */

    for (file = file->next; file; file = file->next)
    {
        int section_index = find_section( file, section->name );
        if (section_index == -1) continue;
        if (file->sections[section_index]->nb_lines)
        {
            context_out->Inf        = context_in->Inf;
            context_out->CurrentInf = file;
            context_out->Section    = section_index;
            context_out->Line       = 0;
            SetLastError( 0 );
            return TRUE;
        }
    }
 error:
    SetLastError( ERROR_LINE_NOT_FOUND );
    return FALSE;
}


/***********************************************************************
 *            SetupFindNextMatchLineA   (SETUPAPI.@)
 */
BOOL WINAPI SetupFindNextMatchLineA( PINFCONTEXT context_in, PCSTR key,
                                     PINFCONTEXT context_out )
{
    UNICODE_STRING keyW;
    BOOL ret = FALSE;

    if (!key) return SetupFindNextLine( context_in, context_out );

    if (!RtlCreateUnicodeStringFromAsciiz( &keyW, key ))
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    else
    {
        ret = SetupFindNextMatchLineW( context_in, keyW.Buffer, context_out );
        RtlFreeUnicodeString( &keyW );
    }
    return ret;
}


/***********************************************************************
 *            SetupFindNextMatchLineW   (SETUPAPI.@)
 */
BOOL WINAPI SetupFindNextMatchLineW( PINFCONTEXT context_in, PCWSTR key,
                                     PINFCONTEXT context_out )
{
    struct inf_file *file = context_in->CurrentInf;
    WCHAR buffer[MAX_STRING_LEN + 1];
    struct section *section;
    struct line *line;
    unsigned int i;

    if (!key) return SetupFindNextLine( context_in, context_out );

    if (context_in->Section >= file->nb_sections) goto error;

    section = file->sections[context_in->Section];

    for (i = context_in->Line+1, line = &section->lines[i]; i < section->nb_lines; i++, line++)
    {
        if (line->key_field == -1) continue;
        PARSER_string_substW( file, file->fields[line->key_field].text, buffer, ARRAY_SIZE(buffer) );
        if (!wcsicmp( key, buffer ))
        {
            if (context_out != context_in) *context_out = *context_in;
            context_out->Line = i;
            SetLastError( 0 );
            TRACE( "(%p,%s,%s): returning %d\n",
                   file, debugstr_w(section->name), debugstr_w(key), i );
            return TRUE;
        }
    }

    /* now search the appended files */

    for (file = file->next; file; file = file->next)
    {
        int section_index = find_section( file, section->name );
        if (section_index == -1) continue;
        section = file->sections[section_index];
        for (i = 0, line = section->lines; i < section->nb_lines; i++, line++)
        {
            if (line->key_field == -1) continue;
            if (!wcsicmp( key, file->fields[line->key_field].text ))
            {
                context_out->Inf        = context_in->Inf;
                context_out->CurrentInf = file;
                context_out->Section    = section_index;
                context_out->Line       = i;
                SetLastError( 0 );
                TRACE( "(%p,%s,%s): returning %d/%d\n",
                       file, debugstr_w(section->name), debugstr_w(key), section_index, i );
                return TRUE;
            }
        }
    }
    TRACE( "(%p,%s,%s): not found\n",
           context_in->CurrentInf, debugstr_w(section->name), debugstr_w(key) );
 error:
    SetLastError( ERROR_LINE_NOT_FOUND );
    return FALSE;
}


/***********************************************************************
 *		SetupGetLineTextW    (SETUPAPI.@)
 */
BOOL WINAPI SetupGetLineTextW( PINFCONTEXT context, HINF hinf, PCWSTR section_name,
                               PCWSTR key_name, PWSTR buffer, DWORD size, PDWORD required )
{
    struct inf_file *file;
    struct line *line;
    struct field *field;
    int i;
    DWORD total = 0;

    if (!context)
    {
        INFCONTEXT new_context;
        if (!SetupFindFirstLineW( hinf, section_name, key_name, &new_context )) return FALSE;
        file = new_context.CurrentInf;
        line = get_line( file, new_context.Section, new_context.Line );
    }
    else
    {
        file = context->CurrentInf;
        if (!(line = get_line( file, context->Section, context->Line )))
        {
            SetLastError( ERROR_LINE_NOT_FOUND );
            return FALSE;
        }
    }

    for (i = 0, field = &file->fields[line->first_field]; i < line->nb_fields; i++, field++)
        total += PARSER_string_substW( file, field->text, NULL, 0 ) + 1;

    if (required) *required = total;
    if (buffer)
    {
        if (total > size)
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
        for (i = 0, field = &file->fields[line->first_field]; i < line->nb_fields; i++, field++)
        {
            unsigned int len = PARSER_string_substW( file, field->text, buffer, size );
            if (i+1 < line->nb_fields) buffer[len] = ',';
            buffer += len + 1;
        }
    }
    return TRUE;
}


/***********************************************************************
 *		SetupGetLineTextA    (SETUPAPI.@)
 */
BOOL WINAPI SetupGetLineTextA( PINFCONTEXT context, HINF hinf, PCSTR section_name,
                               PCSTR key_name, PSTR buffer, DWORD size, PDWORD required )
{
    struct inf_file *file;
    struct line *line;
    struct field *field;
    int i;
    DWORD total = 0;

    if (!context)
    {
        INFCONTEXT new_context;
        if (!SetupFindFirstLineA( hinf, section_name, key_name, &new_context )) return FALSE;
        file = new_context.CurrentInf;
        line = get_line( file, new_context.Section, new_context.Line );
    }
    else
    {
        file = context->CurrentInf;
        if (!(line = get_line( file, context->Section, context->Line )))
        {
            SetLastError( ERROR_LINE_NOT_FOUND );
            return FALSE;
        }
    }

    for (i = 0, field = &file->fields[line->first_field]; i < line->nb_fields; i++, field++)
        total += PARSER_string_substA( file, field->text, NULL, 0 ) + 1;

    if (required) *required = total;
    if (buffer)
    {
        if (total > size)
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
        for (i = 0, field = &file->fields[line->first_field]; i < line->nb_fields; i++, field++)
        {
            unsigned int len = PARSER_string_substA( file, field->text, buffer, size );
            if (i+1 < line->nb_fields) buffer[len] = ',';
            buffer += len + 1;
        }
    }
    return TRUE;
}


/***********************************************************************
 *		SetupGetFieldCount    (SETUPAPI.@)
 */
DWORD WINAPI SetupGetFieldCount( PINFCONTEXT context )
{
    struct inf_file *file = context->CurrentInf;
    struct line *line = get_line( file, context->Section, context->Line );

    if (!line) return 0;
    return line->nb_fields;
}


/***********************************************************************
 *		SetupGetStringFieldA    (SETUPAPI.@)
 */
BOOL WINAPI SetupGetStringFieldA( PINFCONTEXT context, DWORD index, PSTR buffer,
                                  DWORD size, PDWORD required )
{
    struct inf_file *file = context->CurrentInf;
    struct field *field = get_field( file, context->Section, context->Line, index );
    unsigned int len;

    SetLastError(0);
    if (!field) return FALSE;
    len = PARSER_string_substA( file, field->text, NULL, 0 );
    if (required) *required = len + 1;
    if (buffer)
    {
        if (size <= len)
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
        PARSER_string_substA( file, field->text, buffer, size );

        TRACE( "context %p/%p/%d/%d index %ld returning %s\n",
               context->Inf, context->CurrentInf, context->Section, context->Line,
               index, debugstr_a(buffer) );
    }
    return TRUE;
}


/***********************************************************************
 *		SetupGetStringFieldW    (SETUPAPI.@)
 */
BOOL WINAPI SetupGetStringFieldW( PINFCONTEXT context, DWORD index, PWSTR buffer,
                                  DWORD size, PDWORD required )
{
    struct inf_file *file = context->CurrentInf;
    struct field *field = get_field( file, context->Section, context->Line, index );
    unsigned int len;

    SetLastError(0);
    if (!field) return FALSE;
    len = PARSER_string_substW( file, field->text, NULL, 0 );
    if (required) *required = len + 1;
    if (buffer)
    {
        if (size <= len)
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
        PARSER_string_substW( file, field->text, buffer, size );

        TRACE( "context %p/%p/%d/%d index %ld returning %s\n",
               context->Inf, context->CurrentInf, context->Section, context->Line,
               index, debugstr_w(buffer) );
    }
    return TRUE;
}


/***********************************************************************
 *		SetupGetIntField    (SETUPAPI.@)
 */
BOOL WINAPI SetupGetIntField( PINFCONTEXT context, DWORD index, PINT result )
{
    char localbuff[20];
    char *end, *buffer = localbuff;
    DWORD required;
    INT res;
    BOOL ret;

    if (!(ret = SetupGetStringFieldA( context, index, localbuff, sizeof(localbuff), &required )))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return FALSE;
        if (!(buffer = malloc( required ))) return FALSE;
        if (!(ret = SetupGetStringFieldA( context, index, buffer, required, NULL ))) goto done;
    }
    /* The call to SetupGetStringFieldA succeeded. If buffer is empty we have an optional field */
    if (!*buffer) *result = 0;
    else
    {
        res = strtol( buffer, &end, 0 );
        if (end != buffer && !*end) *result = res;
        else
        {
            SetLastError( ERROR_INVALID_DATA );
            ret = FALSE;
        }
    }

 done:
    if (buffer != localbuff) free( buffer );
    return ret;
}


static int xdigit_to_int(WCHAR c)
{
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    if ('A' <= c && c <= 'F') return c - 'A' + 10;
    return -1;
}


/***********************************************************************
 *		SetupGetBinaryField    (SETUPAPI.@)
 */
BOOL WINAPI SetupGetBinaryField( PINFCONTEXT context, DWORD index, BYTE *buffer,
                                 DWORD size, LPDWORD required )
{
    struct inf_file *file = context->CurrentInf;
    struct line *line = get_line( file, context->Section, context->Line );
    struct field *field;
    int i;

    if (!line)
    {
        SetLastError( ERROR_LINE_NOT_FOUND );
        return FALSE;
    }
    if (!index || index > line->nb_fields)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    index--;  /* fields start at 0 */
    if (required) *required = line->nb_fields - index;
    if (!buffer) return TRUE;
    if (size < line->nb_fields - index)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
    field = &file->fields[line->first_field + index];
    for (i = index; i < line->nb_fields; i++, field++)
    {
        const WCHAR *p;
        DWORD value = 0;
        int d;
        for (p = field->text; *p && (d = xdigit_to_int(*p)) != -1; p++)
        {
            if ((value <<= 4) > 255)
            {
                SetLastError( ERROR_INVALID_DATA );
                return FALSE;
            }
            value |= d;
        }
        buffer[i - index] = value;
    }
    TRACE( "%p/%p/%d/%d index %ld\n",
           context->Inf, context->CurrentInf, context->Section, context->Line, index );
    return TRUE;
}


/***********************************************************************
 *		SetupGetMultiSzFieldA    (SETUPAPI.@)
 */
BOOL WINAPI SetupGetMultiSzFieldA( PINFCONTEXT context, DWORD index, PSTR buffer,
                                   DWORD size, LPDWORD required )
{
    struct inf_file *file = context->CurrentInf;
    struct line *line = get_line( file, context->Section, context->Line );
    struct field *field;
    unsigned int len;
    int i;
    DWORD total = 1;

    if (!line)
    {
        SetLastError( ERROR_LINE_NOT_FOUND );
        return FALSE;
    }
    if (!index || index > line->nb_fields)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    index--;  /* fields start at 0 */
    field = &file->fields[line->first_field + index];
    for (i = index; i < line->nb_fields; i++, field++)
    {
        if (!(len = PARSER_string_substA( file, field->text, NULL, 0 ))) break;
        total += len + 1;
    }

    if (required) *required = total;
    if (!buffer) return TRUE;
    if (total > size)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
    field = &file->fields[line->first_field + index];
    for (i = index; i < line->nb_fields; i++, field++)
    {
        if (!(len = PARSER_string_substA( file, field->text, buffer, size ))) break;
        buffer += len + 1;
    }
    *buffer = 0;  /* add final null */
    return TRUE;
}


/***********************************************************************
 *		SetupGetMultiSzFieldW    (SETUPAPI.@)
 */
BOOL WINAPI SetupGetMultiSzFieldW( PINFCONTEXT context, DWORD index, PWSTR buffer,
                                   DWORD size, LPDWORD required )
{
    struct inf_file *file = context->CurrentInf;
    struct line *line = get_line( file, context->Section, context->Line );
    struct field *field;
    unsigned int len;
    int i;
    DWORD total = 1;

    if (!line)
    {
        SetLastError( ERROR_LINE_NOT_FOUND );
        return FALSE;
    }
    if (!index || index > line->nb_fields)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    index--;  /* fields start at 0 */
    field = &file->fields[line->first_field + index];
    for (i = index; i < line->nb_fields; i++, field++)
    {
        if (!(len = PARSER_string_substW( file, field->text, NULL, 0 ))) break;
        total += len + 1;
    }

    if (required) *required = total;
    if (!buffer) return TRUE;
    if (total > size)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
    field = &file->fields[line->first_field + index];
    for (i = index; i < line->nb_fields; i++, field++)
    {
        if (!(len = PARSER_string_substW( file, field->text, buffer, size ))) break;
        buffer += len + 1;
    }
    *buffer = 0;  /* add final null */
    return TRUE;
}

/***********************************************************************
 *      pSetupGetField    (SETUPAPI.@)
 */
LPCWSTR WINAPI pSetupGetField( PINFCONTEXT context, DWORD index )
{
    struct inf_file *file = context->CurrentInf;
    struct field *field = get_field( file, context->Section, context->Line, index );

    if (!field)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    return field->text;
}
