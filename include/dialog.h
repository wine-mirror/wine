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
typedef struct
{
    INT32       msgResult;   /* Result of EndDialog() / Default button id */
    HWINDOWPROC dlgProc;     /* Dialog procedure */
    LONG        userInfo;    /* User information (for DWL_USER) */
    HWND16      hwndFocus;   /* Current control with focus */
    HFONT16     hUserFont;   /* Dialog font */
    HMENU16     hMenu;       /* Dialog menu */
    WORD        xBaseUnit;   /* Dialog units (depends on the font) */
    WORD        yBaseUnit;
    WORD        fEnd;        /* EndDialog() called for this dialog */
    HGLOBAL16   hDialogHeap;
} DIALOGINFO;

extern BOOL32 DIALOG_Init(void);

#endif  /* DIALOG_H */
