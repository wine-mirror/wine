/*
 * WineCfg properties management
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

#include <windows.h>

#include "properties.h"

static VERSION_DESC sWinVersions[] = {
    {"win20", "Windows 2.0"},
    {"win30", "Windows 3.0"},
    {"win31", "Windows 3.1"},
    {"nt351", "Windows NT 3.5"},
    {"nt40", "Windows NT 4.0"},
    {"win95", "Windows 95"},
    {"win98", "Windows 98"},
    {"winme", "Windows ME"},
    {"win2k", "Windows 2000"},
    {"win2k3", "Windows 2003"},
    {"winxp", "Windows XP"},
    {"", ""}
};

static VERSION_DESC sDOSVersions[] = {
    {"6.22", "MS-DOS 6.22"},
    {"", ""}
};

static VERSION_DESC sWineLook[] = {
    {"win31", "Windows 3.1"},
    {"win95", "Windows 95"},
    {"win98", "Windows 98"},
    {"", ""}
};

#if 0
static VERSION_DESC sWineDrivers[] = {
    {"x11drv", "X11 Interface"},
    {"ttydrv", "TTY Interface"},
    {"", ""}
};
#endif

static DLL_DESC sDLLType[] = {
    {"oleaut32", DLL_BUILTIN},
    {"ole32", DLL_BUILTIN},
    {"commdlg", DLL_BUILTIN},
    {"comdlg32", DLL_BUILTIN},
    {"shell", DLL_BUILTIN},
    {"shell32", DLL_BUILTIN},
    {"shfolder", DLL_BUILTIN},
    {"shlwapi", DLL_BUILTIN},
    {"shdocvw", DLL_BUILTIN},
    {"advapi32", DLL_BUILTIN},
    {"msvcrt", DLL_NATIVE},
    {"mciavi.drv", DLL_NATIVE},
    {"mcianim.drv", DLL_NATIVE},
    {"*", DLL_NATIVE},
    {"", -1}
};


/*****************************************************************************
 */
VERSION_DESC* getWinVersions(void)
{
    return sWinVersions;
}


/*****************************************************************************
 */
VERSION_DESC* getDOSVersions(void)
{
    return sDOSVersions;
}


/*****************************************************************************
 */
VERSION_DESC* getWinelook(void)
{
    return sWineLook;
}


/*****************************************************************************
 */
DLL_DESC* getDLLDefaults(void)
{
    return sDLLType;
}
