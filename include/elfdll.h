#ifndef __WINE_ELFDLL_H
#define __WINE_ELFDLL_H

#include "module.h"
#include "windef.h"

WINE_MODREF *ELFDLL_LoadLibraryExA(LPCSTR libname, DWORD flags, DWORD *err);
HINSTANCE16 ELFDLL_LoadModule16(LPCSTR libname);
void ELFDLL_UnloadLibrary(WINE_MODREF *wm);

#if defined(HAVE_DL_API)

void *ELFDLL_dlopen(const char *libname, int flags);
extern char *extra_ld_library_path;

#endif

#endif
