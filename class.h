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
    HCLASS       hNext;         /* Next class */
    WORD         wMagic;        /* Magic number (must be CLASS_MAGIC) */
    ATOM         atomName;      /* Name of the class */
    HANDLE       hDCE;          /* Class DC Entry (if CS_CLASSDC) */
    WORD         cWindows;      /* Count of existing windows of this class */
    WNDCLASS     wc __attribute__ ((packed));  /* Class information */
    WORD         wExtra[1];     /* Class extra bytes */
} CLASS;


 /* The caller must GlobalUnlock the pointer returned 
  * by these functions (except when NULL).
  */
HCLASS CLASS_FindClassByName( char * name, CLASS **ptr );
CLASS * CLASS_FindClassPtr( HCLASS hclass );


#endif  /* CLASS_H */
