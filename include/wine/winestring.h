#ifndef __WINE_WINE_WINESTRING_H
#define __WINE_WINE_WINESTRING_H

#include "windef.h"

INT         WINAPI WideCharToLocal(LPSTR,LPCWSTR,INT);
INT         WINAPI LocalToWideChar(LPWSTR,LPCSTR,INT);
LPWSTR      WINAPI lstrcpyAtoW(LPWSTR,LPCSTR);
LPSTR       WINAPI lstrcpyWtoA(LPSTR,LPCWSTR);
LPWSTR      WINAPI lstrcpynAtoW(LPWSTR,LPCSTR,INT);
LPSTR       WINAPI lstrcpynWtoA(LPSTR,LPCWSTR,INT);

#define lstrncmpiA strncasecmp

#endif /* __WINE_WINE_WINESTRING_H */
