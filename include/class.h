/*
 * Window classes definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef CLASS_H
#define CLASS_H

#include "windows.h"

#define CLASS_MAGIC   0x4b4e      /* 'NK' */

#ifndef WINELIB
#pragma pack(1)
#endif

  /* !! Don't change this structure (see GetClassLong()) */
typedef struct tagCLASS
{
    HCLASS       hNext;         /* Next class */
    WORD         wMagic;        /* Magic number (must be CLASS_MAGIC) */
    ATOM         atomName;      /* Name of the class */
    HANDLE       hdce;          /* Class DCE (if CS_CLASSDC) */
    WORD         cWindows;      /* Count of existing windows of this class */
    WNDCLASS     wc WINE_PACKED;  /* Class information */
    WORD         wExtra[1];     /* Class extra bytes */
} CLASS;

#ifndef WINELIB
#pragma pack(4)
#endif


HCLASS CLASS_FindClassByName( SEGPTR name, HINSTANCE hinstance, CLASS **ptr );
CLASS * CLASS_FindClassPtr( HCLASS hclass );


#endif  /* CLASS_H */
