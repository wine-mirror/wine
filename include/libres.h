/*
 * WINElib-Resources
 */
#ifndef __WINE_LIBRES_H
#define __WINE_LIBRES_H

#ifdef WINELIB

#include "wintypes.h"
#include "resource.h"

extern INT     LIBRES_AccessResource( HINSTANCE hModule, HRSRC32 hRsrc );
extern HGLOBAL32 LIBRES_AllocResource( HINSTANCE hModule, HRSRC32 hRsrc, DWORD size );
extern HRSRC32 LIBRES_FindResource( HINSTANCE hModule, LPCSTR name, LPCSTR type );
extern BOOL    LIBRES_FreeResource( HGLOBAL32 handle );
extern HGLOBAL32 LIBRES_LoadResource( HINSTANCE hModule, HRSRC32 hRsrc );
extern LPVOID  LIBRES_LockResource( HGLOBAL32 handle );
extern DWORD   LIBRES_SizeofResource( HINSTANCE hModule, HRSRC32 hRsrc );

#endif /* WINELIB */

#endif
