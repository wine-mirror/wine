/*
 * Old C RunTime DLL - All functionality is provided by msvcrt.
 *
 * Copyright 2000 Jon Griffiths
 */
#include "config.h"
#include "windef.h"
#include "winbase.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(crtdll);

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
