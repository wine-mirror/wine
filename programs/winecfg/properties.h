/*
 * WineCfg configuration properties
 *
 * Copyright 2002 Jaco Greeff
 * Copyright 2003 Dimitrie O. Paun
 * Copyright 2003 Mark Westcott
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

#ifndef WINE_CFG_PROPERTIES_H
#define WINE_CFG_PROPERTIES_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"

#define MAX_NAME_LENGTH         64
#define MAX_VERSION_LENGTH      48
#define MAX_DESCRIPTION_LENGTH 128

typedef struct
{
    char  szVersion[MAX_VERSION_LENGTH];
    char  szDescription[MAX_DESCRIPTION_LENGTH];
} VERSION_DESC;

#define DLL_NATIVE  0x0000
#define DLL_BUILTIN 0x0001

typedef struct
{
    char  szName[MAX_NAME_LENGTH];
    int   nType;
} DLL_DESC;

typedef struct
{
    char  szName[MAX_NAME_LENGTH];
    char  szWinVer[MAX_VERSION_LENGTH];
    char  szDOSVer[MAX_VERSION_LENGTH];
    HDPA  DLLs;
} APP_DESC;

typedef struct
{
  char szName[MAX_NAME_LENGTH];
  char szDriver[MAX_NAME_LENGTH];
} AUDIO_DRIVER;

typedef struct
{
    char   szX11Display[MAX_NAME_LENGTH];
    int    nSysColors;
    int    nPrivateMap;
    int    nPerfect;
    int    nDepth;
    int    nManaged;
    int    nDesktopSizeX;
    int    nDesktopSizeY;
    int    nDGA;
    int    nXVidMode;
    int    nXShm;
    int    nTextCP;
    int    nXVideoPort;
    int    nTakeFocus;
    int    nDXGrab;
    int    nDoubleBuffered;
    int    nSynchronous;
} X11DRV_DESC;

VERSION_DESC *getWinVersions(void);
VERSION_DESC *getDOSVersions(void);
VERSION_DESC *getWinelook(void);
DLL_DESC *getDLLDefaults(void);
AUDIO_DRIVER *getAudioDrivers(void);

#endif
