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
#include "neexe.h"
#include "accel.h"
#include "module.h"
#include "resource.h"
#include "stddebug.h"
#include "debug.h"
#include "libres.h"
#include "string32.h"
#include "xmalloc.h"

#define PrintId(name) \
    if (HIWORD((DWORD)name)) \
        dprintf_resource( stddeb, "'%s'", (char *)PTR_SEG_TO_LIN(name)); \
    else \
        dprintf_resource( stddeb, "#%04x", LOWORD(name)); 


/**********************************************************************
 *	    FindResource16    (KERNEL.60)
 */
HRSRC16 FindResource16( HMODULE16 hModule, SEGPTR name, SEGPTR type )
{
    NE_MODULE *pModule;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
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
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
#ifndef WINELIB
    if (pModule->flags & NE_FFLAGS_WIN32)
    {
        fprintf(stderr,"Don't know how to FindResource16() for Win32 module\n");
        return 0;
    }
    return NE_FindResource( hModule, type, name );
#else
    return LIBRES_FindResource( hModule, name, type );
#endif
}


/**********************************************************************
 *	    FindResource32A    (KERNEL32.128)
 */
HANDLE32 FindResource32A( HINSTANCE32 hModule, LPCSTR name, LPCSTR type )
{
    LPWSTR xname,xtype;
    HANDLE32 ret;

    if (HIWORD((DWORD)name)) xname = STRING32_DupAnsiToUni(name);
    else xname = (LPWSTR)name;
    if (HIWORD((DWORD)type)) xtype = STRING32_DupAnsiToUni(type);
    else xtype = (LPWSTR)type;
    ret = FindResource32W(hModule,xname,xtype);
    if (HIWORD((DWORD)name)) free(xname);
    if (HIWORD((DWORD)type)) free(xtype);
    return ret;
}


/**********************************************************************
 *	    FindResource32W    (KERNEL32.131)
 */
HRSRC32 FindResource32W( HINSTANCE32 hModule, LPCWSTR name, LPCWSTR type )
{
#ifndef WINELIB
    NE_MODULE *pModule;

    /* Sometimes we get passed hModule = 0x00000000. FIXME: is GetTaskDS()
     * ok?
     */
    if (!hModule) hModule = GetTaskDS();
    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "FindResource32W: module=%08x type=", hModule );
    PrintId( type );
    dprintf_resource( stddeb, " name=" );
    PrintId( name );
    dprintf_resource( stddeb, "\n" );
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (!(pModule->flags & NE_FFLAGS_WIN32)) return 0;
    return PE_FindResource32W(hModule,name,type);
#else
    return LIBRES_FindResource( hModule, name, type );
#endif
}


/**********************************************************************
 *	    LoadResource16    (KERNEL.61)
 */
HGLOBAL16 LoadResource16( HMODULE16 hModule, HRSRC16 hRsrc )
{
    NE_MODULE *pModule;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "LoadResource16: module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!hRsrc) return 0;
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
#ifndef WINELIB
    if (pModule->flags & NE_FFLAGS_WIN32)
    {
        fprintf(stderr,"Don't know how to LoadResource16() for Win32 module\n");
        return 0;
    }
    return NE_LoadResource( hModule, hRsrc );
#else
    return LIBRES_LoadResource( hModule, hRsrc );
#endif
}

/**********************************************************************
 *	    LoadResource32    (KERNEL32.370)
 */
HGLOBAL32 LoadResource32( HINSTANCE32 hModule, HRSRC32 hRsrc )
{
#ifndef WINELIB
    NE_MODULE *pModule;

    if (!hModule) hModule = GetTaskDS(); /* FIXME: see FindResource32W */
    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "LoadResource32: module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!hRsrc) return 0;

    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (!(pModule->flags & NE_FFLAGS_WIN32))
    {
    	fprintf(stderr,"LoadResource32: tried to load a non win32 resource.\n");
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
SEGPTR WIN16_LockResource16(HGLOBAL16 handle)
{
#ifndef WINELIB
    HMODULE16 hModule;
    NE_MODULE *pModule;

    dprintf_resource(stddeb, "LockResource: handle=%04x\n", handle );
    if (!handle) return (SEGPTR)0;
    hModule = GetExePtr( handle );
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (pModule->flags & NE_FFLAGS_WIN32)
    {
        fprintf(stderr,"Don't know how to LockResource() for Win32 module\n");
        return 0;
    }
    return NE_LockResource( hModule, handle );
#else
    return LIBRES_LockResource( handle );
#endif
}

/* WINELIB 16-bit version */
LPVOID LockResource16( HGLOBAL16 handle )
{
#ifndef WINELIB
    HMODULE16 hModule;
    NE_MODULE *pModule;

    dprintf_resource(stddeb, "LockResource: handle=%04x\n", handle );
    if (!handle) return NULL;
    hModule = GetExePtr( handle );
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (pModule->flags & NE_FFLAGS_WIN32)
    {
        fprintf(stderr,"Don't know how to LockResource16() for Win32 module\n");
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
LPVOID LockResource32( HGLOBAL32 handle )
{
    return (LPVOID)handle;
}


/**********************************************************************
 *	    FreeResource16    (KERNEL.63)
 */
BOOL16 FreeResource16( HGLOBAL16 handle )
{
#ifndef WINELIB
    HMODULE16 hModule;
    NE_MODULE *pModule;

    dprintf_resource(stddeb, "FreeResource16: handle=%04x\n", handle );
    if (!handle) return FALSE;
    hModule = GetExePtr( handle );
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (pModule->flags & NE_FFLAGS_WIN32)
    {
        fprintf(stderr,"Don't know how to FreeResource16() for Win32 module\n");
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
BOOL32 FreeResource32( HGLOBAL32 handle )
{
    /* no longer used in Win32 */
    return TRUE;
}


/**********************************************************************
 *	    AccessResource16    (KERNEL.64)
 */
INT16 AccessResource16( HINSTANCE16 hModule, HRSRC16 hRsrc )
{
    NE_MODULE *pModule;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "AccessResource16: module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!hRsrc) return 0;
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
#ifndef WINELIB
    if (pModule->flags & NE_FFLAGS_WIN32)
    {
        fprintf(stderr,"Don't know how to AccessResource16() for Win32 module\n");
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
INT32 AccessResource32( HINSTANCE32 hModule, HRSRC32 hRsrc )
{
    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "AccessResource: module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!hRsrc) return 0;
    fprintf(stderr,"AccessResource32: not implemented\n");
    return 0;
}


/**********************************************************************
 *	    SizeofResource16    (KERNEL.65)
 */
DWORD SizeofResource16( HMODULE16 hModule, HRSRC16 hRsrc )
{
    NE_MODULE *pModule;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "SizeofResource16: module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
#ifndef WINELIB
    if (pModule->flags & NE_FFLAGS_WIN32)
    {
        fprintf(stderr,"Don't know how to SizeOfResource16() for Win32 module\n");
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
DWORD SizeofResource32( HINSTANCE32 hModule, HRSRC32 hRsrc )
{
    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "SizeofResource32: module=%04x res=%04x\n",
                     hModule, hRsrc );
    fprintf(stderr,"SizeofResource32: not implemented\n");
    return 0;
}


/**********************************************************************
 *	    AllocResource16    (KERNEL.66)
 */
HGLOBAL16 AllocResource16( HMODULE16 hModule, HRSRC16 hRsrc, DWORD size )
{
    NE_MODULE *pModule;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "AllocResource: module=%04x res=%04x size=%ld\n",
                     hModule, hRsrc, size );
    if (!hRsrc) return 0;
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
#ifndef WINELIB
    if (pModule->flags & NE_FFLAGS_WIN32)
    {
        fprintf(stderr,"Don't know how to AllocResource() for Win32 module\n");
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
HANDLE DirectResAlloc(HANDLE hInstance, WORD wType, WORD wSize)
{
    dprintf_resource(stddeb,"DirectResAlloc(%04x,%04x,%04x)\n",
                     hInstance, wType, wSize );
    hInstance = GetExePtr(hInstance);
    if(!hInstance)return 0;
    if(wType != 0x10)	/* 0x10 is the only observed value, passed from
                           CreateCursorIndirect. */
        fprintf(stderr, "DirectResAlloc: wType = %x\n", wType);
    return GLOBAL_Alloc(GMEM_MOVEABLE, wSize, hInstance, FALSE, FALSE, FALSE);
}


/**********************************************************************
 *			LoadAccelerators16	[USER.177]
 */
HACCEL16 LoadAccelerators16(HINSTANCE16 instance, SEGPTR lpTableName)
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
 */
HACCEL32 LoadAccelerators32W(HINSTANCE32 instance,LPCWSTR lpTableName)
{
#if 0
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
    FreeResource32(rsc_mem);
    return hAccel;
#else
	fprintf(stderr,"LoadAcceleratorsW: not implemented\n");
	return 0x100;  /* Return something anyway */
#endif
}

HACCEL32 LoadAccelerators32A(HINSTANCE32 instance,LPCSTR lpTableName)
{
	LPWSTR	 uni;
	HACCEL32 result;
	if (HIWORD(lpTableName))
		uni=STRING32_DupAnsiToUni(lpTableName);
	else
		uni=(LPWSTR)lpTableName;
	result=LoadAccelerators32W(instance,uni);
	if (HIWORD(uni))
		free(uni);
	return result;
}


/**********************************************************************
 *			TranslateAccelerator 	[USER.178]
 */
int TranslateAccelerator(HWND hWnd, HANDLE hAccel, LPMSG16 msg)
{
    ACCELHEADER	*lpAccelTbl;
    int 	i;
    
    if (hAccel == 0 || msg == NULL) return 0;
    if (msg->message != WM_KEYDOWN &&
    	msg->message != WM_KEYUP &&
	msg->message != WM_SYSKEYDOWN &&
	msg->message != WM_SYSKEYUP &&
    	msg->message != WM_CHAR) return 0;

    dprintf_accel(stddeb, "TranslateAccelerators hAccel=%04x !\n", hAccel);

    lpAccelTbl = (LPACCELHEADER)GlobalLock16(hAccel);
    for (i = 0; i < lpAccelTbl->wCount; i++) {
	if(lpAccelTbl->tbl[i].type & VIRTKEY_ACCEL) {
	    if(msg->wParam == lpAccelTbl->tbl[i].wEvent &&
	       (msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN)) {
		INT mask = 0;

		if(GetKeyState(VK_SHIFT) & 0x8000) mask |= SHIFT_ACCEL;
		if(GetKeyState(VK_CONTROL) & 0x8000) mask |= CONTROL_ACCEL;
		if(GetKeyState(VK_MENU) & 0x8000) mask |= ALT_ACCEL;
		if(mask == (lpAccelTbl->tbl[i].type &
			    (SHIFT_ACCEL | CONTROL_ACCEL | ALT_ACCEL))) {
		    SendMessage16(hWnd, WM_COMMAND, lpAccelTbl->tbl[i].wIDval,
				0x00010000L);
		    GlobalUnlock16(hAccel);
		    return 1;
	        }
		if (msg->message == WM_KEYUP || msg->message == WM_SYSKEYUP)
		    return 1;
	    }
	}
	else {
	    if (msg->wParam == lpAccelTbl->tbl[i].wEvent &&
		msg->message == WM_CHAR) {
		SendMessage16(hWnd, WM_COMMAND, lpAccelTbl->tbl[i].wIDval, 0x00010000L);
		GlobalUnlock16(hAccel);
		return 1;
		}
	    }
	}
    GlobalUnlock16(hAccel);
    return 0;
}

/**********************************************************************
 *					LoadString16
 */
INT16
LoadString16(HINSTANCE16 instance,UINT16 resource_id,LPSTR buffer,INT16 buflen)
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
INT32
LoadString32W(HINSTANCE32 instance,UINT32 resource_id,LPWSTR buffer,int buflen)
{
    HGLOBAL32 hmem;
    HRSRC32 hrsrc;
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
#if 0
    dprintf_resource(stddeb,"LoadString // '%s' copied !\n", buffer);
#endif
    return i;
}

/**********************************************************************
 *	LoadString32A	(USER32.374)
 */
INT32
LoadString32A(HINSTANCE32 instance,UINT32 resource_id,LPSTR buffer,int buflen)
{
    LPWSTR buffer2 = (LPWSTR)xmalloc(buflen*2);
    INT32 retval = LoadString32W(instance,resource_id,buffer2,buflen);

    STRING32_UniToAnsi(buffer,buffer2);
    free(buffer2);
    return retval;
}

