#ifndef __WINE_MAPIDEFS_H
#define __WINE_MAPIDEFS_H

#include "wintypes.h"

typedef union tagCY CY;

union tagCY{
   struct {
      unsigned long Lo;
      long Hi;
   } u;
   LONGLONG int64;
};

#endif /*__WINE_MAPIDEFS_H*/
