/*
 * WINElib-Resources
 *
 * Copied and modified heavily from loader/resource.c
 */

#include <stdio.h>
#include <stdlib.h>
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
 *	    LIBRES_FindResource16    
 */
HRSRC32 LIBRES_FindResource16( HINSTANCE32 hModule, LPCSTR name, LPCSTR type )
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
      fprintf(stderr,"LIBRES_FindResource16(*,*,type=string)");
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
 *	    LIBRES_FindResource32    
 */
HRSRC32 LIBRES_FindResource32( HINSTANCE32 hModule, LPCWSTR name, LPCWSTR type )
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
      fprintf(stderr,"LIBRES_FindResource32(*,*,type=string)");
      return 0;
    }
  }
  else
    typeid=LOWORD(type);
  
  for(ResBlock=ResourceList; ResBlock; ResBlock=ResBlock->next)
    for(Res=ResBlock->Resources; *Res; Res++)
      if(name)
      {
	if((*Res)->type==typeid && !lstrcmpi32W((*Res)->name,name))
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
 *	    LIBRES_LockResource    
 */
LPVOID LIBRES_LockResource( HGLOBAL32 handle )
{
  return (LPVOID)handle;
}


/**********************************************************************
 *	    LIBRES_FreeResource    
 */
BOOL32 LIBRES_FreeResource( HGLOBAL32 handle )
{
  return 0; /* Obsolete in Win32 */
}


/**********************************************************************
 *	    LIBRES_AccessResource    
 */
INT32 LIBRES_AccessResource( HINSTANCE32 hModule, HRSRC32 hRsrc )
{
  fprintf(stderr,"LIBRES_AccessResource()");
  return -1; /* Obsolete in Win32 */
}


/**********************************************************************
 *	    LIBRES_SizeofResource    
 */
DWORD LIBRES_SizeofResource( HINSTANCE32 hModule, HRSRC32 hRsrc )
{
  return (DWORD)(((struct resource*)hRsrc)->size);
}


/**********************************************************************
 *	    LIBRES_AllocResource    
 */
HGLOBAL32 LIBRES_AllocResource( HINSTANCE32 hModule, HRSRC32 hRsrc, DWORD size)
{
  fprintf(stderr,"LIBRES_AllocResource()");
  return 0; /* Obsolete in Win32 */
}

