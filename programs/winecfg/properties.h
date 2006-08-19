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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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
  UINT nameID;
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

typedef struct
{
  char szNode[MAX_NAME_LENGTH];
  int nType;
} DEV_NODES;

#endif
