/*
 * SHLWAPI initialisation
 *
 *  Copyright 1998 Marcus Meissner
 *  Copyright 1998 Juergen Schmied (jsch)
 */

#include "winbase.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(shell);

HINSTANCE shlwapi_hInstance = 0; 

/*************************************************************************
 * SHLWAPI LibMain
 *
 * NOTES
 *  calling oleinitialize here breaks sone apps.
 */
BOOL WINAPI SHLWAPI_LibMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
	TRACE("0x%x 0x%lx %p\n", hinstDLL, fdwReason, fImpLoad);
	switch (fdwReason)
	{
	  case DLL_PROCESS_ATTACH:
	    shlwapi_hInstance = hinstDLL;
	    break;
	}
	return TRUE;
}
