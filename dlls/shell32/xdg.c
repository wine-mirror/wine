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
 */
 
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
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
        lstrcpyA(ret, env);
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
const char *XDG_GetPath(int path_id)
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
 * XDG_GetPath    [internal]
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
    int only_spc = TRUE;
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
    static const char *unsafechars = "^&`{}|[]'<>\\#%\"+";
    static const char *hexchars = "0123456789ABCDEF";
    int num_written = 0;
    const char *c;

    for (c = value; *c; c++)
    {
        if (*c<=0x20 || *c>=0x7f || strchr(unsafechars, *c))
        {
            if (output)
            {
                *(output++) = '%';
                *(output++) = hexchars[(unsigned)(*c)/16];
                *(output++) = hexchars[(unsigned)(*c)%16];
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
