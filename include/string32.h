/*
 * Unicode string management
 *
 * Copyright 1995 Martin von Loewis
 *
 */

#ifndef _STRING32_H
#define _STRING32_H

#include "wintypes.h"

int STRING32_UniLen(LPWSTR s);
void STRING32_UniToAnsi(LPSTR dest,LPCWSTR src);
void STRING32_AnsiToUni(LPWSTR dest,LPCSTR src);
LPSTR STRING32_DupUniToAnsi(LPWSTR src);
LPWSTR STRING32_DupAnsiToUni(LPSTR src);
LPWSTR STRING32_lstrcmpnW(LPCWSTR a,LPCWSTR b,DWORD len);

#endif
