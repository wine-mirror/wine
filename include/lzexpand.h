/* Includefile for the decompression library, lzexpand
 *
 * Copyright 1996 Marcus Meissner
 * FIXME: Who's copyright are the prototypes?
 */

#ifndef __WINE_LZEXPAND_H
#define __WINE_LZEXPAND_H

#include "windef.h"

#define LZERROR_BADINHANDLE	-1	/* -1 */
#define LZERROR_BADOUTHANDLE	-2	/* -2 */
#define LZERROR_READ		-3	/* -3 */
#define LZERROR_WRITE		-4	/* -4 */
#define LZERROR_GLOBALLOC	-5	/* -5 */
#define LZERROR_GLOBLOCK	-6	/* -6 */
#define LZERROR_BADVALUE	-7	/* -7 */
#define LZERROR_UNKNOWNALG	-8	/* -8 */

VOID        WINAPI LZDone(void);
LONG        WINAPI CopyLZFile16(HFILE16,HFILE16);
LONG        WINAPI CopyLZFile(HFILE,HFILE);
HFILE16     WINAPI LZOpenFile16(LPCSTR,LPOFSTRUCT,UINT16);
HFILE     WINAPI LZOpenFileA(LPCSTR,LPOFSTRUCT,UINT);
HFILE     WINAPI LZOpenFileW(LPCWSTR,LPOFSTRUCT,UINT);
#define     LZOpenFile WINELIB_NAME_AW(LZOpenFile)
INT16       WINAPI LZRead16(HFILE16,LPVOID,UINT16); 
INT       WINAPI LZRead(HFILE,LPVOID,UINT); 
INT16       WINAPI LZStart16(void);
INT       WINAPI LZStart(void);
void        WINAPI LZClose16(HFILE16);
void        WINAPI LZClose(HFILE);
LONG        WINAPI LZCopy16(HFILE16,HFILE16);
LONG        WINAPI LZCopy(HFILE,HFILE);
HFILE16     WINAPI LZInit16(HFILE16);
HFILE     WINAPI LZInit(HFILE);
LONG        WINAPI LZSeek16(HFILE16,LONG,INT16);
LONG        WINAPI LZSeek(HFILE,LONG,INT);
INT16       WINAPI GetExpandedName16(LPCSTR,LPSTR);
INT       WINAPI GetExpandedNameA(LPCSTR,LPSTR);
INT       WINAPI GetExpandedNameW(LPCWSTR,LPWSTR);
#define     GetExpandedName WINELIB_NAME_AW(GetExpandedName)

#endif  /* __WINE_LZEXPAND_H */
