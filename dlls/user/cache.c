/*
 * Wine internally cached objects to speedup some things and prevent 
 * infinite duplication of trivial code and data. 
 * 
 * Copyright 1997 Bertho A. Stultiens
 *
 */

#include "windef.h"
#include "wingdi.h"
#include "cache.h"

static const WORD wPattern55AA[] =
{ 
    0x5555, 0xaaaa, 0x5555, 0xaaaa,
    0x5555, 0xaaaa, 0x5555, 0xaaaa
};

static HBRUSH  hPattern55AABrush = 0;
static HBITMAP hPattern55AABitmap = 0;


/*********************************************************************
 *	CACHE_GetPattern55AABrush
 */
HBRUSH CACHE_GetPattern55AABrush(void)
{
    if (!hPattern55AABrush)
        hPattern55AABrush = CreatePatternBrush(CACHE_GetPattern55AABitmap());
    return hPattern55AABrush;
}


/*********************************************************************
 *	CACHE_GetPattern55AABitmap
 */
HBITMAP CACHE_GetPattern55AABitmap(void)
{
    if (!hPattern55AABitmap)
        hPattern55AABitmap = CreateBitmap( 8, 8, 1, 1, wPattern55AA );
    return hPattern55AABitmap;
}
