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
#include "dlls.h"
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


/**********************************************************************
 *	    FindResource    (KERNEL.60)
 */
HRSRC FindResource( HMODULE hModule, SEGPTR name, SEGPTR type )
{
    WORD *pModule;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "FindResource: module="NPFMT" type=", hModule );
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
    if (!(pModule = (WORD *)GlobalLock( hModule ))) return 0;
#ifndef WINELIB
    switch(*pModule)
    {
      case NE_SIGNATURE:
        return NE_FindResource( hModule, type, name );
      case PE_SIGNATURE:
        return 0;
      default:
        return 0;
    }
#else
    return LIBRES_FindResource( hModule, type, name );
#endif
}


/**********************************************************************
 *	    LoadResource    (KERNEL.61)
 */
HGLOBAL LoadResource( HMODULE hModule, HRSRC hRsrc )
{
    WORD *pModule;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "LoadResource: module="NPFMT" res="NPFMT"\n",
                     hModule, hRsrc );
    if (!hRsrc) return 0;
    if (!(pModule = (WORD *)GlobalLock( hModule ))) return 0;
#ifndef WINELIB
    switch(*pModule)
    {
      case NE_SIGNATURE:
        return NE_LoadResource( hModule, hRsrc );
      case PE_SIGNATURE:
        return 0;
      default:
        return 0;
    }
#else
    return LIBRES_LoadResource( hModule, hRsrc );
#endif
}


/**********************************************************************
 *	    LockResource    (KERNEL.62)
 */
/* 16-bit version */
SEGPTR WIN16_LockResource( HGLOBAL handle )
{
    HMODULE hModule;
    WORD *pModule;

    dprintf_resource(stddeb, "LockResource: handle="NPFMT"\n", handle );
    if (!handle) return (SEGPTR)0;
    hModule = GetExePtr( handle );
    if (!(pModule = (WORD *)GlobalLock( hModule ))) return 0;
#ifndef WINELIB
    switch(*pModule)
    {
      case NE_SIGNATURE:
        return NE_LockResource( hModule, handle );
      case PE_SIGNATURE:
        return 0;
      default:
        return 0;
    }
#else
    return LIBRES_LockResource( hModule, handle );
#endif
}

/* 32-bit version */
LPSTR LockResource( HGLOBAL handle )
{
    HMODULE hModule;
    WORD *pModule;

    dprintf_resource(stddeb, "LockResource: handle="NPFMT"\n", handle );
    if (!handle) return NULL;
    hModule = GetExePtr( handle );
    if (!(pModule = (WORD *)GlobalLock( hModule ))) return 0;
#ifndef WINELIB
    switch(*pModule)
    {
      case NE_SIGNATURE:
        return (LPSTR)PTR_SEG_TO_LIN( NE_LockResource( hModule, handle ) );
      case PE_SIGNATURE:
        return 0;
      default:
        return 0;
    }
#else
    return LIBRES_LockResource( hModule, handle );
#endif
}


/**********************************************************************
 *	    FreeResource    (KERNEL.63)
 */
BOOL FreeResource( HGLOBAL handle )
{
    HMODULE hModule;
    WORD *pModule;

    dprintf_resource(stddeb, "FreeResource: handle="NPFMT"\n", handle );
    if (!handle) return FALSE;
    hModule = GetExePtr( handle );
    if (!(pModule = (WORD *)GlobalLock( hModule ))) return 0;
#ifndef WINELIB
    switch(*pModule)
    {
      case NE_SIGNATURE:
        return NE_FreeResource( hModule, handle );
      case PE_SIGNATURE:
        return FALSE;
      default:
        return FALSE;
    }
#else
    return LIBRES_FreeResource( hModule, handle );
#endif
}


/**********************************************************************
 *	    AccessResource    (KERNEL.64)
 */
INT AccessResource( HINSTANCE hModule, HRSRC hRsrc )
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
DWORD SizeofResource( HMODULE hModule, HRSRC hRsrc )
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
 *	    AllocResource    (KERNEL.66)
 */
HGLOBAL AllocResource( HMODULE hModule, HRSRC hRsrc, DWORD size )
{
    WORD *pModule;

    hModule = GetExePtr( hModule );  /* In case we were passed an hInstance */
    dprintf_resource(stddeb, "AllocResource: module="NPFMT" res="NPFMT" size=%ld\n",
                     hModule, hRsrc, size );
    if (!hRsrc) return 0;
    if (!(pModule = (WORD *)GlobalLock( hModule ))) return 0;
#ifndef WINELIB
    switch(*pModule)
    {
      case NE_SIGNATURE:
        return NE_AllocResource( hModule, hRsrc, size );
      case PE_SIGNATURE:
        return 0;
      default:
        return 0;
    }
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
    dprintf_resource(stddeb,"DirectResAlloc("NPFMT",%x,%x)\n",hInstance,wType,wSize);
    hInstance = GetExePtr(hInstance);
    if(!hInstance)return 0;
    if(wType != 0x10)	/* 0x10 is the only observed value, passed from
                           CreateCursorIndirect. */
        fprintf(stderr, "DirectResAlloc: wType = %x\n", wType);
    return GLOBAL_Alloc(GMEM_MOVEABLE, wSize, hInstance, FALSE, FALSE, FALSE);
}


/**********************************************************************
 *			LoadAccelerators	[USER.177]
 */
HANDLE LoadAccelerators(HANDLE instance, SEGPTR lpTableName)
{
    HANDLE 	hAccel;
    HANDLE 	rsc_mem;
    HRSRC hRsrc;
    BYTE 	*lp;
    ACCELHEADER	*lpAccelTbl;
    int 	i, n;

    if (HIWORD(lpTableName))
        dprintf_accel( stddeb, "LoadAccelerators: "NPFMT" '%s'\n",
                      instance, (char *)PTR_SEG_TO_LIN( lpTableName ) );
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
 *			TranslateAccelerator 	[USER.178]
 */
int TranslateAccelerator(HWND hWnd, HANDLE hAccel, LPMSG msg)
{
    ACCELHEADER	*lpAccelTbl;
    int 	i;
    
    if (hAccel == 0 || msg == NULL) return 0;
    if (msg->message != WM_KEYDOWN &&
    	msg->message != WM_KEYUP &&
	msg->message != WM_SYSKEYDOWN &&
	msg->message != WM_SYSKEYUP &&
    	msg->message != WM_CHAR) return 0;

    dprintf_accel(stddeb, "TranslateAccelerators hAccel="NPFMT" !\n", hAccel);

    lpAccelTbl = (LPACCELHEADER)GlobalLock(hAccel);
    for (i = 0; i < lpAccelTbl->wCount; i++) {
	if(lpAccelTbl->tbl[i].type & VIRTKEY_ACCEL) {
	    if(msg->wParam == lpAccelTbl->tbl[i].wEvent &&
	       (msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN)) {
		INT mask = 0;

		if(GetKeyState(VK_SHIFT) & 0xf) mask |= SHIFT_ACCEL;
		if(GetKeyState(VK_CONTROL) & 0xf) mask |= CONTROL_ACCEL;
		if(GetKeyState(VK_MENU) & 0xf) mask |= ALT_ACCEL;
		if(mask == (lpAccelTbl->tbl[i].type &
			    (SHIFT_ACCEL | CONTROL_ACCEL | ALT_ACCEL))) {
		    SendMessage(hWnd, WM_COMMAND, lpAccelTbl->tbl[i].wIDval,
				0x00010000L);
		    GlobalUnlock(hAccel);
		    return 1;
	        }
		if (msg->message == WM_KEYUP || msg->message == WM_SYSKEYUP)
		    return 1;
	    }
	}
	else {
	    if (msg->wParam == lpAccelTbl->tbl[i].wEvent &&
		msg->message == WM_CHAR) {
		SendMessage(hWnd, WM_COMMAND, lpAccelTbl->tbl[i].wIDval, 0x00010000L);
		GlobalUnlock(hAccel);
		return 1;
		}
	    }
	}
    GlobalUnlock(hAccel);
    return 0;
}

/**********************************************************************
 *					LoadString
 */
int
LoadString(HANDLE instance, WORD resource_id, LPSTR buffer, int buflen)
{
    HANDLE hmem, hrsrc;
    unsigned char *p;
    int string_num;
    int i;

    dprintf_resource(stddeb, "LoadString: instance = "NPFMT", id = %04x, buffer = %08x, "
	   "length = %d\n", instance, resource_id, (int) buffer, buflen);

    hrsrc = FindResource( instance, (SEGPTR)((resource_id>>4)+1), RT_STRING );
    if (!hrsrc) return 0;
    hmem = LoadResource( instance, hrsrc );
    if (!hmem) return 0;
    
    p = LockResource(hmem);
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
    FreeResource( hmem );

    dprintf_resource(stddeb,"LoadString // '%s' copied !\n", buffer);
    return i;
}



