/*
 * WINElib-Resources
 *
 * Copied and modified heavily from loader/resource.c
 */

#include <stdio.h>
#include "windows.h"

struct resource /* This needs to coincide with what winerc generates. */
{               /* It should really only appear in one place.         */
  int id, type;
  char *name;
  unsigned char *bytes;
  unsigned int size;
};

typedef struct RLE
{
  struct resource** Resources  /* NULL-terminated array of pointers */
  struct RLE* next;
} ResListE;

static ResListE* ResourceList=NULL;

void LIBRES_RegisterResources(struct resource** Res)
{
  ResListE** Curr;
  ResListE* n;
  for(Curr=&ResourceList; *Curr; Curr=&((*Curr)->next)) { }
  n=malloc(sizeof(ResListE));
  if(n)
  {
    n.Resources=Res;
    n.next=NULL;
    *Curr=n;
  }
  else
    fprintf(stderr,"LIBRES_RegisterResources(): Out of memory.\n");
}

/**********************************************************************
 *	    LIBRES_FindResource    
 */
HRSRC LIBRES_FindResource( HMODULE hModule, SEGPTR name, SEGPTR type )
{
  WINELIB_UNIMP("LIBRES_FindResource()");
  return 0;
}


/**********************************************************************
 *	    LIBRES_LoadResource    
 */
HGLOBAL LIBRES_LoadResource( HMODULE hModule, HRSRC hRsrc )
{
  return (HGLOBAL)(((struct resource*)hRsrc)->bytes);
}


/**********************************************************************
 *	    LIBRES_LockResource    
 */
LPVOID LIBRES_LockResource( HMODULE hModule, HGLOBAL handle )
{
  return handle;
}


/**********************************************************************
 *	    LIBRES_FreeResource    
 */
BOOL LIBRES_FreeResource( HMODULE hModule, HGLOBAL handle )
{
  return 0;
}


/**********************************************************************
 *	    LIBRES_AccessResource    
 */
INT LIBRES_AccessResource( HINSTANCE hModule, HRSRC hRsrc )
{
  WINELIB_UNIMP("LIBRES_AccessResource()");
  return -1;
}


/**********************************************************************
 *	    LIBRES_SizeofResource    
 */
DWORD LIBRES_SizeofResource( HMODULE hModule, HRSRC hRsrc )
{
  return (HGLOBAL)(((struct resource*)hRsrc)->size);
}


/**********************************************************************
 *	    LIBRES_AllocResource    
 */
HGLOBAL LIBRES_AllocResource( HMODULE hModule, HRSRC hRsrc, DWORD size )
{
  WINELIB_UNIMP("LIBRES_AllocResource()");
  return 0;
}

