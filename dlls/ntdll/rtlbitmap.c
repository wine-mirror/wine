/*
 * NTDLL bitmap functions
 *
 * Copyright 2002 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES
 *  Bitmaps are an efficient type for manipulating large arrays of bits. They
 *  are commonly used by device drivers (e.g. For holding dirty/free blocks),
 *  but are also available to applications that need this functionality.
 *
 *  Bits are set LSB to MSB in each consecutive byte, making this implementation
 *  binary compatible with Win32.
 *
 *  Note that to avoid unexpected behaviour, the size of a bitmap should be set
 *  to a multiple of 32.
 */
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "windef.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);

/* First set bit in a nibble; used for determining least significant bit */
static const BYTE NTDLL_leastSignificant[16] = {
  0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
};

/* Last set bit in a nibble; used for determining most significant bit */
static const signed char NTDLL_mostSignificant[16] = {
  -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3
};

static inline ULONG maskbits( ULONG idx )
{
    return ~0u << (idx & 31);
}

static ULONG popcount( ULONG val )
{
#if defined(__MINGW32__)
    return __builtin_popcount( val );
#else
    val -= val >> 1 & 0x55555555;
    val = (val & 0x33333333) + (val >> 2 & 0x33333333);
    return ((val + (val >> 4)) & 0x0f0f0f0f) * 0x01010101 >> 24;
#endif
}

/*************************************************************************
 * RtlInitializeBitMap	[NTDLL.@]
 *
 * Initialise a new bitmap.
 *
 * PARAMS
 *  lpBits [I] Bitmap pointer
 *  lpBuff [I] Memory for the bitmap
 *  ulSize [I] Size of lpBuff in bits
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  lpBuff must be aligned on a ULONG suitable boundary, to a multiple of 32 bytes
 *  in size (irrespective of ulSize given).
 */
VOID WINAPI RtlInitializeBitMap(PRTL_BITMAP lpBits, PULONG lpBuff, ULONG ulSize)
{
  TRACE("(%p,%p,%lu)\n", lpBits,lpBuff,ulSize);
  lpBits->SizeOfBitMap = ulSize;
  lpBits->Buffer = lpBuff;
}

/*************************************************************************
 * RtlSetAllBits	[NTDLL.@]
 *
 * Set all bits in a bitmap.
 *
 * PARAMS
 *  lpBits [I] Bitmap pointer
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI RtlSetAllBits(PRTL_BITMAP lpBits)
{
  TRACE("(%p)\n", lpBits);
  memset(lpBits->Buffer, 0xff, ((lpBits->SizeOfBitMap + 31) & ~31) >> 3);
}

/*************************************************************************
 * RtlClearAllBits	[NTDLL.@]
 *
 * Clear all bits in a bitmap.
 *
 * PARAMS
 *  lpBit [I] Bitmap pointer
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI RtlClearAllBits(PRTL_BITMAP lpBits)
{
  TRACE("(%p)\n", lpBits);
  memset(lpBits->Buffer, 0, ((lpBits->SizeOfBitMap + 31) & ~31) >> 3);
}

/*************************************************************************
 * RtlSetBits	[NTDLL.@]
 */
void WINAPI RtlSetBits( RTL_BITMAP *bitmap, ULONG start, ULONG count )
{
    ULONG end = start + count;
    ULONG pos = start / 32;
    ULONG end_pos = end / 32;

    TRACE( "(%p,%lu,%lu)\n", bitmap, start, count );

    if (!count || start >= bitmap->SizeOfBitMap || count > bitmap->SizeOfBitMap - start) return;

    if (end_pos > pos)
    {
        bitmap->Buffer[pos++] |= maskbits( start );
        while (pos < end_pos) bitmap->Buffer[pos++] = ~0u;
        if (end & 31) bitmap->Buffer[pos] |= ~maskbits( end );
    }
    else bitmap->Buffer[pos] |= maskbits( start ) & ~maskbits( end );
}

/*************************************************************************
 * RtlClearBits	[NTDLL.@]
 */
void WINAPI RtlClearBits( RTL_BITMAP *bitmap, ULONG start, ULONG count)
{
    ULONG end = start + count;
    ULONG pos = start / 32;
    ULONG end_pos = end / 32;

    TRACE( "(%p,%lu,%lu)\n", bitmap, start, count );

    if (!count || start >= bitmap->SizeOfBitMap || count > bitmap->SizeOfBitMap - start) return;

    if (end_pos > pos)
    {
        bitmap->Buffer[pos++] &= ~maskbits( start );
        while (pos < end_pos) bitmap->Buffer[pos++] = 0;
        if (end & 31) bitmap->Buffer[pos] &= maskbits( end );
    }
    else bitmap->Buffer[pos] &= ~maskbits( start ) | maskbits( end );
}

/*************************************************************************
 * RtlAreBitsSet	[NTDLL.@]
 */
BOOLEAN WINAPI RtlAreBitsSet( const RTL_BITMAP *bitmap, ULONG start, ULONG count )
{
    ULONG end = start + count;
    ULONG pos = start / 32;
    ULONG end_pos = end / 32;

    TRACE( "(%p,%lu,%lu)\n", bitmap, start, count );

    if (!count || start >= bitmap->SizeOfBitMap || count > bitmap->SizeOfBitMap - start)
        return FALSE;

    if (end_pos == pos) return (bitmap->Buffer[pos] | ~maskbits( start ) | maskbits( end )) == ~0u;

    if ((bitmap->Buffer[pos++] | ~maskbits( start )) != ~0u) return FALSE;
    while (pos < end_pos) if (bitmap->Buffer[pos++] != ~0u) return FALSE;
    if (!(end & 31)) return TRUE;
    return (bitmap->Buffer[pos] | maskbits( end )) == ~0u;
}

/*************************************************************************
 * RtlAreBitsClear	[NTDLL.@]
 */
BOOLEAN WINAPI RtlAreBitsClear( const RTL_BITMAP *bitmap, ULONG start, ULONG count )
{
    ULONG end = start + count;
    ULONG pos = start / 32;
    ULONG end_pos = end / 32;

    TRACE( "(%p,%lu,%lu)\n", bitmap, start, count );

    if (!count || start >= bitmap->SizeOfBitMap || count > bitmap->SizeOfBitMap - start)
        return FALSE;

    if (end_pos == pos) return !(bitmap->Buffer[pos] & maskbits( start ) & ~maskbits( end ));

    if (bitmap->Buffer[pos++] & maskbits( start )) return FALSE;
    while (pos < end_pos) if (bitmap->Buffer[pos++]) return FALSE;
    if (!(end & 31)) return TRUE;
    return !(bitmap->Buffer[pos] & ~maskbits( end ));
}

/*************************************************************************
 * RtlFindSetBits	[NTDLL.@]
 *
 * Find a block of set bits in a bitmap.
 *
 * PARAMS
 *  lpBits  [I] Bitmap pointer
 *  ulCount [I] Number of consecutive set bits to find
 *  ulHint  [I] Suggested starting position for set bits
 *
 * RETURNS
 *  The bit at which the match was found, or -1 if no match was found.
 */
ULONG WINAPI RtlFindSetBits(PCRTL_BITMAP lpBits, ULONG ulCount, ULONG ulHint)
{
  ULONG ulPos, ulEnd;

  TRACE("(%p,%lu,%lu)\n", lpBits, ulCount, ulHint);

  if (!lpBits || !ulCount || ulCount > lpBits->SizeOfBitMap)
    return ~0U;

  ulEnd = lpBits->SizeOfBitMap;

  if (ulHint + ulCount > lpBits->SizeOfBitMap)
    ulHint = 0;

  ulPos = ulHint;

  while (ulPos < ulEnd)
  {
    /* FIXME: This could be made a _lot_ more efficient */
    if (RtlAreBitsSet(lpBits, ulPos, ulCount))
      return ulPos;

    /* Start from the beginning if we hit the end and had a hint */
    if (ulPos == ulEnd - 1 && ulHint)
    {
      ulEnd = ulHint;
      ulPos = ulHint = 0;
    }
    else
      ulPos++;
  }
  return ~0U;
}

/*************************************************************************
 * RtlFindClearBits	[NTDLL.@]
 *
 * Find a block of clear bits in a bitmap.
 *
 * PARAMS
 *  lpBits  [I] Bitmap pointer
 *  ulCount [I] Number of consecutive clear bits to find
 *  ulHint  [I] Suggested starting position for clear bits
 *
 * RETURNS
 *  The bit at which the match was found, or -1 if no match was found.
 */
ULONG WINAPI RtlFindClearBits(PCRTL_BITMAP lpBits, ULONG ulCount, ULONG ulHint)
{
  ULONG ulPos, ulEnd;

  TRACE("(%p,%lu,%lu)\n", lpBits, ulCount, ulHint);

  if (!lpBits || !ulCount || ulCount > lpBits->SizeOfBitMap)
    return ~0U;

  ulEnd = lpBits->SizeOfBitMap;

  if (ulHint + ulCount > lpBits->SizeOfBitMap)
    ulHint = 0;

  ulPos = ulHint;

  while (ulPos < ulEnd)
  {
    /* FIXME: This could be made a _lot_ more efficient */
    if (RtlAreBitsClear(lpBits, ulPos, ulCount))
      return ulPos;

    /* Start from the beginning if we hit the end and started from ulHint */
    if (ulPos == ulEnd - 1 && ulHint)
    {
      ulEnd = ulHint;
      ulPos = ulHint = 0;
    }
    else
      ulPos++;
  }
  return ~0U;
}

/*************************************************************************
 * RtlFindSetBitsAndClear	[NTDLL.@]
 *
 * Find a block of set bits in a bitmap, and clear them if found.
 *
 * PARAMS
 *  lpBits  [I] Bitmap pointer
 *  ulCount [I] Number of consecutive set bits to find
 *  ulHint  [I] Suggested starting position for set bits
 *
 * RETURNS
 *  The bit at which the match was found, or -1 if no match was found.
 */
ULONG WINAPI RtlFindSetBitsAndClear(PRTL_BITMAP lpBits, ULONG ulCount, ULONG ulHint)
{
  ULONG ulPos;

  TRACE("(%p,%lu,%lu)\n", lpBits, ulCount, ulHint);

  ulPos = RtlFindSetBits(lpBits, ulCount, ulHint);
  if (ulPos != ~0U)
    RtlClearBits(lpBits, ulPos, ulCount);
  return ulPos;
}

/*************************************************************************
 * RtlFindClearBitsAndSet	[NTDLL.@]
 *
 * Find a block of clear bits in a bitmap, and set them if found.
 *
 * PARAMS
 *  lpBits  [I] Bitmap pointer
 *  ulCount [I] Number of consecutive clear bits to find
 *  ulHint  [I] Suggested starting position for clear bits
 *
 * RETURNS
 *  The bit at which the match was found, or -1 if no match was found.
 */
ULONG WINAPI RtlFindClearBitsAndSet(PRTL_BITMAP lpBits, ULONG ulCount, ULONG ulHint)
{
  ULONG ulPos;

  TRACE("(%p,%lu,%lu)\n", lpBits, ulCount, ulHint);

  ulPos = RtlFindClearBits(lpBits, ulCount, ulHint);
  if (ulPos != ~0U)
    RtlSetBits(lpBits, ulPos, ulCount);
  return ulPos;
}

/*************************************************************************
 * RtlNumberOfSetBits	[NTDLL.@]
 */
ULONG WINAPI RtlNumberOfSetBits( const RTL_BITMAP *bitmap )
{
    ULONG i, ret = 0;

    for (i = 0; i < bitmap->SizeOfBitMap / 32; i++) ret += popcount( bitmap->Buffer[i] );
    if (bitmap->SizeOfBitMap & 31) ret += popcount( bitmap->Buffer[i] & ~maskbits( bitmap->SizeOfBitMap ));

    TRACE( "%p -> %lu\n", bitmap, ret );
    return ret;
}

/*************************************************************************
 * RtlNumberOfClearBits	[NTDLL.@]
 *
 * Find the number of clear bits in a bitmap.
 *
 * PARAMS
 *  lpBits [I] Bitmap pointer
 *
 * RETURNS
 *  The number of clear bits.
 */
ULONG WINAPI RtlNumberOfClearBits(PCRTL_BITMAP lpBits)
{
  TRACE("(%p)\n", lpBits);

  if (lpBits)
    return lpBits->SizeOfBitMap - RtlNumberOfSetBits(lpBits);
  return 0;
}

/*************************************************************************
 * RtlFindMostSignificantBit	[NTDLL.@]
 *
 * Find the most significant bit in a 64 bit integer.
 *
 * RETURNS
 *  The position of the most significant bit.
 */
CCHAR WINAPI RtlFindMostSignificantBit(ULONGLONG ulLong)
{
    signed char ret = 32;
    DWORD dw;

    if (!(dw = (ulLong >> 32)))
    {
        ret = 0;
        dw = (DWORD)ulLong;
    }
    if (dw & 0xffff0000)
    {
        dw >>= 16;
        ret += 16;
    }
    if (dw & 0xff00)
    {
        dw >>= 8;
        ret += 8;
    }
    if (dw & 0xf0)
    {
        dw >>= 4;
        ret += 4;
    }
    return ret + NTDLL_mostSignificant[dw];
}

/*************************************************************************
 * RtlFindLeastSignificantBit	[NTDLL.@]
 *
 * Find the least significant bit in a 64 bit integer.
 *
 * RETURNS
 *  The position of the least significant bit.
 */
CCHAR WINAPI RtlFindLeastSignificantBit(ULONGLONG ulLong)
{
    signed char ret = 0;
    DWORD dw;

    if (!(dw = (DWORD)ulLong))
    {
        ret = 32;
        if (!(dw = ulLong >> 32)) return -1;
    }
    if (!(dw & 0xffff))
    {
        dw >>= 16;
        ret += 16;
    }
    if (!(dw & 0xff))
    {
        dw >>= 8;
        ret += 8;
    }
    if (!(dw & 0x0f))
    {
        dw >>= 4;
        ret += 4;
    }
    return ret + NTDLL_leastSignificant[dw & 0x0f];
}

/*************************************************************************
 * NTDLL_RunSortFn
 *
 * Internal helper: qsort comparison function for RTL_BITMAP_RUN arrays
 */
static int __cdecl NTDLL_RunSortFn(const void *lhs, const void *rhs)
{
  if (((const RTL_BITMAP_RUN*)lhs)->NumberOfBits > ((const RTL_BITMAP_RUN*)rhs)->NumberOfBits)
    return -1;
  return 1;
}

/*************************************************************************
 * NTDLL_FindSetRun
 *
 * Internal helper: Find the next run of set bits in a bitmap.
 */
static ULONG NTDLL_FindSetRun(PCRTL_BITMAP lpBits, ULONG ulStart, PULONG lpSize)
{
  LPBYTE lpOut;
  ULONG ulFoundAt = 0, ulCount = 0;

  /* FIXME: It might be more efficient/cleaner to manipulate four bytes
   * at a time. But beware of the pointer arithmetics...
   */
  lpOut = ((BYTE*)lpBits->Buffer) + (ulStart >> 3u);

  while (1)
  {
    /* Check bits in first byte */
    const BYTE bMask = (0xff << (ulStart & 7)) & 0xff;
    const BYTE bFirst = *lpOut & bMask;

    if (bFirst)
    {
      /* Have a set bit in first byte */
      if (bFirst != bMask)
      {
        /* Not every bit is set */
        ULONG ulOffset;

        if (bFirst & 0x0f)
          ulOffset = NTDLL_leastSignificant[bFirst & 0x0f];
        else
          ulOffset = 4 + NTDLL_leastSignificant[bFirst >> 4];
        ulStart += ulOffset;
        ulFoundAt = ulStart;
        for (;ulOffset < 8; ulOffset++)
        {
          if (!(bFirst & (1 << ulOffset)))
          {
            *lpSize = ulCount;
            return ulFoundAt; /* Set from start, but not until the end */
          }
          ulCount++;
          ulStart++;
        }
        /* Set to the end - go on to count further bits */
        lpOut++;
        break;
      }
      /* every bit from start until the end of the byte is set */
      ulFoundAt = ulStart;
      ulCount = 8 - (ulStart & 7);
      ulStart = (ulStart & ~7u) + 8;
      lpOut++;
      break;
    }
    ulStart = (ulStart & ~7u) + 8;
    lpOut++;
    if (ulStart >= lpBits->SizeOfBitMap)
      return ~0U;
  }

  /* Check if reached the end of bitmap */
  if (ulStart >= lpBits->SizeOfBitMap) {
    *lpSize = ulCount - (ulStart - lpBits->SizeOfBitMap);
    return ulFoundAt;
  }

  /* Count blocks of 8 set bits */
  while (*lpOut == 0xff)
  {
    ulCount += 8;
    ulStart += 8;
    if (ulStart >= lpBits->SizeOfBitMap)
    {
      *lpSize = ulCount - (ulStart - lpBits->SizeOfBitMap);
      return ulFoundAt;
    }
    lpOut++;
  }

  /* Count remaining contiguous bits, if any */
  if (*lpOut & 1)
  {
    ULONG ulOffset = 0;

    for (;ulOffset < 7u; ulOffset++)
    {
      if (!(*lpOut & (1 << ulOffset)))
        break;
      ulCount++;
    }
  }
  *lpSize = ulCount;
  return ulFoundAt;
}

/*************************************************************************
 * NTDLL_FindClearRun
 *
 * Internal helper: Find the next run of set bits in a bitmap.
 */
static ULONG NTDLL_FindClearRun(PCRTL_BITMAP lpBits, ULONG ulStart, PULONG lpSize)
{
  LPBYTE lpOut;
  ULONG ulFoundAt = 0, ulCount = 0;

  /* FIXME: It might be more efficient/cleaner to manipulate four bytes
   * at a time. But beware of the pointer arithmetics...
   */
  lpOut = ((BYTE*)lpBits->Buffer) + (ulStart >> 3u);

  while (1)
  {
    /* Check bits in first byte */
    const BYTE bMask = (0xff << (ulStart & 7)) & 0xff;
    const BYTE bFirst = (~*lpOut) & bMask;

    if (bFirst)
    {
      /* Have a clear bit in first byte */
      if (bFirst != bMask)
      {
        /* Not every bit is clear */
        ULONG ulOffset;

        if (bFirst & 0x0f)
          ulOffset = NTDLL_leastSignificant[bFirst & 0x0f];
        else
          ulOffset = 4 + NTDLL_leastSignificant[bFirst >> 4];
        ulStart += ulOffset;
        ulFoundAt = ulStart;
        for (;ulOffset < 8; ulOffset++)
        {
          if (!(bFirst & (1 << ulOffset)))
          {
            *lpSize = ulCount;
            return ulFoundAt; /* Clear from start, but not until the end */
          }
          ulCount++;
          ulStart++;
        }
        /* Clear to the end - go on to count further bits */
        lpOut++;
        break;
      }
      /* Every bit from start until the end of the byte is clear */
      ulFoundAt = ulStart;
      ulCount = 8 - (ulStart & 7);
      ulStart = (ulStart & ~7u) + 8;
      lpOut++;
      break;
    }
    ulStart = (ulStart & ~7u) + 8;
    lpOut++;
    if (ulStart >= lpBits->SizeOfBitMap)
      return ~0U;
  }

  /* Check if reached the end of bitmap */
  if (ulStart >= lpBits->SizeOfBitMap) {
    *lpSize = ulCount - (ulStart - lpBits->SizeOfBitMap);
    return ulFoundAt;
  }

  /* Count blocks of 8 clear bits */
  while (!*lpOut)
  {
    ulCount += 8;
    ulStart += 8;
    if (ulStart >= lpBits->SizeOfBitMap)
    {
      *lpSize = ulCount - (ulStart - lpBits->SizeOfBitMap);
      return ulFoundAt;
    }
    lpOut++;
  }

  /* Count remaining contiguous bits, if any */
  if (!(*lpOut & 1))
  {
    ULONG ulOffset = 0;

    for (;ulOffset < 7u; ulOffset++)
    {
      if (*lpOut & (1 << ulOffset))
        break;
      ulCount++;
    }
  }
  *lpSize = ulCount;
  return ulFoundAt;
}

/*************************************************************************
 * RtlFindNextForwardRunSet	[NTDLL.@]
 *
 * Find the next run of set bits in a bitmap.
 *
 * PARAMS
 *  lpBits  [I] Bitmap pointer
 *  ulStart [I] Bit position to start searching from
 *  lpPos   [O] Start of run
 *
 * RETURNS
 *  Success: The length of the next set run in the bitmap. lpPos is set to
 *           the start of the run.
 *  Failure: 0, if no run is found or any parameters are invalid.
 */
ULONG WINAPI RtlFindNextForwardRunSet(PCRTL_BITMAP lpBits, ULONG ulStart, PULONG lpPos)
{
  ULONG ulSize = 0;

  TRACE("(%p,%lu,%p)\n", lpBits, ulStart, lpPos);

  if (lpBits && ulStart < lpBits->SizeOfBitMap && lpPos)
    *lpPos = NTDLL_FindSetRun(lpBits, ulStart, &ulSize);

  return ulSize;
}

/*************************************************************************
 * RtlFindNextForwardRunClear	[NTDLL.@]
 *
 * Find the next run of clear bits in a bitmap.
 *
 * PARAMS
 *  lpBits  [I] Bitmap pointer
 *  ulStart [I] Bit position to start searching from
 *  lpPos   [O] Start of run
 *
 * RETURNS
 *  Success: The length of the next clear run in the bitmap. lpPos is set to
 *           the start of the run.
 *  Failure: 0, if no run is found or any parameters are invalid.
 */
ULONG WINAPI RtlFindNextForwardRunClear(PCRTL_BITMAP lpBits, ULONG ulStart, PULONG lpPos)
{
  ULONG ulSize = 0;

  TRACE("(%p,%lu,%p)\n", lpBits, ulStart, lpPos);

  if (lpBits && ulStart < lpBits->SizeOfBitMap && lpPos)
    *lpPos = NTDLL_FindClearRun(lpBits, ulStart, &ulSize);

  return ulSize;
}

/*************************************************************************
 * RtlFindLastBackwardRunSet	[NTDLL.@]
 *
 * Find a previous run of set bits in a bitmap.
 *
 * PARAMS
 *  lpBits  [I] Bitmap pointer
 *  ulStart [I] Bit position to start searching from
 *  lpPos   [O] Start of run
 *
 * RETURNS
 *  Success: The length of the previous set run in the bitmap. lpPos is set to
 *           the start of the run.
 *  Failure: 0, if no run is found or any parameters are invalid.
 */
ULONG WINAPI RtlFindLastBackwardRunSet(PCRTL_BITMAP lpBits, ULONG ulStart, PULONG lpPos)
{
  FIXME("(%p,%lu,%p)-stub!\n", lpBits, ulStart, lpPos);
  return 0;
}

/*************************************************************************
 * RtlFindLastBackwardRunClear	[NTDLL.@]
 *
 * Find a previous run of clear bits in a bitmap.
 *
 * PARAMS
 *  lpBits  [I] Bitmap pointer
 *  ulStart [I] Bit position to start searching from
 *  lpPos   [O] Start of run
 *
 * RETURNS
 *  Success: The length of the previous clear run in the bitmap. lpPos is set
 *           to the start of the run.
 *  Failure: 0, if no run is found or any parameters are invalid.
 */
ULONG WINAPI RtlFindLastBackwardRunClear(PCRTL_BITMAP lpBits, ULONG ulStart, PULONG lpPos)
{
  FIXME("(%p,%lu,%p)-stub!\n", lpBits, ulStart, lpPos);
  return 0;
}

/*************************************************************************
 * NTDLL_FindRuns
 *
 * Internal implementation of RtlFindSetRuns/RtlFindClearRuns.
 */
static ULONG NTDLL_FindRuns(PCRTL_BITMAP lpBits, PRTL_BITMAP_RUN lpSeries,
                            ULONG ulCount, BOOLEAN bLongest,
                            ULONG (*fn)(PCRTL_BITMAP,ULONG,PULONG))
{
  BOOL bNeedSort = ulCount > 1;
  ULONG ulPos = 0, ulRuns = 0;

  TRACE("(%p,%p,%ld,%d)\n", lpBits, lpSeries, ulCount, bLongest);

  if (!ulCount)
    return ~0U;

  while (ulPos < lpBits->SizeOfBitMap)
  {
    /* Find next set/clear run */
    ULONG ulSize, ulNextPos = fn(lpBits, ulPos, &ulSize);

    if (ulNextPos == ~0U)
      break;

    if (bLongest && ulRuns == ulCount)
    {
      /* Sort runs with shortest at end, if they are out of order */
      if (bNeedSort)
        qsort(lpSeries, ulRuns, sizeof(RTL_BITMAP_RUN), NTDLL_RunSortFn);

      /* Replace last run if this one is bigger */
      if (ulSize > lpSeries[ulRuns - 1].NumberOfBits)
      {
        lpSeries[ulRuns - 1].StartingIndex = ulNextPos;
        lpSeries[ulRuns - 1].NumberOfBits = ulSize;

        /* We need to re-sort the array, _if_ we didn't leave it sorted */
        if (ulRuns > 1 && ulSize > lpSeries[ulRuns - 2].NumberOfBits)
          bNeedSort = TRUE;
      }
    }
    else
    {
      /* Append to found runs */
      lpSeries[ulRuns].StartingIndex = ulNextPos;
      lpSeries[ulRuns].NumberOfBits = ulSize;
      ulRuns++;

      if (!bLongest && ulRuns == ulCount)
        break;
    }
    ulPos = ulNextPos + ulSize;
  }
  return ulRuns;
}

/*************************************************************************
 * RtlFindSetRuns	[NTDLL.@]
 *
 * Find a series of set runs in a bitmap.
 *
 * PARAMS
 *  lpBits   [I] Bitmap pointer
 *  lpSeries [O] Array for each run found
 *  ulCount  [I] Number of runs to find
 *  bLongest [I] Whether to find the very longest runs or not
 *
 * RETURNS
 *  The number of set runs found.
 */
ULONG WINAPI RtlFindSetRuns(PCRTL_BITMAP lpBits, PRTL_BITMAP_RUN lpSeries,
                            ULONG ulCount, BOOLEAN bLongest)
{
  TRACE("(%p,%p,%lu,%d)\n", lpBits, lpSeries, ulCount, bLongest);

  return NTDLL_FindRuns(lpBits, lpSeries, ulCount, bLongest, NTDLL_FindSetRun);
}

/*************************************************************************
 * RtlFindClearRuns	[NTDLL.@]
 *
 * Find a series of clear runs in a bitmap.
 *
 * PARAMS
 *  lpBits   [I] Bitmap pointer
 *  ulSeries [O] Array for each run found
 *  ulCount  [I] Number of runs to find
 *  bLongest [I] Whether to find the very longest runs or not
 *
 * RETURNS
 *  The number of clear runs found.
 */
ULONG WINAPI RtlFindClearRuns(PCRTL_BITMAP lpBits, PRTL_BITMAP_RUN lpSeries,
                              ULONG ulCount, BOOLEAN bLongest)
{
  TRACE("(%p,%p,%lu,%d)\n", lpBits, lpSeries, ulCount, bLongest);

  return NTDLL_FindRuns(lpBits, lpSeries, ulCount, bLongest, NTDLL_FindClearRun);
}

/*************************************************************************
 * RtlFindLongestRunSet	[NTDLL.@]
 *
 * Find the longest set run in a bitmap.
 *
 * PARAMS
 *  lpBits   [I] Bitmap pointer
 *  pulStart [O] Destination for start of run
 *
 * RETURNS
 *  The length of the run found, or 0 if no run is found.
 */
ULONG WINAPI RtlFindLongestRunSet(PCRTL_BITMAP lpBits, PULONG pulStart)
{
  RTL_BITMAP_RUN br;

  TRACE("(%p,%p)\n", lpBits, pulStart);

  if (RtlFindSetRuns(lpBits, &br, 1, TRUE) == 1)
  {
    if (pulStart)
      *pulStart = br.StartingIndex;
    return br.NumberOfBits;
  }
  return 0;
}

/*************************************************************************
 * RtlFindLongestRunClear	[NTDLL.@]
 *
 * Find the longest clear run in a bitmap.
 *
 * PARAMS
 *  lpBits   [I] Bitmap pointer
 *  pulStart [O] Destination for start of run
 *
 * RETURNS
 *  The length of the run found, or 0 if no run is found.
 */
ULONG WINAPI RtlFindLongestRunClear(PCRTL_BITMAP lpBits, PULONG pulStart)
{
  RTL_BITMAP_RUN br;

  TRACE("(%p,%p)\n", lpBits, pulStart);

  if (RtlFindClearRuns(lpBits, &br, 1, TRUE) == 1)
  {
    if (pulStart)
      *pulStart = br.StartingIndex;
    return br.NumberOfBits;
  }
  return 0;
}
