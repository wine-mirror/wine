/*
 * Win32 Resources
 *
 * Copyright 1995 Thomas Sandford
 *
 * Based on the Win16 resource handling code in loader/resource.c
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 *
 * This is not even at ALPHA level yet. Don't expect it to work!
 */

#include <sys/types.h>
#include "wintypes.h"
#include "windows.h"
#include "kernel32.h"
#include "pe_image.h"
#include "handle32.h"
#include "resource32.h"
#include "neexe.h"
#include "accel.h"
#include "xmalloc.h"
#include "stddebug.h"
#include "debug.h"

int language = 0x0409;

#define PrintId(name) \
    if (HIWORD((DWORD)name)) \
        dprintf_resource( stddeb, "'%s'", name); \
    else \
        dprintf_resource( stddeb, "#%04x", LOWORD(name)); 

/**********************************************************************
 *	    GetResDirEntry
 *
 *	Helper function - goes down one level of PE resource tree
 *
 */
PIMAGE_RESOURCE_DIRECTORY GetResDirEntry(PIMAGE_RESOURCE_DIRECTORY resdirptr,
					 LPCSTR name,
					 DWORD root)
{
    int entrynum;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY entryTable;

    if (HIWORD(name)) {
    /* FIXME: what about #xxx names? */
	entryTable = (PIMAGE_RESOURCE_DIRECTORY_ENTRY) (
			(BYTE *) resdirptr + 
                        sizeof(IMAGE_RESOURCE_DIRECTORY));
	for (entrynum = 0; entrynum < resdirptr->NumberOfNamedEntries; entrynum++)
	    /* do what??? */ ;
	return NULL;
    } else {
	entryTable = (PIMAGE_RESOURCE_DIRECTORY_ENTRY) (
			(BYTE *) resdirptr + 
                        sizeof(IMAGE_RESOURCE_DIRECTORY) +
			resdirptr->NumberOfNamedEntries * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));
	for (entrynum = 0; entrynum < resdirptr->NumberOfIdEntries; entrynum++)
	    if ((DWORD)entryTable[entrynum].Name == (DWORD)name)
		return (PIMAGE_RESOURCE_DIRECTORY) (
			root +
			(entryTable[entrynum].OffsetToData & 0x7fffffff));
	return NULL;
    }
}

/**********************************************************************
 *	    FindResource    (KERNEL.60)
 */
HANDLE32 FindResource32( HINSTANCE hModule, LPCTSTR name, LPCTSTR type )
{
#ifndef WINELIB
    struct w_files *wptr = wine_files;
    PIMAGE_RESOURCE_DIRECTORY resdirptr;
    DWORD root;

#if 0
    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
#endif
    dprintf_resource(stddeb, "FindResource: module=%08x type=", hModule );
    PrintId( type );
    dprintf_resource( stddeb, " name=" );
    PrintId( name );
    dprintf_resource( stddeb, "\n" );
    while (wptr != NULL && (wptr->hModule != hModule))
	wptr = wptr->next;
    if ((wptr == NULL) || (wptr->pe == NULL) || (wptr->pe->pe_resource == NULL))
	return 0;
    resdirptr = (PIMAGE_RESOURCE_DIRECTORY) wptr->pe->pe_resource;
    root = (DWORD) resdirptr;
    if ((resdirptr = GetResDirEntry(resdirptr, type, root)) == NULL)
	return 0;
    if ((resdirptr = GetResDirEntry(resdirptr, name, root)) == NULL)
	return 0;
    return (HANDLE32) GetResDirEntry(resdirptr, (LPCSTR)language, root);
#else
    return LIBRES_FindResource( hModule, name, type );
#endif
}


/**********************************************************************
 *	    LoadResource    (KERNEL.61)
 */
HANDLE32 LoadResource32( HINSTANCE hModule, HANDLE32 hRsrc )
{
#ifndef WINELIB
    struct w_files *wptr = wine_files;

#if 0
    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
#endif
    dprintf_resource(stddeb, "LoadResource: module="NPFMT" res="NPFMT"\n",
                     hModule, hRsrc );
    if (!hRsrc) return 0;
    while (wptr != NULL && (wptr->hModule != hModule))
	wptr = wptr->next;
    if ((wptr == NULL) || (wptr->pe == NULL) || (wptr->pe->pe_resource == NULL))
	return 0;
    return (HANDLE32) (wptr->load_addr+((PIMAGE_RESOURCE_DATA_ENTRY)hRsrc)->OffsetToData);
#else
    return LIBRES_LoadResource( hModule, hRsrc );
#endif
}


/**********************************************************************
 *	    LockResource    (KERNEL.62)
 */
LPVOID LockResource32( HANDLE32 handle )
{
	return (LPVOID) handle;
}

/**********************************************************************
 *	    FreeResource    (KERNEL.63)
 */
BOOL FreeResource32( HANDLE32 handle )
{
    /* no longer used in Win32 */
    return TRUE;
}

/**********************************************************************
 *	    AccessResource    (KERNEL.64)
 */
INT AccessResource32( HINSTANCE hModule, HRSRC hRsrc )
{
    WORD *pModule;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "AccessResource: module="NPFMT" res="NPFMT"\n",
                     hModule, hRsrc );
    if (!hRsrc) return 0;
    if (!(pModule = (WORD *)GlobalLock( hModule ))) return 0;
#ifndef WINELIB
    switch(*pModule)
    {
      case NE_SIGNATURE:
        return NE_AccessResource( hModule, hRsrc );
      case PE_SIGNATURE:
        return 0;
      default:
        return 0;
    }
#else
        return LIBRES_AccessResource( hModule, hRsrc );
#endif
}


/**********************************************************************
 *	    SizeofResource    (KERNEL.65)
 */
DWORD SizeofResource32( HINSTANCE hModule, HRSRC hRsrc )
{
    WORD *pModule;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "SizeofResource: module="NPFMT" res="NPFMT"\n",
                     hModule, hRsrc );
    if (!(pModule = (WORD *)GlobalLock( hModule ))) return 0;
#ifndef WINELIB
    switch(*pModule)
    {
      case NE_SIGNATURE:
        return NE_SizeofResource( hModule, hRsrc );
      case PE_SIGNATURE:
        return 0;
      default:
        return 0;
    }
#else
    return LIBRES_SizeofResource( hModule, hRsrc );
#endif
}

/**********************************************************************
 *			LoadAccelerators	[USER.177]
*/
HANDLE LoadAccelerators32(HINSTANCE instance, LPCTSTR lpTableName)
{
    HANDLE 	hAccel;
    HANDLE 	rsc_mem;
    HRSRC hRsrc;
    BYTE 	*lp;
    ACCELHEADER	*lpAccelTbl;
    int 	i, n;

    if (HIWORD(lpTableName))
        dprintf_accel( stddeb, "LoadAccelerators: "NPFMT" '%s'\n",
                      instance, (char *)( lpTableName ) );
    else
        dprintf_accel( stddeb, "LoadAccelerators: "NPFMT" %04x\n",
                       instance, LOWORD(lpTableName) );

    if (!(hRsrc = FindResource( instance, lpTableName, RT_ACCELERATOR )))
      return 0;
    if (!(rsc_mem = LoadResource( instance, hRsrc ))) return 0;

    lp = (BYTE *)LockResource(rsc_mem);
    n = SizeofResource( instance, hRsrc ) / sizeof(ACCELENTRY);
    hAccel = GlobalAlloc(GMEM_MOVEABLE, 
    	sizeof(ACCELHEADER) + (n + 1)*sizeof(ACCELENTRY));
    lpAccelTbl = (LPACCELHEADER)GlobalLock(hAccel);
    lpAccelTbl->wCount = 0;
    for (i = 0; i < n; i++) {
	lpAccelTbl->tbl[i].type = *(lp++);
	lpAccelTbl->tbl[i].wEvent = *((WORD *)lp);
	lp += 2;
	lpAccelTbl->tbl[i].wIDval = *((WORD *)lp);
	lp += 2;
    	if (lpAccelTbl->tbl[i].wEvent == 0) break;
	dprintf_accel(stddeb,
		"Accelerator #%u / event=%04X id=%04X type=%02X \n", 
		i, lpAccelTbl->tbl[i].wEvent, lpAccelTbl->tbl[i].wIDval, 
		lpAccelTbl->tbl[i].type);
	lpAccelTbl->wCount++;
 	}
    GlobalUnlock(hAccel);
    FreeResource( rsc_mem );
    return hAccel;
}

/**********************************************************************
 *					LoadString
 */
int
LoadString32(HINSTANCE instance, DWORD resource_id, LPTSTR buffer, int buflen)
{
    HANDLE32 hmem, hrsrc;
    WCHAR *p;
    int string_num;
    int i;

    dprintf_resource(stddeb, "LoadString: instance = "NPFMT", id = %04x, buffer = %08x, "
	   "length = %d\n", instance, resource_id, (int) buffer, buflen);

    hrsrc = FindResource32( instance, (resource_id>>4)+1, RT_STRING );
    if (!hrsrc) return 0;
    hmem = LoadResource32( instance, hrsrc );
    if (!hmem) return 0;
    
    p = LockResource32(hmem);
    string_num = resource_id & 0x000f;
    for (i = 0; i < string_num; i++)
	p += *p + 1;
    
    dprintf_resource( stddeb, "strlen = %d\n", (int)*p );
    
    i = MIN(buflen - 1, *p);
    if (buffer == NULL)
	return i;
    if (i > 0) {
	memcpy(buffer, p + 1, i * sizeof (WCHAR));
	buffer[i] = (WCHAR) 0;
    } else {
	if (buflen > 1) {
	    buffer[0] = (WCHAR) 0;
	    return 0;
	}
	fprintf(stderr,"LoadString // I dont know why , but caller give buflen=%d *p=%d !\n", buflen, *p);
	fprintf(stderr,"LoadString // and try to obtain string '%s'\n", p + 1);
    }
    dprintf_resource(stddeb,"LoadString // '%s' copied !\n", buffer);
    return i;
}

/**********************************************************************
 *					LoadStringA
 */
int
LoadStringA32(HINSTANCE instance, DWORD resource_id, LPSTR buffer, int buflen)
{
    WCHAR *buffer2 = xmalloc(buflen*2);
    int retval = LoadString32(instance, resource_id, buffer2, buflen);

    while (*buffer2) 
	*buffer++ = (char) *buffer2++;
    *buffer = 0;
    return retval;
}

HICON LoadIconW32(HINSTANCE hisnt, LPCTSTR lpszIcon)

{
	return LoadIcon(0, IDI_APPLICATION);
}

HICON LoadIconA32(HINSTANCE hinst, LPCTSTR lpszIcon)

{
	return LoadIconW32(hinst, lpszIcon);
}
/**********************************************************************
 *	    LoadBitmapW
 */
HBITMAP LoadBitmapW32( HANDLE instance, LPCTSTR name )
{
    HBITMAP hbitmap = 0;
    HDC hdc;
    HANDLE32 hRsrc;
    HANDLE32 handle;
    BITMAPINFO *info;

    if (!instance)  /* OEM bitmap */
    {
        if (HIWORD((int)name)) return 0;
        return OBM_LoadBitmap( LOWORD((int)name) );
    }

    if (!(hRsrc = FindResource32( instance, name, RT_BITMAP ))) return 0;
    if (!(handle = LoadResource32( instance, hRsrc ))) return 0;

    info = (BITMAPINFO *)LockResource32( handle );
    if ((hdc = GetDC(0)) != 0)
    {
        char *bits = (char *)info + DIB_BitmapInfoSize( info, DIB_RGB_COLORS );
        hbitmap = CreateDIBitmap( hdc, &info->bmiHeader, CBM_INIT,
                                  bits, info, DIB_RGB_COLORS );
        ReleaseDC( 0, hdc );
    }
    return hbitmap;
}
/**********************************************************************
 *	    LoadBitmapA
 */
HBITMAP LoadBitmapA32( HANDLE instance, LPCTSTR name )
{
    return LoadBitmapW32(instance, name);
}
