/*
 * Resources
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "arch.h"
#include "windows.h"
#include "gdi.h"
#include "global.h"
#include "heap.h"
#include "neexe.h"
#include "task.h"
#include "accel.h"
#include "module.h"
#include "resource.h"
#include "stddebug.h"
#include "debug.h"
#include "libres.h"

#define PrintId(name) \
    if (HIWORD((DWORD)name)) \
        dprintf_resource( stddeb, "'%s'", (char *)PTR_SEG_TO_LIN(name)); \
    else \
        dprintf_resource( stddeb, "#%04x", LOWORD(name)); 

extern WORD WINE_LanguageId;

/* error message when 16-bit resource function is called for Win32 module */
static const char* NEWin32FailureString = "fails with Win32 module\n";
/* error message when 32-bit resource function is called for Win16 module */
static const char* PEWin16FailureString = "fails with Win16 module\n";

/**********************************************************************
 *	    FindResource16    (KERNEL.60)
 */
HRSRC16 WINAPI FindResource16( HMODULE16 hModule, SEGPTR name, SEGPTR type )
{
    NE_MODULE *pModule;

    hModule = MODULE_HANDLEtoHMODULE16( hModule ); 
    dprintf_resource(stddeb, "FindResource16: module=%04x type=", hModule );
    PrintId( type );

    if (HIWORD(name))  /* Check for '#xxx' name */
    {
	char *ptr = PTR_SEG_TO_LIN( name );
	if (ptr[0] == '#') {
	    if (!(name = (SEGPTR)atoi( ptr + 1 ))) return 0;
	}
    }

    dprintf_resource( stddeb, " name=" );
    PrintId( name );
    dprintf_resource( stddeb, "\n" );

    if ((pModule = MODULE_GetPtr( hModule )))
    {
#ifndef WINELIB
	if (pModule->flags & NE_FFLAGS_WIN32)
	    fprintf(stderr,"FindResource16: %s", NEWin32FailureString);
	else
	    return NE_FindResource( hModule, type, name );
#else
	return LIBRES_FindResource16( hModule, name, type );
#endif
    }
    return 0;
}


/**********************************************************************
 *	    FindResource32A    (KERNEL32.128)
 */
HANDLE32 WINAPI FindResource32A( HINSTANCE32 hModule, LPCSTR name, LPCSTR type)
{
    return FindResourceEx32A(hModule,name,type,WINE_LanguageId);
}

/**********************************************************************
 *	    FindResourceEx32A    (KERNEL32.129)
 */
HANDLE32 WINAPI FindResourceEx32A( HINSTANCE32 hModule, LPCSTR name,
                                   LPCSTR type, WORD lang )
{
    LPWSTR xname,xtype;
    HANDLE32 ret;

    if (HIWORD((DWORD)name))
        xname = HEAP_strdupAtoW( GetProcessHeap(), 0, name );
    else
        xname = (LPWSTR)name;
    if (HIWORD((DWORD)type))
        xtype = HEAP_strdupAtoW( GetProcessHeap(), 0, type);
    else
        xtype = (LPWSTR)type;
    ret = FindResourceEx32W( hModule, xname, xtype, lang );
    if (HIWORD((DWORD)name)) HeapFree( GetProcessHeap(), 0, xname );
    if (HIWORD((DWORD)type)) HeapFree( GetProcessHeap(), 0, xtype );
    return ret;
}


/**********************************************************************
 *	    FindResourceEx32W    (KERNEL32.130)
 */
HRSRC32 WINAPI FindResourceEx32W( HINSTANCE32 hModule, LPCWSTR name,
                                  LPCWSTR type, WORD lang )
{
#ifndef WINELIB
    NE_MODULE *pModule;

    if (!hModule) hModule = GetTaskDS();
    hModule = MODULE_HANDLEtoHMODULE32( hModule );
    dprintf_resource(stddeb, "FindResource32W: module=%08x type=", hModule );
    if (HIWORD(type))
    	dprintf_resource(stddeb,"%p",type);
    else
	dprintf_resource(stddeb,"#%p",type);
    dprintf_resource( stddeb, " name=" );
    if (HIWORD(name))
    	dprintf_resource(stddeb,"%p",name);
    else
	dprintf_resource(stddeb,"#%p",name);
    dprintf_resource( stddeb, "\n" );
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (!(pModule->flags & NE_FFLAGS_WIN32)) return 0;
    return PE_FindResourceEx32W(hModule,name,type,lang);
#else
    return LIBRES_FindResource32( hModule, name, type );
#endif
}

/**********************************************************************
 *	    FindResource32W    (KERNEL32.131)
 */
HRSRC32 WINAPI FindResource32W(HINSTANCE32 hModule, LPCWSTR name, LPCWSTR type)
{
    return FindResourceEx32W(hModule,name,type,WINE_LanguageId);
}


/**********************************************************************
 *	    LoadResource16    (KERNEL.61)
 */
HGLOBAL16 WINAPI LoadResource16( HMODULE16 hModule, HRSRC16 hRsrc )
{
    NE_MODULE *pModule;

    hModule = MODULE_HANDLEtoHMODULE16( hModule );
    dprintf_resource(stddeb, "LoadResource16: module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!hRsrc) return 0;
    if ((pModule = MODULE_GetPtr( hModule )))
    {
#ifndef WINELIB
	if (pModule->flags & NE_FFLAGS_WIN32)
	    fprintf(stderr,"LoadResource16: %s", NEWin32FailureString);
	else
	    return NE_LoadResource( hModule, hRsrc );
#else
	return LIBRES_LoadResource( hModule, hRsrc );
#endif
    }
    return 0;
}

/**********************************************************************
 *	    LoadResource32    (KERNEL32.370)
 */
HGLOBAL32 WINAPI LoadResource32( HINSTANCE32 hModule, HRSRC32 hRsrc )
{
#ifndef WINELIB
    NE_MODULE *pModule;

    if (!hModule) hModule = GetTaskDS(); /* FIXME: see FindResource32W */
    hModule = MODULE_HANDLEtoHMODULE32( hModule );
    dprintf_resource(stddeb, "LoadResource32: module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!hRsrc) return 0;

    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (!(pModule->flags & NE_FFLAGS_WIN32))
    {
    	fprintf(stderr,"LoadResource32: %s", PEWin16FailureString );
   	return 0;  /* FIXME? */
    }
    return PE_LoadResource32(hModule,hRsrc);
#else
    return LIBRES_LoadResource( hModule, hRsrc );
#endif
}


/**********************************************************************
 *	    LockResource    (KERNEL.62)
 */
/* 16-bit version */
SEGPTR WINAPI WIN16_LockResource16(HGLOBAL16 handle)
{
#ifndef WINELIB
    HMODULE16 hModule;
    NE_MODULE *pModule;

    dprintf_resource(stddeb, "LockResource: handle=%04x\n", handle );
    if (!handle) return (SEGPTR)0;
    hModule = MODULE_HANDLEtoHMODULE16( handle );
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (pModule->flags & NE_FFLAGS_WIN32)
    {
        fprintf(stderr,"LockResource16: %s", NEWin32FailureString);
        return 0;
    }
    return NE_LockResource( hModule, handle );
#else
    return LIBRES_LockResource( handle );
#endif
}

/* WINELIB 16-bit version */
LPVOID WINAPI LockResource16( HGLOBAL16 handle )
{
#ifndef WINELIB
    HMODULE16 hModule;
    NE_MODULE *pModule;

    dprintf_resource(stddeb, "LockResource: handle=%04x\n", handle );
    if (!handle) return NULL;
    hModule = MODULE_HANDLEtoHMODULE16( handle );
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (pModule->flags & NE_FFLAGS_WIN32)
    {
        fprintf(stderr,"LockResource16: %s", NEWin32FailureString);
        return 0;
    }
    return (LPSTR)PTR_SEG_TO_LIN( NE_LockResource( hModule, handle ) );
#else
    return LIBRES_LockResource( handle );
#endif
}


/**********************************************************************
 *	    LockResource32    (KERNEL32.384)
 */
LPVOID WINAPI LockResource32( HGLOBAL32 handle )
{
    return (LPVOID)handle;
}


/**********************************************************************
 *	    FreeResource16    (KERNEL.63)
 */
BOOL16 WINAPI FreeResource16( HGLOBAL16 handle )
{
#ifndef WINELIB
    HMODULE16 hModule;
    NE_MODULE *pModule;

    dprintf_resource(stddeb, "FreeResource16: handle=%04x\n", handle );
    if (!handle) return FALSE;
    hModule = MODULE_HANDLEtoHMODULE16( handle );
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (pModule->flags & NE_FFLAGS_WIN32)
    {
        fprintf(stderr,"FreeResource16: %s", NEWin32FailureString);
        return 0;
    }
    return NE_FreeResource( hModule, handle );
#else
    return LIBRES_FreeResource( handle );
#endif
}

/**********************************************************************
 *	    FreeResource32    (KERNEL32.145)
 */
BOOL32 WINAPI FreeResource32( HGLOBAL32 handle )
{
    /* no longer used in Win32 */
    return TRUE;
}


/**********************************************************************
 *	    AccessResource16    (KERNEL.64)
 */
INT16 WINAPI AccessResource16( HINSTANCE16 hModule, HRSRC16 hRsrc )
{
    NE_MODULE *pModule;

    hModule = MODULE_HANDLEtoHMODULE16( hModule );
    dprintf_resource(stddeb, "AccessResource16: module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!hRsrc) return 0;
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
#ifndef WINELIB
    if (pModule->flags & NE_FFLAGS_WIN32)
    {
        fprintf(stderr,"AccessResource16: %s", NEWin32FailureString);
        return 0;
    }
    return NE_AccessResource( hModule, hRsrc );
#else
    return LIBRES_AccessResource( hModule, hRsrc );
#endif
}


/**********************************************************************
 *	    AccessResource32    (KERNEL32.64)
 */
INT32 WINAPI AccessResource32( HINSTANCE32 hModule, HRSRC32 hRsrc )
{
    hModule = MODULE_HANDLEtoHMODULE32( hModule );
    dprintf_resource(stddeb, "AccessResource: module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!hRsrc) return 0;
    fprintf(stderr,"AccessResource32: not implemented\n");
    return 0;
}


/**********************************************************************
 *	    SizeofResource16    (KERNEL.65)
 */
DWORD WINAPI SizeofResource16( HMODULE16 hModule, HRSRC16 hRsrc )
{
    NE_MODULE *pModule;

    hModule = MODULE_HANDLEtoHMODULE16( hModule );
    dprintf_resource(stddeb, "SizeofResource16: module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
#ifndef WINELIB
    if (pModule->flags & NE_FFLAGS_WIN32)
    {
        fprintf(stderr,"SizeOfResource16: %s", NEWin32FailureString);
        return 0;
    }
    return NE_SizeofResource( hModule, hRsrc );
#else
    return LIBRES_SizeofResource( hModule, hRsrc );
#endif
}


/**********************************************************************
 *	    SizeofResource32    (KERNEL32.522)
 */
DWORD WINAPI SizeofResource32( HINSTANCE32 hModule, HRSRC32 hRsrc )
{
    hModule = MODULE_HANDLEtoHMODULE32( hModule );
    dprintf_resource(stddeb, "SizeofResource32: module=%04x res=%04x\n",
                     hModule, hRsrc );
#ifndef WINELIB
    return PE_SizeofResource32(hModule,hRsrc);
#else
    fprintf(stderr,"SizeofResource32: not implemented\n");
    return 0;
#endif
}


/**********************************************************************
 *	    AllocResource16    (KERNEL.66)
 */
HGLOBAL16 WINAPI AllocResource16( HMODULE16 hModule, HRSRC16 hRsrc, DWORD size)
{
    NE_MODULE *pModule;

    hModule = MODULE_HANDLEtoHMODULE16( hModule );
    dprintf_resource(stddeb, "AllocResource: module=%04x res=%04x size=%ld\n",
                     hModule, hRsrc, size );
    if (!hRsrc) return 0;
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
#ifndef WINELIB
    if (pModule->flags & NE_FFLAGS_WIN32)
    {
        fprintf(stderr,"AllocResource16: %s", NEWin32FailureString);
        return 0;
    }
    return NE_AllocResource( hModule, hRsrc, size );
#else
    return LIBRES_AllocResource( hModule, hRsrc, size );
#endif
}

/**********************************************************************
 *      DirectResAlloc    (KERNEL.168)
 *
 * Check Schulman, p. 232 for details
 */
HGLOBAL16 WINAPI DirectResAlloc( HINSTANCE16 hInstance, WORD wType,
                                 UINT16 wSize )
{
    dprintf_resource(stddeb,"DirectResAlloc(%04x,%04x,%04x)\n",
                     hInstance, wType, wSize );
    hInstance = MODULE_HANDLEtoHMODULE16(hInstance);
    if(!hInstance)return 0;
    if(wType != 0x10)	/* 0x10 is the only observed value, passed from
                           CreateCursorIndirect. */
        fprintf(stderr, "DirectResAlloc: wType = %x\n", wType);
    return GLOBAL_Alloc(GMEM_MOVEABLE, wSize, hInstance, FALSE, FALSE, FALSE);
}


/**********************************************************************
 *			LoadAccelerators16	[USER.177]
 *
 * FIXME: this code leaks memory because HACCEL must be a result of LoadResource()
 *        (see TWIN for hints).
 */
HACCEL16 WINAPI LoadAccelerators16(HINSTANCE16 instance, SEGPTR lpTableName)
{
    HACCEL16 	hAccel;
    HGLOBAL16 	rsc_mem;
    HRSRC16 hRsrc;
    BYTE 	*lp;
    ACCELHEADER	*lpAccelTbl;
    int 	i, n;

    if (HIWORD(lpTableName))
        dprintf_accel( stddeb, "LoadAccelerators: %04x '%s'\n",
                      instance, (char *)PTR_SEG_TO_LIN( lpTableName ) );
    else
        dprintf_accel( stddeb, "LoadAccelerators: %04x %04x\n",
                       instance, LOWORD(lpTableName) );

    if (!(hRsrc = FindResource16( instance, lpTableName, RT_ACCELERATOR )))
      return 0;
    if (!(rsc_mem = LoadResource16( instance, hRsrc ))) return 0;

    lp = (BYTE *)LockResource16(rsc_mem);
    n = SizeofResource16(instance,hRsrc)/sizeof(ACCELENTRY);
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
    FreeResource16( rsc_mem );
    return hAccel;
}

/**********************************************************************
 *			LoadAccelerators32W	[USER.177]
 * The image layout seems to look like this (not 100% sure):
 * 00:	BYTE	type		type of accelerator
 * 01:	BYTE	pad		(to WORD boundary)
 * 02:	WORD	event
 * 04:	WORD	IDval		
 * 06:	WORD	pad		(to DWORD boundary)
 */
HACCEL32 WINAPI LoadAccelerators32W(HINSTANCE32 instance,LPCWSTR lpTableName)
{
    HACCEL32 	hAccel;
    HGLOBAL32 	rsc_mem;
    HRSRC32 hRsrc;
    BYTE 	*lp;
    ACCELHEADER	*lpAccelTbl;
    int 	i, n;

    if (HIWORD(lpTableName))
        dprintf_accel( stddeb, "LoadAccelerators: %04x '%s'\n",
                      instance, (char *)( lpTableName ) );
    else
        dprintf_accel( stddeb, "LoadAccelerators: %04x %04x\n",
                       instance, LOWORD(lpTableName) );

    if (!(hRsrc = FindResource32W( instance, lpTableName, 
		(LPCWSTR)RT_ACCELERATOR )))
      return 0;
    if (!(rsc_mem = LoadResource32( instance, hRsrc ))) return 0;

    lp = (BYTE *)LockResource32(rsc_mem);
    n = SizeofResource32(instance,hRsrc)/sizeof(ACCELENTRY);
    hAccel = GlobalAlloc16(GMEM_MOVEABLE, 
    	sizeof(ACCELHEADER) + (n + 1)*sizeof(ACCELENTRY));
    lpAccelTbl = (LPACCELHEADER)GlobalLock16(hAccel);
    lpAccelTbl->wCount = 0;
    for (i = 0; i < n; i++) {
	lpAccelTbl->tbl[i].type = *lp;
	lp += 2;
	lpAccelTbl->tbl[i].wEvent = *((WORD *)lp);
	lp += 2;
	lpAccelTbl->tbl[i].wIDval = *((WORD *)lp);
	lp += 4;
    	if (lpAccelTbl->tbl[i].wEvent == 0) break;
	dprintf_accel(stddeb,
		"Accelerator #%u / event=%04X id=%04X type=%02X \n", 
		i, lpAccelTbl->tbl[i].wEvent, lpAccelTbl->tbl[i].wIDval, 
		lpAccelTbl->tbl[i].type);
	lpAccelTbl->wCount++;
 	}
    GlobalUnlock16(hAccel);
    FreeResource32(rsc_mem);
    return hAccel;
}

HACCEL32 WINAPI LoadAccelerators32A(HINSTANCE32 instance,LPCSTR lpTableName)
{
	LPWSTR	 uni;
	HACCEL32 result;
	if (HIWORD(lpTableName))
		uni = HEAP_strdupAtoW( GetProcessHeap(), 0, lpTableName );
	else
		uni = (LPWSTR)lpTableName;
	result = LoadAccelerators32W(instance,uni);
	if (HIWORD(uni)) HeapFree( GetProcessHeap(), 0, uni);
	return result;
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

    dprintf_resource(stddeb,"LoadString: inst=%04x id=%04x buff=%08x len=%d\n",
                     instance, resource_id, (int) buffer, buflen);

    hrsrc = FindResource16( instance, (SEGPTR)((resource_id>>4)+1), RT_STRING );
    if (!hrsrc) return 0;
    hmem = LoadResource16( instance, hrsrc );
    if (!hmem) return 0;
    
    p = LockResource16(hmem);
    string_num = resource_id & 0x000f;
    for (i = 0; i < string_num; i++)
	p += *p + 1;
    
    dprintf_resource( stddeb, "strlen = %d\n", (int)*p );
    
    i = MIN(buflen - 1, *p);
    if (buffer == NULL)
	return i;
    if (i > 0) {
	memcpy(buffer, p + 1, i);
	buffer[i] = '\0';
    } else {
	if (buflen > 1) {
	    buffer[0] = '\0';
	    return 0;
	}
	fprintf(stderr,"LoadString // I dont know why , but caller give buflen=%d *p=%d !\n", buflen, *p);
	fprintf(stderr,"LoadString // and try to obtain string '%s'\n", p + 1);
    }
    FreeResource16( hmem );

    dprintf_resource(stddeb,"LoadString // '%s' copied !\n", buffer);
    return i;
}

/**********************************************************************
 *	LoadString32W		(USER32.375)
 */
INT32 WINAPI LoadString32W( HINSTANCE32 instance, UINT32 resource_id,
                            LPWSTR buffer, int buflen )
{
    HGLOBAL32 hmem;
    HRSRC32 hrsrc;
    WCHAR *p;
    int string_num;
    int i;

    if (HIWORD(resource_id)==0xFFFF) /* netscape 3 passes this */
	resource_id = (UINT32)(-((INT32)resource_id));
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
#if 0
    dprintf_resource(stddeb,"LoadString // '%s' copied !\n", buffer);
#endif
    return i;
}

/**********************************************************************
 *	LoadString32A	(USER32.374)
 */
INT32 WINAPI LoadString32A( HINSTANCE32 instance, UINT32 resource_id,
                            LPSTR buffer,int buflen )
{
    INT32 retval;
    LPWSTR buffer2 = NULL;
    if (buffer) buffer2 = HeapAlloc( GetProcessHeap(), 0, buflen * 2 );
    retval = LoadString32W(instance,resource_id,buffer2,buflen);

    if (buffer2)
    {
        lstrcpynWtoA( buffer, buffer2, buflen );
	HeapFree( GetProcessHeap(), 0, buffer2 );
    }
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
 * 2: WORD unknown (flags?)
 * 4: CHAR[len-4]
 * 	(stringentry i of a subentry refers to the ID 'firstentry+i')
 *
 * Yes, ANSI strings in win32 resources. Go figure.
 */

/**********************************************************************
 *	LoadMessage32A		(internal)
 */
INT32 LoadMessage32A( HINSTANCE32 instance, UINT32 id, WORD lang,
                      LPSTR buffer, int buflen )
{
    HGLOBAL32	hmem;
    HRSRC32	hrsrc;
    BYTE	*p;
    int		nrofentries,i,slen;
    struct	_subentry {
    	DWORD	firstentry;
	DWORD	lastentry;
	DWORD	offset;
    } *se;
    struct	_stringentry {
    	WORD	len;
	WORD	unknown;
	CHAR	str[1];
    } *stre;

    dprintf_resource(stddeb, "LoadMessage: instance = %04x, id = %04x, buffer = %08x, "
	   "length = %d\n", instance, (int)id, (int) buffer, buflen);

    /*FIXME: I am not sure about the '1' ... But I've only seen those entries*/
    hrsrc = FindResourceEx32W(instance,(LPWSTR)1,(LPCWSTR)RT_MESSAGELIST,lang);
    if (!hrsrc) return 0;
    hmem = LoadResource32( instance, hrsrc );
    if (!hmem) return 0;
    
    p = LockResource32(hmem);
    nrofentries = *(DWORD*)p;
    stre = NULL;
    se = (struct _subentry*)(p+4);
    for (i=nrofentries;i--;) {
    	if ((id>=se->firstentry) && (id<=se->lastentry)) {
	    stre = (struct _stringentry*)(p+se->offset);
	    id	-= se->firstentry;
	    break;
	}
	se++;
    }
    if (!stre)
    	return 0;
    for (i=id;i--;) {
    	if (!(slen=stre->len))
		return 0;
    	stre = (struct _stringentry*)(((char*)stre)+slen);
    }
    slen=stre->len;
    dprintf_resource(stddeb,"	- strlen=%d\n",slen);
    i = MIN(buflen - 1, slen);
    if (buffer == NULL)
	return slen; /* different to LoadString */
    if (i>0) {
	lstrcpyn32A(buffer,stre->str,i);
	buffer[i]=0;
    } else {
	if (buflen>1) {
	    buffer[0]=0;
	    return 0;
	}
    }
    if (buffer)
	    dprintf_resource(stddeb,"LoadMessage // '%s' copied !\n", buffer);
    return i;
}

/**********************************************************************
 *	LoadMessage32W	(internal)
 */
INT32 LoadMessage32W( HINSTANCE32 instance, UINT32 id, WORD lang,
                      LPWSTR buffer, int buflen )
{
    INT32 retval;
    LPSTR buffer2 = NULL;
    if (buffer) buffer2 = HeapAlloc( GetProcessHeap(), 0, buflen );
    retval = LoadMessage32A(instance,id,lang,buffer2,buflen);
    if (buffer)
    {
        lstrcpynAtoW( buffer, buffer2, buflen );
	HeapFree( GetProcessHeap(), 0, buffer2 );
    }
    return retval;
}


/**********************************************************************
 *	SetResourceHandler	(KERNEL.43)
 */
FARPROC16 WINAPI SetResourceHandler( HMODULE16 hModule, SEGPTR s,
                                     FARPROC16 resourceHandler )
{
    NE_MODULE *pModule;

    hModule = GetExePtr( hModule );

    dprintf_resource(stddeb, "SetResourceHandler: module=%04x type=", hModule );
    PrintId( s );
    dprintf_resource( stddeb, "\n" );

    if ((pModule = MODULE_GetPtr( hModule )))
    {
	if (pModule->flags & NE_FFLAGS_WIN32)
	    fprintf(stderr,"SetResourceHandler: %s", NEWin32FailureString);
	else if (pModule->res_table)
	    return NE_SetResourceHandler( hModule, s, resourceHandler );
    }
    return NULL;
}

#ifndef WINELIB
/**********************************************************************
 *	EnumResourceTypesA	(KERNEL32.90)
 */
BOOL32 WINAPI EnumResourceTypes32A( HMODULE32 hmodule,ENUMRESTYPEPROC32A lpfun,
                                    LONG lParam)
{
    return PE_EnumResourceTypes32A(hmodule,lpfun,lParam);
}

/**********************************************************************
 *	EnumResourceTypesW	(KERNEL32.91)
 */
BOOL32 WINAPI EnumResourceTypes32W( HMODULE32 hmodule,ENUMRESTYPEPROC32W lpfun,
                                    LONG lParam)
{
    return PE_EnumResourceTypes32W(hmodule,lpfun,lParam);
}

/**********************************************************************
 *	EnumResourceNamesA	(KERNEL32.88)
 */
BOOL32 WINAPI EnumResourceNames32A( HMODULE32 hmodule, LPCSTR type,
                                    ENUMRESNAMEPROC32A lpfun, LONG lParam )
{
    return PE_EnumResourceNames32A(hmodule,type,lpfun,lParam);
}
/**********************************************************************
 *	EnumResourceNamesW	(KERNEL32.89)
 */
BOOL32 WINAPI EnumResourceNames32W( HMODULE32 hmodule, LPCWSTR type,
                                    ENUMRESNAMEPROC32W lpfun, LONG lParam )
{
    return PE_EnumResourceNames32W(hmodule,type,lpfun,lParam);
}

/**********************************************************************
 *	EnumResourceLanguagesA	(KERNEL32.86)
 */
BOOL32 WINAPI EnumResourceLanguages32A( HMODULE32 hmodule, LPCSTR type,
                                        LPCSTR name, ENUMRESLANGPROC32A lpfun,
                                        LONG lParam)
{
    return PE_EnumResourceLanguages32A(hmodule,type,name,lpfun,lParam);
}
/**********************************************************************
 *	EnumResourceLanguagesW	(KERNEL32.87)
 */
BOOL32 WINAPI EnumResourceLanguages32W( HMODULE32 hmodule, LPCWSTR type,
                                        LPCWSTR name, ENUMRESLANGPROC32W lpfun,
                                        LONG lParam)
{
    return PE_EnumResourceLanguages32W(hmodule,type,name,lpfun,lParam);
}
#endif  /* WINELIB */
