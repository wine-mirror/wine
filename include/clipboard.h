#ifndef __WINE_CLIPBOARD_H
#define __WINE_CLIPBOARD_H

#include "windef.h"

struct tagWND;

typedef struct tagWINE_CLIPFORMAT {
    WORD	wFormatID;
    WORD	wRefCount;
    WORD	wDataPresent;
    LPSTR	Name;
    HANDLE16    hData16;
    HANDLE	hDataSrc32;
    HANDLE	hData32;
    ULONG       drvData;
    struct tagWINE_CLIPFORMAT *PrevFormat;
    struct tagWINE_CLIPFORMAT *NextFormat;
} WINE_CLIPFORMAT, *LPWINE_CLIPFORMAT;

typedef struct tagCLIPBOARD_DRIVER
{
    void (*pAcquire)(void);                     /* Acquire selection */
    void (*pRelease)(void);                     /* Release selection */
    void (*pSetData)(UINT);                     /* Set specified selection data */
    BOOL (*pGetData)(UINT);                     /* Get specified selection data */
    BOOL (*pIsFormatAvailable)(UINT);           /* Check if specified format is available */
    BOOL (*pRegisterFormat)(LPCSTR);            /* Register a clipboard format */
    BOOL (*pIsSelectionOwner)(void);            /* Check if we own the selection */
    void (*pResetOwner)(struct tagWND *, BOOL);
} CLIPBOARD_DRIVER;

extern CLIPBOARD_DRIVER *CLIPBOARD_Driver;

extern LPWINE_CLIPFORMAT CLIPBOARD_LookupFormat( WORD wID );
extern BOOL CLIPBOARD_IsCacheRendered();
extern void CLIPBOARD_DeleteRecord(LPWINE_CLIPFORMAT lpFormat, BOOL bChange);
extern void CLIPBOARD_EmptyCache( BOOL bChange );
extern BOOL CLIPBOARD_IsPresent(WORD wFormat);
extern char * CLIPBOARD_GetFormatName(UINT wFormat);
extern void CLIPBOARD_ReleaseOwner();


#endif /* __WINE_CLIPBOARD_H */
