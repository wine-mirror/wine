/*
 * Dialog definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef DIALOG_H
#define DIALOG_H

#include "windows.h"

extern BOOL DIALOG_Init(void);
extern HWND DIALOG_GetFirstTabItem( HWND hwndDlg );

#pragma pack(1)

  /* Dialog info structure.
   * This structure is stored into the window extra bytes (cbWndExtra).
   * sizeof(DIALOGINFO) must be <= DLGWINDOWEXTRA (=30).
   */
typedef struct
{
    LONG      msgResult;   /* Result of EndDialog() / Default button id */
    WNDPROC   dlgProc;     /* Dialog procedure */
    LONG      userInfo;    /* User information (for DWL_USER) */
    HWND      hwndFocus;   /* Current control with focus */
    HFONT     hUserFont;   /* Dialog font */
    HMENU     hMenu;       /* Dialog menu */
    WORD      xBaseUnit;   /* Dialog units (depends on the font) */
    WORD      yBaseUnit;
    WORD      fEnd;        /* EndDialog() called for this dialog */
    HANDLE    hDialogHeap;
} DIALOGINFO;


  /* Dialog control header */
typedef struct
{
    DWORD      style;
    WORD       x;
    WORD       y;
    WORD       cx;
    WORD       cy;
    WORD       id;
} DLGCONTROLHEADER;


  /* Dialog template */
typedef struct
{
    DWORD              style;
    WORD               nbItems;
    WORD               x;
    WORD               y;
    WORD               cx;
    WORD               cy;
    SEGPTR             menuName;
    SEGPTR             className;
    SEGPTR             caption;
    WORD               pointSize;
    SEGPTR             faceName;
} DLGTEMPLATE;

#pragma pack(4)

extern WORD xBaseUnit,yBaseUnit;
int DIALOG_DoDialogBox( HWND hwnd, HWND owner );

#endif  /* DIALOG_H */
