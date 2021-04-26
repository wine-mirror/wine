/*
 * WineCfg libraries tabsheet
 *
 * Copyright 2004 Robert van Herk
 * Copyright 2004 Mike Hearn
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
 */

#include "config.h"
#include "wine/port.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <wine/debug.h>
#include <stdio.h>
#include <dirent.h>
#include <assert.h>
#include <stdlib.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "winecfg.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

#ifdef __i386__
static const char pe_dir[] = "\\i386-windows";
#elif defined __x86_64__
static const char pe_dir[] = "\\x86_64-windows";
#elif defined __arm__
static const char pe_dir[] = "\\arm-windows";
#elif defined __aarch64__
static const char pe_dir[] = "\\aarch64-windows";
#else
static const char pe_dir[] = "";
#endif

/* dlls that shouldn't be configured anything other than builtin; list must be sorted*/
static const char * const builtin_only[] =
{
    "advapi32",
    "capi2032",
    "dbghelp",
    "ddraw",
    "gdi32",
    "gphoto2.ds",
    "icmp",
    "iphlpapi",
    "kernel32",
    "l3codeca.acm",
    "mountmgr.sys",
    "mswsock",
    "ntdll",
    "ntoskrnl.exe",
    "opengl32",
    "sane.ds",
    "secur32",
    "twain_32",
    "unicows",
    "user32",
    "vdmdbg",
    "w32skrnl",
    "winmm",
    "wintab32",
    "wnaspi32",
    "wow32",
    "ws2_32",
    "wsock32",
};

enum dllmode
{
	BUILTIN_NATIVE,
	NATIVE_BUILTIN,
	BUILTIN,
	NATIVE,
	DISABLE,
	UNKNOWN /* Special value indicating an erroneous DLL override mode */
};

struct dll
{
        char *name;
        enum dllmode mode;
};

static const WCHAR emptyW[1];

/* Convert a registry string to a dllmode */
static enum dllmode string_to_mode(char *in)
{
    int i, j, len;
    char *out;
    enum dllmode res;

    len = strlen(in);
    out = HeapAlloc(GetProcessHeap(), 0, len + 1);

    /* remove the spaces */
    for (i = j = 0; i <= len; ++i) {
        if (in[i] != ' ') {
            out[j++] = in[i];
        }
    }

    /* parse the string */
    res = UNKNOWN;
    if (strcmp(out, "builtin,native") == 0) res = BUILTIN_NATIVE;
    if (strcmp(out, "native,builtin") == 0) res = NATIVE_BUILTIN;
    if (strcmp(out, "builtin") == 0) res = BUILTIN;
    if (strcmp(out, "native") == 0) res = NATIVE;
    if (strcmp(out, "") == 0) res = DISABLE;

    HeapFree(GetProcessHeap(), 0, out);
    return res;
}

/* Convert a dllmode to a registry string. */
static const char* mode_to_string(enum dllmode mode)
{
    switch( mode )
    {
        case NATIVE: return "native";
        case BUILTIN: return "builtin";
        case NATIVE_BUILTIN: return "native,builtin";
        case BUILTIN_NATIVE: return "builtin,native";
        case DISABLE: return "";
        default: return "";
    }
}

/* Convert a dllmode to a pretty string for display. TODO: use translations. */
static const char* mode_to_label(enum dllmode mode)
{
    static char buffer[256];
    UINT id = 0;

    switch( mode )
    {
    case NATIVE: id = IDS_DLL_NATIVE; break;
    case BUILTIN: id = IDS_DLL_BUILTIN; break;
    case NATIVE_BUILTIN: id = IDS_DLL_NATIVE_BUILTIN; break;
    case BUILTIN_NATIVE: id = IDS_DLL_BUILTIN_NATIVE; break;
    case DISABLE: id = IDS_DLL_DISABLED; break;
    default: return "??";
    }
    if (!LoadStringA( GetModuleHandleA(NULL), id, buffer, sizeof(buffer) )) buffer[0] = 0;
    return buffer;
}

/* Convert a control id (IDC_ constant) to a dllmode */
static enum dllmode id_to_mode(DWORD id)
{
    switch( id )
    {
        case IDC_RAD_BUILTIN: return BUILTIN;
        case IDC_RAD_NATIVE: return NATIVE;
        case IDC_RAD_NATIVE_BUILTIN: return NATIVE_BUILTIN;
        case IDC_RAD_BUILTIN_NATIVE: return BUILTIN_NATIVE;
        case IDC_RAD_DISABLE: return DISABLE;
        default: assert( FALSE ); return 0; /* should not be reached  */
    }
}

/* Convert a dllmode to a control id (IDC_ constant) */
static DWORD mode_to_id(enum dllmode mode)
{
    switch( mode )
    {
        case BUILTIN: return IDC_RAD_BUILTIN;
        case NATIVE: return IDC_RAD_NATIVE;
        case NATIVE_BUILTIN: return IDC_RAD_NATIVE_BUILTIN;
        case BUILTIN_NATIVE: return IDC_RAD_BUILTIN_NATIVE;
        case DISABLE: return IDC_RAD_DISABLE;
        default: return IDC_RAD_BUILTIN_NATIVE;
    }
}

/* helper for is_builtin_only */
static int compare_dll( const void *ptr1, const void *ptr2 )
{
    const char * const *name1 = ptr1;
    const char * const *name2 = ptr2;
    return strcmp( *name1, *name2 );
}

/* check if dll is recommended as builtin only */
static inline BOOL is_builtin_only( const char *name )
{
    const char *ext = strrchr( name, '.' );

    if (ext)
    {
        if (!strcmp( ext, ".vxd" ) ||
            !strcmp( ext, ".drv" ) ||
            !strcmp( ext, ".tlb" ))
            return TRUE;
    }
    if (!strncmp( name, "wine", 4 )) return TRUE;
    return bsearch( &name, builtin_only, ARRAY_SIZE(builtin_only),
                    sizeof(builtin_only[0]), compare_dll ) != NULL;
}

/* check if dll should be offered in the drop-down list */
static BOOL show_dll_in_list( const char *name )
{
    const char *ext = strrchr( name, '.' );

    if (ext)
    {
        /* skip 16-bit dlls */
        if (strlen(ext) > 2 && !strcmp( ext + strlen(ext) - 2, "16" )) return FALSE;
        /* skip exes */
        if (!strcmp( ext, ".exe" )) return FALSE;
    }
    /* skip api set placeholders */
    if (!strncmp( name, "api-ms-", 7 ) || !strncmp( name, "ext-ms-", 7 )) return FALSE;
    /* skip dlls that should always be builtin */
    return !is_builtin_only( name );
}

static void set_controls_from_selection(HWND dialog)
{
    /* FIXME: display/update some information about the selected dll (purpose, recommended load order) maybe? */
}

static void clear_settings(HWND dialog)
{
    int count = SendDlgItemMessageW(dialog, IDC_DLLS_LIST, LB_GETCOUNT, 0, 0);
    int i;

    WINE_TRACE("count=%d\n", count);
    
    for (i = 0; i < count; i++)
    {
        struct dll *dll = (struct dll *) SendDlgItemMessageW(dialog, IDC_DLLS_LIST, LB_GETITEMDATA, 0, 0);

        SendDlgItemMessageW(dialog, IDC_DLLS_LIST, LB_DELETESTRING, 0, 0);
        HeapFree(GetProcessHeap(), 0, dll->name);
        HeapFree(GetProcessHeap(), 0, dll);
    }
}

/* load the list of available libraries from a given dir */
static void load_library_list_from_dir( HWND dialog, const char *dir_path, int check_subdirs )
{
    static const char * const ext[] = { ".dll", ".dll.so", ".so", "" };
    char *buffer, *p, name[256];
    unsigned int i;
    HANDLE handle;
    WIN32_FIND_DATAA data;

    buffer = HeapAlloc( GetProcessHeap(), 0, strlen(dir_path) + 2 * sizeof(name) + 10 );

    strcpy( buffer, dir_path );
    strcat( buffer, "\\*" );
    buffer[1] = '\\';  /* change \??\ to \\?\ */
    p = buffer + strlen(buffer) - 1;

    if ((handle = FindFirstFileA( buffer, &data )) == INVALID_HANDLE_VALUE)
    {
        HeapFree( GetProcessHeap(), 0, buffer );
        return;
    }

    do
    {
        size_t len = strlen(data.cFileName);
        if (len > sizeof(name)) continue;
        if (check_subdirs)
        {
            if (!strcmp( data.cFileName, "." )) continue;
            if (!strcmp( data.cFileName, ".." )) continue;
            if (!show_dll_in_list( data.cFileName )) continue;
            for (i = 0; i < ARRAY_SIZE( ext ); i++)
            {
                sprintf( p, "%s\\%s%s", data.cFileName, data.cFileName, ext[i] );
                if (GetFileAttributesA( buffer ) != INVALID_FILE_ATTRIBUTES)
                {
                    SendDlgItemMessageA( dialog, IDC_DLLCOMBO, CB_ADDSTRING, 0, (LPARAM)data.cFileName );
                    break;
                }
            }
        }
        else
        {
            for (i = 0; i < ARRAY_SIZE( ext ); i++)
            {
                if (!ext[i][0]) continue;
                if (len > strlen(ext[i]) && !strcmp( data.cFileName + len - strlen(ext[i]), ext[i]))
                {
                    len -= strlen( ext[i] );
                    memcpy( name, data.cFileName, len );
                    name[len] = 0;
                    if (!show_dll_in_list( name )) continue;
                    SendDlgItemMessageA( dialog, IDC_DLLCOMBO, CB_ADDSTRING, 0, (LPARAM)name );
                }
            }
        }
    } while (FindNextFileA( handle, &data ));

    FindClose( handle );
    HeapFree( GetProcessHeap(), 0, buffer );
}

/* load the list of available libraries */
static void load_library_list( HWND dialog )
{
    unsigned int i = 0;
    char item1[256], item2[256], var[32], path[MAX_PATH];
    HCURSOR old_cursor = SetCursor( LoadCursorW(0, (LPWSTR)IDC_WAIT) );

    if (GetEnvironmentVariableA( "WINEBUILDDIR", path, MAX_PATH ))
    {
        char *dir = HeapAlloc( GetProcessHeap(), 0, strlen(path) + sizeof("\\dlls") );
        strcpy( dir, path );
        strcat( dir, "\\dlls" );
        load_library_list_from_dir( dialog, dir, TRUE );
        HeapFree( GetProcessHeap(), 0, dir );
    }

    for (;;)
    {
        sprintf( var, "WINEDLLDIR%u", i++ );
        if (!GetEnvironmentVariableA( var, path, MAX_PATH )) break;
        load_library_list_from_dir( dialog, path, FALSE );
        strcat( path, pe_dir );
        load_library_list_from_dir( dialog, path, FALSE );
    }

    /* get rid of duplicate entries */

    SendDlgItemMessageA( dialog, IDC_DLLCOMBO, CB_GETLBTEXT, 0, (LPARAM)item1 );
    i = 1;
    while (SendDlgItemMessageA( dialog, IDC_DLLCOMBO, CB_GETLBTEXT, i, (LPARAM)item2 ) >= 0)
    {
        if (!strcmp( item1, item2 ))
        {
            SendDlgItemMessageA( dialog, IDC_DLLCOMBO, CB_DELETESTRING, i, 0 );
        }
        else
        {
            strcpy( item1, item2 );
            i++;
        }
    }
    SetCursor( old_cursor );
}

static void load_library_settings(HWND dialog)
{
    char **overrides = enumerate_values(config_key, keypath("DllOverrides"));
    char **p;
    int sel, count = 0;

    sel = SendDlgItemMessageW(dialog, IDC_DLLS_LIST, LB_GETCURSEL, 0, 0);

    WINE_TRACE("sel=%d\n", sel);

    clear_settings(dialog);
    
    if (!overrides || *overrides == NULL)
    {
        set_controls_from_selection(dialog);
        disable(IDC_DLLS_EDITDLL);
        disable(IDC_DLLS_REMOVEDLL);
        HeapFree(GetProcessHeap(), 0, overrides);
        return;
    }

    enable(IDC_DLLS_EDITDLL);
    enable(IDC_DLLS_REMOVEDLL);
    
    for (p = overrides; *p != NULL; p++)
    {
        int index;
        char *str, *value;
        const char *label;
        struct dll *dll;

        value = get_reg_key(config_key, keypath("DllOverrides"), *p, NULL);

        label = mode_to_label(string_to_mode(value));
        
        str = HeapAlloc(GetProcessHeap(), 0, strlen(*p) + 2 + strlen(label) + 2);
        strcpy(str, *p);
        strcat(str, " (");
        strcat(str, label);
        strcat(str, ")");

        dll = HeapAlloc(GetProcessHeap(), 0, sizeof(struct dll));
        dll->name = *p;
        dll->mode = string_to_mode(value);

        index = SendDlgItemMessageA(dialog, IDC_DLLS_LIST, LB_ADDSTRING, (WPARAM) -1, (LPARAM) str);
        SendDlgItemMessageW(dialog, IDC_DLLS_LIST, LB_SETITEMDATA, index, (LPARAM) dll);

        HeapFree(GetProcessHeap(), 0, str);

        count++;
    }

    HeapFree(GetProcessHeap(), 0, overrides);

    /* restore the previous selection, if possible  */
    if (sel >= count - 1) sel = count - 1;
    else if (sel == -1) sel = 0;

    SendDlgItemMessageW(dialog, IDC_DLLS_LIST, LB_SETCURSEL, sel, 0);

    set_controls_from_selection(dialog);
}

/* Called when the application is initialized (cannot reinit!)  */
static void init_libsheet(HWND dialog)
{
    /* clear the add dll controls  */
    SendDlgItemMessageW(dialog, IDC_DLLCOMBO, WM_SETTEXT, 1, (LPARAM)emptyW);
    load_library_list( dialog );
    disable(IDC_DLLS_ADDDLL);
}

static void on_add_combo_change(HWND dialog)
{
    WCHAR buffer[1024];
    int sel, len;

    SendDlgItemMessageW(dialog, IDC_DLLCOMBO, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    /* if lib was chosen from combobox, we receive an empty buffer, check manually */
    sel=SendDlgItemMessageW(dialog, IDC_DLLCOMBO, CB_GETCURSEL, 0, 0);
    len=SendDlgItemMessageW(dialog, IDC_DLLCOMBO, CB_GETLBTEXTLEN, sel, 0);

    if (buffer[0] || len>0)
    {
        enable(IDC_DLLS_ADDDLL)
        SendMessageW(GetParent(dialog), DM_SETDEFID, IDC_DLLS_ADDDLL, 0);
    }
    else
    {
        disable(IDC_DLLS_ADDDLL);
        SendMessageW(GetParent(dialog), DM_SETDEFID, IDOK, 0);
    }
}

static void set_dllmode(HWND dialog, DWORD id)
{
    enum dllmode mode;
    struct dll *dll;
    int sel;
    const char *str;

    mode = id_to_mode(id);

    sel = SendDlgItemMessageW(dialog, IDC_DLLS_LIST, LB_GETCURSEL, 0, 0);
    if (sel == -1) return;

    dll = (struct dll *) SendDlgItemMessageW(dialog, IDC_DLLS_LIST, LB_GETITEMDATA, sel, 0);

    str = mode_to_string(mode);
    WINE_TRACE("Setting %s to %s\n", dll->name, str);

    SendMessageW(GetParent(dialog), PSM_CHANGED, 0, 0);
    set_reg_key(config_key, keypath("DllOverrides"), dll->name, str);

    load_library_settings(dialog);  /* ... and refresh  */
}

static void on_add_click(HWND dialog)
{
    static const char dotDll[] = ".dll";
    char buffer[1024], *ptr;

    ZeroMemory(buffer, sizeof(buffer));

    SendDlgItemMessageA(dialog, IDC_DLLCOMBO, WM_GETTEXT, sizeof(buffer), (LPARAM) buffer);
    if (lstrlenA(buffer) >= sizeof(dotDll))
    {
        ptr = buffer + lstrlenA(buffer) - sizeof(dotDll) + 1;
        if (!lstrcmpiA(ptr, dotDll))
        {
            WINE_TRACE("Stripping dll extension\n");
            *ptr = '\0';
        }
    }

    /* check if dll is in the builtin-only list */
    if (!(ptr = strrchr( buffer, '\\' )))
    {
        ptr = buffer;
        if (*ptr == '*') ptr++;
    }
    else ptr++;
    if (is_builtin_only( ptr ))
    {
        MSGBOXPARAMSA params;
        params.cbSize = sizeof(params);
        params.hwndOwner = dialog;
        params.hInstance = GetModuleHandleA( NULL );
        params.lpszText = MAKEINTRESOURCEA( IDS_DLL_WARNING );
        params.lpszCaption = MAKEINTRESOURCEA( IDS_DLL_WARNING_CAPTION );
        params.dwStyle = MB_ICONWARNING | MB_YESNO;
        params.lpszIcon = NULL;
        params.dwContextHelpId = 0;
        params.lpfnMsgBoxCallback = NULL;
        params.dwLanguageId = 0;
        if (MessageBoxIndirectA( &params ) != IDYES) return;
    }

    SendDlgItemMessageW(dialog, IDC_DLLCOMBO, WM_SETTEXT, 0, (LPARAM)emptyW);
    disable(IDC_DLLS_ADDDLL);
    SendMessageW(GetParent(dialog), DM_SETDEFID, IDOK, 0);

    WINE_TRACE("Adding %s as native, builtin\n", buffer);

    SendMessageW(GetParent(dialog), PSM_CHANGED, 0, 0);
    set_reg_key(config_key, keypath("DllOverrides"), buffer, "native,builtin");

    load_library_settings(dialog);

    SendDlgItemMessageA(dialog, IDC_DLLS_LIST, LB_SELECTSTRING, 0, (LPARAM) buffer);

    set_controls_from_selection(dialog);
}

static INT_PTR CALLBACK loadorder_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static WORD sel;

    switch(uMsg) 
    {
    case WM_INITDIALOG:
        CheckRadioButton(hwndDlg, IDC_RAD_BUILTIN, IDC_RAD_DISABLE, lParam);
        sel = lParam;
        return TRUE;

    case WM_COMMAND:
        if(HIWORD(wParam) != BN_CLICKED) break;
        switch (LOWORD(wParam))
        {
        case IDC_RAD_BUILTIN:
        case IDC_RAD_NATIVE:
        case IDC_RAD_BUILTIN_NATIVE:
        case IDC_RAD_NATIVE_BUILTIN:
        case IDC_RAD_DISABLE:
            sel = LOWORD(wParam);
            return TRUE;
        case IDOK:
            EndDialog(hwndDlg, sel);
            return TRUE;
        case IDCANCEL:
            EndDialog(hwndDlg, wParam);
            return TRUE;
        }
    }
    return FALSE;
}

static void on_edit_click(HWND hwnd)
{
    INT_PTR ret;
    int index = SendDlgItemMessageW(hwnd, IDC_DLLS_LIST, LB_GETCURSEL, 0, 0);
    struct dll *dll;
    DWORD id;

    /* if no override is selected the edit button should be disabled... */
    assert(index != -1);

    dll = (struct dll *) SendDlgItemMessageW(hwnd, IDC_DLLS_LIST, LB_GETITEMDATA, index, 0);
    id = mode_to_id(dll->mode);

    ret = DialogBoxParamW(0, MAKEINTRESOURCEW(IDD_LOADORDER), hwnd, loadorder_dlgproc, id);

    if(ret != IDCANCEL)
        set_dllmode(hwnd, ret);
}

static void on_remove_click(HWND dialog)
{
    int sel = SendDlgItemMessageW(dialog, IDC_DLLS_LIST, LB_GETCURSEL, 0, 0);
    struct dll *dll;

    if (sel == LB_ERR) return;

    dll = (struct dll *) SendDlgItemMessageW(dialog, IDC_DLLS_LIST, LB_GETITEMDATA, sel, 0);

    SendDlgItemMessageW(dialog, IDC_DLLS_LIST, LB_DELETESTRING, sel, 0);

    SendMessageW(GetParent(dialog), PSM_CHANGED, 0, 0);
    set_reg_key(config_key, keypath("DllOverrides"), dll->name, NULL);

    HeapFree(GetProcessHeap(), 0, dll->name);
    HeapFree(GetProcessHeap(), 0, dll);

    if (SendDlgItemMessageW(dialog, IDC_DLLS_LIST, LB_GETCOUNT, 0, 0) > 0)
        SendDlgItemMessageW(dialog, IDC_DLLS_LIST, LB_SETCURSEL, max(sel - 1, 0), 0);
    else
    {
        disable(IDC_DLLS_EDITDLL);
        disable(IDC_DLLS_REMOVEDLL);
    }

    set_controls_from_selection(dialog);
}

INT_PTR CALLBACK
LibrariesDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		init_libsheet(hDlg);
		break;
        case WM_SHOWWINDOW:
                set_window_title(hDlg);
                break;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
                case PSN_SETACTIVE:
                    load_library_settings(hDlg);
                    break;
		}
		break;
	case WM_COMMAND:
		switch(HIWORD(wParam)) {
                case CBN_EDITCHANGE:
                    if (LOWORD(wParam) == IDC_DLLCOMBO)
                        on_add_combo_change(hDlg);
                    break;
                case CBN_SETFOCUS:
                    if (LOWORD(wParam) == IDC_DLLCOMBO)
                        on_add_combo_change(hDlg);
                    break;
                case CBN_KILLFOCUS:
                    if (LOWORD(wParam) == IDC_DLLCOMBO)
                        SendMessageW(GetParent(hDlg), DM_SETDEFID, IDOK, 0);
                    break;
		case BN_CLICKED:
			switch(LOWORD(wParam)) {
			case IDC_DLLS_ADDDLL:
                            on_add_click(hDlg);
                            break;
			case IDC_DLLS_EDITDLL:
			    on_edit_click(hDlg);
			    break;
			case IDC_DLLS_REMOVEDLL:
                            on_remove_click(hDlg);
                            break;
			}
			break;
                case LBN_SELCHANGE:
                        if(LOWORD(wParam) == IDC_DLLCOMBO)
                            on_add_combo_change(hDlg);
                        else
                            set_controls_from_selection(hDlg);
                        break;
		}
		break;
	}

	return 0;
}
