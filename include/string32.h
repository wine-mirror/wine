/*
 * Unicode string management
 *
 * Copyright 1995 Martin von Loewis
 *
 */

#ifndef _STRING32_H
#define _STRING32_H

#include "wintypes.h"

int	STRING32_UniLen(LPCWSTR s);
void	STRING32_UniToAnsi(LPSTR dest,LPCWSTR src);
void	STRING32_AnsiToUni(LPWSTR dest,LPCSTR src);
LPSTR	STRING32_DupUniToAnsi(LPCWSTR src);
LPWSTR	STRING32_DupAnsiToUni(LPCSTR src);
LPWSTR  STRING32_lstrcpyW(LPWSTR dst, LPCWSTR src);
int	STRING32_lstrcmpnW(LPCWSTR a,LPCWSTR b,DWORD len);
int	STRING32_lstrcmpniW(LPCWSTR a,LPCWSTR b,DWORD len);
DWORD	STRING32_lstrlenW(LPCWSTR);
LPWSTR	STRING32_strdupW(LPCWSTR);
int	STRING32_lstrcmpW(LPCWSTR,LPCWSTR);

#endif
