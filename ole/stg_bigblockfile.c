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

#include "windows.h"
#include "winerror.h"
#include "ole.h"
#include "ole2.h"
#include "wine/obj_base.h"
#include "wine/obj_storage.h"

#include "storage32.h"

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

#define BLOCKS_PER_PAGE 128
#define PAGE_SIZE 65536

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

static DWORD BIGBLOCKFILE_GetProtectMode(DWORD openFlags);

/******************************************************************************
 *      BIGBLOCKFILE_Construct
 *
 * Construct a big block file. Create the file mapping object. 
 * Create the read only mapped pages list, the writeable mapped page list
 * and the blocks in use list.
 */
BigBlockFile * BIGBLOCKFILE_Construct(
  HANDLE32 hFile, 
    DWORD    openFlags,
  ULONG    blocksize)
{
  LPBIGBLOCKFILE This;

  This = (LPBIGBLOCKFILE)HeapAlloc(GetProcessHeap(), 0, sizeof(BigBlockFile));

  if (This == NULL)
    return NULL;

  This->hfile = hFile;
  
  if (This->hfile == INVALID_HANDLE_VALUE32)
  {
    HeapFree(GetProcessHeap(), 0, This);
    return NULL;
  }

  This->flProtect = BIGBLOCKFILE_GetProtectMode(openFlags);

  /* create the file mapping object
   */
  This->hfilemap = CreateFileMapping32A(This->hfile,
					NULL,
					This->flProtect, 
					0, 0,
					NULL);
  
  if (This->hfilemap == NULL)
  {
    CloseHandle(This->hfile);
    HeapFree(GetProcessHeap(), 0, This);
    return NULL;
  }

  /* initialize this
   */
  This->filesize.LowPart = GetFileSize(This->hfile, NULL);
  This->blocksize = blocksize;
  
  /* create the read only mapped pages list
   */
  This->headmap_ro = HeapAlloc(GetProcessHeap(), 0, sizeof(MappedPage));
  
  if (This->headmap_ro == NULL)
  {
    CloseHandle(This->hfilemap);
    CloseHandle(This->hfile);
    HeapFree(GetProcessHeap(), 0, This);
    return NULL;
  }

  This->headmap_ro->next = NULL;
  
  /* create the writeable mapped pages list
   */
  This->headmap_w = HeapAlloc(GetProcessHeap(), 0, sizeof(MappedPage));
  
  if (This->headmap_w == NULL)
  {
    CloseHandle(This->hfilemap);
    CloseHandle(This->hfile);
    HeapFree(GetProcessHeap(), 0, This->headmap_ro);
    HeapFree(GetProcessHeap(), 0, This);
    return NULL;
  }

  This->headmap_w->next = NULL;

  /* initialize the block list
   */
  This->headblock = NULL;

  return This;
}

/******************************************************************************
 *      BIGBLOCKFILE_Destructor
 *
 * Destructor. Clean up, free memory.
 */
void BIGBLOCKFILE_Destructor(
  LPBIGBLOCKFILE This)
{
  /* unmap all views and destroy the mapped page lists
   */
  BIGBLOCKFILE_FreeAllMappedPages(This);
  HeapFree(GetProcessHeap(), 0, This->headmap_ro);
  HeapFree(GetProcessHeap(), 0, This->headmap_w);

  /* close all open handles
   */
  CloseHandle(This->hfilemap);
  CloseHandle(This->hfile);

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
      (This->filesize.LowPart +
       (This->blocksize - (This->filesize.LowPart % This->blocksize))))
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
  if ((This->blocksize * (index + 1)) > This->filesize.LowPart)
  {
    ULARGE_INTEGER newSize;

    newSize.HighPart = 0;
    newSize.LowPart = This->blocksize * (index + 1);

    BIGBLOCKFILE_SetSize(This, newSize);
  }

  return BIGBLOCKFILE_GetBigBlockPointer(This, index, FILE_MAP_WRITE);
}

/******************************************************************************
 *      BIGBLOCKFILE_ReleaseBigBlock
 *
 * Releases the specified block.
 * 
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

  /*
   * find out which page this block is in
   */
  page_num = theBigBlock->index / BLOCKS_PER_PAGE;

  /*
   * release this page
   */
  BIGBLOCKFILE_ReleaseMappedPage(This, page_num, theBigBlock->access_mode);

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
  if (This->filesize.LowPart == newSize.LowPart)
    return;

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
  SetFilePointer(This->hfile, newSize.LowPart, NULL, FILE_BEGIN);
  SetEndOfFile(This->hfile);

  /*
   * re-create the file mapping object
   */
  This->hfilemap = CreateFileMapping32A(This->hfile,
					NULL,
					This->flProtect,
					0, 0, 
					NULL);

  This->filesize.LowPart = newSize.LowPart;
  This->filesize.HighPart = newSize.HighPart;
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
  DWORD page_num, block_num;
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

  /* find out which page this block is in
   */
  page_num = index / BLOCKS_PER_PAGE;

  /* offset of the block in the page
   */
  block_num = index % BLOCKS_PER_PAGE;

  /* get a pointer to the first byte in the page
   */
  pBytes = BIGBLOCKFILE_GetMappedView(This, page_num, desired_access);

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
  MappedPage * current;
  MappedPage * newMappedPage;
  DWORD hioffset, lowoffset;
  DWORD numBytesToMap;

  /* use correct list
   */
  if (desired_access == FILE_MAP_READ)
    current = This->headmap_ro;
  else if (desired_access == FILE_MAP_WRITE)
    current = This->headmap_w;
  else
    return NULL;

  hioffset = 0;
  lowoffset = PAGE_SIZE * pagenum;

  while (current->next != NULL)
  {
    if (current->next->number == pagenum) /* page already mapped */
    {
      current->next->ref++;
      return current->next->lpBytes;
    }
    else if (current->next->number > pagenum) /* this page is not mapped yet */
    {
      /* allocate new MappedPage
       */
      newMappedPage = HeapAlloc(GetProcessHeap(), 0, sizeof(MappedPage));

      if (newMappedPage == NULL)
        return NULL;

      /* initialize the new MappedPage
       */
      newMappedPage->number = pagenum;
      newMappedPage->ref = 1;

      newMappedPage->next = current->next;
      current->next = newMappedPage;

      /* actually map the page
       */
      if (((pagenum + 1) * PAGE_SIZE) > This->filesize.LowPart)
        numBytesToMap = This->filesize.LowPart - (pagenum * PAGE_SIZE);
      else
        numBytesToMap = PAGE_SIZE;

      newMappedPage->lpBytes = MapViewOfFile(This->hfilemap,
                                             desired_access,
                                             hioffset,
                                             lowoffset,
                                             numBytesToMap);

      return newMappedPage->lpBytes;
    }
    else
      current = current->next;
  }

  /* reached end of the list, this view is not mapped yet
   */
  if (current->next == NULL)
  {
    /* allocate new MappedPage
     */
    newMappedPage = HeapAlloc(GetProcessHeap(), 0, sizeof(MappedPage));

    if (newMappedPage == NULL)
      return NULL;

    /* initialize the new MappedPage
     */
    newMappedPage->number = pagenum;
    newMappedPage->ref = 1;

    newMappedPage->next = NULL;
    current->next = newMappedPage;

    /* actually map the page
     */
    if (((pagenum + 1) * PAGE_SIZE) > This->filesize.LowPart)
      numBytesToMap = This->filesize.LowPart - (pagenum * PAGE_SIZE);
    else
      numBytesToMap = PAGE_SIZE;

    newMappedPage->lpBytes = MapViewOfFile(This->hfilemap,
                                           desired_access,
                                           hioffset,
                                           lowoffset,
                                           numBytesToMap);

    return newMappedPage->lpBytes;
  }

  return NULL;
}

/******************************************************************************
 *      BIGBLOCKFILE_ReleaseMappedPage      [PRIVATE]
 *
 * Decrements the reference count of the mapped page.
 * If the page is not used anymore it will be unmapped.
 */
static void BIGBLOCKFILE_ReleaseMappedPage(
  LPBIGBLOCKFILE This,
  DWORD          pagenum,
  DWORD          access)
{
  MappedPage * previous;
  MappedPage * current;

  /* use the list corresponding to the desired access mode
   */
  if (access == FILE_MAP_READ)
    previous = This->headmap_ro;
  else if (access == FILE_MAP_WRITE)
    previous = This->headmap_w;
  else
    return;

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

      if (current->ref == 0)
      {
        /* this page is not used anymore, we can unmap it
         */
        UnmapViewOfFile(current->lpBytes);
        
        previous->next = current->next;
        HeapFree(GetProcessHeap(), 0, current);
      }

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
 * Empty both mapped pages lists.
 */
static void BIGBLOCKFILE_FreeAllMappedPages(
  LPBIGBLOCKFILE This)
{
  /*
   * start with the read only list
   */
  MappedPage * current = This->headmap_ro->next;

  while (current != NULL)
  {
    /* unmap views
     */
    UnmapViewOfFile(current->lpBytes);
    
    /* free the nodes
     */
    This->headmap_ro->next = current->next;
    HeapFree(GetProcessHeap(), 0, current);

    current = This->headmap_ro->next;
  }

  /*
   * then do the write list
   */
  current = This->headmap_w->next;

  while (current != NULL)
  {
    /* unmap views
     */
    UnmapViewOfFile(current->lpBytes);
    
    /* free the nodes
     */
    This->headmap_w->next = current->next;
    HeapFree(GetProcessHeap(), 0, current);

    current = This->headmap_w->next;
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
  BOOL32 bSTGM_WRITE     = ((openFlags & STGM_WRITE) == STGM_WRITE);
  BOOL32 bSTGM_READWRITE = ((openFlags & STGM_READWRITE) == STGM_READWRITE);
  BOOL32 bSTGM_READ      = ! (bSTGM_WRITE || bSTGM_READWRITE);

  if (bSTGM_READ)
    flProtect = PAGE_READONLY;

  if ((bSTGM_WRITE) || (bSTGM_READWRITE))
    flProtect = PAGE_READWRITE;

  return flProtect;
}

 
