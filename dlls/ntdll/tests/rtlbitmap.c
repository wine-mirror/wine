/* Unit test suite for Rtl bitmap functions
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTES
 * We use function pointers here as some of the bitmap functions exist only
 * in later versions of ntdll.
 */
#include <stdarg.h>

#include "wine/test.h"
#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winreg.h"
#include "winternl.h"

/* Function ptrs for ordinal calls */
static HMODULE hntdll = 0;
static VOID (WINAPI *pRtlInitializeBitMap)(PRTL_BITMAP,LPBYTE,ULONG);
static VOID (WINAPI *pRtlSetAllBits)(PRTL_BITMAP);
static VOID (WINAPI *pRtlClearAllBits)(PRTL_BITMAP);
static VOID (WINAPI *pRtlSetBits)(PRTL_BITMAP,ULONG,ULONG);
static VOID (WINAPI *pRtlClearBits)(PRTL_BITMAP,ULONG,ULONG);
static BOOLEAN (WINAPI *pRtlAreBitsSet)(PRTL_BITMAP,ULONG,ULONG);
static BOOLEAN (WINAPI *pRtlAreBitsClear)(PRTL_BITMAP,ULONG,ULONG);
static ULONG (WINAPI *pRtlFindSetBitsAndClear)(PRTL_BITMAP,ULONG,ULONG);
static ULONG (WINAPI *pRtlFindClearBitsAndSet)(PRTL_BITMAP,ULONG,ULONG);
static CCHAR (WINAPI *pRtlFindMostSignificantBit)(ULONGLONG);
static CCHAR (WINAPI *pRtlFindLeastSignificantBit)(ULONGLONG);
static ULONG (WINAPI *pRtlFindSetRuns)(PRTL_BITMAP,PRTL_BITMAP_RUN,ULONG,BOOLEAN);
static ULONG (WINAPI *pRtlFindClearRuns)(PRTL_BITMAP,PRTL_BITMAP_RUN,ULONG,BOOLEAN);
static ULONG (WINAPI *pRtlNumberOfSetBits)(PRTL_BITMAP);
static ULONG (WINAPI *pRtlNumberOfClearBits)(PRTL_BITMAP);
static ULONG (WINAPI *pRtlFindLongestRunSet)(PRTL_BITMAP,PULONG);
static ULONG (WINAPI *pRtlFindLongestRunClear)(PRTL_BITMAP,PULONG);

static BYTE buff[256];
static RTL_BITMAP bm;

static void InitFunctionPtrs()
{
  hntdll = LoadLibraryA("ntdll.dll");
  ok(hntdll != 0, "LoadLibrary failed");
  if (hntdll)
  {
    pRtlInitializeBitMap = (void *)GetProcAddress(hntdll, "RtlInitializeBitMap");
    pRtlSetAllBits = (void *)GetProcAddress(hntdll, "RtlSetAllBits");
    pRtlClearAllBits = (void *)GetProcAddress(hntdll, "RtlClearAllBits");
    pRtlSetBits = (void *)GetProcAddress(hntdll, "RtlSetBits");
    pRtlClearBits = (void *)GetProcAddress(hntdll, "RtlClearBits");
    pRtlAreBitsSet = (void *)GetProcAddress(hntdll, "RtlAreBitsSet");
    pRtlAreBitsClear = (void *)GetProcAddress(hntdll, "RtlAreBitsClear");
    pRtlNumberOfSetBits = (void *)GetProcAddress(hntdll, "RtlNumberOfSetBits");
    pRtlNumberOfClearBits = (void *)GetProcAddress(hntdll, "RtlNumberOfClearBits");
    pRtlFindSetBitsAndClear = (void *)GetProcAddress(hntdll, "RtlFindSetBitsAndClear");
    pRtlFindClearBitsAndSet = (void *)GetProcAddress(hntdll, "RtlFindClearBitsAndSet");
    pRtlFindMostSignificantBit = (void *)GetProcAddress(hntdll, "RtlFindMostSignificantBit");
    pRtlFindLeastSignificantBit = (void *)GetProcAddress(hntdll, "RtlFindLeastSignificantBit");
    pRtlFindSetRuns = (void *)GetProcAddress(hntdll, "RtlFindSetRuns");
    pRtlFindClearRuns = (void *)GetProcAddress(hntdll, "RtlFindClearRuns");
    pRtlFindLongestRunSet = (void *)GetProcAddress(hntdll, "RtlFindLongestRunSet");
    pRtlFindLongestRunClear = (void *)GetProcAddress(hntdll, "RtlFindLongestRunClear");
  }
}

static void test_RtlInitializeBitMap(void)
{
  bm.SizeOfBitMap = 0;
  bm.BitMapBuffer = 0;

  memset(buff, 0, sizeof(buff));
  buff[0] = 77; /* Check buffer is not written to during init */
  buff[79] = 77;

  pRtlInitializeBitMap(&bm, buff, 800);
  ok(bm.SizeOfBitMap == 800, "size uninitialised");
  ok(bm.BitMapBuffer == buff,"buffer uninitialised");
  ok(buff[0] == 77 && buff[79] == 77, "wrote to buffer");

  /* Test inlined version */
  RtlInitializeBitMap(&bm, buff, 800);
  ok(bm.SizeOfBitMap == 800, "size uninitialised");
  ok(bm.BitMapBuffer == buff,"buffer uninitialised");
  ok(buff[0] == 77 && buff[79] == 77, "wrote to buffer");
}

static void test_RtlSetAllBits(void)
{
  if (!pRtlSetAllBits)
    return;

  memset(buff, 0 , sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, 1);

  pRtlSetAllBits(&bm);
  ok(buff[0] == 0xff && buff[1] == 0xff && buff[2] == 0xff &&
     buff[3] == 0xff, "didnt round up size");
  ok(buff[4] == 0, "set more than rounded size");

  /* Test inlined version */
  memset(buff, 0 , sizeof(buff));
  RtlSetAllBits(&bm);
  ok(buff[0] == 0xff && buff[1] == 0xff && buff[2] == 0xff &&
     buff[3] == 0xff, "didnt round up size");
  ok(buff[4] == 0, "set more than rounded size");
}

static void test_RtlClearAllBits()
{
  if (!pRtlClearAllBits)
    return;

  memset(buff, 0xff , sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, 1);

  pRtlClearAllBits(&bm);
  ok(!buff[0] && !buff[1] && !buff[2] && !buff[3], "didnt round up size");
  ok(buff[4] == 0xff, "cleared more than rounded size");

  /* Test inlined version */
  memset(buff, 0xff , sizeof(buff));
  RtlClearAllBits(&bm);
  ok(!buff[0] && !buff[1] && !buff[2] && !buff[3] , "didnt round up size");
  ok(buff[4] == 0xff, "cleared more than rounded size");
}

static void test_RtlSetBits()
{
  if (!pRtlSetBits)
    return;

  memset(buff, 0 , sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  pRtlSetBits(&bm, 0, 1);
  ok(buff[0] == 1, "didnt set 1st bit");

  buff[0] = 0;
  pRtlSetBits(&bm, 7, 2);
  ok(buff[0] == 0x80 && buff[1] == 1, "didnt span w/len < 8");

  buff[0] = buff[1] = 0;
  pRtlSetBits(&bm, 7, 10);
  ok(buff[0] == 0x80 && buff[1] == 0xff && buff[2] == 1, "didnt span w/len > 8");

  buff[0] = buff[1] = buff[2] = 0;
  pRtlSetBits(&bm, 0, 8); /* 1st byte */
  ok(buff[0] == 0xff, "didnt set all bits");
  ok(!buff[1], "set too many bits");

  pRtlSetBits(&bm, sizeof(buff)*8-1, 1); /* last bit */
  ok(buff[sizeof(buff)-1] == 0x80, "didnt set last bit");
}

static void test_RtlClearBits()
{
  if (!pRtlClearBits)
    return;

  memset(buff, 0xff , sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  pRtlClearBits(&bm, 0, 1);
  ok(buff[0] == 0xfe, "didnt clear 1st bit");

  buff[0] = 0xff;
  pRtlClearBits(&bm, 7, 2);
  ok(buff[0] == 0x7f && buff[1] == 0xfe, "didnt span w/len < 8");

  buff[0] = buff[1] = 0xff;
  pRtlClearBits(&bm, 7, 10);
  ok(buff[0] == 0x7f && buff[1] == 0 && buff[2] == 0xfe, "didnt span w/len > 8");

  buff[0] = buff[1] = buff[2] = 0xff;
  pRtlClearBits(&bm, 0, 8);  /* 1st byte */
  ok(!buff[0], "didnt clear all bits");
  ok(buff[1] == 0xff, "cleared too many bits");

  pRtlClearBits(&bm, sizeof(buff)*8-1, 1);
  ok(buff[sizeof(buff)-1] == 0x7f, "didnt set last bit");
}

static void test_RtlCheckBit()
{
  BOOLEAN bRet;

  memset(buff, 0 , sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);
  pRtlSetBits(&bm, 0, 1);
  pRtlSetBits(&bm, 7, 2);
  pRtlSetBits(&bm, sizeof(buff)*8-1, 1);

  bRet = RtlCheckBit(&bm, 0);
  ok (bRet, "didnt find set bit");
  bRet = RtlCheckBit(&bm, 7);
  ok (bRet, "didnt find set bit");
  bRet = RtlCheckBit(&bm, 8);
  ok (bRet, "didnt find set bit");
  bRet = RtlCheckBit(&bm, sizeof(buff)*8-1);
  ok (bRet, "didnt find set bit");
  bRet = RtlCheckBit(&bm, 1);
  ok (!bRet, "found non set bit");
  bRet = RtlCheckBit(&bm, sizeof(buff)*8-2);
  ok (!bRet, "found non set bit");
}

static void test_RtlAreBitsSet()
{
  BOOLEAN bRet;

  if (!pRtlAreBitsSet)
    return;

  memset(buff, 0 , sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  bRet = pRtlAreBitsSet(&bm, 0, 1);
  ok (!bRet, "found set bits after init");

  pRtlSetBits(&bm, 0, 1);
  bRet = pRtlAreBitsSet(&bm, 0, 1);
  ok (bRet, "didnt find set bits");

  buff[0] = 0;
  pRtlSetBits(&bm, 7, 2);
  bRet = pRtlAreBitsSet(&bm, 7, 2);
  ok(bRet, "didnt find w/len < 8");
  bRet = pRtlAreBitsSet(&bm, 6, 3);
  ok(!bRet, "found non set bit");
  bRet = pRtlAreBitsSet(&bm, 7, 3);
  ok(!bRet, "found non set bit");

  buff[0] = buff[1] = 0;
  pRtlSetBits(&bm, 7, 10);
  bRet = pRtlAreBitsSet(&bm, 7, 10);
  ok(bRet, "didnt find w/len < 8");
  bRet = pRtlAreBitsSet(&bm, 6, 11);
  ok(!bRet, "found non set bit");
  bRet = pRtlAreBitsSet(&bm, 7, 11);
  ok(!bRet, "found non set bit");

  buff[0] = buff[1] = buff[2] = 0;
  pRtlSetBits(&bm, 0, 8); /* 1st byte */
  bRet = pRtlAreBitsSet(&bm, 0, 8);
  ok(bRet, "didn't find whole byte");

  pRtlSetBits(&bm, sizeof(buff)*8-1, 1);
  bRet = pRtlAreBitsSet(&bm, sizeof(buff)*8-1, 1);
  ok(bRet, "didn't find last bit");
}

static void test_RtlAreBitsClear()
{
  BOOLEAN bRet;

  if (!pRtlAreBitsClear)
    return;

  memset(buff, 0xff , sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  bRet = pRtlAreBitsClear(&bm, 0, 1);
  ok (!bRet, "found clear bits after init");

  pRtlClearBits(&bm, 0, 1);
  bRet = pRtlAreBitsClear(&bm, 0, 1);
  ok (bRet, "didnt find set bits");

  buff[0] = 0xff;
  pRtlClearBits(&bm, 7, 2);
  bRet = pRtlAreBitsClear(&bm, 7, 2);
  ok(bRet, "didnt find w/len < 8");
  bRet = pRtlAreBitsClear(&bm, 6, 3);
  ok(!bRet, "found non clear bit");
  bRet = pRtlAreBitsClear(&bm, 7, 3);
  ok(!bRet, "found non clear bit");

  buff[0] = buff[1] = 0xff;
  pRtlClearBits(&bm, 7, 10);
  bRet = pRtlAreBitsClear(&bm, 7, 10);
  ok(bRet, "didnt find w/len < 8");
  bRet = pRtlAreBitsClear(&bm, 6, 11);
  ok(!bRet, "found non clear bit");
  bRet = pRtlAreBitsClear(&bm, 7, 11);
  ok(!bRet, "found non clear bit");

  buff[0] = buff[1] = buff[2] = 0xff;
  pRtlClearBits(&bm, 0, 8); /* 1st byte */
  bRet = pRtlAreBitsClear(&bm, 0, 8);
  ok(bRet, "didn't find whole byte");

  pRtlClearBits(&bm, sizeof(buff)*8-1, 1);
  bRet = pRtlAreBitsClear(&bm, sizeof(buff)*8-1, 1);
  ok(bRet, "didn't find last bit");
}

static void test_RtlNumberOfSetBits()
{
  ULONG ulCount;

  if (!pRtlNumberOfSetBits)
    return;

  memset(buff, 0 , sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  ulCount = pRtlNumberOfSetBits(&bm);
  ok(ulCount == 0, "set bits after init");

  pRtlSetBits(&bm, 0, 1); /* Set 1st bit */
  ulCount = pRtlNumberOfSetBits(&bm);
  ok(ulCount == 1, "count wrong");

  pRtlSetBits(&bm, 7, 8); /* 8 more, spanning bytes 1-2 */
  ulCount = pRtlNumberOfSetBits(&bm);
  ok(ulCount == 8+1, "count wrong");

  pRtlSetBits(&bm, 17, 33); /* 33 more crossing ULONG boundary */
  ulCount = pRtlNumberOfSetBits(&bm);
  ok(ulCount == 8+1+33, "count wrong");

  pRtlSetBits(&bm, sizeof(buff)*8-1, 1); /* Set last bit */
  ulCount = pRtlNumberOfSetBits(&bm);
  ok(ulCount == 8+1+33+1, "count wrong");
}

static void test_RtlNumberOfClearBits()
{
  ULONG ulCount;

  if (!pRtlNumberOfClearBits)
    return;

  memset(buff, 0xff , sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  ulCount = pRtlNumberOfClearBits(&bm);
  ok(ulCount == 0, "cleared bits after init");

  pRtlClearBits(&bm, 0, 1); /* Set 1st bit */
  ulCount = pRtlNumberOfClearBits(&bm);
  ok(ulCount == 1, "count wrong");

  pRtlClearBits(&bm, 7, 8); /* 8 more, spanning bytes 1-2 */
  ulCount = pRtlNumberOfClearBits(&bm);
  ok(ulCount == 8+1, "count wrong");

  pRtlClearBits(&bm, 17, 33); /* 33 more crossing ULONG boundary */
  ulCount = pRtlNumberOfClearBits(&bm);
  ok(ulCount == 8+1+33, "count wrong");

  pRtlClearBits(&bm, sizeof(buff)*8-1, 1); /* Set last bit */
  ulCount = pRtlNumberOfClearBits(&bm);
  ok(ulCount == 8+1+33+1, "count wrong");
}

/* Note: this tests RtlFindSetBits also */
static void test_RtlFindSetBitsAndClear()
{
  BOOLEAN bRet;
  ULONG ulPos;

  if (!pRtlFindSetBitsAndClear)
    return;

  memset(buff, 0, sizeof(buff));
  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  pRtlSetBits(&bm, 0, 32);
  ulPos = pRtlFindSetBitsAndClear(&bm, 32, 0);
  ok (ulPos == 0, "didnt find bits");
  if(ulPos == 0)
  {
    bRet = pRtlAreBitsClear(&bm, 0, 32);
    ok (bRet, "found but didnt clear");
  }

  memset(buff, 0 , sizeof(buff));
  pRtlSetBits(&bm, 40, 77);
  ulPos = pRtlFindSetBitsAndClear(&bm, 77, 0);
  ok (ulPos == 40, "didnt find bits");
  if(ulPos == 40)
  {
    bRet = pRtlAreBitsClear(&bm, 40, 77);
    ok (bRet, "found but didnt clear");
  }
}

/* Note: this tests RtlFindClearBits also */
static void test_RtlFindClearBitsAndSet()
{
  BOOLEAN bRet;
  ULONG ulPos;

  if (!pRtlFindClearBitsAndSet)
    return;

  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  memset(buff, 0xff, sizeof(buff));
  pRtlSetBits(&bm, 0, 32);
  ulPos = pRtlFindSetBitsAndClear(&bm, 32, 0);
  ok (ulPos == 0, "didnt find bits");
  if(ulPos == 0)
  {
      bRet = pRtlAreBitsClear(&bm, 0, 32);
      ok (bRet, "found but didnt clear");
  }

  memset(buff, 0xff , sizeof(buff));
  pRtlClearBits(&bm, 40, 77);
  ulPos = pRtlFindClearBitsAndSet(&bm, 77, 50);
  ok (ulPos == 40, "didnt find bits");
  if(ulPos == 40)
  {
    bRet = pRtlAreBitsSet(&bm, 40, 77);
    ok (bRet, "found but didnt set");
  }
}

static void test_RtlFindMostSignificantBit()
{
  int i;
  CCHAR cPos;
  ULONGLONG ulLong;

  if (!pRtlFindMostSignificantBit)
    return;

  for (i = 0; i < 64; i++)
  {
    ulLong = 1ul;
    ulLong <<= i;

    cPos = pRtlFindMostSignificantBit(ulLong);
    ok (cPos == i, "didnt find MSB %llx %d %d", ulLong, i, cPos);

    /* Set all bits lower than bit i */
    ulLong = ((ulLong - 1) << 1) | 1;

    cPos = pRtlFindMostSignificantBit(ulLong);
    ok (cPos == i, "didnt find MSB %llx %d %d", ulLong, i, cPos);
  }
  cPos = pRtlFindMostSignificantBit(0);
  ok (cPos == -1, "found bit when not set");
}

static void test_RtlFindLeastSignificantBit()
{
  int i;
  CCHAR cPos;
  ULONGLONG ulLong;

  if (!pRtlFindLeastSignificantBit)
    return;

  for (i = 0; i < 64; i++)
  {
    ulLong = (ULONGLONG)1 << i;

    cPos = pRtlFindLeastSignificantBit(ulLong);
    ok (cPos == i, "didnt find LSB %llx %d %d", ulLong, i, cPos);

    ulLong = ~((ULONGLONG)0) << i;

    cPos = pRtlFindLeastSignificantBit(ulLong);
    ok (cPos == i, "didnt find LSB %llx %d %d", ulLong, i, cPos);
  }
  cPos = pRtlFindLeastSignificantBit(0);
  ok (cPos == -1, "found bit when not set");
}

/* Note: Also tests RtlFindLongestRunSet() */
static void test_RtlFindSetRuns()
{
  RTL_BITMAP_RUN runs[16];
  ULONG ulCount;

  if (!pRtlFindSetRuns)
    return;

  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  memset(buff, 0, sizeof(buff));
  ulCount = pRtlFindSetRuns(&bm, runs, 16, TRUE);
  ok (ulCount == 0, "found set bits in empty bitmap");

  memset(runs, 0, sizeof(runs));
  memset(buff, 0xff, sizeof(buff));
  ulCount = pRtlFindSetRuns(&bm, runs, 16, TRUE);
  ok (ulCount == 1, "didnt find set bits");
  ok (runs[0].StartOfRun == 0,"bad start");
  ok (runs[0].SizeOfRun == sizeof(buff)*8,"bad size");

  /* Set up 3 runs */
  memset(runs, 0, sizeof(runs));
  memset(buff, 0, sizeof(buff));
  pRtlSetBits(&bm, 7, 19);
  pRtlSetBits(&bm, 101, 3);
  pRtlSetBits(&bm, 1877, 33);

  /* Get first 2 */
  ulCount = pRtlFindSetRuns(&bm, runs, 2, FALSE);
  ok (runs[0].StartOfRun == 7 || runs[0].StartOfRun == 101,"bad find");
  ok (runs[1].StartOfRun == 7 || runs[1].StartOfRun == 101,"bad find");
  ok (runs[0].SizeOfRun + runs[1].SizeOfRun == 19 + 3,"bad size");
  ok (runs[0].StartOfRun != runs[1].StartOfRun,"found run twice");
  ok (runs[2].StartOfRun == 0,"found extra run");

  /* Get longest 3 */
  memset(runs, 0, sizeof(runs));
  ulCount = pRtlFindSetRuns(&bm, runs, 2, TRUE);
  ok (runs[0].StartOfRun == 7 || runs[0].StartOfRun == 1877,"bad find");
  ok (runs[1].StartOfRun == 7 || runs[1].StartOfRun == 1877,"bad find");
  ok (runs[0].SizeOfRun + runs[1].SizeOfRun == 33 + 19,"bad size");
  ok (runs[0].StartOfRun != runs[1].StartOfRun,"found run twice");
  ok (runs[2].StartOfRun == 0,"found extra run");

  /* Get all 3 */
  memset(runs, 0, sizeof(runs));
  ulCount = pRtlFindSetRuns(&bm, runs, 3, TRUE);
  ok (runs[0].StartOfRun == 7 || runs[0].StartOfRun == 101 ||
      runs[0].StartOfRun == 1877,"bad find");
  ok (runs[1].StartOfRun == 7 || runs[1].StartOfRun == 101 ||
      runs[1].StartOfRun == 1877,"bad find");
  ok (runs[2].StartOfRun == 7 || runs[2].StartOfRun == 101 ||
      runs[2].StartOfRun == 1877,"bad find");
  ok (runs[0].SizeOfRun + runs[1].SizeOfRun
      + runs[2].SizeOfRun == 19 + 3 + 33,"bad size");
  ok (runs[0].StartOfRun != runs[1].StartOfRun,"found run twice");
  ok (runs[1].StartOfRun != runs[2].StartOfRun,"found run twice");
  ok (runs[3].StartOfRun == 0,"found extra run");

  if (pRtlFindLongestRunSet)
  {
    ULONG ulStart = 0;

    ulCount = pRtlFindLongestRunSet(&bm, &ulStart);
    ok(ulCount == 33 && ulStart == 1877,"didn't find longest %ld %ld",ulCount,ulStart);

    memset(buff, 0, sizeof(buff));
    ulCount = pRtlFindLongestRunSet(&bm, &ulStart);
    ok(ulCount == 0,"found longest when none set");
  }
}

/* Note: Also tests RtlFindLongestRunClear() */
static void test_RtlFindClearRuns()
{
  RTL_BITMAP_RUN runs[16];
  ULONG ulCount;

  if (!pRtlFindClearRuns)
    return;

  pRtlInitializeBitMap(&bm, buff, sizeof(buff)*8);

  memset(buff, 0xff, sizeof(buff));
  ulCount = pRtlFindClearRuns(&bm, runs, 16, TRUE);
  ok (ulCount == 0, "found clear bits in full bitmap");

  memset(runs, 0, sizeof(runs));
  memset(buff, 0, sizeof(buff));
  ulCount = pRtlFindClearRuns(&bm, runs, 16, TRUE);
  ok (ulCount == 1, "didnt find clear bits");
  ok (runs[0].StartOfRun == 0,"bad start");
  ok (runs[0].SizeOfRun == sizeof(buff)*8,"bad size");

  /* Set up 3 runs */
  memset(runs, 0, sizeof(runs));
  memset(buff, 0xff, sizeof(buff));
  pRtlClearBits(&bm, 7, 19);
  pRtlClearBits(&bm, 101, 3);
  pRtlClearBits(&bm, 1877, 33);

  /* Get first 2 */
  ulCount = pRtlFindClearRuns(&bm, runs, 2, FALSE);
  ok (runs[0].StartOfRun == 7 || runs[0].StartOfRun == 101,"bad find");
  ok (runs[1].StartOfRun == 7 || runs[1].StartOfRun == 101,"bad find");
  ok (runs[0].SizeOfRun + runs[1].SizeOfRun == 19 + 3,"bad size");
  ok (runs[0].StartOfRun != runs[1].StartOfRun,"found run twice");
  ok (runs[2].StartOfRun == 0,"found extra run");

  /* Get longest 3 */
  memset(runs, 0, sizeof(runs));
  ulCount = pRtlFindClearRuns(&bm, runs, 2, TRUE);
  ok (runs[0].StartOfRun == 7 || runs[0].StartOfRun == 1877,"bad find");
  ok (runs[1].StartOfRun == 7 || runs[1].StartOfRun == 1877,"bad find");
  ok (runs[0].SizeOfRun + runs[1].SizeOfRun == 33 + 19,"bad size");
  ok (runs[0].StartOfRun != runs[1].StartOfRun,"found run twice");
  ok (runs[2].StartOfRun == 0,"found extra run");

  /* Get all 3 */
  memset(runs, 0, sizeof(runs));
  ulCount = pRtlFindClearRuns(&bm, runs, 3, TRUE);
  ok (runs[0].StartOfRun == 7 || runs[0].StartOfRun == 101 ||
      runs[0].StartOfRun == 1877,"bad find");
  ok (runs[1].StartOfRun == 7 || runs[1].StartOfRun == 101 ||
      runs[1].StartOfRun == 1877,"bad find");
  ok (runs[2].StartOfRun == 7 || runs[2].StartOfRun == 101 ||
      runs[2].StartOfRun == 1877,"bad find");
  ok (runs[0].SizeOfRun + runs[1].SizeOfRun
      + runs[2].SizeOfRun == 19 + 3 + 33,"bad size");
  ok (runs[0].StartOfRun != runs[1].StartOfRun,"found run twice");
  ok (runs[1].StartOfRun != runs[2].StartOfRun,"found run twice");
  ok (runs[3].StartOfRun == 0,"found extra run");

  if (pRtlFindLongestRunClear)
  {
    ULONG ulStart = 0;

    ulCount = pRtlFindLongestRunClear(&bm, &ulStart);
    ok(ulCount == 33 && ulStart == 1877,"didn't find longest");

    memset(buff, 0xff, sizeof(buff));
    ulCount = pRtlFindLongestRunClear(&bm, &ulStart);
    ok(ulCount == 0,"found longest when none clear");
  }

}

START_TEST(rtlbitmap)
{
  InitFunctionPtrs();

  if (pRtlInitializeBitMap)
  {
    test_RtlInitializeBitMap();
    test_RtlSetAllBits();
    test_RtlClearAllBits();
    test_RtlSetBits();
    test_RtlClearBits();
    test_RtlCheckBit();
    test_RtlAreBitsSet();
    test_RtlAreBitsClear();
    test_RtlNumberOfSetBits();
    test_RtlNumberOfClearBits();
    test_RtlFindSetBitsAndClear();
    test_RtlFindClearBitsAndSet();
    test_RtlFindMostSignificantBit();
    test_RtlFindLeastSignificantBit();
    test_RtlFindSetRuns();
    test_RtlFindClearRuns();
  }
}
