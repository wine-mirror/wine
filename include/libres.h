/*
 * WINElib-Resources
 */
#ifndef __WINE_LIBRES_H
#define __WINE_LIBRES_H

#include "wintypes.h"

extern HRSRC LIBRES_FindResource( HINSTANCE hModule, LPCWSTR name, LPCWSTR type );
extern HGLOBAL LIBRES_LoadResource( HINSTANCE hModule, HRSRC hRsrc );
extern DWORD LIBRES_SizeofResource( HINSTANCE hModule, HRSRC hRsrc );

#endif /* __WINE_LIBRES_H */
