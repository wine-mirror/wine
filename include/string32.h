/*
 * Unicode string management
 *
 * Copyright 1995 Martin von Loewis
 *
 */

#ifndef __WINE_STRING32_H
#define __WINE_STRING32_H

#include "wintypes.h"

LPWSTR	STRING32_DupAnsiToUni(LPCSTR src);
LPWSTR	STRING32_strdupW(LPCWSTR);

#endif  /* __WINE_STRING32_H */
