/*
 * Win32 Resources
 *
 * Copyright 1995 Thomas Sandford
 * Copyright 1996 Martin von Loewis
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
#include "module.h"
#include "handle32.h"
#include "libres.h"
#include "resource32.h"
#include "stackframe.h"
#include "neexe.h"
#include "accel.h"
#include "xmalloc.h"
#include "string32.h"
#include "stddebug.h"
#include "debug.h"

int language = 0x0409;

#define PrintIdA(name) \
    if (HIWORD((DWORD)name)) \
        dprintf_resource( stddeb, "'%s'", name); \
    else \
        dprintf_resource( stddeb, "#%04x", LOWORD(name)); 
#define PrintIdW(name)
#define PrintId(name)

/**********************************************************************
 *	    GetResDirEntryW
 *
 *	Helper function - goes down one level of PE resource tree
 *
 */
PIMAGE_RESOURCE_DIRECTORY GetResDirEntryW(PIMAGE_RESOURCE_DIRECTORY resdirptr,
					 LPCWSTR name,
					 DWORD root)
{
    int entrynum;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY entryTable;
	int namelen;

    if (HIWORD(name)) {
    /* FIXME: what about #xxx names? */
	entryTable = (PIMAGE_RESOURCE_DIRECTORY_ENTRY) (
			(BYTE *) resdirptr + 
                        sizeof(IMAGE_RESOURCE_DIRECTORY));
	namelen = lstrlen32W(name);
	for (entrynum = 0; entrynum < resdirptr->NumberOfNamedEntries; entrynum++)
	{
		PIMAGE_RESOURCE_DIR_STRING_U str =
		(PIMAGE_RESOURCE_DIR_STRING_U) (root + 
			(entryTable[entrynum].Name & 0x7fffffff));
		if(namelen != str->Length)
			continue;
		if(lstrncmpi32W(name,str->NameString,str->Length)==0)
			return (PIMAGE_RESOURCE_DIRECTORY) (
				root +
				(entryTable[entrynum].OffsetToData & 0x7fffffff));
	}
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
 *	    GetResDirEntryA
 *
 *	Helper function - goes down one level of PE resource tree
 *
 */
PIMAGE_RESOURCE_DIRECTORY GetResDirEntryA(PIMAGE_RESOURCE_DIRECTORY resdirptr,
					 LPCSTR name,
					 DWORD root)
{
	LPWSTR				xname;
	PIMAGE_RESOURCE_DIRECTORY	ret;

	if (HIWORD((DWORD)name))
		xname	= STRING32_DupAnsiToUni(name);
	else
		xname	= (LPWSTR)name;

	ret=GetResDirEntryW(resdirptr,xname,root);
	if (HIWORD((DWORD)name))
		free(xname);
	return ret;
}

/**********************************************************************
 *	    FindResourceA    (KERNEL32.128)
 */
HANDLE32 FindResource32A( HINSTANCE hModule, LPCSTR name, LPCSTR type ) {
	LPWSTR		xname,xtype;
	HANDLE32	ret;

	if (HIWORD((DWORD)name))
		xname = STRING32_DupAnsiToUni(name);
	else
		xname = (LPWSTR)name;
	if (HIWORD((DWORD)type))
		xtype = STRING32_DupAnsiToUni(type);
	else
		xtype = (LPWSTR)type;
	ret=FindResource32W(hModule,xname,xtype);
	if (HIWORD((DWORD)name))
		free(xname);
	if (HIWORD((DWORD)type))
		free(xtype);
	return ret;
}

/**********************************************************************
 *	    FindResourceW    (KERNEL32.131)
 */
HANDLE32 FindResource32W( HINSTANCE hModule, LPCWSTR name, LPCWSTR type )
{
#ifndef WINELIB
    PE_MODULE *pe;
    NE_MODULE *pModule;
    PIMAGE_RESOURCE_DIRECTORY resdirptr;
    DWORD root;
	HANDLE32 result;

    /* Sometimes we get passed hModule = 0x00000000. FIXME: is GetTaskDS()
     * ok?
     */
    if (!hModule) hModule = GetTaskDS();
    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "FindResource: module=%08x type=", hModule );
    PrintId( type );
    dprintf_resource( stddeb, " name=" );
    PrintId( name );
    dprintf_resource( stddeb, "\n" );
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (!(pModule->flags & NE_FFLAGS_WIN32)) return 0;  /* FIXME? */
    if (!(pe = pModule->pe_module) || !pe->pe_resource) return 0;

    resdirptr = (PIMAGE_RESOURCE_DIRECTORY) pe->pe_resource;
    root = (DWORD) resdirptr;
    if ((resdirptr = GetResDirEntryW(resdirptr, type, root)) == NULL)
	return 0;
    if ((resdirptr = GetResDirEntryW(resdirptr, name, root)) == NULL)
	return 0;
    result = (HANDLE32)GetResDirEntryW(resdirptr, (LPCWSTR)language, root);
	/* Try LANG_NEUTRAL, too */
    if(!result)
        return (HANDLE32)GetResDirEntryW(resdirptr, (LPCWSTR)0, root);
    return result;
    
#else
    return LIBRES_FindResource( hModule, name, type );
#endif
}


/**********************************************************************
 *	    LoadResource    (KERNEL32.370)
 */
HANDLE32 LoadResource32( HINSTANCE hModule, HANDLE32 hRsrc )
{
#ifndef WINELIB
    NE_MODULE *pModule;
    PE_MODULE *pe;

    if (!hModule) hModule = GetTaskDS(); /* FIXME: see FindResource32W */
    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "LoadResource: module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!hRsrc) return 0;

    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (!(pModule->flags & NE_FFLAGS_WIN32)) return 0;  /* FIXME? */
    if (!(pe = pModule->pe_module) || !pe->pe_resource) return 0;
    return (HANDLE32) (pe->load_addr+((PIMAGE_RESOURCE_DATA_ENTRY)hRsrc)->OffsetToData);
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
    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "AccessResource: module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!hRsrc) return 0;
	fprintf(stderr,"AccessResource32: not implemented\n");
	return 0;
}


/**********************************************************************
 *	    SizeofResource    (KERNEL.65)
 */
DWORD SizeofResource32( HINSTANCE hModule, HRSRC hRsrc )
{
    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "SizeofResource: module=%04x res=%04x\n",
                     hModule, hRsrc );
	fprintf(stderr,"SizeofResource32: not implemented\n");
	return 0;
}

/**********************************************************************
 *			LoadAccelerators	[USER.177]
*/
HANDLE32 WIN32_LoadAcceleratorsW(HINSTANCE instance, LPCWSTR lpTableName)
{
#if 0
    HANDLE32 	hAccel;
    HANDLE32 	rsc_mem;
    HANDLE32 hRsrc;
    BYTE 	*lp;
    ACCELHEADER	*lpAccelTbl;
    int 	i, n;

    if (HIWORD(lpTableName))
        dprintf_accel( stddeb, "LoadAccelerators: %04x '%s'\n",
                      instance, (char *)( lpTableName ) );
    else
        dprintf_accel( stddeb, "LoadAccelerators: %04x %04x\n",
                       instance, LOWORD(lpTableName) );

    if (!(hRsrc = FindResource32( instance, lpTableName, 
		(LPCWSTR)RT_ACCELERATOR )))
      return 0;
    if (!(rsc_mem = LoadResource32( instance, hRsrc ))) return 0;

    lp = (BYTE *)LockResource32(rsc_mem);
    n = SizeofResource( instance, hRsrc ) / sizeof(ACCELENTRY);
    hAccel = GlobalAlloc16(GMEM_MOVEABLE, 
    	sizeof(ACCELHEADER) + (n + 1)*sizeof(ACCELENTRY));
    lpAccelTbl = (LPACCELHEADER)GlobalLock16(hAccel);
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
    GlobalUnlock16(hAccel);
    FreeResource( rsc_mem );
    return hAccel;
#else
	fprintf(stderr,"LoadAcceleratorsW: not implemented\n");
	return 0x100;  /* Return something anyway */
#endif
}

HANDLE32 WIN32_LoadAcceleratorsA(HINSTANCE instance, LPCSTR lpTableName)
{
	LPWSTR		uni;
	HANDLE32	result;
	if (HIWORD(lpTableName))
		uni=STRING32_DupAnsiToUni(lpTableName);
	else
		uni=(LPWSTR)lpTableName;
	result=WIN32_LoadAcceleratorsW(instance,uni);
	if (HIWORD(uni))
		free(uni);
	return result;
}

/**********************************************************************
 *					LoadString
 */
int
WIN32_LoadStringW(HINSTANCE instance, DWORD resource_id, LPWSTR buffer, int buflen)
{
    HANDLE32 hmem, hrsrc;
    WCHAR *p;
    int string_num;
    int i;

    dprintf_resource(stddeb, "LoadString: instance = %04x, id = %04x, buffer = %08x, "
	   "length = %d\n", instance, (int)resource_id, (int) buffer, buflen);

    hrsrc = FindResource32W( instance, (LPCWSTR)((resource_id>>4)+1), 
		(LPCWSTR)RT_STRING );
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
#if 0
	fprintf(stderr,"LoadString // I dont know why , but caller give buflen=%d *p=%d !\n", buflen, *p);
	fprintf(stderr,"LoadString // and try to obtain string '%s'\n", p + 1);
#endif
    }
    dprintf_resource(stddeb,"LoadString // '%s' copied !\n", buffer);
    return i;
}

/**********************************************************************
 *					LoadStringA
 */
int
WIN32_LoadStringA(HINSTANCE instance, DWORD resource_id, LPSTR buffer, int buflen)
{
    WCHAR *buffer2 = xmalloc(buflen*2);
    int retval = WIN32_LoadStringW(instance, resource_id, buffer2, buflen);

    while (*buffer2) 
	*buffer++ = (char) *buffer2++;
    *buffer = 0;
    return retval;
}

HICON LoadIconW32(HINSTANCE hisnt, LPCWSTR lpszIcon)

{
	return LoadIcon(0, IDI_APPLICATION);
}

HICON LoadIconA32(HINSTANCE hinst, LPCSTR lpszIcon)

{
	return LoadIconW32(hinst, lpszIcon);
}
/**********************************************************************
 *	    LoadBitmapW
 */
HBITMAP WIN32_LoadBitmapW( HANDLE instance, LPCWSTR name )
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

    if (!(hRsrc = FindResource32W( instance, name, 
		(LPWSTR)RT_BITMAP ))) return 0;
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
HBITMAP WIN32_LoadBitmapA( HANDLE instance, LPCSTR name )
{
    HBITMAP res;
    if(!HIWORD(name))
        res = WIN32_LoadBitmapW(instance,(LPWSTR)name);
    else{
        LPWSTR uni=STRING32_DupAnsiToUni(name);
        res=WIN32_LoadBitmapW(instance,uni);
        free(uni);
    }
    return res;
}

/*****************************************************************
 *        LoadMenuW                 (USER32.372)
 */
HMENU WIN32_LoadMenuW(HANDLE instance, LPCWSTR name)
{
	HANDLE32 hrsrc;
	hrsrc=FindResource32W(instance,name,(LPWSTR)RT_MENU);
	if(!hrsrc)return 0;
        return LoadMenuIndirect32W( LoadResource32(instance, hrsrc) );
}

/*****************************************************************
 *        LoadMenuA                 (USER32.370)
 */
HMENU WIN32_LoadMenuA(HANDLE instance,LPCSTR name)
{
	HMENU res;
	if(!HIWORD(name))
		res = WIN32_LoadMenuW(instance,(LPWSTR)name);
	else{
		LPWSTR uni=STRING32_DupAnsiToUni(name);
		res=WIN32_LoadMenuW(instance,uni);
		free(uni);
	}
	return res;
}
