#ifndef __WINE_ELFDLL_H
#define __WINE_ELFDLL_H

WINE_MODREF *ELFDLL_LoadLibraryExA(LPCSTR libname, DWORD flags, DWORD *err);
HINSTANCE16 ELFDLL_LoadModule16(LPCSTR libname, BOOL implicit);
void ELFDLL_UnloadLibrary(WINE_MODREF *wm);

#endif
