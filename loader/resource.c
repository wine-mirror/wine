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
#include "windows.h"
#include "gdi.h"
#include "global.h"
#include "heap.h"
#include "neexe.h"
#include "task.h"
#include "process.h"
#include "module.h"
#include "resource.h"
#include "debug.h"
#include "libres.h"
#include "winerror.h"
#include "debugstr.h"

extern WORD WINE_LanguageId;


/**********************************************************************
 *	    FindResource32A    (KERNEL32.128)
 */
HANDLE32 WINAPI FindResource32A( HMODULE32 hModule, LPCSTR name, LPCSTR type)
{
    return FindResourceEx32A(hModule,type,name,WINE_LanguageId);
}

/**********************************************************************
 *	    FindResourceEx32A    (KERNEL32.129)
 */
HANDLE32 WINAPI FindResourceEx32A( HMODULE32 hModule, LPCSTR type, LPCSTR name,
				   WORD lang
) {
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
    ret = FindResourceEx32W( hModule, xtype, xname, lang );
    if (HIWORD((DWORD)name)) HeapFree( GetProcessHeap(), 0, xname );
    if (HIWORD((DWORD)type)) HeapFree( GetProcessHeap(), 0, xtype );
    return ret;
}


/**********************************************************************
 *	    FindResourceEx32W    (KERNEL32.130)
 */
HRSRC32 WINAPI FindResourceEx32W( HMODULE32 hModule, LPCWSTR type,
                                  LPCWSTR name, WORD lang )
{	HRSRC32 ret;
    WINE_MODREF	*wm = MODULE32_LookupHMODULE(PROCESS_Current(),hModule);
    HRSRC32	hrsrc;

    TRACE(resource, "module=%08x(%s) type=%s name=%s\n",
	  hModule, wm->modname,
	  debugres_w (type),
	  debugres_w (name));

    if (__winelib) {
    	hrsrc = LIBRES_FindResource( hModule, name, type );
	if (hrsrc)
	    return hrsrc;
    }
    if (wm) {
    	switch (wm->type) {
	case MODULE32_PE:
	    ret =  PE_FindResourceEx32W(wm,name,type,lang);
        if ( ret==0 )
          ERR(resource,"(0x%08x(%s),%s not found!\n",hModule,wm->modname,debugres_w (name));
	    return ret;
	default:
	    ERR(module,"unknown module type %d\n",wm->type);
	    break;
	}
    }
    return (HRSRC32)0;
}


/**********************************************************************
 *	    FindResource32W    (KERNEL32.131)
 */
HRSRC32 WINAPI FindResource32W(HINSTANCE32 hModule, LPCWSTR name, LPCWSTR type)
{
    return FindResourceEx32W(hModule,type,name,WINE_LanguageId);
}


/**********************************************************************
 *	    LoadResource32    (KERNEL32.370)
 * 'loads' a resource. The current implementation just returns a pointer
 * into the already mapped image.
 * RETURNS
 *	pointer into the mapped resource of the passed module
 */
HGLOBAL32 WINAPI LoadResource32( 
	HINSTANCE32 hModule,	/* [in] module handle */
	HRSRC32 hRsrc )		/* [in] resource handle */
{
    WINE_MODREF	*wm = MODULE32_LookupHMODULE(PROCESS_Current(),hModule);

    TRACE(resource, "module=%04x res=%04x\n",
		     hModule, hRsrc );
    if (!hRsrc) {
    	ERR(resource,"hRsrc is 0, return 0.\n");
	return 0;
    }
    if (wm)
    	switch (wm->type) {
	case MODULE32_PE:
            return PE_LoadResource32(wm,hRsrc);
	default:
	    ERR(resource,"unknown module type %d\n",wm->type);
	    break;
	}
    if (__winelib)
    	return LIBRES_LoadResource( hModule, hRsrc );
    return 0;
}


/**********************************************************************
 *	    LockResource32    (KERNEL32.384)
 */
LPVOID WINAPI LockResource32( HGLOBAL32 handle )
{
    return (LPVOID)handle;
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
 *	    AccessResource32    (KERNEL32.64)
 */
INT32 WINAPI AccessResource32( HMODULE32 hModule, HRSRC32 hRsrc )
{
    FIXME(resource,"(module=%08x res=%08x),not implemented\n", hModule, hRsrc);
    return 0;
}


/**********************************************************************
 *	    SizeofResource32    (KERNEL32.522)
 */
DWORD WINAPI SizeofResource32( HINSTANCE32 hModule, HRSRC32 hRsrc )
{
    WINE_MODREF	*wm = MODULE32_LookupHMODULE(PROCESS_Current(),hModule);

    TRACE(resource, "module=%08x res=%08x\n", hModule, hRsrc );
    if (wm)
        switch (wm->type)
        {
	case MODULE32_PE:
            {
                DWORD ret;
                ret = PE_SizeofResource32(hModule,hRsrc);
                if (ret) 
                    return ret;
                break;
            }
	default:
	    ERR(module,"unknown module type %d\n",wm->type);
	    break;
	}
    if (__winelib)
        FIXME(module,"Not implemented for WINELIB\n");
    return 0;
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

    if (!(hRsrc = FindResource16( instance, lpTableName, RT_ACCELERATOR16 ))) {
      WARN(accel, "couldn't find accelerator table resource\n");
      return 0;
    }

    TRACE(accel, "returning HACCEL 0x%x\n", hRsrc);
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
    HACCEL32 hRetval;
    DWORD size;

    if (HIWORD(lpTableName))
        TRACE(accel, "%p '%s'\n",
                      (LPVOID)instance, (char *)( lpTableName ) );
    else
        TRACE(accel, "%p 0x%04x\n",
                       (LPVOID)instance, LOWORD(lpTableName) );

    if (!(hRsrc = FindResource32W( instance, lpTableName, RT_ACCELERATOR32W )))
    {
      WARN(accel, "couldn't find accelerator table resource\n");
      hRetval = 0;
    }
    else {
      hRetval = LoadResource32( instance, hRsrc );
      size = SizeofResource32( instance, hRsrc );
      if(size>=sizeof(ACCEL32))
      {
	LPACCEL32 accel_table = (LPACCEL32) hRetval;
	/* mark last element as such - sometimes it is not marked in image */
	accel_table[size/sizeof(ACCEL32)-1].fVirt |= 0x80;
      }
    }

    TRACE(accel, "returning HACCEL 0x%x\n", hRsrc);
    return hRetval;
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
  return CopyAcceleratorTable32W(src, dst, entries);
}

/**********************************************************************
 *             CopyAcceleratorTable32W   (USER32.59)
 *
 * By mortene@pvv.org 980321
 */
INT32 WINAPI CopyAcceleratorTable32W(HACCEL32 src, LPACCEL32 dst,
				     INT32 entries)
{
  int i;
  LPACCEL32 accel = (LPACCEL32)src;
  BOOL32 done = FALSE;

  /* Do parameter checking to avoid the explosions and the screaming
     as far as possible. */
  if((dst && (entries < 1)) || (src == (HACCEL32)NULL)) {
    WARN(accel, "Application sent invalid parameters (%p %p %d).\n",
	 (LPVOID)src, (LPVOID)dst, entries);
    return 0;
  }


  i=0;
  while(!done) {
    /* Spit out some debugging information. */
    TRACE(accel, "accel %d: type 0x%02x, event '%c', IDval 0x%04x.\n",
	  i, accel[i].fVirt, accel[i].key, accel[i].cmd);

    /* Copy data to the destination structure array (if dst == NULL,
       we're just supposed to count the number of entries). */
    if(dst) {
      memcpy(&dst[i], &accel[i], sizeof(ACCEL32));

      /* Check if we've reached the end of the application supplied
         accelerator table. */
      if(i+1 == entries) {
	/* Turn off the high order bit, just in case. */
	dst[i].fVirt &= 0x7f;
	done = TRUE;
      }
    }

    /* The highest order bit seems to mark the end of the accelerator
       resource table. (?) */
    if((accel[i].fVirt & 0x80) != 0) done = TRUE;

    i++;
  }

  return i;
}

/*********************************************************************
 *                    CreateAcceleratorTable   (USER32.64)
 *
 * By mortene@pvv.org 980321
 */
HACCEL32 WINAPI CreateAcceleratorTable32A(LPACCEL32 lpaccel, INT32 cEntries)
{
  HACCEL32 hAccel;

  /* Do parameter checking just in case someone's trying to be
     funny. */
  if(cEntries < 1) {
    WARN(accel, "Application sent invalid parameters (%p %d).\n",
	 lpaccel, cEntries);
    SetLastError(ERROR_INVALID_PARAMETER);
    return (HACCEL32)NULL;
  }
  FIXME(accel, "should check that the accelerator descriptions are valid,"
	" return NULL and SetLastError() if not.\n");


  /* Allocate memory and copy the table. */
  hAccel = (HACCEL32)HeapAlloc(GetProcessHeap(), 0,
			       cEntries * sizeof(ACCEL32));
  TRACE(accel, "handle %p\n", (LPVOID)hAccel);
  if(!hAccel) {
    ERR(accel, "Out of memory.\n");
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return (HACCEL32)NULL;
  }
  memcpy((LPACCEL32)hAccel, lpaccel, cEntries * sizeof(ACCEL32));

  /* Set the end-of-table terminator. */
  ((LPACCEL32)hAccel)[cEntries-1].fVirt |= 0x80;

  TRACE(accel, "Allocated accelerator handle %x\n", hAccel);
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
BOOL32 WINAPI DestroyAcceleratorTable( HACCEL32 handle )
{
    FIXME(accel, "(0x%x): stub\n", handle);


  /* Weird.. I thought this should work. According to the API
     specification, DestroyAcceleratorTable() should only be called on
     HACCEL32's made by CreateAcceleratorTable(), but Microsoft Visual
     Studio 97 calls this function with a series of different handle
     values without ever calling CreateAcceleratorTable(). Something
     is very fishy in Denmark... */
  /* Update: looks like the calls to this function matches the calls
     to LoadAccelerators() in M$ Visual Studio, except that the handle
     values are off by some variable size from the HACCEL's returned
     from LoadAccelerators(). WTH? */
  
  /* Parameter checking to avoid any embarassing situations. */
#if 0
     if(!handle) {
       WARN(accel, "Application sent NULL ptr.\n");
       SetLastError(ERROR_INVALID_PARAMETER);
       return FALSE;
     }
  
     HeapFree(GetProcessHeap(), 0, (LPACCEL32)handle);
#endif

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

    hrsrc = FindResource16( instance, (SEGPTR)((resource_id>>4)+1), RT_STRING16 );
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
	WARN(resource,"Dont know why caller give buflen=%d *p=%d trying to obtain string '%s'\n", buflen, *p, p + 1);
    }
    FreeResource16( hmem );

    TRACE(resource,"'%s' loaded !\n", buffer);
    return i;
}

/**********************************************************************
 *	LoadString32W		(USER32.376)
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
                             RT_STRING32W );
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
	WARN(resource,"Dont know why caller give buflen=%d *p=%d trying to obtain string '%s'\n", buflen, *p, p + 1);
#endif
    }

    TRACE(resource,"%s loaded !\n", debugstr_w(buffer));
    return i;
}

/**********************************************************************
 *	LoadString32A	(USER32.375)
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
	else
	    *buffer = 0;
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
INT32 WINAPI LoadMessage32A( HMODULE32 instance, UINT32 id, WORD lang,
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
    hrsrc = FindResourceEx32W(instance,RT_MESSAGELIST32W,(LPWSTR)1,lang);
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
INT32 WINAPI LoadMessage32W( HMODULE32 instance, UINT32 id, WORD lang,
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
 *	EnumResourceTypesA	(KERNEL32.90)
 */
BOOL32 WINAPI EnumResourceTypes32A( HMODULE32 hmodule,ENUMRESTYPEPROC32A lpfun,
                                    LONG lParam)
{
	/* FIXME: move WINE_MODREF stuff here */
    return PE_EnumResourceTypes32A(hmodule,lpfun,lParam);
}

/**********************************************************************
 *	EnumResourceTypesW	(KERNEL32.91)
 */
BOOL32 WINAPI EnumResourceTypes32W( HMODULE32 hmodule,ENUMRESTYPEPROC32W lpfun,
                                    LONG lParam)
{
	/* FIXME: move WINE_MODREF stuff here */
    return PE_EnumResourceTypes32W(hmodule,lpfun,lParam);
}

/**********************************************************************
 *	EnumResourceNamesA	(KERNEL32.88)
 */
BOOL32 WINAPI EnumResourceNames32A( HMODULE32 hmodule, LPCSTR type,
                                    ENUMRESNAMEPROC32A lpfun, LONG lParam )
{
	/* FIXME: move WINE_MODREF stuff here */
    return PE_EnumResourceNames32A(hmodule,type,lpfun,lParam);
}
/**********************************************************************
 *	EnumResourceNamesW	(KERNEL32.89)
 */
BOOL32 WINAPI EnumResourceNames32W( HMODULE32 hmodule, LPCWSTR type,
                                    ENUMRESNAMEPROC32W lpfun, LONG lParam )
{
	/* FIXME: move WINE_MODREF stuff here */
    return PE_EnumResourceNames32W(hmodule,type,lpfun,lParam);
}

/**********************************************************************
 *	EnumResourceLanguagesA	(KERNEL32.86)
 */
BOOL32 WINAPI EnumResourceLanguages32A( HMODULE32 hmodule, LPCSTR type,
                                        LPCSTR name, ENUMRESLANGPROC32A lpfun,
                                        LONG lParam)
{
	/* FIXME: move WINE_MODREF stuff here */
    return PE_EnumResourceLanguages32A(hmodule,type,name,lpfun,lParam);
}
/**********************************************************************
 *	EnumResourceLanguagesW	(KERNEL32.87)
 */
BOOL32 WINAPI EnumResourceLanguages32W( HMODULE32 hmodule, LPCWSTR type,
                                        LPCWSTR name, ENUMRESLANGPROC32W lpfun,
                                        LONG lParam)
{
	/* FIXME: move WINE_MODREF stuff here */
    return PE_EnumResourceLanguages32W(hmodule,type,name,lpfun,lParam);
}
