#ifndef __WINE_CLIPBOARD_H
#define __WINE_CLIPBOARD_H

#include "windef.h"

struct tagWND;

typedef struct tagWINE_CLIPFORMAT {
    WORD	wFormatID;
    WORD	wRefCount;
    WORD	wDataPresent;
    LPSTR	Name;
    HANDLE	hDataSrc32;
    HANDLE	hData32;
    DWORD	BufSize;
    struct tagWINE_CLIPFORMAT *PrevFormat;
    struct tagWINE_CLIPFORMAT *NextFormat;
    HANDLE16    hData16;
} WINE_CLIPFORMAT, *LPWINE_CLIPFORMAT;

typedef struct tagCLIPBOARD_DRIVER
{
    void (*pEmpty)(void);
    void (*pSetData)(UINT);
    BOOL (*pGetData)(UINT);
    void (*pResetOwner)(struct tagWND *, BOOL);
} CLIPBOARD_DRIVER;

extern CLIPBOARD_DRIVER *CLIPBOARD_Driver;

extern void CLIPBOARD_ResetLock(HQUEUE16 hqRef, HQUEUE16 hqNew);
extern void CLIPBOARD_DeleteRecord(LPWINE_CLIPFORMAT lpFormat, BOOL bChange);
extern BOOL CLIPBOARD_IsPresent(WORD wFormat);

#endif /* __WINE_CLIPBOARD_H */
