#ifndef __WINE_WINE_WINESTRING_H
#define __WINE_WINE_WINESTRING_H

#include "windef.h"
#include "winnls.h"

LPWSTR      WINAPI lstrcpynAtoW(LPWSTR,LPCSTR,INT);
LPSTR       WINAPI lstrcpynWtoA(LPSTR,LPCWSTR,INT);

/* compatibility macros; will be removed some day, please don't use them */
#define lstrcpyAtoW(dst,src) ((void)MultiByteToWideChar(CP_ACP,0,(src),-1,(dst),0x7fffffff))
#define lstrcpyWtoA(dst,src) ((void)WideCharToMultiByte(CP_ACP,0,(src),-1,(dst),0x7fffffff,NULL,NULL))
#define lstrncmpiA strncasecmp

#endif /* __WINE_WINE_WINESTRING_H */
