/*
 * WINElib-Resources
 *
 * Copied and modified heavily from loader/resource.c
 */

#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include "libres.h"
#include "heap.h"
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
HRSRC32 LIBRES_FindResource( HINSTANCE32 hModule, LPCWSTR name, LPCWSTR type )
{
  int nameid=0,typeid;
  ResListE* ResBlock;
  const struct resource* const * Res;

  if(HIWORD(name))
  {
    if(*name=='#')
    {
        LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
        nameid = atoi(nameA+1);
        HeapFree( GetProcessHeap(), 0, nameA );
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
    {
        LPSTR typeA = HEAP_strdupWtoA( GetProcessHeap(), 0, type );
        typeid=atoi(typeA+1);
        HeapFree( GetProcessHeap(), 0, typeA );
    }
    else
    {
      TRACE(resource, "(*,*,type=string): Returning 0\n");
      return 0;
    }
  }
  else
    typeid=LOWORD(type);
  
  for(ResBlock=ResourceList; ResBlock; ResBlock=ResBlock->next)
    for(Res=ResBlock->Resources; *Res; Res++)
      if(name)
      {
	if((*Res)->type==typeid && !lstrcmpi32W((LPCWSTR)(*Res)->name,name))
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
HGLOBAL32 LIBRES_LoadResource( HINSTANCE32 hModule, HRSRC32 hRsrc )
{
  return (HGLOBAL32)(((struct resource*)hRsrc)->bytes);
}


/**********************************************************************
 *	    LIBRES_SizeofResource    
 */
DWORD LIBRES_SizeofResource( HINSTANCE32 hModule, HRSRC32 hRsrc )
{
  return (DWORD)(((struct resource*)hRsrc)->size);
}
