/*
 * WINElib-Resources
 */
#ifndef __WINE_LIBRES_H
#define __WINE_LIBRES_H

#include "wintypes.h"
#include "resource.h"

extern HRSRC32 LIBRES_FindResource( HINSTANCE32 hModule, LPCWSTR name, LPCWSTR type );
extern HGLOBAL32 LIBRES_LoadResource( HINSTANCE32 hModule, HRSRC32 hRsrc );
extern DWORD LIBRES_SizeofResource( HINSTANCE32 hModule, HRSRC32 hRsrc );

#endif /* __WINE_LIBRES_H */
