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
    {"", "Use default(Global setting)"},
    {"win20", "Windows 2.0"},
    {"win30", "Windows 3.0"},
    {"win31", "Windows 3.1"},
    {"nt351", "Windows NT 3.5"},
    {"nt40", "Windows NT 4.0"},
    {"win95", "Windows 95"},
    {"win98", "Windows 98"},
    {"winme", "Windows ME"},
    {"win2k", "Windows 2000"},
    {"winxp", "Windows XP"},
    {"win2003", "Windows 2003"},
    {"", ""}
};

static VERSION_DESC sDOSVersions[] = {
    {"", "Use default(Global setting)"},
    {"6.22", "MS-DOS 6.22"},
    {"", ""}
};

static VERSION_DESC sWineLook[] = {
    {"", "Use default(Global setting)"},
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

static AUDIO_DRIVER sAudioDrivers[] = {
  {"Alsa", "winealsa.drv"},
  {"aRts", "winearts.drv"},
  {"OSS", "wineoss.drv"},
  {"Jack", "winejack.drv"},
  {"Nas", "winenas.drv"},
  {"Audio IO(Solaris)", "wineaudioio.drv"},
  {"Disable sound", ""},
  {"", ""}
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

/*****************************************************************************
 */
AUDIO_DRIVER* getAudioDrivers(void)
{
    return sAudioDrivers;
}


/* Functions to convert from version to description and back */
char* getVersionFromDescription(VERSION_DESC* pVer, char *desc)
{
  for (; *pVer->szVersion; pVer++)
  {
    if(!strcasecmp(pVer->szDescription, desc))
      return pVer->szVersion;
  }

  return NULL;
}

char* getDescriptionFromVersion(VERSION_DESC* pVer, char *ver)
{
  for (; *pVer->szDescription; pVer++)
  {
    if(!strcasecmp(pVer->szVersion, ver))
      return pVer->szDescription;
  }

  return NULL;
}
