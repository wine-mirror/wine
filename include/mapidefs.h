#ifndef __WINE_MAPIDEFS_H
#define __WINE_MAPIDEFS_H

#include "windef.h"

/* Some types */

typedef unsigned char*          LPBYTE;
#ifndef __LHANDLE
#define __LHANDLE
typedef unsigned long           LHANDLE, *LPLHANDLE;
#endif

#ifndef _tagCY_DEFINED
#define _tagCY_DEFINED
typedef union tagCY
{
   struct {
#ifdef BIG_ENDIAN
        long Hi;
        long Lo;
#else
      unsigned long Lo;
      long Hi;
#endif
   } u;
   LONGLONG int64;
} CY;
#endif /* _tagCY_DEFINED */

#endif /*__WINE_MAPIDEFS_H*/
