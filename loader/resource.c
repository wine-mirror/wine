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
#include "windows.h"
#include "gdi.h"
#include "global.h"
#include "heap.h"
#include "neexe.h"
#include "task.h"
#include "module.h"
#include "resource.h"
#include "debugstr.h"
#include "debug.h"
#include "libres.h"

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

    if (HIWORD(name))  /* Check for '#xxx' name */
    {
	char *ptr = PTR_SEG_TO_LIN( name );
	if (ptr[0] == '#')
	    if (!(name = (SEGPTR)atoi( ptr + 1 ))) {
	      WARN(resource, "Incorrect resource name: %s\n", ptr);
	      return 0;
	    }
    }

    if (HIWORD(type))  /* Check for '#xxx' type */
    {
	char *ptr = PTR_SEG_TO_LIN( type );
	if (ptr[0] == '#')
	  if (!(type = (SEGPTR)atoi( ptr + 1 ))){
	    WARN(resource, "Incorrect resource type: %s\n", ptr);
	    return 0;
	  }
    }

    TRACE(resource, "module=%04x name=%s type=%s\n", 
		 hModule, debugres(PTR_SEG_TO_LIN(name)), 
		 debugres(PTR_SEG_TO_LIN(type)) );

    if ((pModule = MODULE_GetPtr( hModule )))
    {
        if (!__winelib)
        {
            if (pModule->flags & NE_FFLAGS_WIN32)
                fprintf(stderr,"FindResource16: %s", NEWin32FailureString);
            else
                return NE_FindResource( hModule, type, name );
        }
        else return LIBRES_FindResource16( hModule,
                                           (LPCSTR)PTR_SEG_TO_LIN(name),
                                           (LPCSTR)PTR_SEG_TO_LIN(type) );
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
    if (!__winelib)
    {
        NE_MODULE *pModule;

        if (!hModule) hModule = GetTaskDS();
        hModule = MODULE_HANDLEtoHMODULE32( hModule );
        TRACE(resource, "module=%08x "
			 "type=%s%p name=%s%p\n", hModule,
			 (HIWORD(type))? "" : "#", type, 
			 (HIWORD(name))? "" : "#", name);

        if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
        if (!(pModule->flags & NE_FFLAGS_WIN32)) return 0;
        return PE_FindResourceEx32W(hModule,name,type,lang);
    }
    else return LIBRES_FindResource32( hModule, name, type );
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
    TRACE(resource, "module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!hRsrc) return 0;
    if ((pModule = MODULE_GetPtr( hModule )))
    {
        if (!__winelib)
        {
            if (pModule->flags & NE_FFLAGS_WIN32)
                fprintf(stderr,"LoadResource16: %s", NEWin32FailureString);
            else
                return NE_LoadResource( hModule, hRsrc );
        }
        else return LIBRES_LoadResource( hModule, hRsrc );
    }
    return 0;
}

/**********************************************************************
 *	    LoadResource32    (KERNEL32.370)
 */
HGLOBAL32 WINAPI LoadResource32( HINSTANCE32 hModule, HRSRC32 hRsrc )
{
    if (!__winelib)
    {
        NE_MODULE *pModule;

        if (!hModule) hModule = GetTaskDS(); /* FIXME: see FindResource32W */
        hModule = MODULE_HANDLEtoHMODULE32( hModule );
        TRACE(resource, "module=%04x res=%04x\n",
                         hModule, hRsrc );
        if (!hRsrc) return 0;

        if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
        if (!(pModule->flags & NE_FFLAGS_WIN32))
        {
            fprintf(stderr,"LoadResource32: %s", PEWin16FailureString );
            return 0;  /* FIXME? */
        }
        return PE_LoadResource32(hModule,hRsrc);
    }
    else return LIBRES_LoadResource( hModule, hRsrc );
}


/**********************************************************************
 *	    LockResource    (KERNEL.62)
 */
/* 16-bit version */
SEGPTR WINAPI WIN16_LockResource16(HGLOBAL16 handle)
{
    HMODULE16 hModule;
    NE_MODULE *pModule;

    TRACE(resource, "handle=%04x\n", handle );
    if (!handle) return (SEGPTR)0;
    hModule = MODULE_HANDLEtoHMODULE16( handle );
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (pModule->flags & NE_FFLAGS_WIN32)
    {
        fprintf(stderr,"LockResource16: %s", NEWin32FailureString);
        return 0;
    }
    return NE_LockResource( hModule, handle );
}

/* Winelib 16-bit version */
LPVOID WINAPI LockResource16( HGLOBAL16 handle )
{
    if (!__winelib)
    {
        HMODULE16 hModule;
        NE_MODULE *pModule;

        TRACE(resource, "handle=%04x\n", handle );
        if (!handle) return NULL;
        hModule = MODULE_HANDLEtoHMODULE16( handle );
        if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
        if (pModule->flags & NE_FFLAGS_WIN32)
        {
            fprintf(stderr,"LockResource16: %s", NEWin32FailureString);
            return 0;
        }
        return (LPSTR)PTR_SEG_TO_LIN( NE_LockResource( hModule, handle ) );
    }
    else return LIBRES_LockResource( handle );
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
    if (!__winelib)
    {
        HMODULE16 hModule;
        NE_MODULE *pModule;

        TRACE(resource, "handle=%04x\n", handle );
        if (!handle) return FALSE;
        hModule = MODULE_HANDLEtoHMODULE16( handle );
        if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
        if (pModule->flags & NE_FFLAGS_WIN32)
        {
            fprintf(stderr,"FreeResource16: %s", NEWin32FailureString);
            return 0;
        }
        return NE_FreeResource( hModule, handle );
    }
    else return LIBRES_FreeResource( handle );
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
    TRACE(resource, "module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!hRsrc) return 0;
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (!__winelib)
    {
        if (pModule->flags & NE_FFLAGS_WIN32)
        {
            fprintf(stderr,"AccessResource16: %s", NEWin32FailureString);
            return 0;
        }
        return NE_AccessResource( hModule, hRsrc );
    }
    else return LIBRES_AccessResource( hModule, hRsrc );
}


/**********************************************************************
 *	    AccessResource32    (KERNEL32.64)
 */
INT32 WINAPI AccessResource32( HINSTANCE32 hModule, HRSRC32 hRsrc )
{
    hModule = MODULE_HANDLEtoHMODULE32( hModule );
    TRACE(resource, "module=%04x res=%04x\n",
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
    TRACE(resource, "module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (!__winelib)
    {
        if (pModule->flags & NE_FFLAGS_WIN32)
        {
            fprintf(stderr,"SizeOfResource16: %s", NEWin32FailureString);
            return 0;
        }
        return NE_SizeofResource( hModule, hRsrc );
    }
    else return LIBRES_SizeofResource( hModule, hRsrc );
}


/**********************************************************************
 *	    SizeofResource32    (KERNEL32.522)
 */
DWORD WINAPI SizeofResource32( HINSTANCE32 hModule, HRSRC32 hRsrc )
{
    hModule = MODULE_HANDLEtoHMODULE32( hModule );
    TRACE(resource, "module=%04x res=%04x\n",
                     hModule, hRsrc );
    if (!__winelib) return PE_SizeofResource32(hModule,hRsrc);
    else
    {
        fprintf(stderr,"SizeofResource32: not implemented\n");
        return 0;
    }
}


/**********************************************************************
 *	    AllocResource16    (KERNEL.66)
 */
HGLOBAL16 WINAPI AllocResource16( HMODULE16 hModule, HRSRC16 hRsrc, DWORD size)
{
    NE_MODULE *pModule;

    hModule = MODULE_HANDLEtoHMODULE16( hModule );
    TRACE(resource, "module=%04x res=%04x size=%ld\n",
                     hModule, hRsrc, size );
    if (!hRsrc) return 0;
    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    if (!__winelib)
    {
        if (pModule->flags & NE_FFLAGS_WIN32)
        {
            fprintf(stderr,"AllocResource16: %s", NEWin32FailureString);
            return 0;
        }
        return NE_AllocResource( hModule, hRsrc, size );
    }
    else return LIBRES_AllocResource( hModule, hRsrc, size );
}

/**********************************************************************
 *      DirectResAlloc    (KERNEL.168)
 *
 * Check Schulman, p. 232 for details
 */
HGLOBAL16 WINAPI DirectResAlloc( HINSTANCE16 hInstance, WORD wType,
                                 UINT16 wSize )
{
    TRACE(resource,"(%04x,%04x,%04x)\n",
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
 */
HACCEL16 WINAPI LoadAccelerators16(HINSTANCE16 instance, SEGPTR lpTableName)
{
    HRSRC16	hRsrc;

    if (HIWORD(lpTableName))
        TRACE(accel, "%04x '%s'\n",
                      instance, (char *)PTR_SEG_TO_LIN( lpTableName ) );
    else
        TRACE(accel, "%04x %04x\n",
                       instance, LOWORD(lpTableName) );

    if (!(hRsrc = FindResource16( instance, lpTableName, RT_ACCELERATOR )))
      return 0;
    return LoadResource16(instance,hRsrc);
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
    HRSRC32 hRsrc;

    if (HIWORD(lpTableName))
        TRACE(accel, "%04x '%s'\n",
                      instance, (char *)( lpTableName ) );
    else
        TRACE(accel, "%04x %04x\n",
                       instance, LOWORD(lpTableName) );

    if (!(hRsrc = FindResource32W( instance, lpTableName, 
		(LPCWSTR)RT_ACCELERATOR )))
      return 0;
    return LoadResource32( instance, hRsrc );
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
 *             CopyAcceleratorTable32A   (USER32.58)
 */
INT32 WINAPI CopyAcceleratorTable32A(HACCEL32 src, LPACCEL32 dst, INT32 entries)
{
  fprintf(stderr,"CopyAcceleratorTable32A: not implemented: %d\n", entries);
  return 0;
}

/**********************************************************************
 *             CopyAcceleratorTable32W   (USER32.59)
 */
INT32 WINAPI CopyAcceleratorTable32W(HACCEL32 src, LPACCEL32 dst, INT32 entries)
{
  fprintf(stderr,"CopyAcceleratorTable32W: not implemented: %d\n", entries);
  return 0;
}

/**********************************************************************
 *             DestroyAcceleratorTable   (USER32.130)
 */
BOOL32 WINAPI DestroyAcceleratorTable( HACCEL32 handle )
{
  fprintf(stderr,"DestroyAcceleratorTable: not implemented\n");
  return TRUE;
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

    TRACE(resource,"inst=%04x id=%04x buff=%08x len=%d\n",
                     instance, resource_id, (int) buffer, buflen);

    hrsrc = FindResource16( instance, (SEGPTR)((resource_id>>4)+1), RT_STRING );
    if (!hrsrc) return 0;
    hmem = LoadResource16( instance, hrsrc );
    if (!hmem) return 0;
    
    p = LockResource16(hmem);
    string_num = resource_id & 0x000f;
    for (i = 0; i < string_num; i++)
	p += *p + 1;
    
    TRACE(resource, "strlen = %d\n", (int)*p );
    
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

    TRACE(resource,"'%s' copied !\n", buffer);
    return i;
}

/**********************************************************************
 *	LoadString32W		(USER32.375)
 */
INT32 WINAPI LoadString32W( HINSTANCE32 instance, UINT32 resource_id,
                            LPWSTR buffer, INT32 buflen )
{
    HGLOBAL32 hmem;
    HRSRC32 hrsrc;
    WCHAR *p;
    int string_num;
    int i;

    if (HIWORD(resource_id)==0xFFFF) /* netscape 3 passes this */
	resource_id = (UINT32)(-((INT32)resource_id));
    TRACE(resource, "instance = %04x, id = %04x, buffer = %08x, "
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
    
    TRACE(resource, "strlen = %d\n", (int)*p );
    
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
    TRACE(resource,"'%s' copied !\n", buffer);
#endif
    return i;
}

/**********************************************************************
 *	LoadString32A	(USER32.374)
 */
INT32 WINAPI LoadString32A( HINSTANCE32 instance, UINT32 resource_id,
                            LPSTR buffer, INT32 buflen )
{
    INT32 retval;
    LPWSTR buffer2 = NULL;
    if (buffer && buflen)
	buffer2 = HeapAlloc( GetProcessHeap(), 0, buflen * 2 );
    retval = LoadString32W(instance,resource_id,buffer2,buflen);

    if (buffer2)
    {
	if (retval) {
	    lstrcpynWtoA( buffer, buffer2, buflen );
	    retval = lstrlen32A( buffer );
	}
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
                      LPSTR buffer, INT32 buflen )
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

    TRACE(resource, "instance = %08lx, id = %08lx, buffer = %p, length = %ld\n", (DWORD)instance, (DWORD)id, buffer, (DWORD)buflen);

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
    TRACE(resource,"	- strlen=%d\n",slen);
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
	    TRACE(resource,"'%s' copied !\n", buffer);
    return i;
}

/**********************************************************************
 *	LoadMessage32W	(internal)
 */
INT32 LoadMessage32W( HINSTANCE32 instance, UINT32 id, WORD lang,
                      LPWSTR buffer, INT32 buflen )
{
    INT32 retval;
    LPSTR buffer2 = NULL;
    if (buffer && buflen)
	buffer2 = HeapAlloc( GetProcessHeap(), 0, buflen );
    retval = LoadMessage32A(instance,id,lang,buffer2,buflen);
    if (buffer)
    {
	if (retval) {
	    lstrcpynAtoW( buffer, buffer2, buflen );
	    retval = lstrlen32W( buffer );
	}
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

    TRACE(resource, "module=%04x type=%s\n", 
		 hModule, debugres(PTR_SEG_TO_LIN(s)) );

    if ((pModule = MODULE_GetPtr( hModule )))
    {
	if (pModule->flags & NE_FFLAGS_WIN32)
	    fprintf(stderr,"SetResourceHandler: %s\n", NEWin32FailureString);
	else if (pModule->res_table)
	    return NE_SetResourceHandler( hModule, s, resourceHandler );
    }
    return NULL;
}


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
