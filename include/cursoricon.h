/*
 * Cursor and icon definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_CURSORICON_H
#define __WINE_CURSORICON_H

#include "windef.h"

#include "pshpack1.h"

typedef struct
{
    BYTE   bWidth;
    BYTE   bHeight;
    BYTE   bColorCount;
    BYTE   bReserved;
} ICONRESDIR;

typedef struct
{
    WORD   wWidth;
    WORD   wHeight;
} CURSORDIR;

typedef struct
{   union
    { ICONRESDIR icon;
      CURSORDIR  cursor;
    } ResInfo;
    WORD   wPlanes;
    WORD   wBitCount;
    DWORD  dwBytesInRes;
    WORD   wResId;
} CURSORICONDIRENTRY;

typedef struct
{
    WORD                idReserved;
    WORD                idType;
    WORD                idCount;
    CURSORICONDIRENTRY  idEntries[1] WINE_PACKED;
} CURSORICONDIR;

typedef struct {
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD xHotspot;
    WORD yHotspot;
    DWORD dwDIBSize;
    DWORD dwDIBOffset;
} CURSORICONFILEDIRENTRY;

typedef struct
{
    WORD                idReserved;
    WORD                idType;
    WORD                idCount;
    CURSORICONFILEDIRENTRY  idEntries[1];
} CURSORICONFILEDIR;


#include "poppack.h"

#define CID_RESOURCE  0x0001
#define CID_WIN32     0x0004
#define CID_NONSHARED 0x0008

extern void CURSORICON_Init( void );

extern HCURSOR16 CURSORICON_IconToCursor( HICON16 hIcon,
                                          BOOL bSemiTransparent );

extern HGLOBAL CURSORICON_Load( HINSTANCE hInstance, LPCWSTR name,
                                int width, int height, int colors,
                                BOOL fCursor, UINT loadflags);

extern WORD WINAPI CURSORICON_Destroy( HGLOBAL16 handle, UINT16 flags );

extern void CURSORICON_FreeModuleIcons( HMODULE hModule );
				    
#endif /* __WINE_CURSORICON_H */
