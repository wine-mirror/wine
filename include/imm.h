/*
 *	imm.h	-	Declarations for IMM32
 */

#ifndef __WINE_IMM_H
#define __WINE_IMM_H

typedef DWORD HIMC32;
typedef DWORD HIMCC32;

typedef HKL32 *LPHKL32;

typedef int (CALLBACK *REGISTERWORDENUMPROCA)(LPCSTR, DWORD, LPCSTR, LPVOID);
typedef int (CALLBACK *REGISTERWORDENUMPROCW)(LPCWSTR, DWORD, LPCWSTR, LPVOID);

typedef void *LPCANDIDATELIST;
typedef void *LPCANDIDATEFORM;

typedef void *LPSTYLEBUFA;
typedef void *LPSTYLEBUFW;

typedef void *LPCOMPOSITIONFORM;

#endif  /* __WINE_IMM_H */
