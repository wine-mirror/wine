/*
 * WineCfg configuration management
 *
 * Copyright 2002 Jaco Greeff
 * Copyright 2003 Dimitrie O. Paun
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
 */

#ifndef WINE_CFG_H
#define WINE_CFG_H

#include "properties.h"

#define IS_OPTION_TRUE(ch) \
    ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')
#define IS_OPTION_FALSE(ch) \
    ((ch) == 'n' || (ch) == 'N' || (ch) == 'f' || (ch) == 'F' || (ch) == '0')

typedef struct structWineCfg
{
    char   szWinVer[MAX_VERSION_LENGTH];
    char   szWinLook[MAX_VERSION_LENGTH];
    char   szDOSVer[MAX_VERSION_LENGTH];

    char   szWinDir[MAX_PATH];
    char   szWinSysDir[MAX_PATH];
    char   szWinPath[MAX_PATH];
    char   szWinTmpDir[MAX_PATH];
    char   szWinProfDir[MAX_PATH];

    char   szGraphDriver[MAX_NAME_LENGTH];

    HDPA     pDlls;
    HDPA     pApps;

    X11DRV_DESC sX11Drv;
} WINECFG_DESC;

extern WINECFG_DESC config;

WINECFG_DESC *allocConfig(void);
int freeConfig(WINECFG_DESC *pCfg);

int loadConfig(WINECFG_DESC *pCfg);
int saveConfig(const WINECFG_DESC *pCfg);

int setConfigValue (HKEY hCurrent, char *subkey, char *valueName, const char *value);
int getConfigValue (HKEY hCurrent, char *subkey, char *valueName, char *retVal, int length, char *defaultResult);

void initX11DrvDlg (HWND hDlg);
void saveX11DrvDlgSettings (HWND hDlg);
INT_PTR CALLBACK X11DrvDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define WINE_KEY_ROOT "Software\\Wine\\Wine\\Config"

#endif
