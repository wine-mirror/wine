#ifndef __INCLUDE_WINDEF_H
#define __INCLUDE_WINDEF_H

#include "wintypes.h"

#pragma pack(1)

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

/* FIXME: _MAX_PATH should be defined in stdlib.h and MAX_PATH in windef.h 
 * and mapiwin.h
 */
#define MAX_PATH 260

#define HFILE_ERROR16   ((HFILE16)-1)
#define HFILE_ERROR32   ((HFILE32)-1)
#define HFILE_ERROR     WINELIB_NAME(HFILE_ERROR)

#pragma pack(4)


/*
 * POINTL structure. Used in some OLE calls.
 */
typedef struct _POINTL
{
  LONG x;
  LONG y;
} POINTL;


#endif /* __INCLUDE_WINDEF_H */
