/* Unit test suite for SHLWAPI Compact List and IStream ordinal functions
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
 */

#include <stdarg.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "objbase.h"

typedef struct tagSHLWAPI_CLIST
{
  ULONG ulSize;
  ULONG ulId;
} SHLWAPI_CLIST, *LPSHLWAPI_CLIST;

typedef const SHLWAPI_CLIST* LPCSHLWAPI_CLIST;

/* Items to add */
static const SHLWAPI_CLIST SHLWAPI_CLIST_items[] =
{
  {4, 1},
  {8, 3},
  {12, 2},
  {16, 8},
  {20, 9},
  {3, 11},
  {9, 82},
  {33, 16},
  {32, 55},
  {24, 100},
  {39, 116},
  { 0, 0}
};

/* Dummy IStream object for testing calls */
typedef struct
{
  void* lpVtbl;
  ULONG ref;
  int   readcalls;
  BOOL  failreadcall;
  BOOL  failreadsize;
  BOOL  readbeyondend;
  BOOL  readreturnlarge;
  int   writecalls;
  BOOL  failwritecall;
  BOOL  failwritesize;
  int   seekcalls;
  int   statcalls;
  BOOL  failstatcall;
  LPCSHLWAPI_CLIST item;
  ULARGE_INTEGER   pos;
} _IDummyStream;

static
HRESULT WINAPI QueryInterface(_IDummyStream *This,REFIID riid, LPVOID *ppvObj)
{
  return S_OK;
}

static ULONG WINAPI AddRef(_IDummyStream *This)
{
  return ++This->ref;
}

static ULONG WINAPI Release(_IDummyStream *This)
{
  return --This->ref;
}

static HRESULT WINAPI Read(_IDummyStream* This, LPVOID lpMem, ULONG ulSize,
                           PULONG lpRead)
{
  HRESULT hRet = S_OK;
  ++This->readcalls;

  if (This->failreadcall)
  {
    return STG_E_ACCESSDENIED;
  }
  else if (This->failreadsize)
  {
    *lpRead = ulSize + 8;
    return S_OK;
  }
  else if (This->readreturnlarge)
  {
    *((ULONG*)lpMem) = 0xffff01;
    *lpRead = ulSize;
    This->readreturnlarge = FALSE;
    return S_OK;
  }
  if (ulSize == sizeof(ULONG))
  {
    /* Read size of item */
    *((ULONG*)lpMem) = This->item->ulSize ? This->item->ulSize + sizeof(SHLWAPI_CLIST) : 0;
    *lpRead = ulSize;
  }
  else
  {
    unsigned int i;
    char* buff = (char*)lpMem;

    /* Read item data */
    if (!This->item->ulSize)
    {
      This->readbeyondend = TRUE;
      *lpRead = 0;
      return E_FAIL; /* Should never happen */
    }
    *((ULONG*)lpMem) = This->item->ulId;
    *lpRead = ulSize;

    for (i = 0; i < This->item->ulSize; i++)
      buff[4+i] = i*2;

    This->item++;
  }
  return hRet;
}

static HRESULT WINAPI Write(_IDummyStream* This, LPVOID lpMem, ULONG ulSize,
                            PULONG lpWritten)
{
  HRESULT hRet = S_OK;

  ++This->writecalls;
  if (This->failwritecall)
  {
    return STG_E_ACCESSDENIED;
  }
  else if (This->failwritesize)
  {
    *lpWritten = 0;
  }
  else
    *lpWritten = ulSize;
  return hRet;
}

static HRESULT WINAPI Seek(_IDummyStream* This, LARGE_INTEGER dlibMove,
                           DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition)
{
  ++This->seekcalls;
  This->pos.QuadPart = dlibMove.QuadPart;
  if (plibNewPosition)
    plibNewPosition->QuadPart = dlibMove.QuadPart;
  return S_OK;
}

static HRESULT WINAPI Stat(_IDummyStream* This, STATSTG* pstatstg,
                           DWORD grfStatFlag)
{
  ++This->statcalls;
  if (This->failstatcall)
    return E_FAIL;
  if (pstatstg)
    pstatstg->cbSize.QuadPart = This->pos.QuadPart;
  return S_OK;
}

/* VTable */
static void* iclvt[] =
{
  QueryInterface,
  AddRef,
  Release,
  Read,
  Write,
  Seek,
  NULL, /* SetSize */
  NULL, /* CopyTo */
  NULL, /* Commit */
  NULL, /* Revert */
  NULL, /* LockRegion */
  NULL, /* UnlockRegion */
  Stat,
  NULL  /* Clone */
};

/* Function ptrs for ordinal calls */
static HMODULE SHLWAPI_hshlwapi = 0;

static VOID    (WINAPI *pSHLWAPI_19)(LPSHLWAPI_CLIST);
static HRESULT (WINAPI *pSHLWAPI_20)(LPSHLWAPI_CLIST*,LPCSHLWAPI_CLIST);
static BOOL    (WINAPI *pSHLWAPI_21)(LPSHLWAPI_CLIST*,ULONG);
static LPSHLWAPI_CLIST (WINAPI *pSHLWAPI_22)(LPSHLWAPI_CLIST,ULONG);
static HRESULT (WINAPI *pSHLWAPI_17)(_IDummyStream*,LPSHLWAPI_CLIST);
static HRESULT (WINAPI *pSHLWAPI_18)(_IDummyStream*,LPSHLWAPI_CLIST*);

static BOOL    (WINAPI *pSHLWAPI_166)(_IDummyStream*);
static HRESULT (WINAPI *pSHLWAPI_184)(_IDummyStream*,LPVOID,ULONG);
static HRESULT (WINAPI *pSHLWAPI_212)(_IDummyStream*,LPCVOID,ULONG);
static HRESULT (WINAPI *pSHLWAPI_213)(_IDummyStream*);
static HRESULT (WINAPI *pSHLWAPI_214)(_IDummyStream*,ULARGE_INTEGER*);


static void InitFunctionPtrs()
{
  SHLWAPI_hshlwapi = LoadLibraryA("shlwapi.dll");
  ok(SHLWAPI_hshlwapi != 0, "LoadLibrary failed");
  if (SHLWAPI_hshlwapi)
  {
    pSHLWAPI_17 = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)17);
    ok(pSHLWAPI_17 != 0, "No Ordinal 17");
    pSHLWAPI_18 = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)18);
    ok(pSHLWAPI_18 != 0, "No Ordinal 18");
    pSHLWAPI_19 = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)19);
    ok(pSHLWAPI_19 != 0, "No Ordinal 19");
    pSHLWAPI_20 = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)20);
    ok(pSHLWAPI_20 != 0, "No Ordinal 20");
    pSHLWAPI_21 = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)21);
    ok(pSHLWAPI_21 != 0, "No Ordinal 21");
    pSHLWAPI_22 = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)22);
    ok(pSHLWAPI_22 != 0, "No Ordinal 22");
    pSHLWAPI_166 = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)166);
    ok(pSHLWAPI_166 != 0, "No Ordinal 166");
    pSHLWAPI_184 = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)184);
    ok(pSHLWAPI_184 != 0, "No Ordinal 184");
    pSHLWAPI_212 = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)212);
    ok(pSHLWAPI_212 != 0, "No Ordinal 212");
    pSHLWAPI_213 = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)213);
    ok(pSHLWAPI_213 != 0, "No Ordinal 213");
    pSHLWAPI_214 = (void *)GetProcAddress( SHLWAPI_hshlwapi, (LPSTR)214);
    ok(pSHLWAPI_214 != 0, "No Ordinal 214");
  }
}

static void InitDummyStream(_IDummyStream* iface)
{
  iface->lpVtbl = (void*)iclvt;
  iface->ref = 1;
  iface->readcalls = 0;
  iface->failreadcall = FALSE;
  iface->failreadsize = FALSE;
  iface->readbeyondend = FALSE;
  iface->readreturnlarge = FALSE;
  iface->writecalls = 0;
  iface->failwritecall = FALSE;
  iface->failwritesize = FALSE;
  iface->seekcalls = 0;
  iface->statcalls = 0;
  iface->failstatcall = FALSE;
  iface->item = SHLWAPI_CLIST_items;
  iface->pos.QuadPart = 0;
}


static void test_CList(void)
{
  _IDummyStream streamobj;
  LPSHLWAPI_CLIST list = NULL;
  LPCSHLWAPI_CLIST item = SHLWAPI_CLIST_items;
  HRESULT hRet;
  LPSHLWAPI_CLIST inserted;
  BYTE buff[64];
  unsigned int i;

  if (!pSHLWAPI_17 || !pSHLWAPI_18 || !pSHLWAPI_19 || !pSHLWAPI_20 ||
      !pSHLWAPI_21 || !pSHLWAPI_22)
    return;

  /* Populate a list and test the items are added correctly */
  while (item->ulSize)
  {
    /* Create item and fill with data */
    inserted = (LPSHLWAPI_CLIST)buff;
    inserted->ulSize = item->ulSize + sizeof(SHLWAPI_CLIST);
    inserted->ulId = item->ulId;
    for (i = 0; i < item->ulSize; i++)
      buff[sizeof(SHLWAPI_CLIST)+i] = i*2;

    /* Add it */
    hRet = pSHLWAPI_20(&list, inserted);
    ok(hRet > S_OK, "failed list add");

    if (hRet > S_OK)
    {
      ok(list && list->ulSize, "item not added");

      /* Find it */
      inserted = pSHLWAPI_22(list, item->ulId);
      ok(inserted != NULL, "lost after adding");

      ok(!inserted || inserted->ulId != -1, "find returned a container");

      /* Check size */
      if (inserted && inserted->ulSize & 0x3)
      {
        /* Contained */
        ok(inserted[-1].ulId == -1, "invalid size is not countained");
        ok(inserted[-1].ulSize > inserted->ulSize+sizeof(SHLWAPI_CLIST),
           "container too small");
      }
      else if (inserted)
      {
        ok(inserted->ulSize==item->ulSize+sizeof(SHLWAPI_CLIST),
           "id %ld size wrong (%ld!=%ld)", inserted->ulId, inserted->ulSize,
           item->ulSize+sizeof(SHLWAPI_CLIST));
      }
      if (inserted)
      {
        BOOL bDataOK = TRUE;
        LPBYTE bufftest = (LPBYTE)inserted;

        for (i = 0; i < inserted->ulSize - sizeof(SHLWAPI_CLIST); i++)
          if (bufftest[sizeof(SHLWAPI_CLIST)+i] != i*2)
            bDataOK = FALSE;

        ok(bDataOK == TRUE, "data corrupted on insert");
      }
      ok(!inserted || inserted->ulId==item->ulId, "find got wrong item");
    }
    item++;
  }

  /* Write the list */
  InitDummyStream(&streamobj);

  hRet = pSHLWAPI_17(&streamobj, list);
  ok(hRet == S_OK, "write failed");
  if (hRet == S_OK)
  {
    /* 1 call for each element, + 1 for OK (use our null element for this) */
    ok(streamobj.writecalls == sizeof(SHLWAPI_CLIST_items)/sizeof(SHLWAPI_CLIST),
       "wrong call count");
    ok(streamobj.readcalls == 0,"called Read() in write");
    ok(streamobj.seekcalls == 0,"called Seek() in write");
  }

  /* Failure cases for writing */
  InitDummyStream(&streamobj);
  streamobj.failwritecall = TRUE;
  hRet = pSHLWAPI_17(&streamobj, list);
  ok(hRet == STG_E_ACCESSDENIED, "changed object failure return");
  ok(streamobj.writecalls == 1, "called object after failure");
  ok(streamobj.readcalls == 0,"called Read() after failure");
  ok(streamobj.seekcalls == 0,"called Seek() after failure");

  InitDummyStream(&streamobj);
  streamobj.failwritesize = TRUE;
  hRet = pSHLWAPI_17(&streamobj, list);
  ok(hRet == STG_E_MEDIUMFULL, "changed size failure return");
  ok(streamobj.writecalls == 1, "called object after size failure");
  ok(streamobj.readcalls == 0,"called Read() after failure");
  ok(streamobj.seekcalls == 0,"called Seek() after failure");

  /* Invalid inputs for adding */
  inserted = (LPSHLWAPI_CLIST)buff;
  inserted->ulSize = sizeof(SHLWAPI_CLIST) -1;
  inserted->ulId = 33;
  hRet = pSHLWAPI_20(&list, inserted);
  /* The call succeeds but the item is not inserted */
  ok(hRet == S_OK, "failed bad element size");
  inserted = pSHLWAPI_22(list, 33);
  ok(inserted == NULL, "inserted bad element size");

  inserted = (LPSHLWAPI_CLIST)buff;
  inserted->ulSize = 44;
  inserted->ulId = -1;
  hRet = pSHLWAPI_20(&list, inserted);
  /* The call succeeds but the item is not inserted */
  ok(hRet == S_OK, "failed adding a container");

  item = SHLWAPI_CLIST_items;

  /* Look for non-existing item in populated list */
  inserted = pSHLWAPI_22(list, 99999999);
  ok(inserted == NULL, "found a non-existing item");

  while (item->ulSize)
  {
    /* Delete items */
    BOOL bRet = pSHLWAPI_21(&list, item->ulId);
    ok(bRet == TRUE, "couldn't find item to delete");
    item++;
  }

  /* Look for non-existing item in empty list */
  inserted = pSHLWAPI_22(list, 99999999);
  ok(inserted == NULL, "found an item in empty list");

  /* Create a list by reading in data */
  InitDummyStream(&streamobj);

  hRet = pSHLWAPI_18(&streamobj, &list);
  ok(hRet == S_OK, "failed create from Read()");
  if (hRet == S_OK)
  {
    ok(streamobj.readbeyondend == FALSE, "read beyond end");
    /* 2 calls per item, but only 1 for the terminator */
    ok(streamobj.readcalls == sizeof(SHLWAPI_CLIST_items)/sizeof(SHLWAPI_CLIST)*2-1,
       "wrong call count");
    ok(streamobj.writecalls == 0, "called Write() from create");
    ok(streamobj.seekcalls == 0,"called Seek() from create");

    item = SHLWAPI_CLIST_items;

    /* Check the items were added correctly */
    while (item->ulSize)
    {
      inserted = pSHLWAPI_22(list, item->ulId);
      ok(inserted != NULL, "lost after adding");

      ok(!inserted || inserted->ulId != -1, "find returned a container");

      /* Check size */
      if (inserted && inserted->ulSize & 0x3)
      {
        /* Contained */
        ok(inserted[-1].ulId == -1, "invalid size is not countained");
        ok(inserted[-1].ulSize > inserted->ulSize+sizeof(SHLWAPI_CLIST),
           "container too small");
      }
      else if (inserted)
      {
        ok(inserted->ulSize==item->ulSize+sizeof(SHLWAPI_CLIST),
           "id %ld size wrong (%ld!=%ld)", inserted->ulId, inserted->ulSize,
           item->ulSize+sizeof(SHLWAPI_CLIST));
      }
      ok(!inserted || inserted->ulId==item->ulId, "find got wrong item");
      if (inserted)
      {
        BOOL bDataOK = TRUE;
        LPBYTE bufftest = (LPBYTE)inserted;

        for (i = 0; i < inserted->ulSize - sizeof(SHLWAPI_CLIST); i++)
          if (bufftest[sizeof(SHLWAPI_CLIST)+i] != i*2)
            bDataOK = FALSE;

        ok(bDataOK == TRUE, "data corrupted on insert");
      }
      item++;
    }
  }

  /* Failure cases for reading */
  InitDummyStream(&streamobj);
  streamobj.failreadcall = TRUE;
  hRet = pSHLWAPI_18(&streamobj, &list);
  ok(hRet == STG_E_ACCESSDENIED, "changed object failure return");
  ok(streamobj.readbeyondend == FALSE, "read beyond end");
  ok(streamobj.readcalls == 1, "called object after read failure");
  ok(streamobj.writecalls == 0,"called Write() after read failure");
  ok(streamobj.seekcalls == 0,"called Seek() after read failure");

  /* Read returns large object */
  InitDummyStream(&streamobj);
  streamobj.readreturnlarge = TRUE;
  hRet = pSHLWAPI_18(&streamobj, &list);
  ok(hRet == S_OK, "failed create from Read() with large item");
  ok(streamobj.readbeyondend == FALSE, "read beyond end");
  ok(streamobj.readcalls == 1,"wrong call count");
  ok(streamobj.writecalls == 0,"called Write() after read failure");
  ok(streamobj.seekcalls == 2,"wrong Seek() call count (%d)", streamobj.seekcalls);

  pSHLWAPI_19(list);
}

static void test_SHLWAPI_166(void)
{
  _IDummyStream streamobj;
  BOOL bRet;

  if (!pSHLWAPI_166)
    return;

  InitDummyStream(&streamobj);
  bRet = pSHLWAPI_166(&streamobj);

  ok(bRet == TRUE, "failed before seek adjusted");
  ok(streamobj.readcalls == 0, "called Read()");
  ok(streamobj.writecalls == 0, "called Write()");
  ok(streamobj.seekcalls == 0, "called Seek()");
  ok(streamobj.statcalls == 1, "wrong call count");

  streamobj.statcalls = 0;
  streamobj.pos.QuadPart = 50001;

  bRet = pSHLWAPI_166(&streamobj);

  ok(bRet == FALSE, "failed after seek adjusted");
  ok(streamobj.readcalls == 0, "called Read()");
  ok(streamobj.writecalls == 0, "called Write()");
  ok(streamobj.seekcalls == 0, "called Seek()");
  ok(streamobj.statcalls == 1, "wrong call count");

  /* Failure cases */
  InitDummyStream(&streamobj);
  streamobj.pos.QuadPart = 50001;
  streamobj.failstatcall = TRUE; /* 1: Stat() Bad, Read() OK */
  bRet = pSHLWAPI_166(&streamobj);
  ok(bRet == FALSE, "should be FALSE after read is OK");
  ok(streamobj.readcalls == 1, "wrong call count");
  ok(streamobj.writecalls == 0, "called Write()");
  ok(streamobj.seekcalls == 1, "wrong call count");
  ok(streamobj.statcalls == 1, "wrong call count");
  ok(streamobj.pos.QuadPart == 0, "Didn't seek to start");

  InitDummyStream(&streamobj);
  streamobj.pos.QuadPart = 50001;
  streamobj.failstatcall = TRUE;
  streamobj.failreadcall = TRUE; /* 2: Stat() Bad, Read() Bad Also */
  bRet = pSHLWAPI_166(&streamobj);
  ok(bRet == TRUE, "Should be true after read fails");
  ok(streamobj.readcalls == 1, "wrong call count");
  ok(streamobj.writecalls == 0, "called Write()");
  ok(streamobj.seekcalls == 0, "Called Seek()");
  ok(streamobj.statcalls == 1, "wrong call count");
  ok(streamobj.pos.QuadPart == 50001, "called Seek() after read failed");
}

static void test_SHLWAPI_184(void)
{
  _IDummyStream streamobj;
  char buff[256];
  HRESULT hRet;

  if (!pSHLWAPI_184)
    return;

  InitDummyStream(&streamobj);
  hRet = pSHLWAPI_184(&streamobj, buff, sizeof(buff));

  ok(hRet == S_OK, "failed Read()");
  ok(streamobj.readcalls == 1, "wrong call count");
  ok(streamobj.writecalls == 0, "called Write()");
  ok(streamobj.seekcalls == 0, "called Seek()");
}

static void test_SHLWAPI_212(void)
{
  _IDummyStream streamobj;
  char buff[256];
  HRESULT hRet;

  if (!pSHLWAPI_212)
    return;

  InitDummyStream(&streamobj);
  hRet = pSHLWAPI_212(&streamobj, buff, sizeof(buff));

  ok(hRet == S_OK, "failed Write()");
  ok(streamobj.readcalls == 0, "called Read()");
  ok(streamobj.writecalls == 1, "wrong call count");
  ok(streamobj.seekcalls == 0, "called Seek()");
}

static void test_SHLWAPI_213(void)
{
  _IDummyStream streamobj;
  ULARGE_INTEGER ul;
  LARGE_INTEGER ll;
  HRESULT hRet;

  if (!pSHLWAPI_213 || !pSHLWAPI_214)
    return;

  InitDummyStream(&streamobj);
  ll.QuadPart = 5000l;
  Seek(&streamobj, ll, 0, NULL); /* Seek to 5000l */

  streamobj.seekcalls = 0;
  pSHLWAPI_213(&streamobj); /* Should rewind */
  ok(streamobj.statcalls == 0, "called Stat()");
  ok(streamobj.readcalls == 0, "called Read()");
  ok(streamobj.writecalls == 0, "called Write()");
  ok(streamobj.seekcalls == 1, "wrong call count");

  ul.QuadPart = 50001;
  hRet = pSHLWAPI_214(&streamobj, &ul);
  ok(hRet == S_OK, "failed Stat()");
  ok(ul.QuadPart == 0, "213 didn't rewind stream");
}

static void test_SHLWAPI_214(void)
{
  _IDummyStream streamobj;
  ULARGE_INTEGER ul;
  LARGE_INTEGER ll;
  HRESULT hRet;

  if (!pSHLWAPI_214)
    return;

  InitDummyStream(&streamobj);
  ll.QuadPart = 5000l;
  Seek(&streamobj, ll, 0, NULL);
  ul.QuadPart = 0;
  streamobj.seekcalls = 0;
  hRet = pSHLWAPI_214(&streamobj, &ul);

  ok(hRet == S_OK, "failed Stat()");
  ok(streamobj.statcalls == 1, "wrong call count");
  ok(streamobj.readcalls == 0, "called Read()");
  ok(streamobj.writecalls == 0, "called Write()");
  ok(streamobj.seekcalls == 0, "called Seek()");
  ok(ul.QuadPart == 5000l, "Stat gave wrong size");
}

START_TEST(clist)
{
  InitFunctionPtrs();

  test_CList();

  test_SHLWAPI_166();
  test_SHLWAPI_184();
  test_SHLWAPI_212();
  test_SHLWAPI_213();
  test_SHLWAPI_214();

  if (SHLWAPI_hshlwapi)
    FreeLibrary(SHLWAPI_hshlwapi);
}
