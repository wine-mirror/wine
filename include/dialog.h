/*
 * Dialog definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_DIALOG_H
#define __WINE_DIALOG_H

#include "winproc.h"

  /* Dialog info structure.
   * This structure is stored into the window extra bytes (cbWndExtra).
   * sizeof(DIALOGINFO) must be <= DLGWINDOWEXTRA (=30).
   */

#pragma pack(1)

typedef struct
{
    INT32       msgResult;   /* +00 Last message result */
    HWINDOWPROC dlgProc;     /* +04 Dialog procedure */
    LONG        userInfo;    /* +08 User information (for DWL_USER) */

    /* implementation-dependent part */

    HWND16      hwndFocus;   /* Current control with focus */
    HFONT16     hUserFont;   /* Dialog font */
    HMENU16     hMenu;       /* Dialog menu */
    UINT16      xBaseUnit;   /* Dialog units (depends on the font) */
    UINT16      yBaseUnit;
    INT32	idResult;    /* EndDialog() result / default pushbutton ID */
    UINT16      flags;       /* EndDialog() called for this dialog */
    HGLOBAL16   hDialogHeap;
} DIALOGINFO;

#pragma pack(4)

#define DF_END  0x0001

extern BOOL32 DIALOG_Init(void);
extern HWND32 DIALOG_CreateIndirect( HINSTANCE32 hInst, LPCSTR dlgTemplate,
                                     BOOL32 win32Template, HWND32 owner,
                                     DLGPROC16 dlgProc, LPARAM param,
                                     WINDOWPROCTYPE procType );
extern INT32 DIALOG_DoDialogBox( HWND32 hwnd, HWND32 owner );

#endif  /* __WINE_DIALOG_H */
