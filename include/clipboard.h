#ifndef __WINE_CLIPBOARD_H
#define __WINE_CLIPBOARD_H

#include "win.h"
#include "windef.h"

typedef struct tagWINE_CLIPFORMAT {
    WORD	wFormatID;
    WORD	wRefCount;
    WORD	wDataPresent;
    LPSTR	Name;
    HANDLE	hData32;
    DWORD	BufSize;
    struct tagWINE_CLIPFORMAT *PrevFormat;
    struct tagWINE_CLIPFORMAT *NextFormat;
    HANDLE16    hData16;
} WINE_CLIPFORMAT, *LPWINE_CLIPFORMAT;

typedef struct _CLIPBOARD_DRIVER
{
    void (*pEmptyClipboard)(void);
    void (*pSetClipboardData)(UINT);
    BOOL (*pRequestSelection)(void);
    void (*pResetOwner)(WND *, BOOL);
} CLIPBOARD_DRIVER;

CLIPBOARD_DRIVER *CLIPBOARD_GetDriver(void);

extern void CLIPBOARD_ResetLock(HQUEUE16 hqRef, HQUEUE16 hqNew);
extern void CLIPBOARD_DeleteRecord(LPWINE_CLIPFORMAT lpFormat, BOOL bChange);
extern BOOL CLIPBOARD_IsPresent(WORD wFormat);

#endif /* __WINE_CLIPBOARD_H */
