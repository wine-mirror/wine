/*
 * Window classes definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef CLASS_H
#define CLASS_H

#include "windows.h"

#define CLASS_MAGIC   0x4b4e      /* 'NK' */

typedef struct tagCLASS
{
    struct tagCLASS *next;      /* Next class */
    HCLASS           self;      /* Handle to this class */
    WORD             wMagic;    /* Magic number (must be CLASS_MAGIC) */
    ATOM             atomName;  /* Name of the class */
    HANDLE           hdce;      /* Class DCE (if CS_CLASSDC) */
    WORD             cWindows;  /* Count of existing windows of this class */
    WNDCLASS         wc;        /* Class information */
    WORD             wExtra[1]; /* Class extra bytes */
} CLASS;

extern void CLASS_DumpClass( CLASS *class );
extern void CLASS_WalkClasses(void);
extern void CLASS_FreeModuleClasses( HMODULE hModule );
extern CLASS * CLASS_FindClassByName( SEGPTR name, HINSTANCE hinstance );

#endif  /* CLASS_H */
