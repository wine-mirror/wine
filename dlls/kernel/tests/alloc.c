/*
 * Unit test suite for memory allocation functions.
 *
 * Copyright 2002 Geoffrey Hausheer
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
 */

#include "wine/test.h"
#include "winbase.h"
#include "winerror.h"

/* Currently Wine doesn't have macros for LocalDiscard and GlobalDiscard
   so I am disableing the checks for them.  These macros are equivalent
   to reallocing '0' bytes, so we try that instead
*/
#define DISCARD_DEFINED 0

/* The following functions don't have tests, because either I don't know how
   to test them, or they are WinNT only, or require multiple threads.
   Since the last two issues shouldn't really stop the tests from being
   written, assume for now that it is all due to the first case
       HeapCompact
       HeapLock
       HeapQueryInformation
       HeapSetInformation
       HeapUnlock
       HeapValidate
       HeapWalk
*/
/* In addition, these features aren't being tested
       HEAP_NO_SERIALIZE
       HEAP_GENERATE_EXCEPTIONS
       STATUS_ACCESS_VIOLATION (error code from HeapAlloc)
*/

static void test_Heap(void)
{
    SYSTEM_INFO sysInfo;
    ULONG memchunk;
    HANDLE heap;
    LPVOID mem1,mem1a,mem3;
    UCHAR *mem2,*mem2a;
    UINT error,i;
    DWORD dwSize;

/* Retrieve the page size for this system */
    sysInfo.dwPageSize=0;
    GetSystemInfo(&sysInfo);
    ok(sysInfo.dwPageSize>0,"GetSystemInfo should return a valid page size");

/* Create a Heap with a minimum and maximum size */
/* Note that Windows and Wine seem to behave a bit differently with respect
   to memory allocation.  In Windows, you can't access all the memory
   specified in the heap (due to overhead), so choosing a reasonable maximum
   size for the heap was done mostly by trial-and-error on Win2k.  It may need
   more tweaking for otherWindows variants.
*/
    memchunk=10*sysInfo.dwPageSize;
    heap=HeapCreate((DWORD)NULL,2*memchunk,5*memchunk);

/* Check that HeapCreate allocated the right amount of ram */
    todo_wine {
    /* Today HeapCreate seems to return a memory block larger than specified.
       MSDN says the maximum heap size should be dwMaximumSize rounded up to the
       nearest page boundary
    */
      mem1=HeapAlloc(heap,(DWORD)NULL,5*memchunk+1);
      ok(mem1==NULL,"HeapCreate allocated more Ram than it should have");
      if(mem1) {
        HeapFree(heap,(DWORD)NULL,mem1);
      }
    }

/* Check that a normal alloc works */
    mem1=HeapAlloc(heap,(DWORD)NULL,memchunk);
    ok(mem1!=NULL,"HeapAlloc failed");
    if(mem1) {
      ok(HeapSize(heap,(DWORD)NULL,mem1)>=memchunk, "HeapAlloc should return a big enough memory block");
    }

/* Check that a 'zeroing' alloc works */
    mem2=(UCHAR *)HeapAlloc(heap,HEAP_ZERO_MEMORY,memchunk);
    ok(mem2!=NULL,"HeapAlloc failed");
    if(mem2) {
      ok(HeapSize(heap,(DWORD)NULL,mem2)>=memchunk,"HeapAlloc should return a big enough memory block");
      error=0;
      for(i=0;i<memchunk;i++) {
        if(mem2[i]!=0) {
          error=1;
        }
      }
      ok(!error,"HeapAlloc should have zeroed out it's allocated memory");
    }

/* Check that HeapAlloc returns NULL when requested way too much memory */
    mem3=HeapAlloc(heap,(DWORD)NULL,5*memchunk);
    ok(mem3==NULL,"HeapAlloc should return NULL");
    if(mem3) {
      ok(HeapFree(heap,(DWORD)NULL,mem3),"HeapFree didn't pass successfully");
    }

/* Check that HeapRealloc works */
    mem2a=(UCHAR *)HeapReAlloc(heap,HEAP_ZERO_MEMORY,mem2,memchunk+5*sysInfo.dwPageSize);
    ok(mem2a!=NULL,"HeapReAlloc failed");
    if(mem2a) {
      ok(HeapSize(heap,(DWORD)NULL,mem2a)>=memchunk+5*sysInfo.dwPageSize,"HeapReAlloc failed");
      error=0;
      for(i=0;i<5*sysInfo.dwPageSize;i++) {
        if(mem2a[memchunk+i]!=0) {
          error=1;
        }
      }
      ok(!error,"HeapReAlloc should have zeroed out it's allocated memory");
    }

/* Check that HeapRealloc honours HEAP_REALLOC_IN_PLACE_ONLY */
    error=0;
    mem1a=HeapReAlloc(heap,HEAP_REALLOC_IN_PLACE_ONLY,mem1,memchunk+sysInfo.dwPageSize);
    if(mem1a!=NULL) {
      if(mem1a!=mem1) {
        error=1;
      }
    }
    ok(mem1a==NULL || error==0,"HeapReAlloc didn't honour HEAP_REALLOC_IN_PLACE_ONLY");

/* Check that HeapFree works correctly */
   if(mem1a) {
     ok(HeapFree(heap,(DWORD)NULL,mem1a),"HeapFree failed");
   } else {
     ok(HeapFree(heap,(DWORD)NULL,mem1),"HeapFree failed");
   }
   if(mem2a) {
     ok(HeapFree(heap,(DWORD)NULL,mem2a),"HeapFree failed");
   } else {
     ok(HeapFree(heap,(DWORD)NULL,mem2),"HeapFree failed");
   }

   /* take just freed pointer */
   if (mem1a)
       mem1 = mem1a;

   /* try to free it one more time */
   HeapFree(heap, 0, mem1);

   dwSize = HeapSize(heap, 0, mem1);
   ok(dwSize == 0xFFFFFFFF, "The size");

   /* 0-length buffer */
   mem1 = HeapAlloc(heap, 0, 0);
   ok(mem1 != NULL, "Reserved memory");

   dwSize = HeapSize(heap, 0, mem1);
   /* should work with 0-length buffer */
   ok((dwSize >= 0) && (dwSize < 0xFFFFFFFF),
      "The size of the 0-length buffer");
   ok(HeapFree(heap, 0, mem1), "Freed the 0-length buffer");

/* Check that HeapDestry works */
   ok(HeapDestroy(heap),"HeapDestroy failed");
}

/* The following functions don't have tests, because either I don't know how
   to test them, or they are WinNT only, or require multiple threads.
   Since the last two issues shouldn't really stop the tests from being
   written, assume for now that it is all due to the first case
       GlobalFlags
       GlobalMemoryStatus
       GlobalMemoryStatusEx
*/
/* In addition, these features aren't being tested
       GMEM_DISCADABLE
       GMEM_NOCOMPACT
*/
static void test_Global(void)
{
    ULONG memchunk;
    HGLOBAL mem1,mem2,mem2a,mem2b;
    UCHAR *mem2ptr;
    UINT error,i;
    memchunk=100000;

    SetLastError(NO_ERROR);
/* Check that a normal alloc works */
    mem1=GlobalAlloc((UINT)NULL,memchunk);
    ok(mem1!=NULL,"GlobalAlloc failed");
    if(mem1) {
      ok(GlobalSize(mem1)>=memchunk, "GlobalAlloc should return a big enough memory block");
    }

/* Check that a 'zeroing' alloc works */
    mem2=GlobalAlloc(GMEM_ZEROINIT,memchunk);
    ok(mem2!=NULL,"GlobalAlloc failed");
    if(mem2) {
      ok(GlobalSize(mem2)>=memchunk,"GlobalAlloc should return a big enough memory block");
      mem2ptr=(UCHAR *)GlobalLock(mem2);
      ok(mem2ptr==(UCHAR *)mem2,"GlobalLock should have returned the same memory as was allocated");
      if(mem2ptr) {
        error=0;
        for(i=0;i<memchunk;i++) {
          if(mem2ptr[i]!=0) {
            error=1;
          }
        }
        ok(!error,"GlobalAlloc should have zeroed out it's allocated memory");
      }
   }
/* Check that GlobalReAlloc works */
/* Check that we can change GMEM_FIXED to GMEM_MOVEABLE */
    mem2a=GlobalReAlloc(mem2,0,GMEM_MODIFY | GMEM_MOVEABLE);
    ok(mem2a!=NULL,"GlobalReAlloc failed to convert FIXED to MOVEABLE");
    if(mem2a!=NULL) {
      mem2=mem2a;
    }
    mem2ptr=GlobalLock(mem2a);
    ok(mem2ptr!=NULL && !GlobalUnlock(mem2a)&&GetLastError()==NO_ERROR,
        "Converting from FIXED to MOVEABLE didn't REALLY work");

/* Check that ReAllocing memory works as expected */
    mem2a=GlobalReAlloc(mem2,2*memchunk,GMEM_MOVEABLE | GMEM_ZEROINIT);
    ok(mem2a!=NULL,"GlobalReAlloc failed");
    if(mem2a) {
      ok(GlobalSize(mem2a)>=2*memchunk,"GlobalReAlloc failed");
      mem2ptr=(UCHAR *)GlobalLock(mem2a);
      ok(mem2ptr!=NULL,"GlobalLock Failed.");
      if(mem2ptr) {
        error=0;
        for(i=0;i<memchunk;i++) {
          if(mem2ptr[memchunk+i]!=0) {
            error=1;
          }
        }
        ok(!error,"GlobalReAlloc should have zeroed out it's allocated memory");

/* Check that GlobalHandle works */
        mem2b=GlobalHandle(mem2ptr);
        ok(mem2b==mem2a,"GlobalHandle didn't return the correct memory handle");

/* Check that we can't discard locked memory */
#if DISCARD_DEFINED
        /* Wine doesn't include the GlobalDiscard function */
        mem2b=GlobalDiscard(mem2a);
        ok(mem2b==NULL,"Discarded memory we shouldn't have");
#else
        /* This is functionally equivalent to the above */
        mem2b=GlobalReAlloc(mem2a,0,GMEM_MOVEABLE);
        ok(mem2b==NULL,"Discarded memory we shouldn't have");
#endif
        ok(!GlobalUnlock(mem2a) && GetLastError()==NO_ERROR,"GlobalUnlock Failed.");
      }
    }
    if(mem1) {
      ok(GlobalFree(mem1)==NULL,"GlobalFree failed");
    }
    if(mem2a) {
      ok(GlobalFree(mem2a)==NULL,"GlobalFree failed");
    } else {
      ok(GlobalFree(mem2)==NULL,"GlobalFree failed");
    }
}


/* The following functions don't have tests, because either I don't know how
   to test them, or they are WinNT only, or require multiple threads.
   Since the last two issues shouldn't really stop the tests from being
   written, assume for now that it is all due to the first case
       LocalDiscard
       LocalFlags
*/
/* In addition, these features aren't being tested
       LMEM_DISCADABLE
       LMEM_NOCOMPACT
*/
static void test_Local(void)
{
    ULONG memchunk;
    HLOCAL mem1,mem2,mem2a,mem2b;
    UCHAR *mem2ptr;
    UINT error,i;
    memchunk=100000;

/* Check that a normal alloc works */
    mem1=LocalAlloc((UINT)NULL,memchunk);
    ok(mem1!=NULL,"LocalAlloc failed");
    if(mem1) {
      ok(LocalSize(mem1)>=memchunk, "LocalAlloc should return a big enough memory block");
    }

/* Check that a 'zeroing' alloc works */
    mem2=LocalAlloc(LMEM_ZEROINIT,memchunk);
    ok(mem2!=NULL,"LocalAlloc failed");
    if(mem2) {
      ok(LocalSize(mem2)>=memchunk,"LocalAlloc should return a big enough memory block");
      mem2ptr=(UCHAR *)LocalLock(mem2);
      ok(mem2ptr==(UCHAR *)mem2,"LocalLock Didn't lock it's memory");
      if(mem2ptr) {
        error=0;
        for(i=0;i<memchunk;i++) {
          if(mem2ptr[i]!=0) {
            error=1;
          }
        }
        ok(!error,"LocalAlloc should have zeroed out it's allocated memory");
        ok(!(error=LocalUnlock(mem2)) &&
                (GetLastError()==ERROR_NOT_LOCKED || GetLastError()==NO_ERROR),
           "LocalUnlock Failed.");
      }
    }

/* Reallocate mem2 as moveable memory */
   mem2a=LocalFree(mem2);
   mem2=LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT,memchunk);
   ok(mem2a==NULL && mem2!=NULL, "LocalAlloc failed to create moveable memory");

/* Check that ReAllocing memory works as expected */
    mem2a=LocalReAlloc(mem2,2*memchunk,LMEM_MOVEABLE | LMEM_ZEROINIT);
    ok(mem2a!=NULL,"LocalReAlloc failed");
    if(mem2a) {
      ok(LocalSize(mem2a)>=2*memchunk,"LocalReAlloc failed");
      mem2ptr=(UCHAR *)LocalLock(mem2a);
      ok(mem2ptr!=NULL,"LocalLock Failed.");
      if(mem2ptr) {
        error=0;
        for(i=0;i<memchunk;i++) {
          if(mem2ptr[memchunk+i]!=0) {
            error=1;
          }
        }
        ok(!error,"LocalReAlloc should have zeroed out it's allocated memory");
/* Check that LocalHandle works */
        mem2b=LocalHandle(mem2ptr);
        ok(mem2b==mem2a,"LocalHandle didn't return the correct memory handle");
/* Check that we can't discard locked memory */
#if DISCARD_DEFINED
        /* Wine doesn't include the LocalDiscard function */
        mem2b=LocalDiscard(mem2a);
        ok(mem2b==NULL,"Discarded memory we shouldn't have");
#else
        /* This is functionally equivalent to the above */
        mem2b=LocalReAlloc(mem2a,0,GMEM_MOVEABLE);
        ok(mem2b==NULL,"Discarded memory we shouldn't have");
#endif
        SetLastError(NO_ERROR);
        ok(!LocalUnlock(mem2a) && GetLastError()==NO_ERROR, "LocalUnlock Failed.");
      }
    }
    if(mem1) {
      ok(LocalFree(mem1)==NULL,"LocalFree failed");
    }
    if(mem2a) {
      ok(LocalFree(mem2a)==NULL,"LocalFree failed");
    } else {
      ok(LocalFree(mem2)==NULL,"LocalFree failed");
    }
}

/* The Virtual* routines are not tested as thoroughly,
   since I don't really understand how to use them correctly :)
   The following routines are not tested at all
      VirtualAllocEx
      VirtualFreeEx
      VirtualLock
      VirtualProtect
      VirtualProtectEx
      VirtualQuery
      VirtualQueryEx
      VirtualUnlock
    And the only features (flags) being tested are
      MEM_COMMIT
      MEM_RELEASE
      PAGE_READWRITE
    Testing the rest requires using exceptions, which I really don't
    understand well
*/
static void test_Virtual(void)
{
    SYSTEM_INFO sysInfo;
    ULONG memchunk;
    UCHAR *mem1;
    UINT error,i;

/* Retrieve the page size for this system */
    sysInfo.dwPageSize=0;
    GetSystemInfo(&sysInfo);
    ok(sysInfo.dwPageSize>0,"GetSystemInfo should return a valid page size");

/* Choose a reasonable allocation size */
    memchunk=10*sysInfo.dwPageSize;

/* Check that a normal alloc works */
    mem1=VirtualAlloc(NULL,memchunk,MEM_COMMIT,PAGE_READWRITE);
    ok(mem1!=NULL,"VirtualAlloc failed");
    if(mem1) {
/* check that memory is initialized to 0 */
      error=0;
      for(i=0;i<memchunk;i++) {
        if(mem1[i]!=0) {
          error=1;
        }
      }
      ok(!error,"VirtualAlloc did not initialize memory to '0's");
/* Check that we can read/write to memory */
      error=0;
      for(i=0;i<memchunk;i+=100) {
        mem1[i]='a';
        if(mem1[i]!='a') {
          error=1;
        }
      }
      ok(!error,"Virtual memory was not writable");
    }
    ok(VirtualFree(mem1,0,MEM_RELEASE),"VirtualFree failed");
}
START_TEST(alloc)
{
    test_Heap();
    test_Global();
    test_Local();
    test_Virtual();
}
