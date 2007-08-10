/*
 * Registry processing routines. Routines, common for registry
 * processing frontends.
 *
 * Copyright 1999 Sylvain St-Germain
 * Copyright 2002 Andriy Palamarchuk
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
#include <windows.h>
#include <winnt.h>
#include <winreg.h>
#include <assert.h>
#include "regproc.h"

#define REG_VAL_BUF_SIZE        4096

/* maximal number of characters in hexadecimal data line,
   not including '\' character */
#define REG_FILE_HEX_LINE_LEN   76

static const CHAR *reg_class_names[] = {
                                     "HKEY_LOCAL_MACHINE", "HKEY_USERS", "HKEY_CLASSES_ROOT",
                                     "HKEY_CURRENT_CONFIG", "HKEY_CURRENT_USER", "HKEY_DYN_DATA"
                                 };

#define REG_CLASS_NUMBER (sizeof(reg_class_names) / sizeof(reg_class_names[0]))

static HKEY reg_class_keys[REG_CLASS_NUMBER] = {
            HKEY_LOCAL_MACHINE, HKEY_USERS, HKEY_CLASSES_ROOT,
            HKEY_CURRENT_CONFIG, HKEY_CURRENT_USER, HKEY_DYN_DATA
        };

/* return values */
#define NOT_ENOUGH_MEMORY     1
#define IO_ERROR              2

/* processing macros */

/* common check of memory allocation results */
#define CHECK_ENOUGH_MEMORY(p) \
if (!(p)) \
{ \
    fprintf(stderr,"%s: file %s, line %d: Not enough memory\n", \
            getAppName(), __FILE__, __LINE__); \
    exit(NOT_ENOUGH_MEMORY); \
}

/******************************************************************************
 * Converts a hex representation of a DWORD into a DWORD.
 */
static BOOL convertHexToDWord(char* str, DWORD *dw)
{
    char dummy;
    if (strlen(str) > 8 || sscanf(str, "%x%c", dw, &dummy) != 1) {
        fprintf(stderr,"%s: ERROR, invalid hex value\n", getAppName());
        return FALSE;
    }
    return TRUE;
}

/******************************************************************************
 * Converts a hex comma separated values list into a binary string.
 */
static BYTE* convertHexCSVToHex(char *str, DWORD *size)
{
    char *s;
    BYTE *d, *data;

    /* The worst case is 1 digit + 1 comma per byte */
    *size=(strlen(str)+1)/2;
    data=HeapAlloc(GetProcessHeap(), 0, *size);
    CHECK_ENOUGH_MEMORY(data);

    s = str;
    d = data;
    *size=0;
    while (*s != '\0') {
        UINT wc;
        char dummy;

        if (s[1] != ',' && s[1] != '\0' && s[2] != ',' && s[2] != '\0') {
            fprintf(stderr,"%s: ERROR converting CSV hex stream. Invalid sequence at '%s'\n",
                    getAppName(), s);
            HeapFree(GetProcessHeap(), 0, data);
            return NULL;
        }
        if (sscanf(s, "%x%c", &wc, &dummy) < 1 || dummy != ',') {
            fprintf(stderr,"%s: ERROR converting CSV hex stream. Invalid value at '%s'\n",
                    getAppName(), s);
            HeapFree(GetProcessHeap(), 0, data);
            return NULL;
        }
        *d++ =(BYTE)wc;
        (*size)++;

        /* Skip one or two digits and any comma */
        while (*s && *s!=',') s++;
        if (*s) s++;
    }

    return data;
}

/******************************************************************************
 * This function returns the HKEY associated with the data type encoded in the
 * value.  It modifies the input parameter (key value) in order to skip this
 * "now useless" data type information.
 *
 * Note: Updated based on the algorithm used in 'server/registry.c'
 */
static DWORD getDataType(LPSTR *lpValue, DWORD* parse_type)
{
    struct data_type { const char *tag; int len; int type; int parse_type; };

    static const struct data_type data_types[] = {                   /* actual type */  /* type to assume for parsing */
                { "\"",        1,   REG_SZ,              REG_SZ },
                { "str:\"",    5,   REG_SZ,              REG_SZ },
                { "str(2):\"", 8,   REG_EXPAND_SZ,       REG_SZ },
                { "hex:",      4,   REG_BINARY,          REG_BINARY },
                { "dword:",    6,   REG_DWORD,           REG_DWORD },
                { "hex(",      4,   -1,                  REG_BINARY },
                { NULL,        0,    0,                  0 }
            };

    const struct data_type *ptr;
    int type;

    for (ptr = data_types; ptr->tag; ptr++) {
        if (memcmp( ptr->tag, *lpValue, ptr->len ))
            continue;

        /* Found! */
        *parse_type = ptr->parse_type;
        type=ptr->type;
        *lpValue+=ptr->len;
        if (type == -1) {
            char* end;
            /* "hex(xx):" is special */
            type = (int)strtoul( *lpValue , &end, 16 );
            if (**lpValue=='\0' || *end!=')' || *(end+1)!=':') {
                type=REG_NONE;
            } else {
                *lpValue=end+2;
            }
        }
        return type;
    }
    *parse_type=REG_NONE;
    return REG_NONE;
}

/******************************************************************************
 * Replaces escape sequences with the characters.
 */
static void REGPROC_unescape_string(LPSTR str)
{
    int str_idx = 0;            /* current character under analysis */
    int val_idx = 0;            /* the last character of the unescaped string */
    int len = strlen(str);
    for (str_idx = 0; str_idx < len; str_idx++, val_idx++) {
        if (str[str_idx] == '\\') {
            str_idx++;
            switch (str[str_idx]) {
            case 'n':
                str[val_idx] = '\n';
                break;
            case '\\':
            case '"':
                str[val_idx] = str[str_idx];
                break;
            default:
                fprintf(stderr,"Warning! Unrecognized escape sequence: \\%c'\n",
                        str[str_idx]);
                str[val_idx] = str[str_idx];
                break;
            }
        } else {
            str[val_idx] = str[str_idx];
        }
    }
    str[val_idx] = '\0';
}

/******************************************************************************
 * Parses HKEY_SOME_ROOT\some\key\path to get the root key handle and
 * extract the key path (what comes after the first '\').
 */
static BOOL parseKeyName(LPSTR lpKeyName, HKEY *hKey, LPSTR *lpKeyPath)
{
    LPSTR lpSlash;
    unsigned int i, len;

    if (lpKeyName == NULL)
        return FALSE;

    lpSlash = strchr(lpKeyName, '\\');
    if (lpSlash)
    {
        len = lpSlash-lpKeyName;
    }
    else
    {
        len = strlen(lpKeyName);
        lpSlash = lpKeyName+len;
    }
    *hKey = NULL;
    for (i = 0; i < REG_CLASS_NUMBER; i++) {
        if (strncmp(lpKeyName, reg_class_names[i], len) == 0 &&
            len == strlen(reg_class_names[i])) {
            *hKey = reg_class_keys[i];
            break;
        }
    }
    if (*hKey == NULL)
        return FALSE;

    if (*lpSlash != '\0')
        lpSlash++;
    *lpKeyPath = lpSlash;
    return TRUE;
}

/* Globals used by the setValue() & co */
static LPSTR currentKeyName;
static HKEY  currentKeyHandle = NULL;

/******************************************************************************
 * Sets the value with name val_name to the data in val_data for the currently
 * opened key.
 *
 * Parameters:
 * val_name - name of the registry value
 * val_data - registry value data
 */
static LONG setValue(LPSTR val_name, LPSTR val_data)
{
    LONG res;
    DWORD  dwDataType, dwParseType;
    LPBYTE lpbData;
    DWORD  dwData, dwLen;

    if ( (val_name == NULL) || (val_data == NULL) )
        return ERROR_INVALID_PARAMETER;

    if (strcmp(val_data, "-") == 0)
    {
        res=RegDeleteValue(currentKeyHandle,val_name);
        return (res == ERROR_FILE_NOT_FOUND ? ERROR_SUCCESS : res);
    }

    /* Get the data type stored into the value field */
    dwDataType = getDataType(&val_data, &dwParseType);

    if (dwParseType == REG_SZ)          /* no conversion for string */
    {
        REGPROC_unescape_string(val_data);
        /* Compute dwLen after REGPROC_unescape_string because it may
         * have changed the string length and we don't want to store
         * the extra garbage in the registry.
         */
        dwLen = strlen(val_data);
        if (dwLen>0 && val_data[dwLen-1]=='"')
        {
            dwLen--;
            val_data[dwLen]='\0';
        }
        lpbData = (BYTE*) val_data;
    }
    else if (dwParseType == REG_DWORD)  /* Convert the dword types */
    {
        if (!convertHexToDWord(val_data, &dwData))
            return ERROR_INVALID_DATA;
        lpbData = (BYTE*)&dwData;
        dwLen = sizeof(dwData);
    }
    else if (dwParseType == REG_BINARY) /* Convert the binary data */
    {
        lpbData = convertHexCSVToHex(val_data, &dwLen);
        if (!lpbData)
            return ERROR_INVALID_DATA;
    }
    else                                /* unknown format */
    {
        fprintf(stderr,"%s: ERROR, unknown data format\n", getAppName());
        return ERROR_INVALID_DATA;
    }

    res = RegSetValueEx(
               currentKeyHandle,
               val_name,
               0,                  /* Reserved */
               dwDataType,
               lpbData,
               dwLen);
    if (dwParseType == REG_BINARY)
        HeapFree(GetProcessHeap(), 0, lpbData);
    return res;
}

/******************************************************************************
 * A helper function for processRegEntry() that opens the current key.
 * That key must be closed by calling closeKey().
 */
static LONG openKey(LPSTR stdInput)
{
    HKEY keyClass;
    LPSTR keyPath;
    DWORD dwDisp;
    LONG res;

    /* Sanity checks */
    if (stdInput == NULL)
        return ERROR_INVALID_PARAMETER;

    /* Get the registry class */
    if (!parseKeyName(stdInput, &keyClass, &keyPath))
        return ERROR_INVALID_PARAMETER;

    res = RegCreateKeyEx(
               keyClass,                 /* Class     */
               keyPath,                  /* Sub Key   */
               0,                        /* MUST BE 0 */
               NULL,                     /* object type */
               REG_OPTION_NON_VOLATILE,  /* option, REG_OPTION_NON_VOLATILE ... */
               KEY_ALL_ACCESS,           /* access mask, KEY_ALL_ACCESS */
               NULL,                     /* security attribute */
               &currentKeyHandle,        /* result */
               &dwDisp);                 /* disposition, REG_CREATED_NEW_KEY or
                                                        REG_OPENED_EXISTING_KEY */

    if (res == ERROR_SUCCESS)
    {
        currentKeyName = HeapAlloc(GetProcessHeap(), 0, strlen(stdInput)+1);
        CHECK_ENOUGH_MEMORY(currentKeyName);
        strcpy(currentKeyName, stdInput);
    }
    else
    {
        currentKeyHandle = NULL;
    }

    return res;

}

/******************************************************************************
 * Close the currently opened key.
 */
static void closeKey(void)
{
    if (currentKeyHandle)
    {
        HeapFree(GetProcessHeap(), 0, currentKeyName);
        RegCloseKey(currentKeyHandle);
        currentKeyHandle = NULL;
    }
}

/******************************************************************************
 * This function is a wrapper for the setValue function.  It prepares the
 * land and clean the area once completed.
 * Note: this function modifies the line parameter.
 *
 * line - registry file unwrapped line. Should have the registry value name and
 *      complete registry value data.
 */
static void processSetValue(LPSTR line)
{
    LPSTR val_name;                   /* registry value name   */
    LPSTR val_data;                   /* registry value data   */

    int line_idx = 0;                 /* current character under analysis */
    LONG res;

    /* get value name */
    if (line[line_idx] == '@' && line[line_idx + 1] == '=') {
        line[line_idx] = '\0';
        val_name = line;
        line_idx++;
    } else if (line[line_idx] == '\"') {
        line_idx++;
        val_name = line + line_idx;
        while (TRUE) {
            if (line[line_idx] == '\\')   /* skip escaped character */
            {
                line_idx += 2;
            } else {
                if (line[line_idx] == '\"') {
                    line[line_idx] = '\0';
                    line_idx++;
                    break;
                } else {
                    line_idx++;
                }
            }
        }
        if (line[line_idx] != '=') {
            line[line_idx] = '\"';
            fprintf(stderr,"Warning! unrecognized line:\n%s\n", line);
            return;
        }

    } else {
        fprintf(stderr,"Warning! unrecognized line:\n%s\n", line);
        return;
    }
    line_idx++;                   /* skip the '=' character */
    val_data = line + line_idx;

    REGPROC_unescape_string(val_name);
    res = setValue(val_name, val_data);
    if ( res != ERROR_SUCCESS )
        fprintf(stderr,"%s: ERROR Key %s not created. Value: %s, Data: %s\n",
                getAppName(),
                currentKeyName,
                val_name,
                val_data);
}

/******************************************************************************
 * This function receives the currently read entry and performs the
 * corresponding action.
 */
static void processRegEntry(LPSTR stdInput)
{
    /*
     * We encountered the end of the file, make sure we
     * close the opened key and exit
     */
    if (stdInput == NULL) {
        closeKey();
        return;
    }

    if      ( stdInput[0] == '[')      /* We are reading a new key */
    {
        LPSTR keyEnd;
        closeKey();                    /* Close the previous key */

        /* Get rid of the square brackets */
        stdInput++;
        keyEnd = strrchr(stdInput, ']');
        if (keyEnd)
            *keyEnd='\0';

        /* delete the key if we encounter '-' at the start of reg key */
        if ( stdInput[0] == '-')
            delete_registry_key(stdInput+1);
        else if ( openKey(stdInput) != ERROR_SUCCESS )
            fprintf(stderr,"%s: setValue failed to open key %s\n",
                    getAppName(), stdInput);
    } else if( currentKeyHandle &&
               (( stdInput[0] == '@') || /* reading a default @=data pair */
                ( stdInput[0] == '\"'))) /* reading a new value=data pair */
    {
        processSetValue(stdInput);
    } else
    {
        /* Since we are assuming that the file format is valid we must be
         * reading a blank line which indicates the end of this key processing
         */
        closeKey();
    }
}

/******************************************************************************
 * Processes a registry file.
 * Correctly processes comments (in # form), line continuation.
 *
 * Parameters:
 *   in - input stream to read from
 */
void processRegLines(FILE *in)
{
    LPSTR line           = NULL;  /* line read from input stream */
    ULONG lineSize       = REG_VAL_BUF_SIZE;

    line = HeapAlloc(GetProcessHeap(), 0, lineSize);
    CHECK_ENOUGH_MEMORY(line);

    while (!feof(in)) {
        LPSTR s; /* The pointer into line for where the current fgets should read */
        s = line;
        for (;;) {
            size_t size_remaining;
            int size_to_get;
            char *s_eol; /* various local uses */

            /* Do we need to expand the buffer ? */
            assert (s >= line && s <= line + lineSize);
            size_remaining = lineSize - (s-line);
            if (size_remaining < 2) /* room for 1 character and the \0 */
            {
                char *new_buffer;
                size_t new_size = lineSize + REG_VAL_BUF_SIZE;
                if (new_size > lineSize) /* no arithmetic overflow */
                    new_buffer = HeapReAlloc (GetProcessHeap(), 0, line, new_size);
                else
                    new_buffer = NULL;
                CHECK_ENOUGH_MEMORY(new_buffer);
                line = new_buffer;
                s = line + lineSize - size_remaining;
                lineSize = new_size;
                size_remaining = lineSize - (s-line);
            }

            /* Get as much as possible into the buffer, terminated either by
             * eof, error, eol or getting the maximum amount.  Abort on error.
             */
            size_to_get = (size_remaining > INT_MAX ? INT_MAX : size_remaining);
            if (NULL == fgets (s, size_to_get, in)) {
                if (ferror(in)) {
                    perror ("While reading input");
                    exit (IO_ERROR);
                } else {
                    assert (feof(in));
                    *s = '\0';
                    /* It is not clear to me from the definition that the
                     * contents of the buffer are well defined on detecting
                     * an eof without managing to read anything.
                     */
                }
            }

            /* If we didn't read the eol nor the eof go around for the rest */
            s_eol = strchr (s, '\n');
            if (!feof (in) && !s_eol) {
                s = strchr (s, '\0');
                /* It should be s + size_to_get - 1 but this is safer */
                continue;
            }

            /* If it is a comment line then discard it and go around again */
            if (line [0] == '#') {
                s = line;
                continue;
            }

            /* Remove any line feed.  Leave s_eol on the \0 */
            if (s_eol) {
                *s_eol = '\0';
                if (s_eol > line && *(s_eol-1) == '\r')
                    *--s_eol = '\0';
            } else
                s_eol = strchr (s, '\0');

            /* If there is a concatenating \\ then go around again */
            if (s_eol > line && *(s_eol-1) == '\\') {
                int c;
                s = s_eol-1;
                /* The following error protection could be made more self-
                 * correcting but I thought it not worth trying.
                 */
                if ((c = fgetc (in)) == EOF || c != ' ' ||
                        (c = fgetc (in)) == EOF || c != ' ')
                    fprintf(stderr,"%s: ERROR - invalid continuation.\n",
                            getAppName());
                continue;
            }

            break; /* That is the full virtual line */
        }

        processRegEntry(line);
    }
    processRegEntry(NULL);

    HeapFree(GetProcessHeap(), 0, line);
}

/****************************************************************************
 * REGPROC_print_error
 *
 * Print the message for GetLastError
 */

static void REGPROC_print_error(void)
{
    LPVOID lpMsgBuf;
    DWORD error_code;
    int status;

    error_code = GetLastError ();
    status = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL, error_code, 0, (LPTSTR) &lpMsgBuf, 0, NULL);
    if (!status) {
        fprintf(stderr,"%s: Cannot display message for error %d, status %d\n",
                getAppName(), error_code, GetLastError());
        exit(1);
    }
    puts(lpMsgBuf);
    LocalFree((HLOCAL)lpMsgBuf);
    exit(1);
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
static void REGPROC_resize_char_buffer(CHAR **buffer, DWORD *len, DWORD required_len)
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
 * Prints string str to file
 */
static void REGPROC_export_string(FILE *file, CHAR *str)
{
    size_t len = strlen(str);
    size_t i;

    /* escaping characters */
    for (i = 0; i < len; i++) {
        CHAR c = str[i];
        switch (c) {
        case '\\':
            fputs("\\\\", file);
            break;
        case '\"':
            fputs("\\\"", file);
            break;
        case '\n':
            fputs("\\\n", file);
            break;
        default:
            fputc(c, file);
            break;
        }
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
 * reg_key_name_len - length of the buffer for the registry class in characters.
 * val_name_buf - buffer for storing value name.
 *      Is resized if necessary.
 * val_name_len - length of the buffer for storing value names in characters.
 * val_buf - buffer for storing values while extracting.
 *      Is resized if necessary.
 * val_size - size of the buffer for storing values in bytes.
 */
static void export_hkey(FILE *file, HKEY key,
                 CHAR **reg_key_name_buf, DWORD *reg_key_name_len,
                 CHAR **val_name_buf, DWORD *val_name_len,
                 BYTE **val_buf, DWORD *val_size)
{
    DWORD max_sub_key_len;
    DWORD max_val_name_len;
    DWORD max_val_size;
    DWORD curr_len;
    DWORD i;
    BOOL more_data;
    LONG ret;

    /* get size information and resize the buffers if necessary */
    if (RegQueryInfoKey(key, NULL, NULL, NULL, NULL,
                        &max_sub_key_len, NULL,
                        NULL, &max_val_name_len, &max_val_size, NULL, NULL
                       ) != ERROR_SUCCESS) {
        REGPROC_print_error();
    }
    curr_len = strlen(*reg_key_name_buf);
    REGPROC_resize_char_buffer(reg_key_name_buf, reg_key_name_len,
                               max_sub_key_len + curr_len + 1);
    REGPROC_resize_char_buffer(val_name_buf, val_name_len,
                               max_val_name_len);
    if (max_val_size > *val_size) {
        *val_size = max_val_size;
        if (!*val_buf) *val_buf = HeapAlloc(GetProcessHeap(), 0, *val_size);
        else *val_buf = HeapReAlloc(GetProcessHeap(), 0, *val_buf, *val_size);
        CHECK_ENOUGH_MEMORY(val_buf);
    }

    /* output data for the current key */
    fputs("\n[", file);
    fputs(*reg_key_name_buf, file);
    fputs("]\n", file);
    /* print all the values */
    i = 0;
    more_data = TRUE;
    while(more_data) {
        DWORD value_type;
        DWORD val_name_len1 = *val_name_len;
        DWORD val_size1 = *val_size;
        ret = RegEnumValue(key, i, *val_name_buf, &val_name_len1, NULL,
                           &value_type, *val_buf, &val_size1);
        if (ret != ERROR_SUCCESS) {
            more_data = FALSE;
            if (ret != ERROR_NO_MORE_ITEMS) {
                REGPROC_print_error();
            }
        } else {
            i++;

            if ((*val_name_buf)[0]) {
                fputs("\"", file);
                REGPROC_export_string(file, *val_name_buf);
                fputs("\"=", file);
            } else {
                fputs("@=", file);
            }

            switch (value_type) {
            case REG_SZ:
            case REG_EXPAND_SZ:
                fputs("\"", file);
                REGPROC_export_string(file, (char*) *val_buf);
                fputs("\"\n", file);
                break;

            case REG_DWORD:
                fprintf(file, "dword:%08x\n", *((DWORD *)*val_buf));
                break;

            default:
                fprintf(stderr,"%s: warning - unsupported registry format '%d', "
                        "treat as binary\n",
                        getAppName(), value_type);
                fprintf(stderr,"key name: \"%s\"\n", *reg_key_name_buf);
                fprintf(stderr,"value name:\"%s\"\n\n", *val_name_buf);
                /* falls through */
            case REG_MULTI_SZ:
                /* falls through */
            case REG_BINARY: {
                    DWORD i1;
                    const CHAR *hex_prefix;
                    CHAR buf[20];
                    int cur_pos;

                    if (value_type == REG_BINARY) {
                        hex_prefix = "hex:";
                    } else {
                        hex_prefix = buf;
                        sprintf(buf, "hex(%d):", value_type);
                    }

                    /* position of where the next character will be printed */
                    /* NOTE: yes, strlen("hex:") is used even for hex(x): */
                    cur_pos = strlen("\"\"=") + strlen("hex:") +
                              strlen(*val_name_buf);

                    fputs(hex_prefix, file);
                    for (i1 = 0; i1 < val_size1; i1++) {
                        fprintf(file, "%02x", (unsigned int)(*val_buf)[i1]);
                        if (i1 + 1 < val_size1) {
                            fputs(",", file);
                        }
                        cur_pos += 3;

                        /* wrap the line */
                        if (cur_pos > REG_FILE_HEX_LINE_LEN) {
                            fputs("\\\n  ", file);
                            cur_pos = 2;
                        }
                    }
                    fputs("\n", file);
                    break;
                }
            }
        }
    }

    i = 0;
    more_data = TRUE;
    (*reg_key_name_buf)[curr_len] = '\\';
    while(more_data) {
        DWORD buf_len = *reg_key_name_len - curr_len;

        ret = RegEnumKeyEx(key, i, *reg_key_name_buf + curr_len + 1, &buf_len,
                           NULL, NULL, NULL, NULL);
        if (ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA) {
            more_data = FALSE;
            if (ret != ERROR_NO_MORE_ITEMS) {
                REGPROC_print_error();
            }
        } else {
            HKEY subkey;

            i++;
            if (RegOpenKey(key, *reg_key_name_buf + curr_len + 1,
                           &subkey) == ERROR_SUCCESS) {
                export_hkey(file, subkey, reg_key_name_buf, reg_key_name_len,
                            val_name_buf, val_name_len, val_buf, val_size);
                RegCloseKey(subkey);
            } else {
                REGPROC_print_error();
            }
        }
    }
    (*reg_key_name_buf)[curr_len] = '\0';
}

/******************************************************************************
 * Open file for export.
 */
static FILE *REGPROC_open_export_file(CHAR *file_name)
{
    FILE *file = fopen(file_name, "w");
    if (!file) {
        perror("");
        fprintf(stderr,"%s: Can't open file \"%s\"\n", getAppName(), file_name);
        exit(1);
    }
    fputs("REGEDIT4\n", file);
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
BOOL export_registry_key(CHAR *file_name, CHAR *reg_key_name)
{
    CHAR *reg_key_name_buf;
    CHAR *val_name_buf;
    BYTE *val_buf;
    DWORD reg_key_name_len = KEY_MAX_LEN;
    DWORD val_name_len = KEY_MAX_LEN;
    DWORD val_size = REG_VAL_BUF_SIZE;
    FILE *file = NULL;

    reg_key_name_buf = HeapAlloc(GetProcessHeap(), 0,
                                 reg_key_name_len  * sizeof(*reg_key_name_buf));
    val_name_buf = HeapAlloc(GetProcessHeap(), 0,
                             val_name_len * sizeof(*val_name_buf));
    val_buf = HeapAlloc(GetProcessHeap(), 0, val_size);
    CHECK_ENOUGH_MEMORY(reg_key_name_buf && val_name_buf && val_buf);

    if (reg_key_name && reg_key_name[0]) {
        HKEY reg_key_class;
        CHAR *branch_name;
        HKEY key;

        REGPROC_resize_char_buffer(&reg_key_name_buf, &reg_key_name_len,
                                   strlen(reg_key_name));
        strcpy(reg_key_name_buf, reg_key_name);

        /* open the specified key */
        if (!parseKeyName(reg_key_name, &reg_key_class, &branch_name)) {
            fprintf(stderr,"%s: Incorrect registry class specification in '%s'\n",
                    getAppName(), reg_key_name);
            exit(1);
        }
        if (!branch_name[0]) {
            /* no branch - registry class is specified */
            file = REGPROC_open_export_file(file_name);
            export_hkey(file, reg_key_class,
                        &reg_key_name_buf, &reg_key_name_len,
                        &val_name_buf, &val_name_len,
                        &val_buf, &val_size);
        } else if (RegOpenKey(reg_key_class, branch_name, &key) == ERROR_SUCCESS) {
            file = REGPROC_open_export_file(file_name);
            export_hkey(file, key,
                        &reg_key_name_buf, &reg_key_name_len,
                        &val_name_buf, &val_name_len,
                        &val_buf, &val_size);
            RegCloseKey(key);
        } else {
            fprintf(stderr,"%s: Can't export. Registry key '%s' does not exist!\n",
                    getAppName(), reg_key_name);
            REGPROC_print_error();
        }
    } else {
        unsigned int i;

        /* export all registry classes */
        file = REGPROC_open_export_file(file_name);
        for (i = 0; i < REG_CLASS_NUMBER; i++) {
            /* do not export HKEY_CLASSES_ROOT */
            if (reg_class_keys[i] != HKEY_CLASSES_ROOT &&
                    reg_class_keys[i] != HKEY_CURRENT_USER &&
                    reg_class_keys[i] != HKEY_CURRENT_CONFIG &&
                    reg_class_keys[i] != HKEY_DYN_DATA) {
                strcpy(reg_key_name_buf, reg_class_names[i]);
                export_hkey(file, reg_class_keys[i],
                            &reg_key_name_buf, &reg_key_name_len,
                            &val_name_buf, &val_name_len,
                            &val_buf, &val_size);
            }
        }
    }

    if (file) {
        fclose(file);
    }
    HeapFree(GetProcessHeap(), 0, reg_key_name);
    HeapFree(GetProcessHeap(), 0, val_buf);
    return TRUE;
}

/******************************************************************************
 * Reads contents of the specified file into the registry.
 */
BOOL import_registry_file(LPTSTR filename)
{
    FILE* reg_file = fopen(filename, "r");

    if (reg_file) {
        processRegLines(reg_file);
        return TRUE;
    }
    return FALSE;
}

/******************************************************************************
 * Recursive function which removes the registry key with all subkeys.
 */
static void delete_branch(HKEY key,
                   CHAR **reg_key_name_buf, DWORD *reg_key_name_len)
{
    HKEY branch_key;
    DWORD max_sub_key_len;
    DWORD subkeys;
    DWORD curr_len;
    LONG ret;
    long int i;

    if (RegOpenKey(key, *reg_key_name_buf, &branch_key) != ERROR_SUCCESS) {
        REGPROC_print_error();
    }

    /* get size information and resize the buffers if necessary */
    if (RegQueryInfoKey(branch_key, NULL, NULL, NULL,
                        &subkeys, &max_sub_key_len,
                        NULL, NULL, NULL, NULL, NULL, NULL
                       ) != ERROR_SUCCESS) {
        REGPROC_print_error();
    }
    curr_len = strlen(*reg_key_name_buf);
    REGPROC_resize_char_buffer(reg_key_name_buf, reg_key_name_len,
                               max_sub_key_len + curr_len + 1);

    (*reg_key_name_buf)[curr_len] = '\\';
    for (i = subkeys - 1; i >= 0; i--) {
        DWORD buf_len = *reg_key_name_len - curr_len;

        ret = RegEnumKeyEx(branch_key, i, *reg_key_name_buf + curr_len + 1,
                           &buf_len, NULL, NULL, NULL, NULL);
        if (ret != ERROR_SUCCESS &&
                ret != ERROR_MORE_DATA &&
                ret != ERROR_NO_MORE_ITEMS) {
            REGPROC_print_error();
        } else {
            delete_branch(key, reg_key_name_buf, reg_key_name_len);
        }
    }
    (*reg_key_name_buf)[curr_len] = '\0';
    RegCloseKey(branch_key);
    RegDeleteKey(key, *reg_key_name_buf);
}

/******************************************************************************
 * Removes the registry key with all subkeys. Parses full key name.
 *
 * Parameters:
 * reg_key_name - full name of registry branch to delete. Ignored if is NULL,
 *      empty, points to register key class, does not exist.
 */
void delete_registry_key(CHAR *reg_key_name)
{
    CHAR *key_name;
    HKEY key_class;
    HKEY branch_key;

    if (!reg_key_name || !reg_key_name[0])
        return;

    if (!parseKeyName(reg_key_name, &key_class, &key_name)) {
        fprintf(stderr,"%s: Incorrect registry class specification in '%s'\n",
                getAppName(), reg_key_name);
        exit(1);
    }
    if (!*key_name) {
        fprintf(stderr,"%s: Can't delete registry class '%s'\n",
                getAppName(), reg_key_name);
        exit(1);
    }

    /* open the specified key to make sure it exists */
    if (RegOpenKey(key_class, key_name, &branch_key) == ERROR_SUCCESS) {
        CHAR *branch_name;
        DWORD branch_name_len;
        RegCloseKey(branch_key);

        /* Copy the key name to a new buffer that delete_branch() can
         * reallocate as needed
         */
        branch_name_len = strlen(key_name);
        branch_name = HeapAlloc(GetProcessHeap(), 0, branch_name_len+1);
        CHECK_ENOUGH_MEMORY(branch_name);
        strcpy(branch_name, key_name);
        delete_branch(key_class, &branch_name, &branch_name_len);
        HeapFree(GetProcessHeap(), 0, branch_name);
    }
}
