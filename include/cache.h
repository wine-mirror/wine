/*
 * Wine internally cached objects to speedup some things and prevent 
 * infinite duplication of trivial code and data. 
 * 
 * Copyright 1997 Bertho A. Stultiens
 *
 */

#ifndef __WINE_CACHE_H
#define __WINE_CACHE_H

#include "wintypes.h"

HBRUSH32  CACHE_GetPattern55AABrush(void);
HBITMAP32 CACHE_GetPattern55AABitmap(void);

#endif /* __WINE_CACHE_H */
