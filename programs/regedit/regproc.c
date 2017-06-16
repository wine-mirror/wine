/*
 * Registry processing routines. Routines, common for registry
 * processing frontends.
 *
 * Copyright 1999 Sylvain St-Germain
 * Copyright 2002 Andriy Palamarchuk
 * Copyright 2008 Alexander N. Sørnes <alex@thehandofagony.com>
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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <winnt.h>
#include <winreg.h>
#include <assert.h>
#include <wine/unicode.h>
#include <wine/debug.h>
#include "regproc.h"

#define REG_VAL_BUF_SIZE        4096

/* maximal number of characters in hexadecimal data line,
 * including the indentation, but not including the '\' character
 */
#define REG_FILE_HEX_LINE_LEN   (2 + 25 * 3)

extern const WCHAR* reg_class_namesW[];

static HKEY reg_class_keys[] = {
            HKEY_LOCAL_MACHINE, HKEY_USERS, HKEY_CLASSES_ROOT,
            HKEY_CURRENT_CONFIG, HKEY_CURRENT_USER, HKEY_DYN_DATA
        };

#define ARRAY_SIZE(A) (sizeof(A)/sizeof(*A))

/******************************************************************************
 * Allocates memory and converts input from multibyte to wide chars
 * Returned string must be freed by the caller
 */
static WCHAR* GetWideString(const char* strA)
{
    if(strA)
    {
        WCHAR* strW;
        int len = MultiByteToWideChar(CP_ACP, 0, strA, -1, NULL, 0);

        strW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        CHECK_ENOUGH_MEMORY(strW);
        MultiByteToWideChar(CP_ACP, 0, strA, -1, strW, len);
        return strW;
    }
    return NULL;
}

/******************************************************************************
 * Allocates memory and converts input from multibyte to wide chars
 * Returned string must be freed by the caller
 */
static WCHAR* GetWideStringN(const char* strA, int chars, DWORD *len)
{
    if(strA)
    {
        WCHAR* strW;
        *len = MultiByteToWideChar(CP_ACP, 0, strA, chars, NULL, 0);

        strW = HeapAlloc(GetProcessHeap(), 0, *len * sizeof(WCHAR));
        CHECK_ENOUGH_MEMORY(strW);
        MultiByteToWideChar(CP_ACP, 0, strA, chars, strW, *len);
        return strW;
    }
    *len = 0;
    return NULL;
}

/******************************************************************************
 * Allocates memory and converts input from wide chars to multibyte
 * Returned string must be freed by the caller
 */
char* GetMultiByteString(const WCHAR* strW)
{
    if(strW)
    {
        char* strA;
        int len = WideCharToMultiByte(CP_ACP, 0, strW, -1, NULL, 0, NULL, NULL);

        strA = HeapAlloc(GetProcessHeap(), 0, len);
        CHECK_ENOUGH_MEMORY(strA);
        WideCharToMultiByte(CP_ACP, 0, strW, -1, strA, len, NULL, NULL);
        return strA;
    }
    return NULL;
}

/******************************************************************************
 * Allocates memory and converts input from wide chars to multibyte
 * Returned string must be freed by the caller
 */
static char* GetMultiByteStringN(const WCHAR* strW, int chars, DWORD* len)
{
    if(strW)
    {
        char* strA;
        *len = WideCharToMultiByte(CP_ACP, 0, strW, chars, NULL, 0, NULL, NULL);

        strA = HeapAlloc(GetProcessHeap(), 0, *len);
        CHECK_ENOUGH_MEMORY(strA);
        WideCharToMultiByte(CP_ACP, 0, strW, chars, strA, *len, NULL, NULL);
        return strA;
    }
    *len = 0;
    return NULL;
}

static WCHAR *(*get_line)(FILE *);

/* parser definitions */
enum parser_state
{
    HEADER,              /* parsing the registry file version header */
    PARSE_WIN31_LINE,    /* parsing a Windows 3.1 registry line */
    LINE_START,          /* at the beginning of a registry line */
    KEY_NAME,            /* parsing a key name */
    DELETE_KEY,          /* deleting a registry key */
    DEFAULT_VALUE_NAME,  /* parsing a default value name */
    QUOTED_VALUE_NAME,   /* parsing a double-quoted value name */
    DATA_START,          /* preparing for data parsing operations */
    DELETE_VALUE,        /* deleting a registry value */
    DATA_TYPE,           /* parsing the registry data type */
    STRING_DATA,         /* parsing REG_SZ data */
    DWORD_DATA,          /* parsing DWORD data */
    HEX_DATA,            /* parsing REG_BINARY, REG_NONE, REG_EXPAND_SZ or REG_MULTI_SZ data */
    UNKNOWN_DATA,        /* parsing an unhandled or invalid data type */
    SET_VALUE,           /* adding a value to the registry */
    NB_PARSER_STATES
};

struct parser
{
    FILE              *file;           /* pointer to a registry file */
    WCHAR              two_wchars[2];  /* first two characters from the encoding check */
    BOOL               is_unicode;     /* parsing Unicode or ASCII data */
    short int          reg_version;    /* registry file version */
    HKEY               hkey;           /* current registry key */
    WCHAR             *key_name;       /* current key name */
    WCHAR             *value_name;     /* value name */
    DWORD              data_type;      /* data type */
    void              *data;           /* value data */
    DWORD              data_size;      /* size of the data (in bytes) */
    BOOL               hex_type;       /* parsing a hex data type */
    enum parser_state  state;          /* current parser state */
};

typedef WCHAR *(*parser_state_func)(struct parser *parser, WCHAR *pos);

/* parser state machine functions */
static WCHAR *header_state(struct parser *parser, WCHAR *pos);
static WCHAR *parse_win31_line_state(struct parser *parser, WCHAR *pos);
static WCHAR *line_start_state(struct parser *parser, WCHAR *pos);
static WCHAR *key_name_state(struct parser *parser, WCHAR *pos);
static WCHAR *delete_key_state(struct parser *parser, WCHAR *pos);
static WCHAR *default_value_name_state(struct parser *parser, WCHAR *pos);
static WCHAR *quoted_value_name_state(struct parser *parser, WCHAR *pos);
static WCHAR *data_start_state(struct parser *parser, WCHAR *pos);
static WCHAR *delete_value_state(struct parser *parser, WCHAR *pos);
static WCHAR *data_type_state(struct parser *parser, WCHAR *pos);
static WCHAR *string_data_state(struct parser *parser, WCHAR *pos);
static WCHAR *dword_data_state(struct parser *parser, WCHAR *pos);
static WCHAR *hex_data_state(struct parser *parser, WCHAR *pos);
static WCHAR *unknown_data_state(struct parser *parser, WCHAR *pos);
static WCHAR *set_value_state(struct parser *parser, WCHAR *pos);

static const parser_state_func parser_funcs[NB_PARSER_STATES] =
{
    header_state,              /* HEADER */
    parse_win31_line_state,    /* PARSE_WIN31_LINE */
    line_start_state,          /* LINE_START */
    key_name_state,            /* KEY_NAME */
    delete_key_state,          /* DELETE_KEY */
    default_value_name_state,  /* DEFAULT_VALUE_NAME */
    quoted_value_name_state,   /* QUOTED_VALUE_NAME */
    data_start_state,          /* DATA_START */
    delete_value_state,        /* DELETE_VALUE */
    data_type_state,           /* DATA_TYPE */
    string_data_state,         /* STRING_DATA */
    dword_data_state,          /* DWORD_DATA */
    hex_data_state,            /* HEX_DATA */
    unknown_data_state,        /* UNKNOWN_DATA */
    set_value_state,           /* SET_VALUE */
};

/* set the new parser state and return the previous one */
static inline enum parser_state set_state(struct parser *parser, enum parser_state state)
{
    enum parser_state ret = parser->state;
    parser->state = state;
    return ret;
}

/******************************************************************************
 * Converts a hex representation of a DWORD into a DWORD.
 */
static BOOL convert_hex_to_dword(WCHAR *str, DWORD *dw)
{
    WCHAR *p, *end;
    int count = 0;

    while (*str == ' ' || *str == '\t') str++;
    if (!*str) goto error;

    p = str;
    while (isxdigitW(*p))
    {
        count++;
        p++;
    }
    if (count > 8) goto error;

    end = p;
    while (*p == ' ' || *p == '\t') p++;
    if (*p && *p != ';') goto error;

    *end = 0;
    *dw = strtoulW(str, &end, 16);
    return TRUE;

error:
    return FALSE;
}

/******************************************************************************
 * Converts comma-separated hex data into a binary string.
 */
static BYTE *convert_hex_csv_to_hex(WCHAR *str, DWORD *size)
{
    WCHAR *s;
    BYTE *d, *data;

    /* The worst case is 1 digit + 1 comma per byte */
    *size=(lstrlenW(str)+1)/2;
    data=HeapAlloc(GetProcessHeap(), 0, *size);
    CHECK_ENOUGH_MEMORY(data);

    s = str;
    d = data;
    *size=0;
    while (*s != '\0') {
        UINT wc;
        WCHAR *end;

        wc = strtoulW(s,&end,16);
        if (end == s || wc > 0xff || (*end && *end != ',')) {
            HeapFree(GetProcessHeap(), 0, data);
            return NULL;
        }
        *d++ =(BYTE)wc;
        (*size)++;
        if (*end) end++;
        s = end;
    }

    return data;
}

/******************************************************************************
 * Parses the data type of the registry value being imported and modifies
 * the input parameter to skip the string representation of the data type.
 *
 * Returns TRUE or FALSE to indicate whether a data type was found.
 */
static BOOL parse_data_type(struct parser *parser, WCHAR **line)
{
    struct data_type { const WCHAR *tag; int len; int type; int parse_type; };

    static const WCHAR quote[] = {'"'};
    static const WCHAR hex[] = {'h','e','x',':'};
    static const WCHAR dword[] = {'d','w','o','r','d',':'};
    static const WCHAR hexp[] = {'h','e','x','('};

    static const struct data_type data_types[] = {
    /*    tag    len  type         parse type    */
        { quote,  1,  REG_SZ,      REG_SZ },
        { hex,    4,  REG_BINARY,  REG_BINARY },
        { dword,  6,  REG_DWORD,   REG_DWORD },
        { hexp,   4,  -1,          REG_BINARY }, /* REG_NONE, REG_EXPAND_SZ, REG_MULTI_SZ */
        { NULL,   0,  0,           0 }
    };

    const struct data_type *ptr;

    for (ptr = data_types; ptr->tag; ptr++)
    {
        if (strncmpW(ptr->tag, *line, ptr->len))
            continue;

        parser->data_type = ptr->parse_type;
        *line += ptr->len;

        if (ptr->type == -1)
        {
            WCHAR *end;
            DWORD val;

            /* "hex(xx):" is special */
            val = strtoulW(*line, &end, 16);
            if (!**line || *end != ')' || *(end + 1) != ':')
                return FALSE;

            parser->data_type = val;
            parser->hex_type = TRUE;
            *line = end + 2;
        }
        return TRUE;
    }
    return FALSE;
}

/******************************************************************************
 * Replaces escape sequences with their character equivalents and
 * null-terminates the string on the first non-escaped double quote.
 *
 * Assigns a pointer to the remaining unparsed data in the line.
 * Returns TRUE or FALSE to indicate whether a closing double quote was found.
 */
static BOOL REGPROC_unescape_string(WCHAR *str, WCHAR **unparsed)
{
    int str_idx = 0;            /* current character under analysis */
    int val_idx = 0;            /* the last character of the unescaped string */
    int len = lstrlenW(str);
    BOOL ret;

    for (str_idx = 0; str_idx < len; str_idx++, val_idx++) {
        if (str[str_idx] == '\\') {
            str_idx++;
            switch (str[str_idx]) {
            case 'n':
                str[val_idx] = '\n';
                break;
            case 'r':
                str[val_idx] = '\r';
                break;
            case '0':
                str[val_idx] = '\0';
                break;
            case '\\':
            case '"':
                str[val_idx] = str[str_idx];
                break;
            default:
                output_message(STRING_ESCAPE_SEQUENCE, str[str_idx]);
                str[val_idx] = str[str_idx];
                break;
            }
        } else if (str[str_idx] == '"') {
            break;
        } else {
            str[val_idx] = str[str_idx];
        }
    }

    ret = (str[str_idx] == '"');
    *unparsed = str + str_idx + 1;
    str[val_idx] = '\0';
    return ret;
}

static HKEY parse_key_name(WCHAR *key_name, WCHAR **key_path)
{
    unsigned int i;

    if (!key_name) return 0;

    *key_path = strchrW(key_name, '\\');
    if (*key_path) (*key_path)++;

    for (i = 0; i < ARRAY_SIZE(reg_class_keys); i++)
    {
        int len = lstrlenW(reg_class_namesW[i]);
        if (!strncmpiW(key_name, reg_class_namesW[i], len) &&
           (key_name[len] == 0 || key_name[len] == '\\'))
        {
            return reg_class_keys[i];
        }
    }

    return 0;
}

static void close_key(struct parser *parser)
{
    if (parser->hkey)
    {
        HeapFree(GetProcessHeap(), 0, parser->key_name);
        parser->key_name = NULL;

        RegCloseKey(parser->hkey);
        parser->hkey = NULL;
    }
}

/******************************************************************************
 * Opens the registry key given by the input path.
 * This key must be closed by calling close_key().
 */
static LONG open_key(struct parser *parser, WCHAR *path)
{
    HKEY key_class;
    WCHAR *key_path;
    LONG res;

    close_key(parser);

    /* Get the registry class */
    if (!path || !(key_class = parse_key_name(path, &key_path)))
        return ERROR_INVALID_PARAMETER;

    res = RegCreateKeyExW(key_class, key_path, 0, NULL, REG_OPTION_NON_VOLATILE,
                          KEY_ALL_ACCESS, NULL, &parser->hkey, NULL);

    if (res == ERROR_SUCCESS)
    {
        parser->key_name = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(path) + 1) * sizeof(WCHAR));
        CHECK_ENOUGH_MEMORY(parser->key_name);
        lstrcpyW(parser->key_name, path);
    }
    else
        parser->hkey = NULL;

    return res;
}

enum reg_versions {
    REG_VERSION_31,
    REG_VERSION_40,
    REG_VERSION_50,
    REG_VERSION_FUZZY,
    REG_VERSION_INVALID
};

static enum reg_versions parse_file_header(const WCHAR *s)
{
    static const WCHAR header_31[] = {'R','E','G','E','D','I','T',0};
    static const WCHAR header_40[] = {'R','E','G','E','D','I','T','4',0};
    static const WCHAR header_50[] = {'W','i','n','d','o','w','s',' ',
                                      'R','e','g','i','s','t','r','y',' ','E','d','i','t','o','r',' ',
                                      'V','e','r','s','i','o','n',' ','5','.','0','0',0};

    while (*s && (*s == ' ' || *s == '\t')) s++;

    if (!strcmpW(s, header_31))
        return REG_VERSION_31;

    if (!strcmpW(s, header_40))
        return REG_VERSION_40;

    if (!strcmpW(s, header_50))
        return REG_VERSION_50;

    /* The Windows version accepts registry file headers beginning with "REGEDIT" and ending
     * with other characters, as long as "REGEDIT" appears at the start of the line. For example,
     * "REGEDIT 4", "REGEDIT9" and "REGEDIT4FOO" are all treated as valid file headers.
     * In all such cases, however, the contents of the registry file are not imported.
     */
    if (!strncmpW(s, header_31, 7)) /* "REGEDIT" without NUL */
        return REG_VERSION_FUZZY;

    return REG_VERSION_INVALID;
}

/* handler for parser HEADER state */
static WCHAR *header_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *line, *header;

    if (!(line = get_line(parser->file)))
        return NULL;

    if (!parser->is_unicode)
    {
        header = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(line) + 3) * sizeof(WCHAR));
        CHECK_ENOUGH_MEMORY(header);
        header[0] = parser->two_wchars[0];
        header[1] = parser->two_wchars[1];
        lstrcpyW(header + 2, line);
        parser->reg_version = parse_file_header(header);
        HeapFree(GetProcessHeap(), 0, header);
    }
    else parser->reg_version = parse_file_header(line);

    switch (parser->reg_version)
    {
    case REG_VERSION_31:
        set_state(parser, PARSE_WIN31_LINE);
        break;
    case REG_VERSION_40:
    case REG_VERSION_50:
        set_state(parser, LINE_START);
        break;
    default:
        get_line(NULL); /* Reset static variables */
        return NULL;
    }

    return line;
}

/* handler for parser PARSE_WIN31_LINE state */
static WCHAR *parse_win31_line_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *line, *value;
    static WCHAR hkcr[] = {'H','K','E','Y','_','C','L','A','S','S','E','S','_','R','O','O','T'};
    unsigned int key_end = 0;

    if (!(line = get_line(parser->file)))
        return NULL;

    if (strncmpW(line, hkcr, ARRAY_SIZE(hkcr)))
        goto invalid;

    /* get key name */
    while (line[key_end] && !isspaceW(line[key_end])) key_end++;

    value = line + key_end;
    while (*value == ' ' || *value == '\t') value++;

    if (*value == '=') value++;
    if (*value == ' ') value++; /* at most one space is skipped */

    line[key_end] = 0;

    if (open_key(parser, line) != ERROR_SUCCESS)
    {
        output_message(STRING_OPEN_KEY_FAILED, line);
        goto invalid;
    }

    parser->value_name = NULL;
    parser->data_type = REG_SZ;
    parser->data = value;
    parser->data_size = (lstrlenW(value) + 1) * sizeof(WCHAR);

    set_state(parser, SET_VALUE);
    return value;

invalid:
    set_state(parser, PARSE_WIN31_LINE);
    return line;
}

/* handler for parser LINE_START state */
static WCHAR *line_start_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *line, *p;

    if (!(line = get_line(parser->file)))
        return NULL;

    for (p = line; *p; p++)
    {
        switch (*p)
        {
        case '[':
            set_state(parser, KEY_NAME);
            return p + 1;
        case '@':
            set_state(parser, DEFAULT_VALUE_NAME);
            return p;
        case '"':
            set_state(parser, QUOTED_VALUE_NAME);
            return p + 1;
        case ' ':
        case '\t':
            break;
        default:
            set_state(parser, LINE_START);
            return p;
        }
    }

    return p;
}

/* handler for parser KEY_NAME state */
static WCHAR *key_name_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *p = pos, *key_end;

    if (*p == ' ' || *p == '\t' || !(key_end = strrchrW(p, ']')))
        goto done;

    *key_end = 0;

    if (*p == '-')
    {
        set_state(parser, DELETE_KEY);
        return p + 1;
    }
    else if (open_key(parser, p) != ERROR_SUCCESS)
        output_message(STRING_OPEN_KEY_FAILED, p);

done:
    set_state(parser, LINE_START);
    return p;
}

/* handler for parser DELETE_KEY state */
static WCHAR *delete_key_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *p = pos;

    if (*p == 'H' || *p == 'h')
        delete_registry_key(p);

    set_state(parser, LINE_START);
    return p;
}

/* handler for parser DEFAULT_VALUE_NAME state */
static WCHAR *default_value_name_state(struct parser *parser, WCHAR *pos)
{
    parser->value_name = NULL;

    set_state(parser, DATA_START);
    return pos + 1;
}

/* handler for parser QUOTED_VALUE_NAME state */
static WCHAR *quoted_value_name_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *val_name = pos, *p;

    if (parser->value_name)
    {
        HeapFree(GetProcessHeap(), 0, parser->value_name);
        parser->value_name = NULL;
    }

    if (!REGPROC_unescape_string(val_name, &p))
        goto invalid;

    /* copy the value name in case we need to parse multiple lines and the buffer is overwritten */
    parser->value_name = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(val_name) + 1) * sizeof(WCHAR));
    CHECK_ENOUGH_MEMORY(parser->value_name);
    lstrcpyW(parser->value_name, val_name);

    set_state(parser, DATA_START);
    return p;

invalid:
    set_state(parser, LINE_START);
    return p;
}

/* handler for parser DATA_START state */
static WCHAR *data_start_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *p = pos;
    unsigned int len;

    while (*p == ' ' || *p == '\t') p++;
    if (*p != '=') goto done;
    p++;
    while (*p == ' ' || *p == '\t') p++;

    /* trim trailing whitespace */
    len = strlenW(p);
    while (len > 0 && (p[len - 1] == ' ' || p[len - 1] == '\t')) len--;
    p[len] = 0;

    if (*p == '-')
        set_state(parser, DELETE_VALUE);
    else
        set_state(parser, DATA_TYPE);
    return p;

done:
    set_state(parser, LINE_START);
    return p;
}

/* handler for parser DELETE_VALUE state */
static WCHAR *delete_value_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *p = pos + 1;

    while (*p == ' ' || *p == '\t') p++;
    if (*p && *p != ';') goto done;

    RegDeleteValueW(parser->hkey, parser->value_name);

done:
    set_state(parser, LINE_START);
    return p;
}

/* handler for parser DATA_TYPE state */
static WCHAR *data_type_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *line = pos;

    if (!parse_data_type(parser, &line))
    {
        set_state(parser, LINE_START);
        return line;
    }

    switch (parser->data_type)
    {
    case REG_SZ:
        set_state(parser, STRING_DATA);
        break;
    case REG_DWORD:
        set_state(parser, DWORD_DATA);
        break;
    case REG_NONE:
    case REG_EXPAND_SZ:
    case REG_BINARY:
    case REG_MULTI_SZ:
        set_state(parser, HEX_DATA);
        break;
    default:
        set_state(parser, UNKNOWN_DATA);
    }

    return line;
}

/* handler for parser STRING_DATA state */
static WCHAR *string_data_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *line;

    parser->data = pos;

    if (!REGPROC_unescape_string(parser->data, &line))
        goto invalid;

    while (*line == ' ' || *line == '\t') line++;
    if (*line && *line != ';') goto invalid;

    parser->data_size = (lstrlenW(parser->data) + 1) * sizeof(WCHAR);

    set_state(parser, SET_VALUE);
    return line;

invalid:
    set_state(parser, LINE_START);
    return line;
}

/* handler for parser DWORD_DATA state */
static WCHAR *dword_data_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *line = pos;

    parser->data = HeapAlloc(GetProcessHeap(), 0, sizeof(DWORD));
    CHECK_ENOUGH_MEMORY(parser->data);

    if (!convert_hex_to_dword(line, parser->data))
        goto invalid;

    parser->data_size = sizeof(DWORD);

    set_state(parser, SET_VALUE);
    return line;

invalid:
    HeapFree(GetProcessHeap(), 0, parser->data);
    parser->data = NULL;

    set_state(parser, LINE_START);
    return line;
}

/* handler for parser HEX_DATA state */
static WCHAR *hex_data_state(struct parser *parser, WCHAR *pos)
{
    WCHAR *line = pos;

    if (!(parser->data = convert_hex_csv_to_hex(line, &parser->data_size)))
        goto invalid;

    if (!parser->is_unicode && (parser->data_type == REG_EXPAND_SZ || parser->data_type == REG_MULTI_SZ))
    {
        void *tmp = parser->data;

        parser->data = GetWideStringN(parser->data, parser->data_size, &parser->data_size);
        parser->data_size *= sizeof(WCHAR);
        HeapFree(GetProcessHeap(), 0, tmp);
    }

    set_state(parser, SET_VALUE);
    return line;

invalid:
    set_state(parser, LINE_START);
    return line;
}

/* handler for parser UNKNOWN_DATA state */
static WCHAR *unknown_data_state(struct parser *parser, WCHAR *pos)
{
    output_message(STRING_UNKNOWN_DATA_FORMAT, parser->data_type);

    set_state(parser, LINE_START);
    return pos;
}

/* handler for parser SET_VALUE state */
static WCHAR *set_value_state(struct parser *parser, WCHAR *pos)
{
    RegSetValueExW(parser->hkey, parser->value_name, 0, parser->data_type,
                   parser->data, parser->data_size);

    if (parser->data_type == REG_DWORD || parser->hex_type)
    {
        HeapFree(GetProcessHeap(), 0, parser->data);
        parser->data = NULL;
        parser->hex_type = FALSE;
    }

    if (parser->reg_version == REG_VERSION_31)
        set_state(parser, PARSE_WIN31_LINE);
    else
        set_state(parser, LINE_START);

    return pos;
}

static WCHAR *get_lineA(FILE *fp)
{
    static WCHAR *lineW;
    static size_t size;
    static char *buf, *next;
    char *line;

    HeapFree(GetProcessHeap(), 0, lineW);

    if (!fp) goto cleanup;

    if (!size)
    {
        size = REG_VAL_BUF_SIZE;
        buf = HeapAlloc(GetProcessHeap(), 0, size);
        CHECK_ENOUGH_MEMORY(buf);
        *buf = 0;
        next = buf;
    }
    line = next;

    while (next)
    {
        char *p = strpbrk(line, "\r\n");
        if (!p)
        {
            size_t len, count;
            len = strlen(next);
            memmove(buf, next, len + 1);
            if (size - len < 3)
            {
                char *new_buf = HeapReAlloc(GetProcessHeap(), 0, buf, size * 2);
                CHECK_ENOUGH_MEMORY(new_buf);
                buf = new_buf;
                size *= 2;
            }
            if (!(count = fread(buf + len, 1, size - len - 1, fp)))
            {
                next = NULL;
                lineW = GetWideString(buf);
                return lineW;
            }
            buf[len + count] = 0;
            next = buf;
            line = buf;
            continue;
        }
        next = p + 1;
        if (*p == '\r' && *(p + 1) == '\n') next++;
        *p = 0;
        if (p > buf && *(p - 1) == '\\')
        {
            while (*next == ' ' || *next == '\t') next++;
            memmove(p - 1, next, strlen(next) + 1);
            next = line;
            continue;
        }
        if (*line == ';' || *line == '#')
        {
            line = next;
            continue;
        }
        lineW = GetWideString(line);
        return lineW;
    }

cleanup:
    lineW = NULL;
    if (size) HeapFree(GetProcessHeap(), 0, buf);
    size = 0;
    return NULL;
}

static WCHAR *get_lineW(FILE *fp)
{
    static size_t size;
    static WCHAR *buf, *next;
    WCHAR *line;

    if (!fp) goto cleanup;

    if (!size)
    {
        size = REG_VAL_BUF_SIZE;
        buf = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
        CHECK_ENOUGH_MEMORY(buf);
        *buf = 0;
        next = buf;
    }
    line = next;

    while (next)
    {
        static const WCHAR line_endings[] = {'\r','\n',0};
        WCHAR *p = strpbrkW(line, line_endings);
        if (!p)
        {
            size_t len, count;
            len = strlenW(next);
            memmove(buf, next, (len + 1) * sizeof(WCHAR));
            if (size - len < 3)
            {
                WCHAR *new_buf = HeapReAlloc(GetProcessHeap(), 0, buf, (size * 2) * sizeof(WCHAR));
                CHECK_ENOUGH_MEMORY(new_buf);
                buf = new_buf;
                size *= 2;
            }
            if (!(count = fread(buf + len, sizeof(WCHAR), size - len - 1, fp)))
            {
                next = NULL;
                return buf;
            }
            buf[len + count] = 0;
            next = buf;
            line = buf;
            continue;
        }
        next = p + 1;
        if (*p == '\r' && *(p + 1) == '\n') next++;
        *p = 0;
        if (p > buf && *(p - 1) == '\\')
        {
            while (*next == ' ' || *next == '\t') next++;
            memmove(p - 1, next, (strlenW(next) + 1) * sizeof(WCHAR));
            next = line;
            continue;
        }
        if (*line == ';' || *line == '#')
        {
            line = next;
            continue;
        }
        return line;
    }

cleanup:
    if (size) HeapFree(GetProcessHeap(), 0, buf);
    size = 0;
    return NULL;
}

/******************************************************************************
 * Checks whether the buffer has enough room for the string or required size.
 * Resizes the buffer if necessary.
 *
 * Parameters:
 * buffer - pointer to a buffer for string
 * len - current length of the buffer in characters.
 * required_len - length of the string to place to the buffer in characters.
 *   The length does not include the terminating null character.
 */
static void REGPROC_resize_char_buffer(WCHAR **buffer, DWORD *len, DWORD required_len)
{
    required_len++;
    if (required_len > *len) {
        *len = required_len;
        if (!*buffer)
            *buffer = HeapAlloc(GetProcessHeap(), 0, *len * sizeof(**buffer));
        else
            *buffer = HeapReAlloc(GetProcessHeap(), 0, *buffer, *len * sizeof(**buffer));
        CHECK_ENOUGH_MEMORY(*buffer);
    }
}

/******************************************************************************
 * Same as REGPROC_resize_char_buffer() but on a regular buffer.
 *
 * Parameters:
 * buffer - pointer to a buffer
 * len - current size of the buffer in bytes
 * required_size - size of the data to place in the buffer in bytes
 */
static void REGPROC_resize_binary_buffer(BYTE **buffer, DWORD *size, DWORD required_size)
{
    if (required_size > *size) {
        *size = required_size;
        if (!*buffer)
            *buffer = HeapAlloc(GetProcessHeap(), 0, *size);
        else
            *buffer = HeapReAlloc(GetProcessHeap(), 0, *buffer, *size);
        CHECK_ENOUGH_MEMORY(*buffer);
    }
}

/******************************************************************************
 * Prints string str to file
 */
static void REGPROC_export_string(WCHAR **line_buf, DWORD *line_buf_size, DWORD *line_len, WCHAR *str, DWORD str_len)
{
    DWORD i, pos;
    DWORD extra = 0;

    REGPROC_resize_char_buffer(line_buf, line_buf_size, *line_len + str_len + 10);

    /* escaping characters */
    pos = *line_len;
    for (i = 0; i < str_len; i++) {
        WCHAR c = str[i];
        switch (c) {
        case '\n':
            extra++;
            REGPROC_resize_char_buffer(line_buf, line_buf_size, *line_len + str_len + extra);
            (*line_buf)[pos++] = '\\';
            (*line_buf)[pos++] = 'n';
            break;

        case '\r':
            extra++;
            REGPROC_resize_char_buffer(line_buf, line_buf_size, *line_len + str_len + extra);
            (*line_buf)[pos++] = '\\';
            (*line_buf)[pos++] = 'r';
            break;

        case '\\':
        case '"':
            extra++;
            REGPROC_resize_char_buffer(line_buf, line_buf_size, *line_len + str_len + extra);
            (*line_buf)[pos++] = '\\';
            /* Fall through */

        default:
            (*line_buf)[pos++] = c;
            break;
        }
    }
    (*line_buf)[pos] = '\0';
    *line_len = pos;
}

static void REGPROC_export_binary(WCHAR **line_buf, DWORD *line_buf_size, DWORD *line_len, DWORD type, BYTE *value, DWORD value_size, BOOL unicode)
{
    DWORD hex_pos, data_pos;
    const WCHAR *hex_prefix;
    const WCHAR hex[] = {'h','e','x',':',0};
    WCHAR hex_buf[17];
    const WCHAR concat[] = {'\\','\r','\n',' ',' ',0};
    DWORD concat_prefix, concat_len;
    const WCHAR newline[] = {'\r','\n',0};
    CHAR* value_multibyte = NULL;

    if (type == REG_BINARY) {
        hex_prefix = hex;
    } else {
        const WCHAR hex_format[] = {'h','e','x','(','%','x',')',':',0};
        hex_prefix = hex_buf;
        sprintfW(hex_buf, hex_format, type);
        if ((type == REG_SZ || type == REG_EXPAND_SZ || type == REG_MULTI_SZ) && !unicode)
        {
            value_multibyte = GetMultiByteStringN((WCHAR*)value, value_size / sizeof(WCHAR), &value_size);
            value = (BYTE*)value_multibyte;
        }
    }

    concat_len = lstrlenW(concat);
    concat_prefix = 2;

    hex_pos = *line_len;
    *line_len += lstrlenW(hex_prefix);
    data_pos = *line_len;
    *line_len += value_size * 3;
    /* - The 2 spaces that concat places at the start of the
     *   line effectively reduce the space available for data.
     * - If the value name and hex prefix are very long
     *   ( > REG_FILE_HEX_LINE_LEN) or *line_len divides
     *   without a remainder then we may overestimate
     *   the needed number of lines by one. But that's ok.
     * - The trailing '\r' takes the place of a comma so
     *   we only need to add 1 for the trailing '\n'
     */
    *line_len += *line_len / (REG_FILE_HEX_LINE_LEN - concat_prefix) * concat_len + 1;
    REGPROC_resize_char_buffer(line_buf, line_buf_size, *line_len);
    lstrcpyW(*line_buf + hex_pos, hex_prefix);
    if (value_size)
    {
        const WCHAR format[] = {'%','0','2','x',0};
        DWORD i, column;

        column = data_pos; /* no line wrap yet */
        i = 0;
        while (1)
        {
            sprintfW(*line_buf + data_pos, format, (unsigned int)value[i]);
            data_pos += 2;
            if (++i == value_size)
                break;

            (*line_buf)[data_pos++] = ',';
            column += 3;

            /* wrap the line */
            if (column >= REG_FILE_HEX_LINE_LEN) {
                lstrcpyW(*line_buf + data_pos, concat);
                data_pos += concat_len;
                column = concat_prefix;
            }
        }
    }
    lstrcpyW(*line_buf + data_pos, newline);
    HeapFree(GetProcessHeap(), 0, value_multibyte);
}

/******************************************************************************
 * Writes the given line to a file, in multi-byte or wide characters
 */
static void REGPROC_write_line(FILE *file, const WCHAR* str, BOOL unicode)
{
    if(unicode)
    {
        fwrite(str, sizeof(WCHAR), lstrlenW(str), file);
    } else
    {
        char* strA = GetMultiByteString(str);
        fputs(strA, file);
        HeapFree(GetProcessHeap(), 0, strA);
    }
}

/******************************************************************************
 * Writes contents of the registry key to the specified file stream.
 *
 * Parameters:
 * file - writable file stream to export registry branch to.
 * key - registry branch to export.
 * reg_key_name_buf - name of the key with registry class.
 *      Is resized if necessary.
 * reg_key_name_size - length of the buffer for the registry class in characters.
 * val_name_buf - buffer for storing value name.
 *      Is resized if necessary.
 * val_name_size - length of the buffer for storing value names in characters.
 * val_buf - buffer for storing values while extracting.
 *      Is resized if necessary.
 * val_size - size of the buffer for storing values in bytes.
 */
static void export_hkey(FILE *file, HKEY key,
                 WCHAR **reg_key_name_buf, DWORD *reg_key_name_size,
                 WCHAR **val_name_buf, DWORD *val_name_size,
                 BYTE **val_buf, DWORD *val_size,
                 WCHAR **line_buf, DWORD *line_buf_size,
                 BOOL unicode)
{
    DWORD max_sub_key_len;
    DWORD max_val_name_len;
    DWORD max_val_size;
    DWORD curr_len;
    DWORD i;
    LONG ret;
    WCHAR key_format[] = {'\r','\n','[','%','s',']','\r','\n',0};

    /* get size information and resize the buffers if necessary */
    if (RegQueryInfoKeyW(key, NULL, NULL, NULL, NULL,
                        &max_sub_key_len, NULL,
                        NULL, &max_val_name_len, &max_val_size, NULL, NULL
                       ) != ERROR_SUCCESS)
        return;
    curr_len = strlenW(*reg_key_name_buf);
    REGPROC_resize_char_buffer(reg_key_name_buf, reg_key_name_size,
                               max_sub_key_len + curr_len + 1);
    REGPROC_resize_char_buffer(val_name_buf, val_name_size,
                               max_val_name_len);
    REGPROC_resize_binary_buffer(val_buf, val_size, max_val_size);
    REGPROC_resize_char_buffer(line_buf, line_buf_size, lstrlenW(*reg_key_name_buf) + 4);
    /* output data for the current key */
    sprintfW(*line_buf, key_format, *reg_key_name_buf);
    REGPROC_write_line(file, *line_buf, unicode);

    /* print all the values */
    i = 0;
    for (;;) {
        DWORD value_type;
        DWORD val_name_size1 = *val_name_size;
        DWORD val_size1 = *val_size;
        ret = RegEnumValueW(key, i, *val_name_buf, &val_name_size1, NULL,
                           &value_type, *val_buf, &val_size1);
        if (ret == ERROR_MORE_DATA) {
            /* Increase the size of the buffers and retry */
            REGPROC_resize_char_buffer(val_name_buf, val_name_size, val_name_size1);
            REGPROC_resize_binary_buffer(val_buf, val_size, val_size1);
        } else if (ret == ERROR_SUCCESS) {
            DWORD line_len;
            i++;

            if ((*val_name_buf)[0]) {
                const WCHAR val_start[] = {'"','%','s','"','=',0};

                line_len = 0;
                REGPROC_export_string(line_buf, line_buf_size, &line_len, *val_name_buf, lstrlenW(*val_name_buf));
                REGPROC_resize_char_buffer(val_name_buf, val_name_size, lstrlenW(*line_buf) + 1);
                lstrcpyW(*val_name_buf, *line_buf);

                line_len = 3 + lstrlenW(*val_name_buf);
                REGPROC_resize_char_buffer(line_buf, line_buf_size, line_len);
                sprintfW(*line_buf, val_start, *val_name_buf);
            } else {
                const WCHAR std_val[] = {'@','=',0};
                line_len = 2;
                REGPROC_resize_char_buffer(line_buf, line_buf_size, line_len);
                lstrcpyW(*line_buf, std_val);
            }

            switch (value_type) {
            case REG_SZ:
            {
                WCHAR* wstr = (WCHAR*)*val_buf;

                if (val_size1 < sizeof(WCHAR) || val_size1 % sizeof(WCHAR) ||
                    wstr[val_size1 / sizeof(WCHAR) - 1]) {
                    REGPROC_export_binary(line_buf, line_buf_size, &line_len, value_type, *val_buf, val_size1, unicode);
                } else {
                    const WCHAR start[] = {'"',0};
                    const WCHAR end[] = {'"','\r','\n',0};
                    DWORD len;

                    len = lstrlenW(start);
                    REGPROC_resize_char_buffer(line_buf, line_buf_size, line_len + len);
                    lstrcpyW(*line_buf + line_len, start);
                    line_len += len;

                    REGPROC_export_string(line_buf, line_buf_size, &line_len, wstr, lstrlenW(wstr));

                    REGPROC_resize_char_buffer(line_buf, line_buf_size, line_len + lstrlenW(end));
                    lstrcpyW(*line_buf + line_len, end);
                }
                break;
            }

            case REG_DWORD:
            {
                WCHAR format[] = {'d','w','o','r','d',':','%','0','8','x','\r','\n',0};

                REGPROC_resize_char_buffer(line_buf, line_buf_size, line_len + 15);
                sprintfW(*line_buf + line_len, format, *((DWORD *)*val_buf));
                break;
            }

            case REG_NONE:
            case REG_EXPAND_SZ:
            case REG_MULTI_SZ:
            case REG_BINARY:
            default:
                REGPROC_export_binary(line_buf, line_buf_size, &line_len, value_type, *val_buf, val_size1, unicode);
            }
            REGPROC_write_line(file, *line_buf, unicode);
        }
        else break;
    }

    i = 0;
    (*reg_key_name_buf)[curr_len] = '\\';
    for (;;) {
        DWORD buf_size = *reg_key_name_size - curr_len - 1;

        ret = RegEnumKeyExW(key, i, *reg_key_name_buf + curr_len + 1, &buf_size,
                           NULL, NULL, NULL, NULL);
        if (ret == ERROR_MORE_DATA) {
            /* Increase the size of the buffer and retry */
            REGPROC_resize_char_buffer(reg_key_name_buf, reg_key_name_size, curr_len + 1 + buf_size);
        } else if (ret == ERROR_SUCCESS) {
            HKEY subkey;

            i++;
            if (RegOpenKeyW(key, *reg_key_name_buf + curr_len + 1,
                           &subkey) == ERROR_SUCCESS) {
                export_hkey(file, subkey, reg_key_name_buf, reg_key_name_size,
                            val_name_buf, val_name_size, val_buf, val_size,
                            line_buf, line_buf_size, unicode);
                RegCloseKey(subkey);
            }
            else break;
        }
        else break;
    }
    (*reg_key_name_buf)[curr_len] = '\0';
}

/******************************************************************************
 * Open file in binary mode for export.
 */
static FILE *REGPROC_open_export_file(WCHAR *file_name, BOOL unicode)
{
    FILE *file;
    WCHAR dash = '-';

    if (strncmpW(file_name,&dash,1)==0) {
        file=stdout;
        _setmode(_fileno(file), _O_BINARY);
    } else
    {
        WCHAR wb_mode[] = {'w','b',0};
        WCHAR regedit[] = {'r','e','g','e','d','i','t',0};

        file = _wfopen(file_name, wb_mode);
        if (!file) {
            _wperror(regedit);
            output_message(STRING_CANNOT_OPEN_FILE, file_name);
            exit(1);
        }
    }
    if(unicode)
    {
        const BYTE unicode_seq[] = {0xff,0xfe};
        const WCHAR header[] = {'W','i','n','d','o','w','s',' ','R','e','g','i','s','t','r','y',' ','E','d','i','t','o','r',' ','V','e','r','s','i','o','n',' ','5','.','0','0','\r','\n'};
        fwrite(unicode_seq, sizeof(BYTE), sizeof(unicode_seq)/sizeof(unicode_seq[0]), file);
        fwrite(header, sizeof(WCHAR), sizeof(header)/sizeof(header[0]), file);
    } else
    {
        fputs("REGEDIT4\r\n", file);
    }

    return file;
}

/******************************************************************************
 * Writes contents of the registry key to the specified file stream.
 *
 * Parameters:
 * file_name - name of a file to export registry branch to.
 * reg_key_name - registry branch to export. The whole registry is exported if
 *      reg_key_name is NULL or contains an empty string.
 */
BOOL export_registry_key(WCHAR *file_name, WCHAR *reg_key_name, DWORD format)
{
    WCHAR *reg_key_name_buf;
    WCHAR *val_name_buf;
    BYTE *val_buf;
    WCHAR *line_buf;
    DWORD reg_key_name_size = KEY_MAX_LEN;
    DWORD val_name_size = KEY_MAX_LEN;
    DWORD val_size = REG_VAL_BUF_SIZE;
    DWORD line_buf_size = KEY_MAX_LEN + REG_VAL_BUF_SIZE;
    FILE *file = NULL;
    BOOL unicode = (format == REG_FORMAT_5);

    reg_key_name_buf = HeapAlloc(GetProcessHeap(), 0,
                                 reg_key_name_size  * sizeof(*reg_key_name_buf));
    val_name_buf = HeapAlloc(GetProcessHeap(), 0,
                             val_name_size * sizeof(*val_name_buf));
    val_buf = HeapAlloc(GetProcessHeap(), 0, val_size);
    line_buf = HeapAlloc(GetProcessHeap(), 0, line_buf_size * sizeof(*line_buf));
    CHECK_ENOUGH_MEMORY(reg_key_name_buf && val_name_buf && val_buf && line_buf);

    if (reg_key_name && reg_key_name[0]) {
        HKEY reg_key_class;
        WCHAR *branch_name = NULL;
        HKEY key;

        REGPROC_resize_char_buffer(&reg_key_name_buf, &reg_key_name_size,
                                   lstrlenW(reg_key_name));
        lstrcpyW(reg_key_name_buf, reg_key_name);

        /* open the specified key */
        if (!(reg_key_class = parse_key_name(reg_key_name, &branch_name))) {
            output_message(STRING_INCORRECT_REG_CLASS, reg_key_name);
            exit(1);
        }
        if (!branch_name || !*branch_name) {
            /* no branch - registry class is specified */
            file = REGPROC_open_export_file(file_name, unicode);
            export_hkey(file, reg_key_class,
                        &reg_key_name_buf, &reg_key_name_size,
                        &val_name_buf, &val_name_size,
                        &val_buf, &val_size, &line_buf,
                        &line_buf_size, unicode);
        } else if (RegOpenKeyW(reg_key_class, branch_name, &key) == ERROR_SUCCESS) {
            file = REGPROC_open_export_file(file_name, unicode);
            export_hkey(file, key,
                        &reg_key_name_buf, &reg_key_name_size,
                        &val_name_buf, &val_name_size,
                        &val_buf, &val_size, &line_buf,
                        &line_buf_size, unicode);
            RegCloseKey(key);
        } else {
            output_message(STRING_REG_KEY_NOT_FOUND, reg_key_name);
        }
    } else {
        unsigned int i;

        /* export all registry classes */
        file = REGPROC_open_export_file(file_name, unicode);
        for (i = 0; i < ARRAY_SIZE(reg_class_keys); i++) {
            /* do not export HKEY_CLASSES_ROOT */
            if (reg_class_keys[i] != HKEY_CLASSES_ROOT &&
                    reg_class_keys[i] != HKEY_CURRENT_USER &&
                    reg_class_keys[i] != HKEY_CURRENT_CONFIG &&
                    reg_class_keys[i] != HKEY_DYN_DATA) {
                lstrcpyW(reg_key_name_buf, reg_class_namesW[i]);
                export_hkey(file, reg_class_keys[i],
                            &reg_key_name_buf, &reg_key_name_size,
                            &val_name_buf, &val_name_size,
                            &val_buf, &val_size, &line_buf,
                            &line_buf_size, unicode);
            }
        }
    }

    if (file) {
        fclose(file);
    }
    HeapFree(GetProcessHeap(), 0, reg_key_name);
    HeapFree(GetProcessHeap(), 0, val_name_buf);
    HeapFree(GetProcessHeap(), 0, val_buf);
    HeapFree(GetProcessHeap(), 0, line_buf);
    return TRUE;
}

/******************************************************************************
 * Reads contents of the specified file into the registry.
 */
BOOL import_registry_file(FILE *reg_file)
{
    BYTE s[2];
    struct parser parser;
    WCHAR *pos;

    if (!reg_file || (fread(s, 2, 1, reg_file) != 1))
        return FALSE;

    parser.is_unicode = (s[0] == 0xff && s[1] == 0xfe);
    get_line = parser.is_unicode ? get_lineW : get_lineA;

    parser.file          = reg_file;
    parser.two_wchars[0] = s[0];
    parser.two_wchars[1] = s[1];
    parser.reg_version   = -1;
    parser.hkey          = NULL;
    parser.key_name      = NULL;
    parser.value_name    = NULL;
    parser.data_type     = 0;
    parser.data          = NULL;
    parser.data_size     = 0;
    parser.hex_type      = FALSE;
    parser.state         = HEADER;

    pos = parser.two_wchars;

    /* parser main loop */
    while (pos)
        pos = (parser_funcs[parser.state])(&parser, pos);

    if (parser.reg_version == REG_VERSION_FUZZY || parser.reg_version == REG_VERSION_INVALID)
        return parser.reg_version == REG_VERSION_FUZZY;

    if (parser.value_name)
        HeapFree(GetProcessHeap(), 0, parser.value_name);

    close_key(&parser);

    return TRUE;
}

/******************************************************************************
 * Removes the registry key with all subkeys. Parses full key name.
 *
 * Parameters:
 * reg_key_name - full name of registry branch to delete. Ignored if is NULL,
 *      empty, points to register key class, does not exist.
 */
void delete_registry_key(WCHAR *reg_key_name)
{
    WCHAR *key_name = NULL;
    HKEY key_class;

    if (!reg_key_name || !reg_key_name[0])
        return;

    if (!(key_class = parse_key_name(reg_key_name, &key_name))) {
        output_message(STRING_INCORRECT_REG_CLASS, reg_key_name);
        exit(1);
    }
    if (!*key_name) {
        output_message(STRING_DELETE_REG_CLASS_FAILED, reg_key_name);
        exit(1);
    }

    RegDeleteTreeW(key_class, key_name);
}
