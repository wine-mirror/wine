/*
 * WINElib-Resources
 */
#ifndef __WINE_LIBRES_H
#define __WINE_LIBRES_H

#include "windows.h"

#ifdef WINELIB
HRSRC LIBRES_FindResource( HMODULE hModule, SEGPTR name, SEGPTR type );
HGLOBAL LIBRES_LoadResource( HMODULE hModule, HRSRC hRsrc );
LPSTR LIBRES_LockResource( HMODULE hModule, HGLOBAL handle );
BOOL LIBRES_FreeResource( HMODULE hModule, HGLOBAL handle );
INT LIBRES_AccessResource( HINSTANCE hModule, HRSRC hRsrc );
DWORD LIBRES_SizeofResource( HMODULE hModule, HRSRC hRsrc );
HGLOBAL LIBRES_AllocResource( HMODULE hModule, HRSRC hRsrc, DWORD size );
#endif

#endif
