/*
 * Window classes definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_CLASS_H
#define __WINE_CLASS_H

#include "wintypes.h"
#include "winproc.h"

#define CLASS_MAGIC   ('C' | ('L' << 8) | ('A' << 16) | ('S' << 24))

struct tagDCE;

typedef struct tagCLASS
{
    struct tagCLASS *next;          /* Next class */
    UINT           magic;         /* Magic number */
    UINT           cWindows;      /* Count of existing windows */
    UINT           style;         /* Class style */
    HWINDOWPROC      winproc;       /* Window procedure */ 
    INT            cbClsExtra;    /* Class extra bytes */
    INT            cbWndExtra;    /* Window extra bytes */
    LPSTR            menuNameA;     /* Default menu name (ASCII string) */
    LPWSTR           menuNameW;     /* Default menu name (Unicode) */
    struct tagDCE   *dce;           /* Class DCE (if CS_CLASSDC) */
    HINSTANCE      hInstance;     /* Module that created the task */
    HICON16          hIcon;         /* Default icon */
    HICON16          hIconSm;       /* Default small icon */
    HCURSOR16        hCursor;       /* Default cursor */
    HBRUSH16         hbrBackground; /* Default background */
    ATOM             atomName;      /* Name of the class */
    LPSTR            classNameA;    /* Class name (ASCII string) */
    LPWSTR           classNameW;    /* Class name (Unicode) */
    LONG             wExtra[1];     /* Class extra bytes */
} CLASS;

extern void CLASS_DumpClass( CLASS *class );
extern void CLASS_WalkClasses(void);
extern void CLASS_FreeModuleClasses( HMODULE16 hModule );
extern CLASS *CLASS_FindClassByAtom( ATOM atom, HINSTANCE hinstance );

#endif  /* __WINE_CLASS_H */
