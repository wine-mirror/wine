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

#if 0
#define PrintId(name) \
    if (HIWORD((DWORD)name)) \
        dprintf_resource( stddeb, "'%s'", name); \
    else \
        dprintf_resource( stddeb, "#%04x", LOWORD(name)); 
#else
#define PrintId(name)
#endif

/**********************************************************************
 *	    GetResDirEntry
 *
 *	Helper function - goes down one level of PE resource tree
 *
 */
PIMAGE_RESOURCE_DIRECTORY GetResDirEntry(PIMAGE_RESOURCE_DIRECTORY resdirptr,
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
	namelen = STRING32_lstrlenW(name);
	for (entrynum = 0; entrynum < resdirptr->NumberOfNamedEntries; entrynum++)
	{
		PIMAGE_RESOURCE_DIR_STRING_U str =
		(PIMAGE_RESOURCE_DIR_STRING_U) (root + 
			(entryTable[entrynum].Name & 0x7fffffff));
		if(namelen != str->Length)
			continue;
		if(STRING32_lstrcmpniW(name,str->NameString,str->Length)==0)
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
 *	    FindResource    (KERNEL.60)
 */
HANDLE32 FindResource32( HINSTANCE hModule, LPCWSTR name, LPCWSTR type )
{
#ifndef WINELIB
    PE_MODULE *pe;
    NE_MODULE *pModule;
    PIMAGE_RESOURCE_DIRECTORY resdirptr;
    DWORD root;
	HANDLE32 result;

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
    if ((resdirptr = GetResDirEntry(resdirptr, type, root)) == NULL)
	return 0;
    if ((resdirptr = GetResDirEntry(resdirptr, name, root)) == NULL)
	return 0;
    result = GetResDirEntry(resdirptr, (LPCWSTR)language, root);
	/* Try LANG_NEUTRAL, too */
    if(!result)
        return GetResDirEntry(resdirptr, (LPCWSTR)0, root);
    return result;
    
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
    NE_MODULE *pModule;
    PE_MODULE *pe;

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
#else
	fprintf(stderr,"LoadAcceleratorsW: not implemented\n");
	return 0;
#endif
}

HANDLE32 WIN32_LoadAcceleratorsA(HINSTANCE instance, LPCSTR lpTableName)
{
	LPWSTR uni=STRING32_DupAnsiToUni(lpTableName);
	HANDLE32 result=WIN32_LoadAcceleratorsW(instance,uni);
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

    hrsrc = FindResource32( instance, (LPCWSTR)((resource_id>>4)+1), 
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

    if (!(hRsrc = FindResource32( instance, name, 
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

/**********************************************************************
 *      WIN32_ParseMenu
 *      LoadMenu helper function
 */
BYTE* WIN32_ParseMenu(HMENU hMenu,BYTE *it)
{
    char entry[200]; /* buffer for ANSI names */
	int bufsize=100;
	int len;
	WORD flags;
	WORD wMenuID;
	WCHAR *utext;
	do{
		flags=*(WORD*)it;
		it+=sizeof(WORD);
		/* POPUP entries have no ID, but a sub menu */
		if(flags & MF_POPUP)
		{
			wMenuID = CreatePopupMenu();
			len = STRING32_lstrlenW((LPWSTR)it);
			utext = (WCHAR*)it;
			it += sizeof(WCHAR)*(len+1);
			it = WIN32_ParseMenu(wMenuID,it);
		} else {
			wMenuID=*(WORD*)it;
			it+=sizeof(WORD);
			utext = (LPWSTR)it;
			len = STRING32_lstrlenW((LPWSTR)it);
			it += sizeof(WCHAR)*(len+1);
			if(!wMenuID && !*utext)
				flags |= MF_SEPARATOR;
		}
		if(len>=bufsize) continue;  /* hack hack */
		STRING32_UniToAnsi(entry,utext);
		AppendMenu(hMenu,flags,wMenuID,MAKE_SEGPTR(entry));
	}while(!(flags & MF_END));
	return it;
}

/*****************************************************************
 *        LoadMenuIndirectW         (USER32.371)
 */
HMENU WIN32_LoadMenuIndirectW(void *menu)
{
	BYTE *it=menu;
	HMENU hMenu = CreateMenu();
	/*skip menu header*/
	if(*(DWORD*)it)
		fprintf(stderr,"Unknown menu header\n");
	it+=2*sizeof(WORD);
	WIN32_ParseMenu(hMenu,it);
	return hMenu;
}

/*****************************************************************
 *        LoadMenuW                 (USER32.372)
 */
HMENU WIN32_LoadMenuW(HANDLE instance, LPCWSTR name)
{
	HANDLE32 hrsrc;
	hrsrc=FindResource32(instance,name,(LPWSTR)RT_MENU);
	if(!hrsrc)return 0;
	return WIN32_LoadMenuIndirectW(LoadResource32(instance, hrsrc));
}

/*****************************************************************
 *        LoadMenuIndirectA         (USER32.370)
 */
HMENU WIN32_LoadMenuIndirectA(void *menu)
{
	fprintf(stderr,"WIN32_LoadMenuIndirectA not implemented\n");
	return 0;
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
