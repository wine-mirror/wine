/*
 * WINElib-Resources
 *
 * Copied and modified heavily from loader/resource.c
 */

#include <stdlib.h>
#include "wine/winestring.h"
#include "libres.h"
#include "resource.h"
#include "debugtools.h"
#include "heap.h"
#include "crtdll.h"
#include "xmalloc.h"

DEFAULT_DEBUG_CHANNEL(resource)

typedef struct RLE
{
    const wrc_resource32_t * const * Resources;  /* NULL-terminated array of pointers */
    struct RLE* next;
} ResListE;

static ResListE* ResourceList=NULL;

void LIBRES_RegisterResources(const wrc_resource32_t * const * Res)
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
typedef int (*CmpFunc_t)(LPCWSTR a, LPCWSTR b, int c);

int CompareOrdinal(LPCWSTR ordinal, LPCWSTR resstr, int resid)
{
    return !resstr && (resid == LOWORD(ordinal));
} 
	
int CompareName(LPCWSTR name, LPCWSTR resstr, int resid)
{
    return resstr && !CRTDLL__wcsnicmp(resstr+1, name, *(resstr));
}

HRSRC LIBRES_FindResource( HINSTANCE hModule, LPCWSTR name, LPCWSTR type )
{
  LPCWSTR nameid = name, typeid = type;
  ResListE* ResBlock;
  const wrc_resource32_t* const * Res;
  CmpFunc_t EqualNames = CompareOrdinal;
  CmpFunc_t EqualTypes = CompareOrdinal;

  if(HIWORD(name))
  {
    if(*name=='#')
    {
        LPSTR nameA = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
        nameid = (LPCWSTR) atoi(nameA+1);
        HeapFree( GetProcessHeap(), 0, nameA );
    }
  else
	EqualNames = CompareName;
  }

  if(HIWORD(type))
  {
    if(*type=='#')
    {
        LPSTR typeA = HEAP_strdupWtoA( GetProcessHeap(), 0, type );
        typeid= (LPCWSTR) atoi(typeA+1);
        HeapFree( GetProcessHeap(), 0, typeA );
    }
    else
	EqualTypes = CompareName;
  }
  
  for(ResBlock=ResourceList; ResBlock; ResBlock=ResBlock->next)
    for(Res=ResBlock->Resources; *Res; Res++)
	if (EqualNames(nameid, (*Res)->resname, (*Res)->resid) && 
		EqualTypes(typeid, (*Res)->restypename, (*Res)->restype))
	  return (HRSRC)*Res;

  return 0;
}


/**********************************************************************
 *	    LIBRES_LoadResource    
 */
HGLOBAL LIBRES_LoadResource( HINSTANCE hModule, HRSRC hRsrc )
{
  return (HGLOBAL)(((wrc_resource32_t*)hRsrc)->data);
}


/**********************************************************************
 *	    LIBRES_SizeofResource    
 */
DWORD LIBRES_SizeofResource( HINSTANCE hModule, HRSRC hRsrc )
{
  return (DWORD)(((wrc_resource32_t*)hRsrc)->datasize);
}
