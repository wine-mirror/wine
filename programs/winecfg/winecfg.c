/*
 * WineCfg configuration management
 *
 * Copyright 2002 Jaco Greeff
 * Copyright 2003 Dimitrie O. Paun
 * Copyright 2003-2004 Mike Hearn
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
 * TODO:
 *  - Use unicode
 *  - Icons in listviews/icons
 *  - Better add app dialog, scan c: for EXE files and add to list in background
 *  - Use [GNOME] HIG style groupboxes rather than win32 style (looks nicer, imho)
 *
 */

#define WIN32_LEAN_AND_MEAN

#include <assert.h>
#include <limits.h>
#include <windows.h>
#include <winreg.h>
#include <wine/debug.h>
#include <wine/list.h>

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

#include "winecfg.h"
#include "resource.h"

static const BOOL is_win64 = (sizeof(void *) > sizeof(int));

HKEY config_key = NULL;
HMENU hPopupMenus = 0;


/* this is called from the WM_SHOWWINDOW handlers of each tab page.
 *
 * it's a nasty hack, necessary because the property sheet insists on resetting the window title
 * to the title of the tab, which is utterly useless. dropping the property sheet is on the todo list.
 */
void set_window_title(HWND dialog)
{
    WCHAR newtitle[256];

    /* update the window title  */
    if (current_app)
    {
        WCHAR apptitle[256];
        LoadStringW(GetModuleHandleW(NULL), IDS_WINECFG_TITLE_APP, apptitle, ARRAY_SIZE(apptitle));
        swprintf(newtitle, ARRAY_SIZE(newtitle), apptitle, current_app);
    }
    else
    {
        LoadStringW(GetModuleHandleW(NULL), IDS_WINECFG_TITLE, newtitle, ARRAY_SIZE(newtitle));
    }

    WINE_TRACE("setting title to %s\n", wine_dbgstr_w (newtitle));
    SendMessageW (GetParent(dialog), PSM_SETTITLEW, 0, (LPARAM) newtitle);
}


WCHAR* load_string (UINT id)
{
    WCHAR buf[1024];
    int len;
    WCHAR* newStr;

    LoadStringW(GetModuleHandleW(NULL), id, buf, ARRAY_SIZE(buf));

    len = wcslen(buf);
    newStr = malloc((len + 1) * sizeof(WCHAR));
    memcpy(newStr, buf, len * sizeof(WCHAR));
    newStr[len] = 0;
    return newStr;
}

/**
 * get_config_key: Retrieves a configuration value from the registry
 *
 * char *subkey : the name of the config section
 * char *name : the name of the config value
 * char *default : if the key isn't found, return this value instead
 *
 * Returns a buffer holding the value if successful, NULL if
 * not. Caller is responsible for releasing the result.
 *
 */
static WCHAR *get_config_key (HKEY root, const WCHAR *subkey, const WCHAR *name, const WCHAR *def)
{
    LPWSTR buffer = NULL;
    DWORD len;
    HKEY hSubKey = NULL;
    DWORD res;

    WINE_TRACE("subkey=%s, name=%s, def=%s\n", wine_dbgstr_w(subkey),
               wine_dbgstr_w(name), wine_dbgstr_w(def));

    res = RegOpenKeyExW(root, subkey, 0, MAXIMUM_ALLOWED, &hSubKey);
    if (res != ERROR_SUCCESS)
    {
        if (res == ERROR_FILE_NOT_FOUND)
        {
            WINE_TRACE("Section key not present - using default\n");
            return wcsdup(def);
        }
        else
        {
            WINE_ERR("RegOpenKey failed on wine config key (res=%ld)\n", res);
        }
        goto end;
    }

    res = RegQueryValueExW(hSubKey, name, NULL, NULL, NULL, &len);
    if (res == ERROR_FILE_NOT_FOUND)
    {
        WINE_TRACE("Value not present - using default\n");
        buffer = wcsdup(def);
	goto end;
    } else if (res != ERROR_SUCCESS)
    {
        WINE_ERR("Couldn't query value's length (res=%ld)\n", res);
        goto end;
    }

    buffer = malloc(len + sizeof(WCHAR));

    RegQueryValueExW(hSubKey, name, NULL, NULL, (LPBYTE) buffer, &len);

    WINE_TRACE("buffer=%s\n", wine_dbgstr_w(buffer));
end:
    RegCloseKey(hSubKey);

    return buffer;
}

/**
 * set_config_key: convenience wrapper to set a key/value pair
 *
 * const char *subKey : the name of the config section
 * const char *valueName : the name of the config value
 * const char *value : the value to set the configuration key to
 *
 * Returns 0 on success, non-zero otherwise
 *
 * If valueName or value is NULL, an empty section will be created
 */
static int set_config_key(HKEY root, const WCHAR *subkey, REGSAM access, const WCHAR *name, const void *value, DWORD type)
{
    DWORD res = 1;
    HKEY key = NULL;

    WINE_TRACE("subkey=%s: name=%s, value=%p, type=%ld\n", wine_dbgstr_w(subkey),
               wine_dbgstr_w(name), value, type);

    assert( subkey != NULL );

    if (subkey[0])
    {
        res = RegCreateKeyExW( root, subkey, 0, NULL, REG_OPTION_NON_VOLATILE,
                               access, NULL, &key, NULL );
        if (res != ERROR_SUCCESS) goto end;
    }
    else key = root;
    if (name == NULL || value == NULL) goto end;

    switch (type)
    {
        case REG_SZ: res = RegSetValueExW(key, name, 0, REG_SZ, value, (lstrlenW(value)+1)*sizeof(WCHAR)); break;
        case REG_DWORD: res = RegSetValueExW(key, name, 0, REG_DWORD, value, sizeof(DWORD)); break;
    }
    if (res != ERROR_SUCCESS) goto end;

    res = 0;
end:
    if (key && key != root) RegCloseKey(key);
    if (res != 0)
        WINE_ERR("Unable to set configuration key %s in section %s, res=%ld\n",
                 wine_dbgstr_w(name), wine_dbgstr_w(subkey), res);
    return res;
}

/* ========================================================================= */

/* This code exists for the following reasons:
 *
 * - It makes working with the registry easier
 * - By storing a mini cache of the registry, we can more easily implement
 *   cancel/revert and apply. The 'settings list' is an overlay on top of
 *   the actual registry data that we can write out at will.
 *
 * Rather than model a tree in memory, we simply store each absolute (rooted
 * at the config key) path.
 *
 */

struct setting
{
    struct list entry;
    HKEY root;    /* the key on which path is rooted */
    WCHAR *path;   /* path in the registry rooted at root  */
    WCHAR *name;   /* name of the registry value. if null, this means delete the key  */
    void  *value;  /* contents of the registry value. if null, this means delete the value  */
    DWORD type;   /* type of registry value. REG_SZ or REG_DWORD for now */
};

static struct list settings = LIST_INIT(settings);

static void free_setting(struct setting *setting)
{
    assert( setting != NULL );
    assert( setting->path );

    WINE_TRACE("destroying %p: %s\n", setting,
               wine_dbgstr_w(setting->path));

    free(setting->path);
    free(setting->name);
    free(setting->value);

    list_remove(&setting->entry);

    free(setting);
}

/**
 * Returns the contents of the value at path. If not in the settings
 * list, it will be fetched from the registry - failing that, the
 * default will be used.
 *
 * If already in the list, the contents as given there will be
 * returned. You are expected to free the result.
 */
WCHAR *get_reg_key(HKEY root, const WCHAR *path, const WCHAR *name, const WCHAR *def)
{
    struct list *cursor;
    struct setting *s;
    WCHAR *val;

    WINE_TRACE("path=%s, name=%s, def=%s\n", wine_dbgstr_w(path),
               wine_dbgstr_w(name), wine_dbgstr_w(def));

    /* check if it's in the list */
    LIST_FOR_EACH( cursor, &settings )
    {
        s = LIST_ENTRY(cursor, struct setting, entry);

        if (root != s->root) continue;
        if (lstrcmpiW(path, s->path) != 0) continue;
        if (!s->name) continue;
        if (lstrcmpiW(name, s->name) != 0) continue;

        WINE_TRACE("found %s:%s in settings list, returning %s\n",
                   wine_dbgstr_w(path), wine_dbgstr_w(name),
                   wine_dbgstr_w(s->value));
        return wcsdup(s->value);
    }

    /* no, so get from the registry */
    val = get_config_key(root, path, name, def);

    WINE_TRACE("returning %s\n", wine_dbgstr_w(val));

    return val;
}

/**
 * Used to set a registry key.
 *
 * path is rooted at the config key, ie use "Version" or
 * "AppDefaults\\fooapp.exe\\Version". You can use keypath()
 * to get such a string.
 *
 * name is the value name, or NULL to delete the path.
 *
 * value is what to set the value to, or NULL to delete it.
 *
 * type is REG_SZ or REG_DWORD.
 *
 * These values will be copied when necessary.
 */
static void set_reg_key_ex(HKEY root, const WCHAR *path, const WCHAR *name, const void *value, DWORD type)
{
    struct list *cursor;
    struct setting *s;

    assert( path != NULL );

    WINE_TRACE("path=%s, name=%s, value=%s\n", wine_dbgstr_w(path),
               wine_dbgstr_w(name), wine_dbgstr_w(value));

    /* firstly, see if we already set this setting  */
    LIST_FOR_EACH( cursor, &settings )
    {
        struct setting *s = LIST_ENTRY(cursor, struct setting, entry);

        if (root != s->root) continue;
        if (lstrcmpiW(s->path, path) != 0) continue;
        if ((s->name && name) && lstrcmpiW(s->name, name) != 0) continue;

        /* are we attempting a double delete? */
        if (!s->name && !name) return;

        /* do we want to undelete this key? */
        if (!s->name && name) s->name = wcsdup(name);

        /* yes, we have already set it, so just replace the content and return  */
        free(s->value);
        s->type = type;
        switch (type)
        {
            case REG_SZ:
                s->value = wcsdup(value);
                break;
            case REG_DWORD:
                s->value = malloc(sizeof(DWORD));
                memcpy( s->value, value, sizeof(DWORD) );
                break;
        }

        /* are we deleting this key? this won't remove any of the
         * children from the overlay so if the user adds it again in
         * that session it will appear to undelete the settings, but
         * in reality only the settings actually modified by the user
         * in that session will be restored. we might want to fix this
         * corner case in future by actually deleting all the children
         * here so that once it's gone, it's gone.
         */
        if (!name) s->name = NULL;

        return;
    }

    /* otherwise add a new setting for it  */
    s = malloc(sizeof(struct setting));
    s->root  = root;
    s->path  = wcsdup(path);
    s->name  = wcsdup(name);
    s->type  = type;
    switch (type)
    {
        case REG_SZ:
            s->value = wcsdup(value);
            break;
        case REG_DWORD:
            s->value = malloc(sizeof(DWORD));
            memcpy( s->value, value, sizeof(DWORD) );
            break;
    }

    list_add_tail(&settings, &s->entry);
}

void set_reg_key(HKEY root, const WCHAR *path, const WCHAR *name, const WCHAR *value)
{
    set_reg_key_ex(root, path, name, value, REG_SZ);
}

void set_reg_key_dword(HKEY root, const WCHAR *path, const WCHAR *name, DWORD value)
{
    set_reg_key_ex(root, path, name, &value, REG_DWORD);
}

/**
 * enumerates the value names at the given path, taking into account
 * the changes in the settings list.
 *
 * you are expected to free each element of the array, which is null
 * terminated, as well as the array itself.
 */
WCHAR **enumerate_values(HKEY root, const WCHAR *path)
{
    HKEY key;
    DWORD res, i = 0, valueslen = 0;
    WCHAR **values = NULL;
    struct list *cursor;

    res = RegOpenKeyExW(root, path, 0, MAXIMUM_ALLOWED, &key);
    if (res == ERROR_SUCCESS)
    {
        while (TRUE)
        {
            WCHAR name[1024];
            DWORD namesize = ARRAY_SIZE(name);
            BOOL removed = FALSE;

            /* find out the needed size, allocate a buffer, read the value  */
            if ((res = RegEnumValueW(key, i, name, &namesize, NULL, NULL, NULL, NULL)) != ERROR_SUCCESS)
                break;

            WINE_TRACE("name=%s\n", wine_dbgstr_w(name));

            /* check if this value name has been removed in the settings list  */
            LIST_FOR_EACH( cursor, &settings )
            {
                struct setting *s = LIST_ENTRY(cursor, struct setting, entry);
                if (lstrcmpiW(s->path, path) != 0) continue;
                if (lstrcmpiW(s->name, name) != 0) continue;

                if (!s->value)
                {
                    WINE_TRACE("this key has been removed, so skipping\n");
                    removed = TRUE;
                    break;
                }
            }

            if (removed)            /* this value was deleted by the user, so don't include it */
            {
                i++;
                continue;
            }

            /* grow the array if necessary, add buffer to it, iterate  */
            values = realloc(values, sizeof(WCHAR*) * (valueslen + 1));

            values[valueslen++] = wcsdup(name);
            WINE_TRACE("valueslen is now %ld\n", valueslen);
            i++;
        }
    }
    else
    {
        WINE_WARN("failed opening registry key %s, res=0x%lx\n",
                  wine_dbgstr_w(path), res);
    }

    WINE_TRACE("adding settings in list but not registry\n");

    /* now we have to add the values that aren't in the registry but are in the settings list */
    LIST_FOR_EACH( cursor, &settings )
    {
        struct setting *setting = LIST_ENTRY(cursor, struct setting, entry);
        BOOL found = FALSE;

        if (lstrcmpiW(setting->path, path) != 0) continue;

        if (!setting->value) continue;

        for (i = 0; i < valueslen; i++)
        {
            if (lstrcmpiW(setting->name, values[i]) == 0)
            {
                found = TRUE;
                break;
            }
        }

        if (found) continue;

        WINE_TRACE("%s in list but not registry\n", wine_dbgstr_w(setting->name));

        /* otherwise it's been set by the user but isn't in the registry */
        values = realloc(values, sizeof(WCHAR*) * (valueslen + 1));

        values[valueslen++] = wcsdup(setting->name);
    }

    WINE_TRACE("adding null terminator\n");
    if (values)
    {
        values = realloc(values, sizeof(WCHAR*) * (valueslen + 1));
        values[valueslen] = NULL;
    }

    RegCloseKey(key);

    return values;
}

/**
 * returns true if the given key/value pair exists in the registry or
 * has been written to.
 */
BOOL reg_key_exists(HKEY root, const WCHAR *path, const WCHAR *name)
{
    WCHAR *val = get_reg_key(root, path, name, NULL);

    free(val);
    return val != NULL;
}

static void process_setting(struct setting *s)
{
    HKEY key;
    BOOL needs_wow64 = (is_win64 && s->root == HKEY_LOCAL_MACHINE && s->path &&
                        !wcsnicmp(s->path, L"Software\\", wcslen(L"Software\\")));

    if (s->value)
    {
        set_config_key(s->root, s->path, MAXIMUM_ALLOWED, s->name, s->value, s->type);
        if (needs_wow64)
            set_config_key(s->root, s->path, MAXIMUM_ALLOWED | KEY_WOW64_32KEY, s->name, s->value, s->type);
    }
    else
    {
	WINE_TRACE("Removing %s:%s\n", wine_dbgstr_w(s->path), wine_dbgstr_w(s->name));
        if (!RegOpenKeyExW( s->root, s->path, 0, MAXIMUM_ALLOWED, &key ))
        {
            /* NULL name means remove that path/section entirely */
            if (s->name) RegDeleteValueW( key, s->name );
            else
            {
                RegDeleteTreeW( key, NULL );
                RegDeleteKeyW( s->root, s->path );
            }
            RegCloseKey( key );
        }
        if (needs_wow64)
        {
            WINE_TRACE("Removing 32-bit %s:%s\n", wine_dbgstr_w(s->path), wine_dbgstr_w(s->name));
            if (!RegOpenKeyExW( s->root, s->path, 0, MAXIMUM_ALLOWED | KEY_WOW64_32KEY, &key ))
            {
                if (s->name) RegDeleteValueW( key, s->name );
                else
                {
                    RegDeleteTreeW( key, NULL );
                    RegDeleteKeyExW( s->root, s->path, KEY_WOW64_32KEY, 0 );
                }
                RegCloseKey( key );
            }
        }
    }
}

void apply(void)
{
    if (list_empty(&settings)) return; /* we will be called for each page when the user clicks OK */

    WINE_TRACE("()\n");

    while (!list_empty(&settings))
    {
        struct setting *s = (struct setting *) list_head(&settings);
        process_setting(s);
        free_setting(s);
    }
}

/* ================================== utility functions ============================ */

WCHAR* current_app = NULL; /* the app we are currently editing, or NULL if editing global */

/* returns a registry key path suitable for passing to addTransaction  */
WCHAR *keypath(const WCHAR *section)
{
    static WCHAR *result = NULL;

    free(result);

    if (current_app)
    {
        DWORD len = sizeof(L"AppDefaults\\") + (lstrlenW(current_app) + lstrlenW(section) + 1) * sizeof(WCHAR);
        result = malloc(len);
        lstrcpyW( result, L"AppDefaults\\" );
        lstrcatW( result, current_app );
        if (section[0])
        {
            len = lstrlenW(result);
            result[len++] = '\\';
            lstrcpyW( result + len, section );
        }
    }
    else
    {
        result = wcsdup(section);
    }

    return result;
}

void PRINTERROR(void)
{
        LPSTR msg;

        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                       0, GetLastError(), MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                       (LPSTR)&msg, 0, NULL);

        /* eliminate trailing newline, is this a Wine bug? */
        *(strrchr(msg, '\r')) = '\0';
        
        WINE_TRACE("error: '%s'\n", msg);
}

BOOL initialize(HINSTANCE hInstance)
{
    DWORD res = RegCreateKeyW(HKEY_CURRENT_USER, WINE_KEY_ROOT, &config_key);

    if (res != ERROR_SUCCESS) {
	WINE_ERR("RegOpenKey failed on wine config key (%ld)\n", res);
        return TRUE;
    }

    return FALSE;
}
