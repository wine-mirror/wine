/*
 * WineCfg configuration management
 *
 * Copyright 2002 Jaco Greeff
 * Copyright 2003 Dimitrie O. Paun
 * Copyright 2003 Mike Hearn
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * TODO:   (in rough order of priority)
 *   - A mind bogglingly vast amount of stuff
 *
 *   - Complete X11DRV page, so all controls are hooked up
 *   - Implement autodetect for drive configuration
 *   - Figure out whether we need the virtual vs real drive selection stuff at the top of the property page
 *   - Implement explicit mode vs instant-apply mode
 *   - AppDefaults handling
 *   - DLL editing
 *   - Make a list of prefs that have been dropped from the UI/should be eliminated
 *   - Multimedia page
 *   - Settings migration code (from old configs)
 *
 */

#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <windows.h>
#include <winreg.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

#include "winecfg.h"

HKEY configKey = NULL;


int initialize(void) {
    DWORD res = RegCreateKey(HKEY_LOCAL_MACHINE, WINE_KEY_ROOT, &configKey);
    if (res != ERROR_SUCCESS) {
	WINE_ERR("RegOpenKey failed on wine config key (%ld)\n", res);
	return 1;
    }
    return 0;
}


/*****************************************************************************
 * getConfigValue: Retrieves a configuration value from the registry
 *
 * char *subKey : the name of the config section
 * char *valueName : the name of the config value
 * char *defaultResult : if the key isn't found, return this value instead
 *
 * Returns a buffer holding the value if successful, NULL if not. Caller is responsible for freeing the result.
 *
 */
char *getConfigValue (char *subkey, char *valueName, char *defaultResult)
{
    char *buffer = NULL;
    DWORD dataLength;
    HKEY hSubKey = NULL;
    DWORD res;

    WINE_TRACE("subkey=%s, valueName=%s, defaultResult=%s\n", subkey, valueName, defaultResult);

    res = RegOpenKeyEx( configKey, subkey, 0, KEY_ALL_ACCESS, &hSubKey );
    if(res != ERROR_SUCCESS)  {
        if( res==ERROR_FILE_NOT_FOUND )
        {
            WINE_TRACE("Section key not present - using default\n");
            return strdup(defaultResult);
        }
        else
        {
            WINE_ERR("RegOpenKey failed on wine config key (res=%ld)\n", res);
        }
        goto end;
    }
    
    res = RegQueryValueExA( hSubKey, valueName, NULL, NULL, NULL, &dataLength);
    if( res == ERROR_FILE_NOT_FOUND ) {
        WINE_TRACE("Value not present - using default\n");
        buffer = strdup(defaultResult);
	goto end;
    } else if( res!=ERROR_SUCCESS )  {
        WINE_ERR("Couldn't query value's length (res=%ld)\n", res );
        goto end;
    }

    buffer = malloc(dataLength);
    if( buffer==NULL )
    {
        WINE_ERR("Couldn't allocate %lu bytes for the value\n", dataLength );
        goto end;
    }
    
    RegQueryValueEx(hSubKey, valueName, NULL, NULL, (LPBYTE)buffer, &dataLength);
    
end:
    if( hSubKey!=NULL )
        RegCloseKey( hSubKey );

    return buffer;
    
}

/*****************************************************************************
 * setConfigValue : Sets a configuration key in the registry. Section will be created if it doesn't already exist
 *
 * HKEY  hCurrent : the registry key that the configuration is rooted at
 * char *subKey : the name of the config section
 * char *valueName : the name of the config value
 * char *value : the value to set the configuration key to
 *
 * Returns 0 on success, non-zero otherwise
 *
 */
int setConfigValue (char *subkey, char *valueName, const char *value) {
    DWORD res = 1;
    HKEY key = NULL;

    WINE_TRACE("subkey=%s, valueName=%s, value=%s\n", subkey, valueName, value);

    assert( subkey != NULL );
    assert( valueName != NULL );
    assert( value != NULL );
    
    res = RegCreateKey(configKey, subkey, &key);
    if (res != ERROR_SUCCESS) goto end;

    res = RegSetValueEx(key, valueName, 0, REG_SZ, value, strlen(value) + 1);
    if (res != ERROR_SUCCESS) goto end;

    res = 0;
end:
    if (key) RegCloseKey(key);
    if (res != 0) WINE_ERR("Unable to set configuration key %s in section %s to %s, res=%ld\n", valueName, subkey, value, res);
    return res;
}

/* returns 0 on success, an HRESULT from the registry funtions otherwise */
HRESULT doesConfigValueExist(char *subkey, char *valueName) {
    HRESULT hr;
    HKEY key;

    WINE_TRACE("subkey=%s, valueName=%s - ", subkey, valueName);
    
    hr = RegOpenKeyEx(configKey, subkey, 0, KEY_READ, &key);
    if (hr != S_OK) {
	WINE_TRACE("no: subkey does not exist\n");
	return hr;
    }

    hr = RegQueryValueEx(key, valueName, NULL, NULL, NULL, NULL);
    if (hr != S_OK) {
	WINE_TRACE("no: key does not exist\n");
	return hr;
    }

    RegCloseKey(key);
    WINE_TRACE("yes\n");
    return S_OK;
}

/* removes the requested value from the registry, however, does not remove the section if empty. Returns S_OK (0) on success. */
HRESULT removeConfigValue(char *subkey, char *valueName) {
    HRESULT hr;
    HKEY key;
    WINE_TRACE("subkey=%s, valueName=%s\n", subkey, valueName);
    
    hr = RegOpenKeyEx(configKey, subkey, 0, KEY_READ, &key);
    if (hr != S_OK) return hr;

    hr = RegDeleteValue(key, valueName);
    if (hr != ERROR_SUCCESS) return hr;

    return S_OK;
}


/* ========================================================================= */
/* Transaction management code */

struct transaction *tqhead, *tqtail;
int instantApply = 1;

void destroyTransaction(struct transaction *trans) {
    assert( trans != NULL );
    
    WINE_TRACE("destroying %p\n", trans);
    
    free(trans->section);
    free(trans->key);
    if (trans->newValue) free(trans->newValue);
    
    if (trans->next) trans->next->prev = trans->prev;
    if (trans->prev) trans->prev->next = trans->next;
    if (trans == tqhead) tqhead = NULL;
    if (trans == tqtail) tqtail = NULL;
    
    free(trans);
}

void addTransaction(char *section, char *key, enum transaction_action action, char *newValue) {
    struct transaction *trans = malloc(sizeof(struct transaction));
    
    assert( section != NULL );
    assert( key != NULL );
    if (action == ACTION_SET) assert( newValue != NULL );

    trans->section = strdup(section);
    trans->key = strdup(key);
    trans->newValue = strdup(newValue);
    trans->action = action;
    trans->next = NULL;
    trans->prev = NULL;
    
    if (tqtail == NULL) {
	tqtail = trans;
	tqhead = tqtail;
    } else {
	tqhead->next = trans;
	trans->prev = tqhead;
	tqhead = trans;
    }

    if (instantApply) {
	processTransaction(trans);
	destroyTransaction(trans);
    }
}

void processTransaction(struct transaction *trans) {
    if (trans->action == ACTION_SET) {
	WINE_TRACE("Setting %s\\%s to '%s'\n", trans->section, trans->key, trans->newValue);
	setConfigValue(trans->section, trans->key, trans->newValue);
    } else if (trans->action == ACTION_REMOVE) {
	WINE_TRACE("Removing %s\\%s", trans->section, trans->key);
	removeConfigValue(trans->section, trans->key);
    }
    /* TODO: implement notifications here */
}

void processTransQueue(void)
{
    WINE_TRACE("\n");
    while (tqtail != NULL) {
	struct transaction *next = tqtail->next;
	processTransaction(tqtail);
	destroyTransaction(tqtail);
	tqtail = next;	
    }
}


/* ================================== utility functions ============================ */

/* returns a string with the window text of the dialog item. user is responsible for freeing the result */
char *getDialogItemText(HWND hDlg, WORD controlID) {
    HWND item = GetDlgItem(hDlg, controlID);
    int len = GetWindowTextLength(item) + 1;
    char *result = malloc(len);
    if (GetWindowText(item, result, len) == 0) return NULL;
    return result;
}
