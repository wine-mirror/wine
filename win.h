/*
 * Window definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef WIN_H
#define WIN_H

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Core.h>

#include "windows.h"


#define WND_MAGIC     0x444e4957  /* 'WIND' */


typedef struct tagWND
{
    HWND         hwndNext;       /* Next sibling */
    HWND         hwndChild;      /* First child */
    DWORD        dwMagic;        /* Magic number (must be WND_MAGIC) */
    HWND         hwndParent;     /* Window parent (from CreateWindow) */
    HWND         hwndOwner;      /* Window owner */
    HCLASS       hClass;         /* Window class */
    HANDLE       hInstance;      /* Window hInstance (from CreateWindow) */
    RECT         rectClient;     /* Window client area screen coords */
    RECT         rectWindow;     /* Window whole area screen coords */
    HRGN         hrgnUpdate;     /* Update region */
    HWND         hwndLastActive; /* Last active popup hwnd */
    FARPROC      lpfnWndProc;    /* Window procedure */
    DWORD        dwStyle;        /* Window style (from CreateWindow) */
    HANDLE       hDCE;           /* Window DC Entry (if CS_OWNDC) */
    HMENU        hmenuSystem;    /* System menu */
    WORD         wIDmenu;        /* ID or hmenu (from CreateWindow) */
    Widget       shellWidget;    /* For top-level windows */
    Widget       winWidget;      /* For all windows */
    WORD         wExtra[1];      /* Window extra bytes */
} WND;


 /* The caller must GlobalUnlock the pointer returned 
  * by this function (except when NULL).
  */
WND * WIN_FindWndPtr( HWND hwnd );


#endif  /* WIN_H */
