/******************************************************************************
 *
 * BigBlockFile
 *
 * This is the implementation of a file that consists of blocks of 
 * a predetermined size.
 * This class is used in the Compound File implementation of the 
 * IStorage and IStream interfaces. It provides the functionality 
 * to read and write any blocks in the file as well as setting and 
 * obtaining the size of the file.
 * The blocks are indexed sequentially from the start of the file
 * starting with -1.
 * 
 * TODO:
 * - Support for a transacted mode
 *
 * Copyright 1999 Thuy Nguyen
 *
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "winbase.h"
#include "winerror.h"
#include "wine/obj_storage.h"
#include "ole2.h"

#include "storage32.h"

#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(storage)

/***********************************************************
 * Data structures used internally by the BigBlockFile
 * class.
 */

/***
 * Itdentifies a single big block and the related 
 * information
 */
struct BigBlock
{
  BigBlock * next;
  DWORD index;
  DWORD access_mode;
  LPVOID lpBlock;
};

/***
 * This structure identifies the paged that are mapped
 * from the file and their position in memory. It is 
 * also used to hold a reference count to those pages.
 */
struct MappedPage
{
  MappedPage * next;
  DWORD number;
  int ref;
  LPVOID lpBytes;
};

#define PAGE_SIZE       131072
#define BLOCKS_PER_PAGE 256

#define NUMBER_OF_MAPPED_PAGES 100

/***********************************************************
 * Prototypes for private methods
 */
static void*     BIGBLOCKFILE_GetMappedView(LPBIGBLOCKFILE This,
                                            DWORD          pagenum,
                                            DWORD          desired_access);
static void      BIGBLOCKFILE_ReleaseMappedPage(LPBIGBLOCKFILE This,
                                                DWORD          pagenum,
                                                DWORD          access);
static void      BIGBLOCKFILE_FreeAllMappedPages(LPBIGBLOCKFILE This);
static void*     BIGBLOCKFILE_GetBigBlockPointer(LPBIGBLOCKFILE This,
                                                 ULONG          index,
                                                 DWORD          desired_access);
static BigBlock* BIGBLOCKFILE_GetBigBlockFromPointer(LPBIGBLOCKFILE This, 
                                                     void*          pBlock);
static void      BIGBLOCKFILE_RemoveBlock(LPBIGBLOCKFILE This,
                                          ULONG          index);
static BigBlock* BIGBLOCKFILE_AddBigBlock(LPBIGBLOCKFILE This, 
                                          ULONG          index);
static BigBlock* BIGBLOCKFILE_CreateBlock(ULONG index);
static DWORD     BIGBLOCKFILE_GetProtectMode(DWORD openFlags);
static BOOL      BIGBLOCKFILE_FileInit(LPBIGBLOCKFILE This, HANDLE hFile);
static BOOL      BIGBLOCKFILE_MemInit(LPBIGBLOCKFILE This, ILockBytes* plkbyt);
static void      BIGBLOCKFILE_RemoveAllBlocks(LPBIGBLOCKFILE This);

/******************************************************************************
 *      BIGBLOCKFILE_Construct
 *
 * Construct a big block file. Create the file mapping object. 
 * Create the read only mapped pages list, the writeable mapped page list
 * and the blocks in use list.
 */
BigBlockFile * BIGBLOCKFILE_Construct(
  HANDLE   hFile,
  ILockBytes* pLkByt,
  DWORD    openFlags,
  ULONG    blocksize,
  BOOL     fileBased)
{
  LPBIGBLOCKFILE This;

  This = (LPBIGBLOCKFILE)HeapAlloc(GetProcessHeap(), 0, sizeof(BigBlockFile));

  if (This == NULL)
    return NULL;

  This->fileBased = fileBased;

  This->flProtect = BIGBLOCKFILE_GetProtectMode(openFlags);

  This->blocksize = blocksize;

  /* initialize the block list
   */
  This->headblock = NULL;

  if (This->fileBased)
  {
    if (!BIGBLOCKFILE_FileInit(This, hFile))
    {
      HeapFree(GetProcessHeap(), 0, This);
      return NULL;
    }
  }
  else
  {
    if (!BIGBLOCKFILE_MemInit(This, pLkByt))
    {
      HeapFree(GetProcessHeap(), 0, This);
      return NULL;
    }
  }

  return This;
}

/******************************************************************************
 *      BIGBLOCKFILE_FileInit
 *
 * Initialize a big block object supported by a file.
 */
static BOOL BIGBLOCKFILE_FileInit(LPBIGBLOCKFILE This, HANDLE hFile)
{
  This->pLkbyt     = NULL;
  This->hbytearray = 0;
  This->pbytearray = NULL;

  This->hfile = hFile;

  if (This->hfile == INVALID_HANDLE_VALUE)
    return FALSE;

  /* create the file mapping object
   */
  This->hfilemap = CreateFileMappingA(This->hfile,
                                      NULL,
                                      This->flProtect,
                                      0, 0,
                                      NULL);

  if (This->hfilemap == NULL)
  {
    CloseHandle(This->hfile);
    return FALSE;
  }

  This->filesize.s.LowPart = GetFileSize(This->hfile, NULL);

  /* create the mapped pages list
   */
  This->maplisthead = HeapAlloc(GetProcessHeap(), 0, sizeof(MappedPage));

  if (This->maplisthead == NULL)
  {
    CloseHandle(This->hfilemap);
    CloseHandle(This->hfile);
    return FALSE;
  }

  This->maplisthead->next = NULL;

  return TRUE;
}

/******************************************************************************
 *      BIGBLOCKFILE_MemInit
 *
 * Initialize a big block object supported by an ILockBytes on HGLOABL.
 */
static BOOL BIGBLOCKFILE_MemInit(LPBIGBLOCKFILE This, ILockBytes* plkbyt)
{
  This->hfile       = 0;
  This->hfilemap    = NULL;
  This->maplisthead = NULL;

  /*
   * Retrieve the handle to the byte array from the LockByte object.
   */
  if (GetHGlobalFromILockBytes(plkbyt, &(This->hbytearray)) != S_OK)
  {
    FIXME("May not be an ILockBytes on HGLOBAL\n");
    return FALSE;
  }

  This->pLkbyt = plkbyt;

  /*
   * Increment the reference count of the ILockByte object since
   * we're keeping a reference to it.
   */
  ILockBytes_AddRef(This->pLkbyt);

  This->filesize.s.LowPart = GlobalSize(This->hbytearray);
  This->filesize.s.HighPart = 0;

  This->pbytearray = GlobalLock(This->hbytearray);

  return TRUE;
}

/******************************************************************************
 *      BIGBLOCKFILE_Destructor
 *
 * Destructor. Clean up, free memory.
 */
void BIGBLOCKFILE_Destructor(
  LPBIGBLOCKFILE This)
{
  if (This->fileBased)
  {
    /* unmap all views and destroy the mapped page list
     */
    BIGBLOCKFILE_FreeAllMappedPages(This);
    HeapFree(GetProcessHeap(), 0, This->maplisthead);
  
    /* close all open handles
     */
    CloseHandle(This->hfilemap);
    CloseHandle(This->hfile);
  }
  else
  {
    GlobalUnlock(This->hbytearray);
    ILockBytes_Release(This->pLkbyt);
  }

  /*
   * Destroy the blocks list.
   */
  BIGBLOCKFILE_RemoveAllBlocks(This);

  /* destroy this
   */
  HeapFree(GetProcessHeap(), 0, This);
}

/******************************************************************************
 *      BIGBLOCKFILE_GetROBigBlock
 *
 * Returns the specified block in read only mode.
 * Will return NULL if the block doesn't exists.
 */
void* BIGBLOCKFILE_GetROBigBlock(
  LPBIGBLOCKFILE This,
  ULONG          index)
{
  /*
   * block index starts at -1
   * translate to zero based index
   */
  if (index == 0xffffffff)
    index = 0;
  else
    index++;

  /*
   * validate the block index
   * 
   */
  if ((This->blocksize * (index + 1)) >
      (This->filesize.s.LowPart +
       (This->blocksize - (This->filesize.s.LowPart % This->blocksize))))
    return 0;

  return BIGBLOCKFILE_GetBigBlockPointer(This, index, FILE_MAP_READ);
}

/******************************************************************************
 *      BIGBLOCKFILE_GetBigBlock
 *
 * Returns the specified block.
 * Will grow the file if necessary.
 */
void* BIGBLOCKFILE_GetBigBlock(LPBIGBLOCKFILE This, ULONG index)
{
  /*
   * block index starts at -1
   * translate to zero based index
   */
  if (index == 0xffffffff)
    index = 0;
  else
    index++;

  /*
   * make sure that the block physically exists
   */
  if ((This->blocksize * (index + 1)) > This->filesize.s.LowPart)
  {
    ULARGE_INTEGER newSize;

    newSize.s.HighPart = 0;
    newSize.s.LowPart = This->blocksize * (index + 1);

    BIGBLOCKFILE_SetSize(This, newSize);
  }

  return BIGBLOCKFILE_GetBigBlockPointer(This, index, FILE_MAP_WRITE);
}

/******************************************************************************
 *      BIGBLOCKFILE_ReleaseBigBlock
 *
 * Releases the specified block.
 */
void BIGBLOCKFILE_ReleaseBigBlock(LPBIGBLOCKFILE This, void *pBlock)
{
  DWORD page_num;
  BigBlock* theBigBlock;

  if (pBlock == NULL)
    return;

  /*
   * get the block from the block list
   */
  theBigBlock = BIGBLOCKFILE_GetBigBlockFromPointer(This, pBlock);

  if (theBigBlock == NULL)
    return;

  if (This->fileBased)
  {
    /*
     * find out which page this block is in
     */
    page_num = theBigBlock->index / BLOCKS_PER_PAGE;
  
    /*
     * release this page
     */
    BIGBLOCKFILE_ReleaseMappedPage(This, page_num, theBigBlock->access_mode);
  }

  /*
   * remove block from list
   */
  BIGBLOCKFILE_RemoveBlock(This, theBigBlock->index);
}

/******************************************************************************
 *      BIGBLOCKFILE_SetSize
 *
 * Sets the size of the file.
 * 
 */
void BIGBLOCKFILE_SetSize(LPBIGBLOCKFILE This, ULARGE_INTEGER newSize)
{
  if (This->filesize.s.LowPart == newSize.s.LowPart)
    return;

  if (This->fileBased)
  {
    /*
     * unmap all views, must be done before call to SetEndFile
     */
    BIGBLOCKFILE_FreeAllMappedPages(This);
  
    /*
     * close file-mapping object, must be done before call to SetEndFile
     */
    CloseHandle(This->hfilemap);
    This->hfilemap = NULL;
  
    /*
     * set the new end of file
     */
    SetFilePointer(This->hfile, newSize.s.LowPart, NULL, FILE_BEGIN);
    SetEndOfFile(This->hfile);
  
    /*
     * re-create the file mapping object
     */
    This->hfilemap = CreateFileMappingA(This->hfile,
                                        NULL,
                                        This->flProtect,
                                        0, 0, 
                                        NULL);
  }
  else
  {
    GlobalUnlock(This->hbytearray);

    /*
     * Resize the byte array object.
     */
    ILockBytes_SetSize(This->pLkbyt, newSize);

    /*
     * Re-acquire the handle, it may have changed.
     */
    GetHGlobalFromILockBytes(This->pLkbyt, &This->hbytearray);
    This->pbytearray = GlobalLock(This->hbytearray);
  }

  /*
   * empty the blocks list.
   */
  BIGBLOCKFILE_RemoveAllBlocks(This);

  This->filesize.s.LowPart = newSize.s.LowPart;
  This->filesize.s.HighPart = newSize.s.HighPart;
}

/******************************************************************************
 *      BIGBLOCKFILE_GetSize
 *
 * Returns the size of the file.
 * 
 */
ULARGE_INTEGER BIGBLOCKFILE_GetSize(LPBIGBLOCKFILE This)
{
  return This->filesize;
}

/******************************************************************************
 *      BIGBLOCKFILE_GetBigBlockPointer     [PRIVATE]
 *
 * Returns a pointer to the specified block.
 */
static void* BIGBLOCKFILE_GetBigBlockPointer(
  LPBIGBLOCKFILE This, 
  ULONG          index, 
  DWORD          desired_access)
{
  DWORD block_num;
  void * pBytes;
  BigBlock *aBigBlock;

  /* get the big block from the list or add it to the list
   */
  aBigBlock = BIGBLOCKFILE_AddBigBlock(This, index);

  if (aBigBlock == NULL)
    return NULL;

  /* we already have an address for this block
   */
  if (aBigBlock->lpBlock != NULL)
  {
    /* make sure the desired access matches what we already have
     */
    if (aBigBlock->access_mode == desired_access)
      return aBigBlock->lpBlock;
    else
      return NULL;
  }

  /*
   * else aBigBlock->lpBigBlock == NULL, it's a new block
   */

  if (This->fileBased)
  {
    DWORD page_num;

    /* find out which page this block is in
     */
    page_num = index / BLOCKS_PER_PAGE;
  
    /* offset of the block in the page
     */
    block_num = index % BLOCKS_PER_PAGE;
  
    /* get a pointer to the first byte in the page
     */
    pBytes = BIGBLOCKFILE_GetMappedView(This, page_num, desired_access);
  }
  else
  {
    pBytes = This->pbytearray; 
    block_num = index;
  }

  if (pBytes == NULL)
    return NULL;

  /* initialize block
   */
  aBigBlock->lpBlock = ((BYTE*)pBytes + (block_num*This->blocksize));
  aBigBlock->access_mode = desired_access;

  return aBigBlock->lpBlock;
}

/******************************************************************************
 *      BIGBLOCKFILE_CreateBlock      [PRIVATE]
 *
 * Creates a node of the blocks list.
 */
static BigBlock* BIGBLOCKFILE_CreateBlock(
  ULONG index)
{
  BigBlock *newBigBlock;

  /* create new list node
   */
  newBigBlock = HeapAlloc(GetProcessHeap(), 0, sizeof(BigBlock));

  if (newBigBlock == NULL)
    return NULL;

  /* initialize node
   */
  newBigBlock->index = index;
  newBigBlock->lpBlock = NULL;

  return newBigBlock;
}

/******************************************************************************
 *      BIGBLOCKFILE_AddBigBlock      [PRIVATE]
 *
 * Returns the specified block from the blocks list.
 * If the block is not found in the list, we will create it and add it to the
 * list.
 */
static BigBlock* BIGBLOCKFILE_AddBigBlock(
  LPBIGBLOCKFILE This, 
  ULONG          index)
{
  BigBlock *current = This->headblock;
  BigBlock *newBigBlock;

  if (current == NULL) /* empty list */
  {
    newBigBlock = BIGBLOCKFILE_CreateBlock(index);
    
    if (newBigBlock != NULL)
    {
      newBigBlock->next = NULL;
      This->headblock = newBigBlock;
    }

    return newBigBlock;
  }
  else
  {
    /* 
     * special handling for head of the list
     */

    if (current->index == index) /* it's already here */
      return current;
    else if (current->index > index) /* insertion at head of the list */
    {
      newBigBlock = BIGBLOCKFILE_CreateBlock(index);

      if (newBigBlock != NULL)
      {
        newBigBlock->next = current;
        This->headblock = newBigBlock;
      }

      return newBigBlock;
    }
  }

  /* iterate through rest the list
   */
  while (current->next != NULL)
  {
    if (current->next->index == index) /* found it */
    {
      return current->next;
    }
    else if (current->next->index > index) /* it's not in the list */
    {
      newBigBlock = BIGBLOCKFILE_CreateBlock(index);

      if (newBigBlock != NULL)
      {
        newBigBlock->next = current->next;
        current->next = newBigBlock;
      }

      return newBigBlock;
    }
    else
      current = current->next;
  }

  /* 
   * insertion at end of the list
   */
  if (current->next == NULL)
  {
    newBigBlock = BIGBLOCKFILE_CreateBlock(index);

    if (newBigBlock != NULL)
    {
      newBigBlock->next = NULL;
      current->next = newBigBlock;
    }

    return newBigBlock;
  }

  return NULL;
}

/******************************************************************************
 *      BIGBLOCKFILE_RemoveAllBlocks      [PRIVATE]
 *
 * Removes all blocks from the blocks list.
 */
static void BIGBLOCKFILE_RemoveAllBlocks(
  LPBIGBLOCKFILE This)
{
  BigBlock *current;

  while (This->headblock != NULL)
  {
    current = This->headblock;
    This->headblock = current->next;
    HeapFree(GetProcessHeap(), 0, current);
  }
}

/******************************************************************************
 *      BIGBLOCKFILE_RemoveBlock      [PRIVATE]
 *
 * Removes the specified block from the blocks list.
 */
static void BIGBLOCKFILE_RemoveBlock(
  LPBIGBLOCKFILE This, 
  ULONG          index)
{
  BigBlock *current = This->headblock;

  /* 
   * empty list
   */
  if (current == NULL)
    return;

  /* 
   *special case: removing head of list
   */
  if (current->index == index)
  {
    /*
     * set new head free the old one
     */
    This->headblock = current->next;
    HeapFree(GetProcessHeap(), 0, current);
    
    return;
  }

  /* 
   * iterate through rest of the list
   */
  while (current->next != NULL)
  {
    if (current->next->index == index) /* found it */
    {
      /* 
       * unlink the block and free the block
       */
      current->next = current->next->next;
      HeapFree(GetProcessHeap(), 0, current->next);

      return;
    }
    else
    {
      /* next node
       */
      current = current->next;
    }
  }
}

/******************************************************************************
 *      BIGBLOCKFILE_GetBigBlockFromPointer     [PRIVATE]
 *
 * Given a block pointer, this will return the corresponding block
 * from the blocks list.
 */
static BigBlock* BIGBLOCKFILE_GetBigBlockFromPointer(
  LPBIGBLOCKFILE This, 
  void*          pBlock)
{
  BigBlock *current = This->headblock;

  while (current != NULL)
  {
    if (current->lpBlock == pBlock)
    {
      break;
    }
    else
      current = current->next;
  }

  return current;
}

/******************************************************************************
 *      BIGBLOCKFILE_GetMappedView      [PRIVATE]
 *
 * Gets the page requested if it is already mapped.
 * If it's not already mapped, this method will map it
 */
static void * BIGBLOCKFILE_GetMappedView(
  LPBIGBLOCKFILE This,
  DWORD          pagenum,
  DWORD          desired_access)
{
  MappedPage* current = This->maplisthead;
  ULONG       count   = 1;
  BOOL        found   = FALSE;

  assert(This->maplisthead != NULL);

  /*
   * Search for the page in the list.
   */
  while ((found == FALSE) && (current->next != NULL))
  {
    if (current->next->number == pagenum)
    {
      found = TRUE;

      /*
       * If it's not already at the head of the list
       * move it there.
       */
      if (current != This->maplisthead)
      {
        MappedPage* temp = current->next;

        current->next = current->next->next;

        temp->next = This->maplisthead->next;
        This->maplisthead->next = temp;
      }
    }

    /*
     * The list is full and we haven't found it.
     * Free the last element of the list because we'll add a new
     * one at the head.
     */
    if ((found == FALSE) && 
        (count >= NUMBER_OF_MAPPED_PAGES) &&
        (current->next != NULL))
    {
      UnmapViewOfFile(current->next->lpBytes);

      HeapFree(GetProcessHeap(), 0, current->next);
      current->next = NULL;
    }

    if (current->next != NULL)
      current = current->next;

    count++;
  }

  /*
   * Add the page at the head of the list.
   */
  if (found == FALSE)
  {
    MappedPage* newMappedPage;
    DWORD       numBytesToMap;
    DWORD       hioffset  = 0;
    DWORD       lowoffset = PAGE_SIZE * pagenum;

    newMappedPage = HeapAlloc(GetProcessHeap(), 0, sizeof(MappedPage));

    if (newMappedPage == NULL)
      return NULL;

    newMappedPage->number = pagenum;
    newMappedPage->ref = 0;

    newMappedPage->next = This->maplisthead->next;
    This->maplisthead->next = newMappedPage;

    if (((pagenum + 1) * PAGE_SIZE) > This->filesize.s.LowPart)
      numBytesToMap = This->filesize.s.LowPart - (pagenum * PAGE_SIZE);
    else
      numBytesToMap = PAGE_SIZE;

    if (This->flProtect == PAGE_READONLY)
      desired_access = FILE_MAP_READ;
    else
      desired_access = FILE_MAP_WRITE;

    newMappedPage->lpBytes = MapViewOfFile(This->hfilemap,
                                           desired_access,
                                           hioffset,
                                           lowoffset,
                                           numBytesToMap);
  }

  /*
   * The page we want should now be at the head of the list.
   */
  assert(This->maplisthead->next != NULL);

  current = This->maplisthead->next;
  current->ref++;

  return current->lpBytes;
}

/******************************************************************************
 *      BIGBLOCKFILE_ReleaseMappedPage      [PRIVATE]
 *
 * Decrements the reference count of the mapped page.
 */
static void BIGBLOCKFILE_ReleaseMappedPage(
  LPBIGBLOCKFILE This,
  DWORD          pagenum,
  DWORD          access)
{
  MappedPage* previous = This->maplisthead;
  MappedPage* current;

  assert(This->maplisthead->next != NULL);

  current = previous->next;

  /* search for the page in the list
   */
  while (current != NULL)
  {
    if (current->number == pagenum)
    {
      /* decrement the reference count
       */
      current->ref--;
      return;
    }
    else
    {
      previous = current;
      current = current->next;
    }
  }
}

/******************************************************************************
 *      BIGBLOCKFILE_FreeAllMappedPages     [PRIVATE]
 *
 * Unmap all currently mapped pages.
 * Empty mapped pages list.
 */
static void BIGBLOCKFILE_FreeAllMappedPages(
  LPBIGBLOCKFILE This)
{
  MappedPage * current = This->maplisthead->next;

  while (current != NULL)
  {
    /* Unmap views.
     */
    UnmapViewOfFile(current->lpBytes);

    /* Free the nodes.
     */
    This->maplisthead->next = current->next;
    HeapFree(GetProcessHeap(), 0, current);

    current = This->maplisthead->next;
  }
}

/****************************************************************************
 *      BIGBLOCKFILE_GetProtectMode
 *
 * This function will return a protection mode flag for a file-mapping object
 * from the open flags of a file.
 */
static DWORD BIGBLOCKFILE_GetProtectMode(DWORD openFlags)
{
  DWORD flProtect        = PAGE_READONLY;
  BOOL bSTGM_WRITE     = ((openFlags & STGM_WRITE) == STGM_WRITE);
  BOOL bSTGM_READWRITE = ((openFlags & STGM_READWRITE) == STGM_READWRITE);
  BOOL bSTGM_READ      = ! (bSTGM_WRITE || bSTGM_READWRITE);

  if (bSTGM_READ)
    flProtect = PAGE_READONLY;

  if ((bSTGM_WRITE) || (bSTGM_READWRITE))
    flProtect = PAGE_READWRITE;

  return flProtect;
}

 
