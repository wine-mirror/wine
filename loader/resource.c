/*
 * Resources
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "wine/exception.h"
#include "ldt.h"
#include "global.h"
#include "heap.h"
#include "callback.h"
#include "cursoricon.h"
#include "neexe.h"
#include "task.h"
#include "process.h"
#include "module.h"
#include "file.h"
#include "debugtools.h"
#include "winerror.h"
#include "winnls.h"

DEFAULT_DEBUG_CHANNEL(resource);
DECLARE_DEBUG_CHANNEL(accel);

#define HRSRC_MAP_BLOCKSIZE 16

typedef struct _HRSRC_ELEM
{
    HANDLE hRsrc;
    WORD     type;
} HRSRC_ELEM;

typedef struct _HRSRC_MAP
{
    int nAlloc;
    int nUsed;
    HRSRC_ELEM *elem;
} HRSRC_MAP;

/**********************************************************************
 *          MapHRsrc32To16
 */
static HRSRC16 MapHRsrc32To16( NE_MODULE *pModule, HANDLE hRsrc32, WORD type )
{
    HRSRC_MAP *map = (HRSRC_MAP *)pModule->hRsrcMap;
    HRSRC_ELEM *newElem;
    int i;

    /* On first call, initialize HRSRC map */
    if ( !map )
    {
        if ( !(map = (HRSRC_MAP *)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 
                                             sizeof(HRSRC_MAP) ) ) )
        {
            ERR("Cannot allocate HRSRC map\n" );
            return 0;
        }
        pModule->hRsrcMap = (LPVOID)map;
    }

    /* Check whether HRSRC32 already in map */
    for ( i = 0; i < map->nUsed; i++ )
        if ( map->elem[i].hRsrc == hRsrc32 )
            return (HRSRC16)(i + 1);

    /* If no space left, grow table */
    if ( map->nUsed == map->nAlloc )
    {
        if ( !(newElem = (HRSRC_ELEM *)HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 
                                                    map->elem, 
                                                    (map->nAlloc + HRSRC_MAP_BLOCKSIZE) 
                                                    * sizeof(HRSRC_ELEM) ) ))
        {
            ERR("Cannot grow HRSRC map\n" );
            return 0;
        }
        map->elem = newElem;
        map->nAlloc += HRSRC_MAP_BLOCKSIZE;
    }

    /* Add HRSRC32 to table */
    map->elem[map->nUsed].hRsrc = hRsrc32;
    map->elem[map->nUsed].type  = type;
    map->nUsed++;

    return (HRSRC16)map->nUsed;
}

/**********************************************************************
 *          MapHRsrc16To32
 */
static HANDLE MapHRsrc16To32( NE_MODULE *pModule, HRSRC16 hRsrc16 )
{
    HRSRC_MAP *map = (HRSRC_MAP *)pModule->hRsrcMap;
    if ( !map || !hRsrc16 || (int)hRsrc16 > map->nUsed ) return 0;

    return map->elem[(int)hRsrc16-1].hRsrc;
}

/**********************************************************************
 *          MapHRsrc16ToType
 */
static WORD MapHRsrc16ToType( NE_MODULE *pModule, HRSRC16 hRsrc16 )
{
    HRSRC_MAP *map = (HRSRC_MAP *)pModule->hRsrcMap;
    if ( !map || !hRsrc16 || (int)hRsrc16 > map->nUsed ) return 0;

    return map->elem[(int)hRsrc16-1].type;
}


/* filter for page-fault exceptions */
static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

static HRSRC RES_FindResource2( HMODULE hModule, LPCSTR type,
				LPCSTR name, WORD lang, 
				BOOL bUnicode, BOOL bRet16 )
{
    HRSRC hRsrc = 0;
    HMODULE16 hMod16   = MapHModuleLS( hModule );
    NE_MODULE *pModule = NE_GetPtr( hMod16 );
    WINE_MODREF *wm    = pModule && pModule->module32? 
	MODULE32_LookupHMODULE( pModule->module32 ) : NULL;
    
    TRACE("(%08x %s, %08x%s, %08x%s, %04x, %s, %s)\n",
	  hModule,
	  pModule ? (char *)NE_MODULE_NAME(pModule) : "NULL dereference",
	  (UINT)type, HIWORD(type)? (bUnicode? debugstr_w((LPWSTR)type) : debugstr_a(type)) : "",
	  (UINT)name, HIWORD(name)? (bUnicode? debugstr_w((LPWSTR)name) : debugstr_a(name)) : "",
	  lang,
	  bUnicode? "W"  : "A",
	  bRet16?   "NE" : "PE" );
    
    if (pModule)
    {
	if ( wm )
	{
	    /* 32-bit PE module */
	    LPWSTR typeStr, nameStr;
	    
	    if ( HIWORD( type ) && !bUnicode )
		typeStr = HEAP_strdupAtoW( GetProcessHeap(), 0, type );
	    else
		typeStr = (LPWSTR)type;
	    if ( HIWORD( name ) && !bUnicode )
		nameStr = HEAP_strdupAtoW( GetProcessHeap(), 0, name );
	    else
		nameStr = (LPWSTR)name;
	    
	    hRsrc = PE_FindResourceExW( wm, nameStr, typeStr, lang );
	    
	    if ( HIWORD( type ) && !bUnicode ) 
		HeapFree( GetProcessHeap(), 0, typeStr );
	    if ( HIWORD( name ) && !bUnicode ) 
		HeapFree( GetProcessHeap(), 0, nameStr );
	    
	    
	    /* If we need to return 16-bit HRSRC, perform conversion */
	    if ( bRet16 )
		hRsrc = MapHRsrc32To16( pModule, hRsrc, 
					HIWORD( type )? 0 : LOWORD( type ) );
	}
	else
	{
	    /* 16-bit NE module */
	    LPSTR typeStr, nameStr;
	    
	    if ( HIWORD( type ) && bUnicode )
		typeStr = HEAP_strdupWtoA( GetProcessHeap(), 0, (LPCWSTR)type );
	    else
		typeStr = (LPSTR)type;
	    if ( HIWORD( name ) && bUnicode )
		nameStr = HEAP_strdupWtoA( GetProcessHeap(), 0, (LPCWSTR)name );
	    else
		nameStr = (LPSTR)name;
	    
	    hRsrc = NE_FindResource( pModule, nameStr, typeStr );
	    
	    if ( HIWORD( type ) && bUnicode ) 
		HeapFree( GetProcessHeap(), 0, typeStr );
	    if ( HIWORD( name ) && bUnicode ) 
		HeapFree( GetProcessHeap(), 0, nameStr );
	    
	    
	    /* If we need to return 32-bit HRSRC, no conversion is necessary,
	       we simply use the 16-bit HRSRC as 32-bit HRSRC */
	}
    }
    return hRsrc;
}

/**********************************************************************
 *          RES_FindResource
 */

static HRSRC RES_FindResource( HMODULE hModule, LPCSTR type,
                               LPCSTR name, WORD lang, 
                               BOOL bUnicode, BOOL bRet16 )
{
    HRSRC hRsrc;
    __TRY
    {
	hRsrc = RES_FindResource2(hModule, type, name, lang, bUnicode, bRet16);
    }
    __EXCEPT(page_fault)
    {
	WARN("page fault\n");
	SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    __ENDTRY
    return hRsrc;
}

/**********************************************************************
 *          RES_SizeofResource
 */
static DWORD RES_SizeofResource( HMODULE hModule, HRSRC hRsrc, BOOL bRet16 )
{
    DWORD size = 0;

    HMODULE16 hMod16   = MapHModuleLS( hModule );
    NE_MODULE *pModule = NE_GetPtr( hMod16 );
    WINE_MODREF *wm    = pModule && pModule->module32? 
                         MODULE32_LookupHMODULE( pModule->module32 ) : NULL;

    TRACE("(%08x %s, %08x, %s)\n",
          hModule, NE_MODULE_NAME(pModule), hRsrc, bRet16? "NE" : "PE" );

    if ( !pModule || !hRsrc ) return 0;

    if ( wm )
    {
        /* 32-bit PE module */

        /* If we got a 16-bit hRsrc, convert it */
        HRSRC hRsrc32 = HIWORD(hRsrc)? hRsrc : MapHRsrc16To32( pModule, hRsrc );

        size = PE_SizeofResource( hModule, hRsrc32 );
    }
    else
    {
        /* 16-bit NE module */

        /* If we got a 32-bit hRsrc, we don't need to convert it */

        size = NE_SizeofResource( pModule, hRsrc );
    }

    return size;
}

/**********************************************************************
 *          RES_AccessResource
 */
static HFILE RES_AccessResource( HMODULE hModule, HRSRC hRsrc, BOOL bRet16 )
{
    HFILE hFile = HFILE_ERROR;

    HMODULE16 hMod16   = MapHModuleLS( hModule );
    NE_MODULE *pModule = NE_GetPtr( hMod16 );
    WINE_MODREF *wm    = pModule && pModule->module32? 
                         MODULE32_LookupHMODULE( pModule->module32 ) : NULL;

    TRACE("(%08x %s, %08x, %s)\n",
          hModule, NE_MODULE_NAME(pModule), hRsrc, bRet16? "NE" : "PE" );

    if ( !pModule || !hRsrc ) return HFILE_ERROR;

    if ( wm )
    {
        /* 32-bit PE module */
#if 0
        /* If we got a 16-bit hRsrc, convert it */
        HRSRC hRsrc32 = HIWORD(hRsrc)? hRsrc : MapHRsrc16To32( pModule, hRsrc );
#endif

        FIXME("32-bit modules not yet supported.\n" );
        hFile = HFILE_ERROR;

        /* If we need to return a 16-bit file handle, convert it */
        if ( bRet16 )
            hFile = FILE_AllocDosHandle( hFile );
    }
    else
    {
        /* 16-bit NE module */

        /* If we got a 32-bit hRsrc, we don't need to convert it */

        hFile = NE_AccessResource( pModule, hRsrc );

        /* If we are to return a 32-bit file handle, convert it */
        if ( !bRet16 )
            hFile = FILE_GetHandle( hFile );
    }

    return hFile;
}

/**********************************************************************
 *          RES_LoadResource
 */
static HGLOBAL RES_LoadResource( HMODULE hModule, HRSRC hRsrc, BOOL bRet16 )
{
    HGLOBAL hMem = 0;

    HMODULE16 hMod16   = MapHModuleLS( hModule );
    NE_MODULE *pModule = NE_GetPtr( hMod16 );
    WINE_MODREF *wm    = pModule && pModule->module32? 
                         MODULE32_LookupHMODULE( pModule->module32 ) : NULL;

    TRACE("(%08x %s, %08x, %s)\n",
          hModule, NE_MODULE_NAME(pModule), hRsrc, bRet16? "NE" : "PE" );

    if ( !pModule || !hRsrc ) return 0;

    if ( wm )
    {
        /* 32-bit PE module */

        /* If we got a 16-bit hRsrc, convert it */
        HRSRC hRsrc32 = HIWORD(hRsrc)? hRsrc : MapHRsrc16To32( pModule, hRsrc );

        hMem = PE_LoadResource( wm, hRsrc32 );

        /* If we need to return a 16-bit resource, convert it */
        if ( bRet16 )
        {
            WORD type   = MapHRsrc16ToType( pModule, hRsrc );
            DWORD size  = SizeofResource( hModule, hRsrc );
            LPVOID bits = LockResource( hMem );

            hMem = NE_LoadPEResource( pModule, type, bits, size );
        }
    }
    else
    {
        /* 16-bit NE module */

        /* If we got a 32-bit hRsrc, we don't need to convert it */

        hMem = NE_LoadResource( pModule, hRsrc );

        /* If we are to return a 32-bit resource, we should probably
           convert it but we don't for now.  FIXME !!! */
    }

    return hMem;
}

/**********************************************************************
 *          RES_LockResource
 */
static LPVOID RES_LockResource( HGLOBAL handle, BOOL bRet16 )
{
    LPVOID bits = NULL;

    TRACE("(%08x, %s)\n", handle, bRet16? "NE" : "PE" );

    if ( HIWORD( handle ) )
    {
        /* 32-bit memory handle */

        if ( bRet16 )
            FIXME("can't return SEGPTR to 32-bit resource %08x.\n", handle );
        else
            bits = (LPVOID)handle;
    }
    else
    {
        /* 16-bit memory handle */

        /* May need to reload the resource if discarded */
        SEGPTR segPtr = WIN16_GlobalLock16( handle );
        
        if ( bRet16 )
            bits = (LPVOID)segPtr;
        else
            bits = PTR_SEG_TO_LIN( segPtr );
    }

    return bits;
}

/**********************************************************************
 *          RES_FreeResource
 */
static BOOL RES_FreeResource( HGLOBAL handle )
{
    HGLOBAL retv = handle;

    TRACE("(%08x)\n", handle );

    if ( HIWORD( handle ) )
    {
        /* 32-bit memory handle: nothing to do */
    }
    else
    {
        /* 16-bit memory handle */
        NE_MODULE *pModule = NE_GetPtr( FarGetOwner16( handle ) );

        /* Try NE resource first */
        retv = NE_FreeResource( pModule, handle );

        /* If this failed, call USER.DestroyIcon32; this will check
           whether it is a shared cursor/icon; if not it will call
           GlobalFree16() */
        if ( retv ) {
            if ( Callout.DestroyIcon32 )
                retv = Callout.DestroyIcon32( handle, CID_RESOURCE );
            else
                retv = GlobalFree16( handle );
	}
    }

    return (BOOL)retv;
}


/**********************************************************************
 *          FindResource16   (KERNEL.60)
 */
HRSRC16 WINAPI FindResource16( HMODULE16 hModule, SEGPTR name, SEGPTR type )
{
    LPCSTR nameStr = HIWORD(name)? PTR_SEG_TO_LIN(name) : (LPCSTR)name;
    LPCSTR typeStr = HIWORD(type)? PTR_SEG_TO_LIN(type) : (LPCSTR)type;

    return RES_FindResource( hModule, typeStr, nameStr, 
                             GetSystemDefaultLangID(), FALSE, TRUE );
}

/**********************************************************************
 *	    FindResourceA    (KERNEL32.128)
 */
HANDLE WINAPI FindResourceA( HMODULE hModule, LPCSTR name, LPCSTR type )
{
    return RES_FindResource( hModule, type, name, 
                             GetSystemDefaultLangID(), FALSE, FALSE );
}

/**********************************************************************
 *	    FindResourceExA  (KERNEL32.129)
 */
HANDLE WINAPI FindResourceExA( HMODULE hModule, 
                               LPCSTR type, LPCSTR name, WORD lang )
{
    return RES_FindResource( hModule, type, name, 
                             lang, FALSE, FALSE );
}

/**********************************************************************
 *	    FindResourceExW  (KERNEL32.130)
 */
HRSRC WINAPI FindResourceExW( HMODULE hModule, 
                              LPCWSTR type, LPCWSTR name, WORD lang )
{
    return RES_FindResource( hModule, (LPCSTR)type, (LPCSTR)name, 
                             lang, TRUE, FALSE );
}

/**********************************************************************
 *	    FindResourceW    (KERNEL32.131)
 */
HRSRC WINAPI FindResourceW(HINSTANCE hModule, LPCWSTR name, LPCWSTR type)
{
    return RES_FindResource( hModule, (LPCSTR)type, (LPCSTR)name, 
                             GetSystemDefaultLangID(), TRUE, FALSE );
}

/**********************************************************************
 *          LoadResource16   (KERNEL.61)
 */
HGLOBAL16 WINAPI LoadResource16( HMODULE16 hModule, HRSRC16 hRsrc )
{
    return RES_LoadResource( hModule, hRsrc, TRUE );
}

/**********************************************************************
 *	    LoadResource     (KERNEL32.370)
 */
HGLOBAL WINAPI LoadResource( HINSTANCE hModule, HRSRC hRsrc )
{
    return RES_LoadResource( hModule, hRsrc, FALSE );
}

/**********************************************************************
 *          LockResource16   (KERNEL.62)
 */
SEGPTR WINAPI WIN16_LockResource16( HGLOBAL16 handle )
{
    return (SEGPTR)RES_LockResource( handle, TRUE );
}
LPVOID WINAPI LockResource16( HGLOBAL16 handle )
{
    return RES_LockResource( handle, FALSE );
}

/**********************************************************************
 *	    LockResource     (KERNEL32.384)
 */
LPVOID WINAPI LockResource( HGLOBAL handle )
{
    return RES_LockResource( handle, FALSE );
}

/**********************************************************************
 *          FreeResource16   (KERNEL.63)
 */
BOOL16 WINAPI FreeResource16( HGLOBAL16 handle )
{
    return RES_FreeResource( handle );
}

/**********************************************************************
 *	    FreeResource     (KERNEL32.145)
 */
BOOL WINAPI FreeResource( HGLOBAL handle )
{
    return RES_FreeResource( handle );
}

/**********************************************************************
 *          AccessResource16 (KERNEL.64)
 */
INT16 WINAPI AccessResource16( HINSTANCE16 hModule, HRSRC16 hRsrc )
{
    return RES_AccessResource( hModule, hRsrc, TRUE );
}

/**********************************************************************
 *	    AccessResource   (KERNEL32.64)
 */
INT WINAPI AccessResource( HMODULE hModule, HRSRC hRsrc )
{
    return RES_AccessResource( hModule, hRsrc, FALSE );
}

/**********************************************************************
 *          SizeofResource16 (KERNEL.65)
 */
DWORD WINAPI SizeofResource16( HMODULE16 hModule, HRSRC16 hRsrc )
{
    return RES_SizeofResource( hModule, hRsrc, TRUE );
}

/**********************************************************************
 *	    SizeofResource   (KERNEL32.522)
 */
DWORD WINAPI SizeofResource( HINSTANCE hModule, HRSRC hRsrc )
{
    return RES_SizeofResource( hModule, hRsrc, FALSE );
}



/**********************************************************************
 *			LoadAccelerators16	[USER.177]
 */
HACCEL16 WINAPI LoadAccelerators16(HINSTANCE16 instance, SEGPTR lpTableName)
{
    HRSRC16	hRsrc;

    if (HIWORD(lpTableName))
        TRACE_(accel)("%04x '%s'\n",
                      instance, (char *)PTR_SEG_TO_LIN( lpTableName ) );
    else
        TRACE_(accel)("%04x %04x\n",
                       instance, LOWORD(lpTableName) );

    if (!(hRsrc = FindResource16( instance, lpTableName, RT_ACCELERATOR16 ))) {
      WARN_(accel)("couldn't find accelerator table resource\n");
      return 0;
    }

    TRACE_(accel)("returning HACCEL 0x%x\n", hRsrc);
    return LoadResource16(instance,hRsrc);
}

/**********************************************************************
 *			LoadAcceleratorsW	[USER.177]
 * The image layout seems to look like this (not 100% sure):
 * 00:	BYTE	type		type of accelerator
 * 01:	BYTE	pad		(to WORD boundary)
 * 02:	WORD	event
 * 04:	WORD	IDval		
 * 06:	WORD	pad		(to DWORD boundary)
 */
HACCEL WINAPI LoadAcceleratorsW(HINSTANCE instance,LPCWSTR lpTableName)
{
    HRSRC hRsrc;
    HACCEL hMem,hRetval=0;
    DWORD size;

    if (HIWORD(lpTableName))
        TRACE_(accel)("%p '%s'\n",
                      (LPVOID)instance, (char *)( lpTableName ) );
    else
        TRACE_(accel)("%p 0x%04x\n",
                       (LPVOID)instance, LOWORD(lpTableName) );

    if (!(hRsrc = FindResourceW( instance, lpTableName, RT_ACCELERATORW )))
    {
      WARN_(accel)("couldn't find accelerator table resource\n");
    } else {
      hMem = LoadResource( instance, hRsrc );
      size = SizeofResource( instance, hRsrc );
      if(size>=sizeof(PE_ACCEL))
      {
	LPPE_ACCEL accel_table = (LPPE_ACCEL) hMem;
	LPACCEL16 accel16;
	int i,nrofaccells = size/sizeof(PE_ACCEL);

	hRetval = GlobalAlloc16(0,sizeof(ACCEL16)*nrofaccells);
	accel16 = (LPACCEL16)GlobalLock16(hRetval);
	for (i=0;i<nrofaccells;i++) {
		accel16[i].fVirt = accel_table[i].fVirt;
		accel16[i].key = accel_table[i].key;
		accel16[i].cmd = accel_table[i].cmd;
	}
	accel16[i-1].fVirt |= 0x80;
      }
    }
    TRACE_(accel)("returning HACCEL 0x%x\n", hRsrc);
    return hRetval;
}

/***********************************************************************
 *		LoadAcceleratorsA
 */
HACCEL WINAPI LoadAcceleratorsA(HINSTANCE instance,LPCSTR lpTableName)
{
	LPWSTR	 uni;
	HACCEL result;
	if (HIWORD(lpTableName))
		uni = HEAP_strdupAtoW( GetProcessHeap(), 0, lpTableName );
	else
		uni = (LPWSTR)lpTableName;
	result = LoadAcceleratorsW(instance,uni);
	if (HIWORD(uni)) HeapFree( GetProcessHeap(), 0, uni);
	return result;
}

/**********************************************************************
 *             CopyAcceleratorTableA   (USER32.58)
 */
INT WINAPI CopyAcceleratorTableA(HACCEL src, LPACCEL dst, INT entries)
{
  return CopyAcceleratorTableW(src, dst, entries);
}

/**********************************************************************
 *             CopyAcceleratorTableW   (USER32.59)
 *
 * By mortene@pvv.org 980321
 */
INT WINAPI CopyAcceleratorTableW(HACCEL src, LPACCEL dst,
				     INT entries)
{
  int i,xsize;
  LPACCEL16 accel = (LPACCEL16)GlobalLock16(src);
  BOOL done = FALSE;

  /* Do parameter checking to avoid the explosions and the screaming
     as far as possible. */
  if((dst && (entries < 1)) || (src == (HACCEL)NULL) || !accel) {
    WARN_(accel)("Application sent invalid parameters (%p %p %d).\n",
	 (LPVOID)src, (LPVOID)dst, entries);
    return 0;
  }
  xsize = GlobalSize16(src)/sizeof(ACCEL16);
  if (xsize>entries) entries=xsize;

  i=0;
  while(!done) {
    /* Spit out some debugging information. */
    TRACE_(accel)("accel %d: type 0x%02x, event '%c', IDval 0x%04x.\n",
	  i, accel[i].fVirt, accel[i].key, accel[i].cmd);

    /* Copy data to the destination structure array (if dst == NULL,
       we're just supposed to count the number of entries). */
    if(dst) {
      dst[i].fVirt = accel[i].fVirt;
      dst[i].key = accel[i].key;
      dst[i].cmd = accel[i].cmd;

      /* Check if we've reached the end of the application supplied
         accelerator table. */
      if(i+1 == entries) {
	/* Turn off the high order bit, just in case. */
	dst[i].fVirt &= 0x7f;
	done = TRUE;
      }
    }

    /* The highest order bit seems to mark the end of the accelerator
       resource table, but not always. Use GlobalSize() check too. */
    if((accel[i].fVirt & 0x80) != 0) done = TRUE;

    i++;
  }

  return i;
}

/*********************************************************************
 *                    CreateAcceleratorTableA   (USER32.64)
 *
 * By mortene@pvv.org 980321
 */
HACCEL WINAPI CreateAcceleratorTableA(LPACCEL lpaccel, INT cEntries)
{
  HACCEL	hAccel;
  LPACCEL16	accel;
  int		i;

  /* Do parameter checking just in case someone's trying to be
     funny. */
  if(cEntries < 1) {
    WARN_(accel)("Application sent invalid parameters (%p %d).\n",
	 lpaccel, cEntries);
    SetLastError(ERROR_INVALID_PARAMETER);
    return (HACCEL)NULL;
  }
  FIXME_(accel)("should check that the accelerator descriptions are valid,"
	" return NULL and SetLastError() if not.\n");


  /* Allocate memory and copy the table. */
  hAccel = GlobalAlloc16(0,cEntries*sizeof(ACCEL16));

  TRACE_(accel)("handle %x\n", hAccel);
  if(!hAccel) {
    ERR_(accel)("Out of memory.\n");
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return (HACCEL)NULL;
  }
  accel = GlobalLock16(hAccel);
  for (i=0;i<cEntries;i++) {
  	accel[i].fVirt = lpaccel[i].fVirt;
  	accel[i].key = lpaccel[i].key;
  	accel[i].cmd = lpaccel[i].cmd;
  }
  /* Set the end-of-table terminator. */
  accel[cEntries-1].fVirt |= 0x80;

  TRACE_(accel)("Allocated accelerator handle %x\n", hAccel);
  return hAccel;
}

/*********************************************************************
 *                    CreateAcceleratorTableW   (USER32.64)
 *
 * 
 */
HACCEL WINAPI CreateAcceleratorTableW(LPACCEL lpaccel, INT cEntries)
{
  HACCEL	hAccel;
  LPACCEL16	accel;
  int		i;
  char		ckey;  

  /* Do parameter checking just in case someone's trying to be
     funny. */
  if(cEntries < 1) {
    WARN_(accel)("Application sent invalid parameters (%p %d).\n",
	 lpaccel, cEntries);
    SetLastError(ERROR_INVALID_PARAMETER);
    return (HACCEL)NULL;
  }
  FIXME_(accel)("should check that the accelerator descriptions are valid,"
	" return NULL and SetLastError() if not.\n");


  /* Allocate memory and copy the table. */
  hAccel = GlobalAlloc16(0,cEntries*sizeof(ACCEL16));

  TRACE_(accel)("handle %x\n", hAccel);
  if(!hAccel) {
    ERR_(accel)("Out of memory.\n");
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return (HACCEL)NULL;
  }
  accel = GlobalLock16(hAccel);


  for (i=0;i<cEntries;i++) {
       accel[i].fVirt = lpaccel[i].fVirt;
       if( !(accel[i].fVirt & FVIRTKEY) ) {
	  ckey = (char) lpaccel[i].key;
         if(!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, &ckey, 1, &accel[i].key, 1))
            WARN_(accel)("Error converting ASCII accelerator table to Unicode");
       }
       else 
         accel[i].key = lpaccel[i].key; 
       accel[i].cmd = lpaccel[i].cmd;
  }

  /* Set the end-of-table terminator. */
  accel[cEntries-1].fVirt |= 0x80;

  TRACE_(accel)("Allocated accelerator handle %x\n", hAccel);
  return hAccel;
}

/******************************************************************************
 * DestroyAcceleratorTable [USER32.130]
 * Destroys an accelerator table
 *
 * NOTES
 *    By mortene@pvv.org 980321
 *
 * PARAMS
 *    handle [I] Handle to accelerator table
 *
 * RETURNS STD
 */
BOOL WINAPI DestroyAcceleratorTable( HACCEL handle )
{
    return !GlobalFree16(handle); 
}
  
/**********************************************************************
 *					LoadString16
 */
INT16 WINAPI LoadString16( HINSTANCE16 instance, UINT16 resource_id,
                           LPSTR buffer, INT16 buflen )
{
    HGLOBAL16 hmem;
    HRSRC16 hrsrc;
    unsigned char *p;
    int string_num;
    int i;

    TRACE("inst=%04x id=%04x buff=%08x len=%d\n",
          instance, resource_id, (int) buffer, buflen);

    hrsrc = FindResource16( instance, (SEGPTR)((resource_id>>4)+1), RT_STRING16 );
    if (!hrsrc) return 0;
    hmem = LoadResource16( instance, hrsrc );
    if (!hmem) return 0;
    
    p = LockResource16(hmem);
    string_num = resource_id & 0x000f;
    for (i = 0; i < string_num; i++)
	p += *p + 1;
    
    TRACE("strlen = %d\n", (int)*p );
    
    if (buffer == NULL) return *p;
    i = min(buflen - 1, *p);
    if (i > 0) {
	memcpy(buffer, p + 1, i);
	buffer[i] = '\0';
    } else {
	if (buflen > 1) {
	    buffer[0] = '\0';
	    return 0;
	}
	WARN("Dont know why caller give buflen=%d *p=%d trying to obtain string '%s'\n", buflen, *p, p + 1);
    }
    FreeResource16( hmem );

    TRACE("'%s' loaded !\n", buffer);
    return i;
}

/**********************************************************************
 *	LoadStringW		(USER32.376)
 */
INT WINAPI LoadStringW( HINSTANCE instance, UINT resource_id,
                            LPWSTR buffer, INT buflen )
{
    HGLOBAL hmem;
    HRSRC hrsrc;
    WCHAR *p;
    int string_num;
    int i;

    if (HIWORD(resource_id)==0xFFFF) /* netscape 3 passes this */
	resource_id = (UINT)(-((INT)resource_id));
    TRACE("instance = %04x, id = %04x, buffer = %08x, "
          "length = %d\n", instance, (int)resource_id, (int) buffer, buflen);

    /* Use bits 4 - 19 (incremented by 1) as resourceid, mask out 
     * 20 - 31. */
    hrsrc = FindResourceW( instance, (LPCWSTR)(((resource_id>>4)&0xffff)+1),
                             RT_STRINGW );
    if (!hrsrc) return 0;
    hmem = LoadResource( instance, hrsrc );
    if (!hmem) return 0;
    
    p = LockResource(hmem);
    string_num = resource_id & 0x000f;
    for (i = 0; i < string_num; i++)
	p += *p + 1;
    
    TRACE("strlen = %d\n", (int)*p );
    
    if (buffer == NULL) return *p;
    i = min(buflen - 1, *p);
    if (i > 0) {
	memcpy(buffer, p + 1, i * sizeof (WCHAR));
	buffer[i] = (WCHAR) 0;
    } else {
	if (buflen > 1) {
	    buffer[0] = (WCHAR) 0;
	    return 0;
	}
#if 0
	WARN("Dont know why caller give buflen=%d *p=%d trying to obtain string '%s'\n", buflen, *p, p + 1);
#endif
    }

    TRACE("%s loaded !\n", debugstr_w(buffer));
    return i;
}

/**********************************************************************
 *	LoadStringA	(USER32.375)
 */
INT WINAPI LoadStringA( HINSTANCE instance, UINT resource_id,
                            LPSTR buffer, INT buflen )
{
    INT    retval;
    LPWSTR wbuf;

    TRACE("instance = %04x, id = %04x, buffer = %08x, "
          "length = %d\n", instance, (int)resource_id, (int) buffer, buflen);

    if(buffer == NULL) /* asked size of string */
	return LoadStringW(instance, resource_id, NULL, 0);
    
    wbuf = HeapAlloc(GetProcessHeap(), 0, buflen * sizeof(WCHAR));
    if(!wbuf)
	return 0;

    retval = LoadStringW(instance, resource_id, wbuf, buflen);
    if(retval != 0)
    {
	retval = WideCharToMultiByte(CP_ACP, 0, wbuf, retval, buffer, buflen - 1, NULL, NULL);
	buffer[retval] = 0;
	TRACE("%s loaded !\n", debugstr_a(buffer));
    }
    HeapFree( GetProcessHeap(), 0, wbuf );

    return retval;
}

/* Messages...used by FormatMessage32* (KERNEL32.something)
 * 
 * They can be specified either directly or using a message ID and
 * loading them from the resource.
 * 
 * The resourcedata has following format:
 * start:
 * 0: DWORD nrofentries
 * nrofentries * subentry:
 *	0: DWORD firstentry
 *	4: DWORD lastentry
 *      8: DWORD offset from start to the stringentries
 *
 * (lastentry-firstentry) * stringentry:
 * 0: WORD len (0 marks end)
 * 2: WORD flags
 * 4: CHAR[len-4]
 * 	(stringentry i of a subentry refers to the ID 'firstentry+i')
 *
 * Yes, ANSI strings in win32 resources. Go figure.
 */

/**********************************************************************
 *	LoadMessageA		(internal)
 */
INT WINAPI LoadMessageA( HMODULE instance, UINT id, WORD lang,
                      LPSTR buffer, INT buflen )
{
    HGLOBAL	hmem;
    HRSRC	hrsrc;
    PMESSAGE_RESOURCE_DATA	mrd;
    PMESSAGE_RESOURCE_BLOCK	mrb;
    PMESSAGE_RESOURCE_ENTRY	mre;
    int		i,slen;

    TRACE("instance = %08lx, id = %08lx, buffer = %p, length = %ld\n", (DWORD)instance, (DWORD)id, buffer, (DWORD)buflen);

    /*FIXME: I am not sure about the '1' ... But I've only seen those entries*/
    hrsrc = FindResourceExW(instance,RT_MESSAGELISTW,(LPWSTR)1,lang);
    if (!hrsrc) return 0;
    hmem = LoadResource( instance, hrsrc );
    if (!hmem) return 0;
    
    mrd = (PMESSAGE_RESOURCE_DATA)LockResource(hmem);
    mre = NULL;
    mrb = &(mrd->Blocks[0]);
    for (i=mrd->NumberOfBlocks;i--;) {
    	if ((id>=mrb->LowId) && (id<=mrb->HighId)) {
	    mre = (PMESSAGE_RESOURCE_ENTRY)(((char*)mrd)+mrb->OffsetToEntries);
	    id	-= mrb->LowId;
	    break;
	}
	mrb++;
    }
    if (!mre)
    	return 0;
    for (i=id;i--;) {
    	if (!mre->Length)
		return 0;
    	mre = (PMESSAGE_RESOURCE_ENTRY)(((char*)mre)+(mre->Length));
    }
    slen=mre->Length;
    TRACE("	- strlen=%d\n",slen);
    i = min(buflen - 1, slen);
    if (buffer == NULL)
	return slen;
    if (i>0) {
	lstrcpynA(buffer,(char*)mre->Text,i);
	buffer[i]=0;
    } else {
	if (buflen>1) {
	    buffer[0]=0;
	    return 0;
	}
    }
    if (buffer)
	    TRACE("'%s' copied !\n", buffer);
    return i;
}

/**********************************************************************
 *	LoadMessageW	(internal)
 */
INT WINAPI LoadMessageW( HMODULE instance, UINT id, WORD lang,
                      LPWSTR buffer, INT buflen )
{
    INT retval;
    LPSTR buffer2 = NULL;
    if (buffer && buflen)
	buffer2 = HeapAlloc( GetProcessHeap(), 0, buflen );
    retval = LoadMessageA(instance,id,lang,buffer2,buflen);
    if (buffer)
    {
	if (retval) {
	    lstrcpynAtoW( buffer, buffer2, buflen );
	    retval = lstrlenW( buffer );
	}
	HeapFree( GetProcessHeap(), 0, buffer2 );
    }
    return retval;
}


/**********************************************************************
 *	EnumResourceTypesA	(KERNEL32.90)
 */
BOOL WINAPI EnumResourceTypesA( HMODULE hmodule,ENUMRESTYPEPROCA lpfun,
                                    LONG lParam)
{
	/* FIXME: move WINE_MODREF stuff here */
    return PE_EnumResourceTypesA(hmodule,lpfun,lParam);
}

/**********************************************************************
 *	EnumResourceTypesW	(KERNEL32.91)
 */
BOOL WINAPI EnumResourceTypesW( HMODULE hmodule,ENUMRESTYPEPROCW lpfun,
                                    LONG lParam)
{
	/* FIXME: move WINE_MODREF stuff here */
    return PE_EnumResourceTypesW(hmodule,lpfun,lParam);
}

/**********************************************************************
 *	EnumResourceNamesA	(KERNEL32.88)
 */
BOOL WINAPI EnumResourceNamesA( HMODULE hmodule, LPCSTR type,
                                    ENUMRESNAMEPROCA lpfun, LONG lParam )
{
	/* FIXME: move WINE_MODREF stuff here */
    return PE_EnumResourceNamesA(hmodule,type,lpfun,lParam);
}
/**********************************************************************
 *	EnumResourceNamesW	(KERNEL32.89)
 */
BOOL WINAPI EnumResourceNamesW( HMODULE hmodule, LPCWSTR type,
                                    ENUMRESNAMEPROCW lpfun, LONG lParam )
{
	/* FIXME: move WINE_MODREF stuff here */
    return PE_EnumResourceNamesW(hmodule,type,lpfun,lParam);
}

/**********************************************************************
 *	EnumResourceLanguagesA	(KERNEL32.86)
 */
BOOL WINAPI EnumResourceLanguagesA( HMODULE hmodule, LPCSTR type,
                                        LPCSTR name, ENUMRESLANGPROCA lpfun,
                                        LONG lParam)
{
	/* FIXME: move WINE_MODREF stuff here */
    return PE_EnumResourceLanguagesA(hmodule,type,name,lpfun,lParam);
}
/**********************************************************************
 *	EnumResourceLanguagesW	(KERNEL32.87)
 */
BOOL WINAPI EnumResourceLanguagesW( HMODULE hmodule, LPCWSTR type,
                                        LPCWSTR name, ENUMRESLANGPROCW lpfun,
                                        LONG lParam)
{
	/* FIXME: move WINE_MODREF stuff here */
    return PE_EnumResourceLanguagesW(hmodule,type,name,lpfun,lParam);
}
