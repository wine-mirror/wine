/*
 * Dialog definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef DIALOG_H
#define DIALOG_H

#include "windows.h"
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
    WORD        xBaseUnit;   /* Dialog units (depends on the font) */
    WORD        yBaseUnit;
    INT32	idResult;    /* EndDialog() result / default pushbutton ID */
    WORD        fEnd;        /* EndDialog() called for this dialog */
    HGLOBAL16   hDialogHeap;
} DIALOGINFO;

#pragma pack(4)

extern BOOL32 DIALOG_Init(void);

#endif  /* DIALOG_H */
