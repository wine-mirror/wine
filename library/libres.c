/*
 * WINElib-Resources
 *
 * Copied and modified heavily from loader/resource.c
 */

#include <stdio.h>
#include <stdlib.h>
#include "libres.h"
#include "windows.h"
#include "xmalloc.h"

typedef struct RLE
{
    const struct resource* const * Resources;  /* NULL-terminated array of pointers */
    struct RLE* next;
} ResListE;

static ResListE* ResourceList=NULL;

void LIBRES_RegisterResources(const struct resource* const * Res)
{
  ResListE** Curr;
  ResListE* n;
  for(Curr=&ResourceList; *Curr; Curr=&((*Curr)->next)) { }
  n=xmalloc(sizeof(ResListE));
  n->Resources=Res;
  n->next=NULL;
  *Curr=n;
}

/**********************************************************************
 *	    LIBRES_FindResource    
 */
HRSRC32 LIBRES_FindResource( HINSTANCE hModule, LPCSTR name, LPCSTR type )
{
  int nameid=0,typeid;
  ResListE* ResBlock;
  const struct resource* const * Res;

  if(HIWORD(name))
  {
    if(*name=='#')
    {
      nameid=atoi(name+1);
      name=NULL;
    }
  }
  else
  {
    nameid=LOWORD(name);
    name=NULL;
  }
  if(HIWORD(type))
  {
    if(*type=='#')
      typeid=atoi(type+1);
    else
    {
      WINELIB_UNIMP("LIBRES_FindResource(*,*,type=string)");
      return 0;
    }
  }
  else
    typeid=LOWORD(type);
  
  for(ResBlock=ResourceList; ResBlock; ResBlock=ResBlock->next)
    for(Res=ResBlock->Resources; *Res; Res++)
      if(name)
      {
	if((*Res)->type==typeid && !lstrcmpi32A((*Res)->name,name))
	  return (HRSRC32)*Res;
      }
      else
	if((*Res)->type==typeid && (*Res)->id==nameid)
	  return (HRSRC32)*Res;
  return 0;
}


/**********************************************************************
 *	    LIBRES_LoadResource    
 */
HGLOBAL32 LIBRES_LoadResource( HINSTANCE hModule, HRSRC32 hRsrc )
{
  return (HGLOBAL32)(((struct resource*)hRsrc)->bytes);
}


/**********************************************************************
 *	    LIBRES_LockResource    
 */
LPVOID LIBRES_LockResource( HGLOBAL32 handle )
{
  return (LPVOID)handle;
}


/**********************************************************************
 *	    LIBRES_FreeResource    
 */
BOOL LIBRES_FreeResource( HGLOBAL handle )
{
  return 0; /* Obsolete in Win32 */
}


/**********************************************************************
 *	    LIBRES_AccessResource    
 */
INT LIBRES_AccessResource( HINSTANCE hModule, HRSRC hRsrc )
{
  WINELIB_UNIMP("LIBRES_AccessResource()");
  return -1; /* Obsolete in Win32 */
}


/**********************************************************************
 *	    LIBRES_SizeofResource    
 */
DWORD LIBRES_SizeofResource( HINSTANCE hModule, HRSRC32 hRsrc )
{
  return (DWORD)(((struct resource*)hRsrc)->size);
}


/**********************************************************************
 *	    LIBRES_AllocResource    
 */
HGLOBAL LIBRES_AllocResource( HINSTANCE hModule, HRSRC hRsrc, DWORD size )
{
  WINELIB_UNIMP("LIBRES_AllocResource()");
  return 0; /* Obsolete in Win32 */
}

