/* Includefile for the decompression library, lzexpand
 *
 * Copyright 1996 Marcus Meissner
 * FIXME: Who's copyright are the prototypes?
 */

#ifndef __WINE_LZEXPAND_H
#define __WINE_LZEXPAND_H

#include "wintypes.h"

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
LONG        WINAPI CopyLZFile32(HFILE32,HFILE32);
#define     CopyLZFile WINELIB_NAME(CopyLZFile)
HFILE16     WINAPI LZOpenFile16(LPCSTR,LPOFSTRUCT,UINT16);
HFILE32     WINAPI LZOpenFile32A(LPCSTR,LPOFSTRUCT,UINT32);
HFILE32     WINAPI LZOpenFile32W(LPCWSTR,LPOFSTRUCT,UINT32);
#define     LZOpenFile WINELIB_NAME_AW(LZOpenFile)
INT16       WINAPI LZRead16(HFILE16,LPVOID,UINT16); 
INT32       WINAPI LZRead32(HFILE32,LPVOID,UINT32); 
#define     LZRead WINELIB_NAME(LZRead)
INT16       WINAPI LZStart16(void);
INT32       WINAPI LZStart32(void);
#define     LZStart WINELIB_NAME(LZStart)
void        WINAPI LZClose16(HFILE16);
void        WINAPI LZClose32(HFILE32);
#define     LZClose WINELIB_NAME(LZClose)
LONG        WINAPI LZCopy16(HFILE16,HFILE16);
LONG        WINAPI LZCopy32(HFILE32,HFILE32);
#define     LZCopy WINELIB_NAME(LZCopy)
HFILE16     WINAPI LZInit16(HFILE16);
HFILE32     WINAPI LZInit32(HFILE32);
#define     LZInit WINELIB_NAME(LZInit)
LONG        WINAPI LZSeek16(HFILE16,LONG,INT16);
LONG        WINAPI LZSeek32(HFILE32,LONG,INT32);
#define     LZSeek WINELIB_NAME(LZSeek)
INT16       WINAPI GetExpandedName16(LPCSTR,LPSTR);
INT32       WINAPI GetExpandedName32A(LPCSTR,LPSTR);
INT32       WINAPI GetExpandedName32W(LPCWSTR,LPWSTR);
#define     GetExpandedName WINELIB_NAME_AW(GetExpandedName)

#endif  /* __WINE_LZEXPAND_H */
