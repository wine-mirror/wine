/*
 * USER resource functions
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 */

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"

#include "heap.h"
#include "ldt.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(resource);
DECLARE_DEBUG_CHANNEL(accel);

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
 *			LoadAcceleratorsW	(USER32.356)
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
 *		LoadAcceleratorsA   (USER32.355)
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
 *     LoadString16   (USER.176)
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
