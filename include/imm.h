/*
 *	imm.h	-	Declarations for IMM32
 */

#ifndef __WINE_IMM_H
#define __WINE_IMM_H

#include "wintypes.h"

typedef HANDLE HIMC;
typedef HANDLE HIMCC;

typedef HKL *LPHKL;

typedef int (CALLBACK *REGISTERWORDENUMPROCA)(LPCSTR, DWORD, LPCSTR, LPVOID);
typedef int (CALLBACK *REGISTERWORDENUMPROCW)(LPCWSTR, DWORD, LPCWSTR, LPVOID);

typedef void *LPCANDIDATELIST;
typedef void *LPCANDIDATEFORM;

typedef void *LPSTYLEBUFA;
typedef void *LPSTYLEBUFW;

typedef void *LPCOMPOSITIONFORM;

#endif  /* __WINE_IMM_H */
