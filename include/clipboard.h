#ifndef __WINE_CLIPBOARD_H
#define __WINE_CLIPBOARD_H

#include "windef.h"
#include "wine/windef16.h"

struct tagWND;

typedef struct tagWINE_CLIPFORMAT {
    UINT	wFormatID;
    UINT	wRefCount;
    BOOL	wDataPresent;
    LPSTR	Name;
    HANDLE16    hData16;
    HANDLE	hDataSrc32;
    HANDLE	hData32;
    ULONG       drvData;
    struct tagWINE_CLIPFORMAT *PrevFormat;
    struct tagWINE_CLIPFORMAT *NextFormat;
} WINE_CLIPFORMAT, *LPWINE_CLIPFORMAT;

extern LPWINE_CLIPFORMAT CLIPBOARD_LookupFormat( WORD wID );
extern BOOL CLIPBOARD_IsCacheRendered();
extern void CLIPBOARD_DeleteRecord(LPWINE_CLIPFORMAT lpFormat, BOOL bChange);
extern void CLIPBOARD_EmptyCache( BOOL bChange );
extern BOOL CLIPBOARD_IsPresent(WORD wFormat);
extern char * CLIPBOARD_GetFormatName(UINT wFormat);
extern void CLIPBOARD_ReleaseOwner();

#endif /* __WINE_CLIPBOARD_H */
