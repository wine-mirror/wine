/*
 * an application for displaying Win32 console
 *      registry and init functions
 *
 * Copyright 2001 Eric Pouech
 */

#include "winbase.h"
#include "winreg.h"
#include "winecon_private.h"

static const WCHAR wszConsole[]           = {'C','o','n','s','o','l','e',0};
static const WCHAR wszCursorSize[]        = {'C','u','r','s','o','r','S','i','z','e',0};
static const WCHAR wszCursorVisible[]     = {'C','u','r','s','o','r','V','i','s','i','b','l','e',0};
static const WCHAR wszFaceName[]          = {'F','a','c','e','N','a','m','e',0};
static const WCHAR wszFontSize[]          = {'F','o','n','t','S','i','z','e',0};
static const WCHAR wszFontWeight[]        = {'F','o','n','t','W','e','i','g','h','t',0};
static const WCHAR wszHistoryBufferSize[] = {'H','i','s','t','o','r','y','B','u','f','f','e','r','S','i','z','e',0};
static const WCHAR wszMenuMask[]          = {'M','e','n','u','M','a','s','k',0};
static const WCHAR wszQuickEdit[]         = {'Q','u','i','c','k','E','d','i','t',0};
static const WCHAR wszScreenBufferSize[]  = {'S','c','r','e','e','n','B','u','f','f','e','r','S','i','z','e',0};
static const WCHAR wszScreenColors[]      = {'S','c','r','e','e','n','C','o','l','o','r','s',0};
static const WCHAR wszWindowSize[]        = {'W','i','n','d','o','w','S','i','z','e',0};

/******************************************************************
 *		WINECON_RegLoad
 *
 *
 */
BOOL WINECON_RegLoad(struct config_data* cfg)
{
    HKEY        hConKey;
    DWORD 	type;
    DWORD 	count;
    DWORD       val;

    if (RegOpenKey(HKEY_CURRENT_USER, wszConsole, &hConKey)) hConKey = 0;

    count = sizeof(val);
    if (!hConKey || RegQueryValueEx(hConKey, wszCursorSize, 0, &type, (char*)&val, &count)) 
        val = 25;
    cfg->cursor_size = val;

    count = sizeof(val);
    if (!hConKey || RegQueryValueEx(hConKey, wszCursorVisible, 0, &type, (char*)&val, &count)) 
        val = 1;
    cfg->cursor_visible = val;

    count = sizeof(cfg->face_name); 
    if (!hConKey || RegQueryValueEx(hConKey, wszFaceName, 0, &type, (char*)&cfg->face_name, &count))
        cfg->face_name[0] = 0;

    count = sizeof(val);
    if (!hConKey || RegQueryValueEx(hConKey, wszFontSize, 0, &type, (char*)&val, &count)) 
        val = 0x000C0008;
    cfg->cell_height = HIWORD(val);
    cfg->cell_width  = LOWORD(val);

    count = sizeof(val);
    if (!hConKey || RegQueryValueEx(hConKey, wszFontWeight, 0, &type, (char*)&val, &count)) 
        val = 0;
    cfg->font_weight = val;

    count = sizeof(val);
    if (!hConKey || RegQueryValueEx(hConKey, wszHistoryBufferSize, 0, &type, (char*)&val, &count)) 
        val = 0;
    cfg->history_size = val;

    count = sizeof(val);
    if (!hConKey || RegQueryValueEx(hConKey, wszMenuMask, 0, &type, (char*)&val, &count)) 
        val = 0;
    cfg->menu_mask = val;

    count = sizeof(val);
    if (!hConKey || RegQueryValueEx(hConKey, wszQuickEdit, 0, &type, (char*)&val, &count)) 
        val = 0;
    cfg->quick_edit = val;

    count = sizeof(val);
    if (!hConKey || RegQueryValueEx(hConKey, wszScreenBufferSize, 0, &type, (char*)&val, &count)) 
        val = 0x00190050;
    cfg->sb_height = HIWORD(val);
    cfg->sb_width  = LOWORD(val);

    count = sizeof(val);
    if (!hConKey || RegQueryValueEx(hConKey, wszScreenColors, 0, &type, (char*)&val, &count)) 
        val = 0x000F;
    cfg->def_attr = val;

    count = sizeof(val);
    if (!hConKey || RegQueryValueEx(hConKey, wszWindowSize, 0, &type, (char*)&val, &count)) 
        val = 0x00190050;
    cfg->win_height = HIWORD(val);
    cfg->win_width  = LOWORD(val);

    /* win_pos isn't read from registry */

    if (hConKey) RegCloseKey(hConKey);
    return TRUE;
}

/******************************************************************
 *		WINECON_RegSave
 *
 *
 */
BOOL WINECON_RegSave(const struct config_data* cfg)
{
    HKEY        hConKey;
    DWORD       val;

    if (RegCreateKey(HKEY_CURRENT_USER, wszConsole, &hConKey)) 
    {
        Trace(0, "Can't open registry for saving\n");
        return FALSE;
    }
   
    val = cfg->cursor_size;
    RegSetValueEx(hConKey, wszCursorSize, 0, REG_DWORD, (char*)&val, sizeof(val));

    val = cfg->cursor_visible;
    RegSetValueEx(hConKey, wszCursorVisible, 0, REG_DWORD, (char*)&val, sizeof(val));

    RegSetValueEx(hConKey, wszFaceName, 0, REG_SZ, (char*)&cfg->face_name, sizeof(cfg->face_name));

    val = MAKELONG(cfg->cell_width, cfg->cell_height);
    RegSetValueEx(hConKey, wszFontSize, 0, REG_DWORD, (char*)&val, sizeof(val));

    val = cfg->font_weight;
    RegSetValueEx(hConKey, wszFontWeight, 0, REG_DWORD, (char*)&val, sizeof(val));

    val = cfg->history_size;
    RegSetValueEx(hConKey, wszHistoryBufferSize, 0, REG_DWORD, (char*)&val, sizeof(val));

    val = cfg->menu_mask;
    RegSetValueEx(hConKey, wszMenuMask, 0, REG_DWORD, (char*)&val, sizeof(val));

    val = cfg->quick_edit;
    RegSetValueEx(hConKey, wszQuickEdit, 0, REG_DWORD, (char*)&val, sizeof(val));

    val = MAKELONG(cfg->sb_width, cfg->sb_height);
    RegSetValueEx(hConKey, wszScreenBufferSize, 0, REG_DWORD, (char*)&val, sizeof(val));

    val = cfg->def_attr;
    RegSetValueEx(hConKey, wszScreenColors, 0, REG_DWORD, (char*)&val, sizeof(val));

    val = MAKELONG(cfg->win_width, cfg->win_height);
    RegSetValueEx(hConKey, wszWindowSize, 0, REG_DWORD, (char*)&val, sizeof(val));

    RegCloseKey(hConKey);
    return TRUE;
}
