/*
 * Window classes definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef CLASS_H
#define CLASS_H

#include "windows.h"
#include "winproc.h"

#define CLASS_MAGIC   ('C' | ('L' << 8) | ('A' << 16) | ('S' << 24))

typedef struct tagCLASS
{
    struct tagCLASS *next;          /* Next class */
    UINT32           magic;         /* Magic number */
    UINT32           cWindows;      /* Count of existing windows */
    UINT32           style;         /* Class style */
    HWINDOWPROC      winproc;       /* Window procedure */ 
    INT32            cbClsExtra;    /* Class extra bytes */
    INT32            cbWndExtra;    /* Window extra bytes */
    LPSTR            menuNameA;     /* Default menu name (ASCII string) */
    LPWSTR           menuNameW;     /* Default menu name (Unicode) */
    HINSTANCE32      hInstance;     /* Module that created the task */
    HICON16          hIcon;         /* Default icon */
    HICON16          hIconSm;       /* Default small icon */
    HCURSOR16        hCursor;       /* Default cursor */
    HBRUSH16         hbrBackground; /* Default background */
    ATOM             atomName;      /* Name of the class */
    HANDLE16         hdce;          /* Class DCE (if CS_CLASSDC) */
    LONG             wExtra[1];     /* Class extra bytes */
} CLASS;

extern void CLASS_DumpClass( CLASS *class );
extern void CLASS_WalkClasses(void);
extern void CLASS_FreeModuleClasses( HMODULE16 hModule );
extern CLASS *CLASS_FindClassByAtom( ATOM atom, HINSTANCE16 hinstance );

#endif  /* CLASS_H */
