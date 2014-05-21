/*
 * Generic freedesktop.org support code
 *
 * Copyright (C) 2006 Mikolaj Zalewski
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
 *
 *
 * XDG_UserDirLookup() and helper functions are based on code from:
 * http://www.freedesktop.org/wiki/Software/xdg-user-dirs
 *
 * Copyright (c) 2007 Red Hat, inc
 *
 * From the xdg-user-dirs license:
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <errno.h>
 
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "shell32_main.h"
#include "xdg.h"

WINE_DEFAULT_DEBUG_CHANNEL(xdg);

/*
 * XDG paths implemented using Desktop Base Directory spec version 0.6
 * (from http://standards.freedesktop.org/basedir-spec/basedir-spec-0.6.html)
 */

static CRITICAL_SECTION XDG_PathsLock;
static CRITICAL_SECTION_DEBUG XDG_PathsLock_Debug =
{
    0, 0, &XDG_PathsLock,
    { &XDG_PathsLock_Debug.ProcessLocksList,
      &XDG_PathsLock_Debug.ProcessLocksList},
    0, 0, { (DWORD_PTR)__FILE__ ": XDG_PathsLock"}
};
static CRITICAL_SECTION XDG_PathsLock = { &XDG_PathsLock_Debug, -1, 0, 0, 0, 0 };

typedef struct
{
    const char *var_name;
    const char *default_value;
} std_path;

static const std_path paths[] = {
    {"XDG_DATA_HOME", "$HOME/.local/share"},
    {"XDG_CONFIG_HOME", "$HOME/.config"},
    {"XDG_DATA_DIRS", "/usr/local/share:/usr/share"},
    {"XDG_CONFIG_DIRS", "/etc/xdg"},
    {"XDG_CACHE_HOME", "$HOME/.cache"}
};

#define PATHS_COUNT (sizeof(paths)/sizeof(paths[0]))

/* will be filled with paths as they are computed */
static const char *path_values[PATHS_COUNT] = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static char *load_path(int path_id)
{
    char *env = getenv(paths[path_id].var_name);
    char *ret;
    
    if (env != NULL && env[0]=='/')
    {
        ret = SHAlloc(strlen(env)+1);
        if (ret != NULL)
            lstrcpyA(ret, env);
        return ret;
    }
    
    if (memcmp(paths[path_id].default_value, "$HOME", 5)==0)
    {
	char *home = getenv("HOME");
	int len;

        if (!home) return NULL;
	ret = SHAlloc(strlen(home)+strlen(paths[path_id].default_value)-5+1);
	if (ret == NULL) return NULL;
	
	lstrcpyA(ret, home);
	len = strlen(ret);
	if (len>0 && ret[len-1]=='/')
	    ret[--len]=0;
	lstrcatA(ret, paths[path_id].default_value+5);
	return ret;
    }
    
    ret = SHAlloc(strlen(paths[path_id].default_value)+1);
    if (ret != NULL)
        lstrcpyA(ret, paths[path_id].default_value);
    return ret;
}

/******************************************************************************
 * XDG_GetPath    [internal]
 *
 * Get one of the XDG standard patch. The return value shouldn't be modified nor
 * freed. A return value of NULL means that the memory is exhausted or the input
 * is invalid
 *
 * For XDG_DATA_HOME, XDG_CONFIG_HOME and XDG_CACHE_HOME the result is a Unix path.
 * For XDG_DATA_DIRS and XDG_CONFIG_DIRS the result is a colon-separated list of Unix
 * paths
 *
 * The paths are guaranteed to start with '/'
 */
static const char *XDG_GetPath(int path_id)
{
    if (path_id >= PATHS_COUNT || path_id < 0)
    {
	ERR("Invalid path_id %d\n", path_id);
	return NULL;
    }
    
    if (path_values[path_id] != NULL)
	return path_values[path_id];
    EnterCriticalSection(&XDG_PathsLock);
    if (path_values[path_id] == NULL)
	path_values[path_id] = load_path(path_id);
    LeaveCriticalSection(&XDG_PathsLock);
    return path_values[path_id];
}

/******************************************************************************
 * XDG_BuildPath    [internal]
 *
 * Build a string with a subpath of one of the XDG standard paths.
 * The root can be one of XDG_DATA_HOME, XDG_CONFIG_HOME and XDG_CACHE_HOME.
 * The subpath is a path relative to that root (it shouldn't start with a slash)
 *
 * The returned path should be freed with SHFree. A return of NULL means that the
 * memory is exhausted or the parameters are invalid
 */
char *XDG_BuildPath(int root_id, const char *subpath)
{
    const char *root_path = XDG_GetPath(root_id);
    char *ret_buffer;
    int root_len;
    
    if (root_id == XDG_DATA_DIRS || root_id == XDG_CONFIG_DIRS)
    {
        ERR("Invalid path id %d\n", root_id);
        return NULL;
    }
    
    if (root_path == NULL) return NULL;
    root_len = strlen(root_path);
    if (root_path[root_len-1]=='/') root_len--;
    ret_buffer = SHAlloc(root_len+1+strlen(subpath)+1);
    if (ret_buffer == NULL) return NULL;
    lstrcpyA(ret_buffer, root_path);
    ret_buffer[root_len]='/';
    lstrcpyA(ret_buffer+root_len+1, subpath);
    return ret_buffer;
}

/******************************************************************************
 * XDG_MakeDirs    [internal]
 *
 * Checks that all the directories on the specified path exists. If some don't exists
 * they are created with mask 0700 as required by many the freedeskop.org specs.
 * If the path doesn't end with '/' it is assumed to be a path to a file and the last
 * segment is not checked
 *
 * In case of a failure the errno is always set and can be used e.g for debugging
 *
 * RETURNS
 *   TRUE on success, FALSE on error
 */
BOOL XDG_MakeDirs(const char *path)
{
    int last_slash = 0;
    BOOL success = TRUE;
    struct stat tmp;
    char *buffer = SHAlloc(strlen(path)+1);
    
    if (buffer == NULL)
    {
        errno = ENOMEM;
        return FALSE;
    }
    lstrcpyA(buffer, path);
    
    TRACE("(%s)\n", debugstr_a(path));
    while (1)
    {
        char *slash=strchr(buffer+last_slash+1, '/');
        if (slash==NULL)
            break;
        
        /* cut the string at that position and create the directory if it doesn't exist */
        *slash=0;
        TRACE("Checking path %s\n", debugstr_a(buffer));
        success = (stat(buffer, &tmp)==0);
        if (!success && errno==ENOENT)
        {
            TRACE("Creating\n");
            success = (mkdir(buffer, 0700)==0);
        }
        if (!success)
        {
            WARN("Couldn't process directory %s (errno=%d)\n", debugstr_a(buffer), errno);
            break;
        }
        *slash='/';
        last_slash = slash-buffer;
    }
    SHFree(buffer);
    return success;
}

/*
 * .desktop files functions
 */


/******************************************************************************
 * dskentry_encode    [internal]
 *
 * Escape the characters that can't be present in a desktop entry value like \n, leading
 * spaces etc. The output parameter may be NULL. Then only the number of characters will
 * be computers.
 *
 * RETURNS
 *   The number of characters after escaping the special characters, including the
 * terminating NUL.
 */
static int dskentry_encode(const char *value, char *output)
{
    BOOL only_spc = TRUE;
    int num_written = 0;
    const char *c;
    for (c = value; *c; c++)
    {
        if (only_spc && *c==' ')
        {
            if (output)
            {
                *(output++) = '\\';
                *(output++) = 's';
            }
            num_written += 2;
            continue;
        }
        only_spc = FALSE;
        
        if (*c=='\t' || *c=='\r' || *c=='\n' || *c=='\\')
        {
            if (output)
            {
                *(output++) = '\\';
                if (*c=='\t') *(output++) = 't';
                if (*c=='\r') *(output++) = 'r';
                if (*c=='\n') *(output++) = 'n';
                if (*c=='\\') *(output++) = '\\';
            }
            num_written += 2;
        }
        else
        {
            if (output)
                *(output++)=*c;
            num_written++;
        }
    }
    
    if (output)
        *(output++) = 0;
    num_written++;
    return num_written;
}

/******************************************************************************
 * dskentry_decode    [internal]
 *
 * Unescape the characters that can be escaped according to the desktop entry spec.
 * The output parameter may be NULL. Then only the number of characters will
 * be computers.
 *
 * RETURNS
 *   The number of characters after unescaping the special characters, including the
 * terminating NUL.
 */
static int dskentry_decode(const char *value, int len, char *output)
{
    int pos = 0;
    int count = 0;
    while (pos<len)
    {
        char c;
        if (value[pos] == '\\' && pos<len-1)
        {
            pos++;
            switch (value[pos])
            {
                case 's': c = ' '; break;
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = 'r'; break;
                case '\\': c = '\\'; break;
                default:
                    /* store both the backslash and the character */
                    if (output)
                        *(output++) = '\\';
                    count++;
                    c = value[pos];
                    break;
            }
        }
        else
            c = value[pos];
            
        if (output)
            *(output++) = c;
        count++;
        pos++;
    }
    
    if (output)
        *(output++) = 0;
    count++;
    return count;
}


/******************************************************************************
 * url_encode    [internal]
 *
 * URL-encode the given string (i.e. use escape codes like %20). Note that an
 * URL-encoded string can be used as a value in desktop entry files as all
 * unsafe characters are escaped.
 *
 * The output can be NULL. Then only the number of characters will be counted
 *
 * RETURNS
 *   The number of characters after escaping the special characters, including the
 * terminating NUL.
 */
static int url_encode(const char *value, char *output)
{
    static const char unsafechars[] = "^&`{}|[]'<>\\#%\"+";
    static const char hexchars[] = "0123456789ABCDEF";
    int num_written = 0;
    const char *c;

    for (c = value; *c; c++)
    {
        if (*c<=0x20 || *c>=0x7f || strchr(unsafechars, *c))
        {
            if (output)
            {
                *(output++) = '%';
                *(output++) = hexchars[(unsigned char)*c / 16];
                *(output++) = hexchars[(unsigned char)*c % 16];
            }
            num_written += 3;
        }
        else
        {
            if (output)
                *(output++) = *c;
            num_written++;
        }
    }

    if (output)
        *(output++) = 0;
    num_written++;

    return num_written;
}

static int decode_url_code(const char *c)
{
    const char *p1, *p2;
    int v1, v2;
    static const char hexchars[] = "0123456789ABCDEF";
    if (*c == 0)
        return -1;

    p1 = strchr(hexchars, toupper(*c));
    p2 = strchr(hexchars, toupper(*(c+1)));
    if (p1 == NULL || p2 == NULL)
        return -1;
    v1 = (int)(p1 - hexchars);
    v2 = (int)(p2 - hexchars);
    return (v1<<4) + v2;
}

/******************************************************************************
 * url_decode    [internal]
 *
 * URL-decode the given string (i.e. unescape codes like %20). The decoded string
 * will never be longer than the encoded one. The decoding can be done in place - the
 * output variable can point to the value buffer.
 *
 * output should not be NULL
 */
static void url_decode(const char *value, char *output)
{
    const char *c = value;
    while (*c)
    {
        if (*c == '%')
        {
            int v = decode_url_code(c+1);
            if (v != -1)
            {
                *(output++) = v;
                c += 3;
                continue;
            }
        }
        
        *(output++) = *c;
        c++;
    }
    *output = 0;
}

static int escape_value(const char *value, DWORD dwFlags, char *output)
{
    if (dwFlags & XDG_URLENCODE)
        return url_encode(value, output);
    return dskentry_encode(value, output);
}

/******************************************************************************
 * XDG_WriteDesktopStringEntry    [internal]
 *
 * Writes a key=value pair into the specified file descriptor.
 *
 * RETURNS
 *   TRUE on success, else FALSE
 */
BOOL XDG_WriteDesktopStringEntry(int writer, const char *keyName, DWORD dwFlags, const char *value)
{
    int keyLen = lstrlenA(keyName);
    int valueLen = escape_value(value, dwFlags, NULL);
    char *string = SHAlloc(keyLen+1+valueLen);
    BOOL ret;
    
    if (string == NULL)
        return FALSE;
    lstrcpyA(string, keyName);
    string[keyLen] = '=';
    escape_value(value, dwFlags, string+keyLen+1);
    string[keyLen+1+valueLen-1]='\n';   /* -1 because valueLen contains the last NUL character */
    ret = (write(writer, string, keyLen+1+valueLen)!=-1);
    SHFree(string);
    return ret;
}

typedef struct
{
    char *str;
    int len;
} PARSED_STRING;

typedef struct tagPARSED_ENTRY PARSED_ENTRY;
struct tagPARSED_ENTRY
{
    PARSED_STRING name;
    PARSED_STRING equals;
    PARSED_STRING value;
    PARSED_ENTRY *next;
};

typedef struct tagPARSED_GROUP PARSED_GROUP;
struct tagPARSED_GROUP
{
    PARSED_STRING name;
    PARSED_ENTRY *entries;
    PARSED_GROUP *next;
};


struct tagXDG_PARSED_FILE
{
    char *contents;
    PARSED_ENTRY *head_comments;
    PARSED_GROUP *groups;
};

static BOOL parsed_str_eq(const PARSED_STRING *str1, const char *str2)
{
    if (strncmp(str1->str, str2, str1->len) != 0)
        return FALSE;
    if (str2[str1->len] != 0)
        return FALSE;
    return TRUE;
}

static void free_entries_list(PARSED_ENTRY *first)
{
    PARSED_ENTRY *next;
    while (first)
    {
        next = first->next;
        SHFree(first);
        first = next;
    }
}

void XDG_FreeParsedFile(XDG_PARSED_FILE *parsed)
{
    PARSED_GROUP *group, *next;
    if (!parsed)
        return;
    free_entries_list(parsed->head_comments);
    
    group = parsed->groups;
    while (group)
    {
        next = group->next;
        free_entries_list(group->entries);
        SHFree(group);
        group = next;
    }
    SHFree(parsed->contents);
    SHFree(parsed);
}

#define LINE_GROUP   1
#define LINE_ENTRY   2
#define LINE_COMMENT 3

static int parse_line(char *content, PARSED_ENTRY *output, int *outType)
{
    char *end;
    
    ZeroMemory(output, sizeof(PARSED_ENTRY));
    end = strchr(content, '\n');
    if (end == NULL)
        end = content + strlen(content) - 1;
    
    if (*content == '#')
    {
        *outType = LINE_COMMENT;
        output->equals.str = content;
        output->equals.len = end - content;
        if (*end != '\n')
            output->equals.len++;
    }
    else if (*content == '[')
    {
        char *last_char = end;
        
        *outType = LINE_GROUP;
        
        /* the standard says nothing about skipping white spaces but e.g. KDE accepts such files */
        while (isspace(*last_char))
            last_char--;
        if (*last_char != ']')
            return -1;
        output->name.str = content + 1;
        output->name.len = last_char - (content + 1);
    }
    else
    {
        /* 'name = value' line */
        char *equal, *eq_begin, *eq_end;
        
        *outType = LINE_ENTRY;
        
        equal = strchr(content, '=');
        if (equal == NULL || equal > end)
            return -1;
        for (eq_begin = equal-1;  isspace(*eq_begin) && eq_begin >= content; eq_begin--)
            ;
        for (eq_end = equal+1; isspace(*eq_end) && *eq_end != '\n'; eq_end++)
            ;

        output->name.str = content;
        output->name.len = eq_begin - content + 1;

        output->equals.str = eq_begin + 1;
        output->equals.len = eq_end - eq_begin - 1;
        
        output->value.str = eq_end;
        output->value.len = end - eq_end;

        if (*end != '\n')
            output->value.len++;
    }
    return end - content + 1;
}

XDG_PARSED_FILE *XDG_ParseDesktopFile(int fd)
{
    struct stat stats;
    XDG_PARSED_FILE *parsed = NULL;
    PARSED_ENTRY **curr_entry;
    PARSED_GROUP **curr_group;
    BOOL is_in_group = FALSE;
    
    int pos = 0;
    
    if (fstat(fd, &stats) == -1) goto failed;
    parsed = SHAlloc(sizeof(XDG_PARSED_FILE));
    if (parsed == NULL) goto failed;
    parsed->groups = NULL;
    parsed->head_comments = NULL;
    parsed->contents = SHAlloc(stats.st_size+1);
    if (parsed->contents == NULL) goto failed;
    
    curr_entry = &parsed->head_comments;
    curr_group = &parsed->groups;
    
    if (read(fd, parsed->contents, stats.st_size) == -1) goto failed;
    parsed->contents[stats.st_size] = 0;

    while (pos < stats.st_size)
    {
        PARSED_ENTRY statement;
        int type, size;

        size = parse_line(parsed->contents + pos, &statement, &type);
        if (size == -1) goto failed;
        if (size == 0)
            break;
        pos += size;
        
        switch (type)
        {
            case LINE_GROUP:
            {
                PARSED_GROUP *group = SHAlloc(sizeof(PARSED_GROUP));
                if (group == NULL) goto failed;
                is_in_group = TRUE;
                
                group->name = statement.name;
                group->entries = NULL;
                group->next = NULL;
                *curr_group = group;
                curr_group = &group->next;
                curr_entry = &group->entries;
                break;
            }

            case LINE_ENTRY:
                if (!is_in_group) goto failed;
                /* fall through */
            case LINE_COMMENT:
            {
                PARSED_ENTRY *new_stat = SHAlloc(sizeof(PARSED_ENTRY));
                if (new_stat == NULL) goto failed;
                *new_stat = statement;
                new_stat->next = NULL;
                *curr_entry = new_stat;
                curr_entry = &new_stat->next;
                break;
            }
        }
    }
    return parsed;
    
failed:
    XDG_FreeParsedFile(parsed);
    return NULL;
}

char *XDG_GetStringValue(XDG_PARSED_FILE *file, const char *group_name, const char *value_name, DWORD dwFlags)
{
    PARSED_GROUP *group;
    PARSED_ENTRY *entry;

    for (group = file->groups; group; group = group->next)
    {
        if (!parsed_str_eq(&group->name, group_name))
            continue;

        for (entry = group->entries; entry; entry = entry->next)
            if (entry->name.str != NULL && parsed_str_eq(&entry->name, value_name))
            {
                int len;
                char *ret;
                
                len = dskentry_decode(entry->value.str, entry->value.len, NULL);
                ret = SHAlloc(len);
                if (ret == NULL) return NULL;
                dskentry_decode(entry->value.str, entry->value.len, ret);
                if (dwFlags & XDG_URLENCODE)
                    url_decode(ret, ret);
                return ret;
            }
    }
    
    return NULL;
}

/* Get the name of the xdg configuration file.
 *
 * [in] home_dir - $HOME
 * [out] config_file - the name of the configuration file
 */
static HRESULT get_xdg_config_file(char * home_dir, char ** config_file)
{
    char *config_home;

    config_home = getenv("XDG_CONFIG_HOME");
    if (!config_home || !config_home[0])
    {
        *config_file = HeapAlloc(GetProcessHeap(), 0, strlen(home_dir) + strlen("/.config/user-dirs.dirs") + 1);
        if (!*config_file)
            return E_OUTOFMEMORY;

        strcpy(*config_file, home_dir);
        strcat(*config_file, "/.config/user-dirs.dirs");
    }
    else
    {
        *config_file = HeapAlloc(GetProcessHeap(), 0, strlen(config_home) + strlen("/user-dirs.dirs") + 1);
        if (!*config_file)
            return E_OUTOFMEMORY;

        strcpy(*config_file, config_home);
        strcat(*config_file, "/user-dirs.dirs");
    }
    return S_OK;
}

/* Parse the key in a line in the xdg-user-dir config file.
 * i.e. XDG_PICTURES_DIR="$HOME/Pictures"
 *      ^                ^
 *
 * [in] xdg_dirs - array of xdg directories to look for
 * [in] num_dirs - number of elements in xdg_dirs
 * [in/out] p_ptr - pointer to where we are in the buffer
 * Returns the index to xdg_dirs if we find the key, or -1 on error.
 */
static int parse_config1(const char * const *xdg_dirs, const unsigned int num_dirs, char ** p_ptr)
{
    char *p;
    int i;

    p = *p_ptr;
    while (*p == ' ' || *p == '\t')
        p++;
    if (strncmp(p, "XDG_", 4))
        return -1;

    p += 4;
    for (i = 0; i < num_dirs; i++)
    {
        if (!strncmp(p, xdg_dirs[i], strlen(xdg_dirs[i])))
        {
            p += strlen(xdg_dirs[i]);
            break;
        }
    }
    if (i == num_dirs)
        return -1;
    if (strncmp(p, "_DIR", 4))
        return -1;
    p += 4;
    while (*p == ' ' || *p == '\t')
        p++;
    if (*p != '=')
        return -1;
    p++;
    while (*p == ' ' || *p == '\t')
        p++;
    if (*p != '"')
        return -1;
    p++;

    *p_ptr = p;
    return i;
}

/* Parse the value in a line in the xdg-user-dir config file.
 * i.e. XDG_PICTURES_DIR="$HOME/Pictures"
 *                        ^             ^
 *
 * [in] p - pointer to the buffer
 * [in] home_dir - $HOME
 * [out] out_ptr - the directory name
 */
static HRESULT parse_config2(char * p, const char * home_dir, char ** out_ptr)
{
    BOOL relative;
    char *out, *d;

    relative = FALSE;

    if (!strncmp(p, "$HOME/", 6))
    {
        p += 6;
        relative = TRUE;
    }
    else if (*p != '/')
        return E_FAIL;

    if (relative)
    {
        out = HeapAlloc(GetProcessHeap(), 0, strlen(home_dir) + strlen(p) + 2);
        if (!out)
            return E_OUTOFMEMORY;

        strcpy(out, home_dir);
        strcat(out, "/");
    }
    else
    {
        out = HeapAlloc(GetProcessHeap(), 0, strlen(p) + 1);
        if (!out)
            return E_OUTOFMEMORY;
        *out = 0;
    }

    d = out + strlen(out);
    while (*p && *p != '"')
    {
        if ((*p == '\\') && (*(p + 1) != 0))
            p++;
        *d++ = *p++;
    }
    *d = 0;
    *out_ptr = out;
    return S_OK;
}

/* Parse part of a line in the xdg-user-dir config file.
 * i.e. XDG_PICTURES_DIR="$HOME/Pictures"
 *                        ^             ^
 *
 * The calling function is responsible for freeing all elements of out_ptr as
 * well as out_ptr itself.
 *
 * [in] xdg_dirs - array of xdg directories to look for
 * [in] num_dirs - number of elements in xdg_dirs
 * [out] out_ptr - an array of the xdg directories names
 */
HRESULT XDG_UserDirLookup(const char * const *xdg_dirs, const unsigned int num_dirs, char *** out_ptr)
{
    FILE *file;
    char **out;
    char *home_dir, *config_file;
    char buffer[512];
    int len;
    unsigned int i;
    HRESULT hr;

    *out_ptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, num_dirs * sizeof(char *));
    out = *out_ptr;
    if (!out)
        return E_OUTOFMEMORY;

    home_dir = getenv("HOME");
    if (!home_dir)
    {
        hr = E_FAIL;
        goto xdg_user_dir_lookup_error;
    }

    hr = get_xdg_config_file(home_dir, &config_file);
    if (FAILED(hr))
        goto xdg_user_dir_lookup_error;

    file = fopen(config_file, "r");
    HeapFree(GetProcessHeap(), 0, config_file);
    if (!file)
    {
        hr = E_HANDLE;
        goto xdg_user_dir_lookup_error;
    }

    while (fgets(buffer, sizeof(buffer), file))
    {
        int idx;
        char *p;

        /* Remove newline at end */
        len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n')
            buffer[len-1] = 0;

        /* Parse the key */
        p = buffer;
        idx = parse_config1(xdg_dirs, num_dirs, &p);
        if (idx < 0)
            continue;
        if (out[idx])
            continue;

        /* Parse the value */
        hr = parse_config2(p, home_dir, &out[idx]);
        if (FAILED(hr))
        {
            if (hr == E_OUTOFMEMORY)
            {
                fclose(file);
                goto xdg_user_dir_lookup_error;
            }
            continue;
        }
    }
    fclose (file);
    hr = S_OK;

    /* Remove entries for directories that do not exist */
    for (i = 0; i <  num_dirs; i++)
    {
        struct stat statFolder;

        if (!out[i])
            continue;
        if (!stat(out[i], &statFolder) && S_ISDIR(statFolder.st_mode))
            continue;
        HeapFree(GetProcessHeap(), 0, out[i]);
        out[i] = NULL;
    }

xdg_user_dir_lookup_error:
    if (FAILED(hr))
    {
        for (i = 0; i < num_dirs; i++) HeapFree(GetProcessHeap(), 0, out[i]);
        HeapFree(GetProcessHeap(), 0, *out_ptr);
    }
    return hr;
}
