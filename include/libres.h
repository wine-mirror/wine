/*
 * WINElib-Resources
 */
#ifndef __WINE_LIBRES_H
#define __WINE_LIBRES_H

#ifdef WINELIB

#include "windows.h"
#include "resource.h"

void    LIBRES_RegisterResources(struct resource** Res);

INT     LIBRES_AccessResource( HINSTANCE hModule, HRSRC hRsrc );
HGLOBAL LIBRES_AllocResource( HINSTANCE hModule, HRSRC hRsrc, DWORD size );
HRSRC   LIBRES_FindResource( HINSTANCE hModule, LPCSTR name, LPCSTR type );
BOOL    LIBRES_FreeResource( HGLOBAL handle );
HGLOBAL LIBRES_LoadResource( HINSTANCE hModule, HRSRC hRsrc );
LPVOID  LIBRES_LockResource( HGLOBAL handle );
DWORD   LIBRES_SizeofResource( HINSTANCE hModule, HRSRC hRsrc );

#endif /* WINELIB */

#endif
