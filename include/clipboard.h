#ifndef __WINE_CLIPBOARD_H
#define __WINE_CLIPBOARD_H

#include "win.h"
#include "wintypes.h"

typedef struct tagCLIPFORMAT {
    WORD	wFormatID;
    WORD	wRefCount;
    WORD	wDataPresent;
    LPSTR	Name;
    HANDLE32	hData32;
    DWORD	BufSize;
    struct tagCLIPFORMAT *PrevFormat;
    struct tagCLIPFORMAT *NextFormat;
    HANDLE16    hData16;
} CLIPFORMAT, *LPCLIPFORMAT;

typedef struct _CLIPBOARD_DRIVER
{
    void (*pEmptyClipboard)();
    void (*pSetClipboardData)(UINT32);
    BOOL32 (*pRequestSelection)();
    void (*pResetOwner)(WND *);
} CLIPBOARD_DRIVER;

CLIPBOARD_DRIVER *CLIPBOARD_GetDriver();

extern void CLIPBOARD_ResetLock(HQUEUE16 hqRef, HQUEUE16 hqNew);
extern void CLIPBOARD_DeleteRecord(LPCLIPFORMAT lpFormat, BOOL32 bChange);
extern BOOL32 CLIPBOARD_IsPresent(WORD wFormat);

#endif /* __WINE_CLIPBOARD_H */
