/*
 * Old C RunTime DLL - All functionality is provided by msvcrt.
 *
 * Copyright 2000 Jon Griffiths
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
 */
#include "config.h"
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crtdll);

/* from msvcrt */
extern void __getmainargs( int *argc, char ***argv, char ***envp,
                           int expand_wildcards, int *new_mode );

/* The following data items are not exported from msvcrt */
unsigned int CRTDLL__basemajor_dll;
unsigned int CRTDLL__baseminor_dll;
unsigned int CRTDLL__baseversion_dll;
unsigned int CRTDLL__cpumode_dll;
unsigned int CRTDLL__osmajor_dll;
unsigned int CRTDLL__osminor_dll;
unsigned int CRTDLL__osmode_dll;
unsigned int CRTDLL__osversion_dll;

/*********************************************************************
 *                  CRTDLL_MainInit  (CRTDLL.init)
 */
BOOL WINAPI CRTDLL_Init(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
  TRACE("(0x%08x,%ld,%p)\n",hinstDLL,fdwReason,lpvReserved);

  if (fdwReason == DLL_PROCESS_ATTACH)
  {
    DWORD version = GetVersion();
    CRTDLL__basemajor_dll   = (version >> 24) & 0xFF;
    CRTDLL__baseminor_dll   = (version >> 16) & 0xFF;
    CRTDLL__baseversion_dll = (version >> 16);
    CRTDLL__cpumode_dll     = 1; /* FIXME */
    CRTDLL__osmajor_dll     = (version>>8) & 0xFF;
    CRTDLL__osminor_dll     = (version & 0xFF);
    CRTDLL__osmode_dll      = 1; /* FIXME */
    CRTDLL__osversion_dll   = (version & 0xFFFF);
  }
  return TRUE;
}


/*********************************************************************
 *                  __GetMainArgs  (CRTDLL.@)
 */
void __GetMainArgs( int *argc, char ***argv, char ***envp, int expand_wildcards )
{
    int new_mode = 0;
    __getmainargs( argc, argv, envp, expand_wildcards, &new_mode );
}
