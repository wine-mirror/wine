/* Includefile for the decompression library, lzexpand
 *
 * Copyright 1996 Marcus Meissner
 * FIXME: Who's copyright are the prototypes?
 */

#ifndef __WINE_LZEXPAND_H
#define __WINE_LZEXPAND_H

#include "windef.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#define LZERROR_BADINHANDLE	-1	/* -1 */
#define LZERROR_BADOUTHANDLE	-2	/* -2 */
#define LZERROR_READ		-3	/* -3 */
#define LZERROR_WRITE		-4	/* -4 */
#define LZERROR_GLOBALLOC	-5	/* -5 */
#define LZERROR_GLOBLOCK	-6	/* -6 */
#define LZERROR_BADVALUE	-7	/* -7 */
#define LZERROR_UNKNOWNALG	-8	/* -8 */

VOID        WINAPI LZDone(void);
LONG        WINAPI CopyLZFile(HFILE,HFILE);
HFILE       WINAPI LZOpenFileA(LPCSTR,LPOFSTRUCT,UINT);
HFILE       WINAPI LZOpenFileW(LPCWSTR,LPOFSTRUCT,UINT);
#define     LZOpenFile WINELIB_NAME_AW(LZOpenFile)
INT         WINAPI LZRead(HFILE,LPVOID,UINT);
INT         WINAPI LZStart(void);
void        WINAPI LZClose(HFILE);
LONG        WINAPI LZCopy(HFILE,HFILE);
HFILE       WINAPI LZInit(HFILE);
LONG        WINAPI LZSeek(HFILE,LONG,INT);
INT         WINAPI GetExpandedNameA(LPCSTR,LPSTR);
INT         WINAPI GetExpandedNameW(LPCWSTR,LPWSTR);
#define     GetExpandedName WINELIB_NAME_AW(GetExpandedName)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif  /* __WINE_LZEXPAND_H */
