#ifndef __INCLUDE_WINDEF_H
#define __INCLUDE_WINDEF_H

#include "wintypes.h"

#pragma pack(1)

/* FIXME: _MAX_PATH should be defined in stdlib.h and MAX_PATH in windef.h 
 * and mapiwin.h
 */
#define MAX_PATH 260

#define HFILE_ERROR16   ((HFILE16)-1)
#define HFILE_ERROR32   ((HFILE32)-1)
#define HFILE_ERROR     WINELIB_NAME(HFILE_ERROR)

#pragma pack(4)

#endif /* __INCLUDE_WINDEF_H */
