/*
 *	COMPOBJ library
 *
 *	Copyright 1995	Martin von Loewis
 *	Copyright 1998	Justin Bradford
 */

#define INITGUID

#include "config.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ole.h"
#include "ole2.h"
#include "winerror.h"
#include "debug.h"
#include "file.h"
#include "compobj.h"
#include "heap.h"
#include "ldt.h"
#include "interfaces.h"
#include "shlobj.h"
#include "oleobj.h"
#include "ddraw.h"
#include "dsound.h"
#include "dinput.h"
#include "d3d.h"
#include "dplay.h"
#include "windows.h"


LPMALLOC16 currentMalloc16=NULL;
LPMALLOC32 currentMalloc32=NULL;

HTASK16 hETask = 0;
WORD Table_ETask[62];


/* this open DLL table belongs in a per process table, but my guess is that
 * it shouldn't live in the kernel, so I'll put them out here in DLL
 * space assuming that there is one OLE32 per process.
 */
typedef struct tagOpenDll {
  char *DllName;                /* really only needed for debugging */
    HINSTANCE32 hLibrary;       
    struct tagOpenDll *next;
} OpenDll;

static OpenDll *openDllList = NULL; /* linked list of open dlls */


/******************************************************************************
 *           CoBuildVersion [COMPOBJ.1]
 *
 * RETURNS
 *	Current built version, hiword is majornumber, loword is minornumber
 */
DWORD WINAPI CoBuildVersion(void)
{
    TRACE(ole,"(void)\n");
    return (rmm<<16)+rup;
}

/******************************************************************************
 *		CoInitialize16	[COMPOBJ.2]
 * Set the win16 IMalloc used for memory management
 */
HRESULT WINAPI CoInitialize16(
	LPMALLOC16 lpReserved	/* [in] pointer to win16 malloc interface */
) {
    currentMalloc16 = lpReserved;
    return S_OK;
}

/******************************************************************************
 *		CoInitialize32	[OLE32.26]
 * Set the win32 IMalloc used for memorymanagement
 */
HRESULT WINAPI CoInitialize32(
	LPMALLOC32 lpReserved	/* [in] pointer to win32 malloc interface */
) {
    /* FIXME: there really should be something here that incrememts a refcount
     * but I'm supposing that it is a real COM object, so I won't bother
     * creating one here.  (Decrement done in CoUnitialize()) */
    currentMalloc32 = lpReserved;
    return S_OK;
}

/***********************************************************************
 *           CoUnitialize   [COMPOBJ.3]
 * Don't know what it does. 
 * 3-Nov-98 -- this was originally misspelled, I changed it to what I
 *   believe is the correct spelling
 */
void WINAPI CoUninitialize(void)
{
    TRACE(ole,"(void)\n");
    CoFreeAllLibraries();
}

/***********************************************************************
 *           CoGetMalloc16    [COMPOBJ.4]
 * RETURNS
 *	The current win16 IMalloc
 */
HRESULT WINAPI CoGetMalloc16(
	DWORD dwMemContext,	/* [in] unknown */
	LPMALLOC16 * lpMalloc	/* [out] current win16 malloc interface */
) {
    if(!currentMalloc16)
	currentMalloc16 = IMalloc16_Constructor();
    *lpMalloc = currentMalloc16;
    return S_OK;
}

/******************************************************************************
 *		CoGetMalloc32	[OLE32.20]
 *
 * RETURNS
 *	The current win32 IMalloc
 */
HRESULT WINAPI CoGetMalloc32(
	DWORD dwMemContext,	/* [in] unknown */
	LPMALLOC32 *lpMalloc	/* [out] current win32 malloc interface */
) {
    if(!currentMalloc32)
	currentMalloc32 = IMalloc32_Constructor();
    *lpMalloc = currentMalloc32;
    return S_OK;
}

/***********************************************************************
 *           CoCreateStandardMalloc16 [COMPOBJ.71]
 */
OLESTATUS WINAPI CoCreateStandardMalloc16(DWORD dwMemContext,
					  LPMALLOC16 *lpMalloc)
{
    /* FIXME: docu says we shouldn't return the same allocator as in
     * CoGetMalloc16 */
    *lpMalloc = IMalloc16_Constructor();
    return S_OK;
}

/******************************************************************************
 *		CoDisconnectObject	[COMPOBJ.15]
 */
OLESTATUS WINAPI CoDisconnectObject( LPUNKNOWN lpUnk, DWORD reserved )
{
    TRACE(ole,"%p %lx\n",lpUnk,reserved);
    return S_OK;
}

/***********************************************************************
 *           IsEqualGUID [COMPOBJ.18]
 * Compares two Unique Identifiers
 * RETURNS
 *	TRUE if equal
 */
BOOL16 WINAPI IsEqualGUID(
	GUID* g1,	/* [in] unique id 1 */
	GUID* g2	/* [in] unique id 2 */
) {
    return !memcmp( g1, g2, sizeof(GUID) );
}

/******************************************************************************
 *		CLSIDFromString16	[COMPOBJ.20]
 * Converts a unique identifier from it's string representation into 
 * the GUID struct.
 *
 * Class id: DWORD-WORD-WORD-BYTES[2]-BYTES[6] 
 *
 * RETURNS
 *	the converted GUID
 */
OLESTATUS WINAPI CLSIDFromString16(
	LPCOLESTR16 idstr,	/* [in] string representation of guid */
	CLSID *id		/* [out] GUID converted from string */
) {
  BYTE *s = (BYTE *) idstr;
  BYTE *p;
  int	i;
  BYTE table[256];

  TRACE(ole,"%s -> %p\n", idstr, id);

  /* quick lookup table */
  memset(table, 0, 256);

  for (i = 0; i < 10; i++) {
    table['0' + i] = i;
  }
  for (i = 0; i < 6; i++) {
    table['A' + i] = i+10;
    table['a' + i] = i+10;
  }

  /* in form {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} */

  if (strlen(idstr) != 38)
    return OLE_ERROR_OBJECT;

  p = (BYTE *) id;

  s++;	/* skip leading brace  */
  for (i = 0; i < 4; i++) {
    p[3 - i] = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }
  p += 4;
  s++;	/* skip - */

  for (i = 0; i < 2; i++) {
    p[1-i] = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }
  p += 2;
  s++;	/* skip - */

  for (i = 0; i < 2; i++) {
    p[1-i] = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }
  p += 2;
  s++;	/* skip - */

  /* these are just sequential bytes */
  for (i = 0; i < 2; i++) {
    *p++ = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }
  s++;	/* skip - */

  for (i = 0; i < 6; i++) {
    *p++ = table[*s]<<4 | table[*(s+1)];
    s += 2;
  }

  return S_OK;
}

/******************************************************************************
 *		CoCreateGuid[OLE32.6]
 * Implemented according the DCE specification for UUID generation.
 * Code is based upon uuid library in e2fsprogs by Theodore Ts'o.
 * Copyright (C) 1996, 1997 Theodore Ts'o.
 */
int CoCreateGuid(GUID *pguid) {
   static char has_init = 0;
   unsigned char a[6];
   static int                      adjustment = 0;
   static struct timeval           last = {0, 0};
   static UINT16                   clock_seq;
   struct timeval                  tv;
   unsigned long long              clock_reg;
   UINT32 clock_high, clock_low;
   UINT16 temp_clock_seq, temp_clock_mid, temp_clock_hi_and_version;
#ifdef HAVE_NET_IF_H
   int             sd;
   struct ifreq    ifr, *ifrp;
   struct ifconf   ifc;
   char buf[1024];
   int             n, i;
#endif
   
   /* Have we already tried to get the MAC address? */
   if (!has_init) {
#ifdef HAVE_NET_IF_H
      /* BSD 4.4 defines the size of an ifreq to be
       * max(sizeof(ifreq), sizeof(ifreq.ifr_name)+ifreq.ifr_addr.sa_len
       * However, under earlier systems, sa_len isn't present, so
       *  the size is just sizeof(struct ifreq)
       */
# ifdef HAVE_SA_LEN
#  ifndef max
#   define max(a,b) ((a) > (b) ? (a) : (b))
#  endif
#  define ifreq_size(i) max(sizeof(struct ifreq),\
sizeof((i).ifr_name)+(i).ifr_addr.sa_len)
# else
#  define ifreq_size(i) sizeof(struct ifreq)
# endif /* HAVE_SA_LEN */
      
      sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
      if (sd < 0) {
	 /* if we can't open a socket, just use random numbers */
	 /* set the multicast bit to prevent conflicts with real cards */
	 a[0] = (rand() & 0xff) | 0x80;
	 a[1] = rand() & 0xff;
	 a[2] = rand() & 0xff;
	 a[3] = rand() & 0xff;
	 a[4] = rand() & 0xff;
	 a[5] = rand() & 0xff;
      } else {
	 memset(buf, 0, sizeof(buf));
	 ifc.ifc_len = sizeof(buf);
	 ifc.ifc_buf = buf;
	 /* get the ifconf interface */
	 if (ioctl (sd, SIOCGIFCONF, (char *)&ifc) < 0) {
	    close(sd);
	    /* no ifconf, so just use random numbers */
	    /* set the multicast bit to prevent conflicts with real cards */
	    a[0] = (rand() & 0xff) | 0x80;
	    a[1] = rand() & 0xff;
	    a[2] = rand() & 0xff;
	    a[3] = rand() & 0xff;
	    a[4] = rand() & 0xff;
	    a[5] = rand() & 0xff;
	 } else {
	    /* loop through the interfaces, looking for a valid one */
	    n = ifc.ifc_len;
	    for (i = 0; i < n; i+= ifreq_size(*ifr) ) {
	       ifrp = (struct ifreq *)((char *) ifc.ifc_buf+i);
	       strncpy(ifr.ifr_name, ifrp->ifr_name, IFNAMSIZ);
	       /* try to get the address for this interface */
# ifdef SIOCGIFHWADDR
	       if (ioctl(sd, SIOCGIFHWADDR, &ifr) < 0)
		   continue;
	       memcpy(a, (unsigned char *)&ifr.ifr_hwaddr.sa_data, 6);
# else
#  ifdef SIOCGENADDR
	       if (ioctl(sd, SIOCGENADDR, &ifr) < 0)
		   continue;
	       memcpy(a, (unsigned char *) ifr.ifr_enaddr, 6);
#  else
	       /* XXX we don't have a way of getting the hardware address */
	       close(sd);
	       a[0] = 0;
	       break;
#  endif /* SIOCGENADDR */
# endif /* SIOCGIFHWADDR */
	       /* make sure it's not blank */
	       if (!a[0] && !a[1] && !a[2] && !a[3] && !a[4] && !a[5])
		   continue;
						                
	       goto valid_address;
	    }
	    /* if we didn't find a valid address, make a random one */
	    /* once again, set multicast bit to avoid conflicts */
	    a[0] = (rand() & 0xff) | 0x80;
	    a[1] = rand() & 0xff;
	    a[2] = rand() & 0xff;
	    a[3] = rand() & 0xff;
	    a[4] = rand() & 0xff;
	    a[5] = rand() & 0xff;

	    valid_address:
	    close(sd);
	 }
      }
#else
      /* no networking info, so generate a random address */
      a[0] = (rand() & 0xff) | 0x80;
      a[1] = rand() & 0xff;
      a[2] = rand() & 0xff;
      a[3] = rand() & 0xff;
      a[4] = rand() & 0xff;
      a[5] = rand() & 0xff;
#endif /* HAVE_NET_IF_H */
      has_init = 1;
   }
   
   /* generate time element of GUID */
   
   /* Assume that the gettimeofday() has microsecond granularity */
#define MAX_ADJUSTMENT 10
                     
   try_again:
   gettimeofday(&tv, 0);
   if ((last.tv_sec == 0) && (last.tv_usec == 0)) {
      clock_seq = ((rand() & 0xff) << 8) + (rand() & 0xff);
      clock_seq &= 0x1FFF;
      last = tv;
      last.tv_sec--;
   }
   if ((tv.tv_sec < last.tv_sec) ||
       ((tv.tv_sec == last.tv_sec) &&
	(tv.tv_usec < last.tv_usec))) {
      clock_seq = (clock_seq+1) & 0x1FFF;
      adjustment = 0;
   } else if ((tv.tv_sec == last.tv_sec) &&
	      (tv.tv_usec == last.tv_usec)) {
      if (adjustment >= MAX_ADJUSTMENT)
	  goto try_again;
      adjustment++;
   } else
       adjustment = 0;
   
   clock_reg = tv.tv_usec*10 + adjustment;
   clock_reg += ((unsigned long long) tv.tv_sec)*10000000;
   clock_reg += (((unsigned long long) 0x01B21DD2) << 32) + 0x13814000;
   
   clock_high = clock_reg >> 32;
   clock_low = clock_reg;
   temp_clock_seq = clock_seq | 0x8000;
   temp_clock_mid = (UINT16)clock_high;
   temp_clock_hi_and_version = (clock_high >> 16) | 0x1000;
   
   /* pack the information into the GUID structure */
   
   ((unsigned char*)&pguid->Data1)[3] = (unsigned char)clock_low;
   clock_low >>= 8;
   ((unsigned char*)&pguid->Data1)[2] = (unsigned char)clock_low;
   clock_low >>= 8;
   ((unsigned char*)&pguid->Data1)[1] = (unsigned char)clock_low;
   clock_low >>= 8;
   ((unsigned char*)&pguid->Data1)[0] = (unsigned char)clock_low;
   
   ((unsigned char*)&pguid->Data2)[1] = (unsigned char)temp_clock_mid;
   temp_clock_mid >>= 8;
   ((unsigned char*)&pguid->Data2)[0] = (unsigned char)temp_clock_mid;
   
   ((unsigned char*)&pguid->Data3)[1] = (unsigned char)temp_clock_hi_and_version;
   temp_clock_hi_and_version >>= 8;
   ((unsigned char*)&pguid->Data3)[0] = (unsigned char)temp_clock_hi_and_version;
      
   ((unsigned char*)pguid->Data4)[1] = (unsigned char)temp_clock_seq;
   temp_clock_seq >>= 8;
   ((unsigned char*)pguid->Data4)[0] = (unsigned char)temp_clock_seq;
   
   ((unsigned char*)pguid->Data4)[2] = a[0];
   ((unsigned char*)pguid->Data4)[3] = a[1];
   ((unsigned char*)pguid->Data4)[4] = a[2];
   ((unsigned char*)pguid->Data4)[5] = a[3];
   ((unsigned char*)pguid->Data4)[6] = a[4];
   ((unsigned char*)pguid->Data4)[7] = a[5];
   
   TRACE(ole, "%p", pguid);
   
   return 1;
}

/******************************************************************************
 *		CLSIDFromString32	[OLE32.3]
 * Converts a unique identifier from it's string representation into 
 * the GUID struct.
 * RETURNS
 *	the converted GUID
 */
OLESTATUS WINAPI CLSIDFromString32(
	LPCOLESTR32 idstr,	/* [in] string representation of GUID */
	CLSID *id		/* [out] GUID represented by above string */
) {
    LPOLESTR16      xid = HEAP_strdupWtoA(GetProcessHeap(),0,idstr);
    OLESTATUS       ret = CLSIDFromString16(xid,id);

    HeapFree(GetProcessHeap(),0,xid);
    return ret;
}

/******************************************************************************
 *		WINE_StringFromCLSID	[???]
 * Converts a GUID into the respective string representation.
 *
 * NOTES
 *    Why is this WINAPI?
 *
 * RETURNS
 *	the string representation and OLESTATUS
 */
OLESTATUS WINAPI WINE_StringFromCLSID(
	const CLSID *id,	/* [in] GUID to be converted */
	LPSTR idstr		/* [out] pointer to buffer to contain converted guid */
) {
  static const char *hex = "0123456789ABCDEF";
  char *s;
  int	i;

  if (!id)
	{ ERR(ole,"called with id=Null\n");
	  *idstr = 0x00;
	  return E_FAIL;
	}
	
  sprintf(idstr, "{%08lX-%04X-%04X-%02x%02X-",
	  id->Data1, id->Data2, id->Data3,
	  id->Data4[0], id->Data4[1]);
  s = &idstr[25];

  /* 6 hex bytes */
  for (i = 2; i < 8; i++) {
    *s++ = hex[id->Data4[i]>>4];
    *s++ = hex[id->Data4[i] & 0xf];
  }

  *s++ = '}';
  *s++ = '\0';

  TRACE(ole,"%p->%s\n", id, idstr);

  return OLE_OK;
}

/******************************************************************************
 *		StringFromCLSID16	[COMPOBJ.19]
 * Converts a GUID into the respective string representation.
 * The target string is allocated using the OLE IMalloc.
 * RETURNS
 *	the string representation and OLESTATUS
 */
OLESTATUS WINAPI StringFromCLSID16(
	const CLSID *id,	/* [in] the GUID to be converted */
	LPOLESTR16 *idstr	/* [out] a pointer to a to-be-allocated segmented pointer pointing to the resulting string */

) {
    LPMALLOC16	mllc;
    OLESTATUS	ret;
    DWORD	args[2];

    ret = CoGetMalloc16(0,&mllc);
    if (ret) return ret;

    args[0] = (DWORD)mllc;
    args[1] = 40;

    /* No need for a Callback entry, we have WOWCallback16Ex which does
     * everything we need.
     */
    if (!WOWCallback16Ex(
    	(FARPROC16)((LPMALLOC16_VTABLE)PTR_SEG_TO_LIN(
		((LPMALLOC16)PTR_SEG_TO_LIN(mllc))->lpvtbl)
	)->fnAlloc,
	WCB16_CDECL,
	2,
	(LPVOID)args,
	(LPDWORD)idstr
    )) {
    	WARN(ole,"CallTo16 IMalloc16 failed\n");
    	return E_FAIL;
    }
    return WINE_StringFromCLSID(id,PTR_SEG_TO_LIN(*idstr));
}

/******************************************************************************
 *		StringFromCLSID32	[OLE32.151]
 * Converts a GUID into the respective string representation.
 * The target string is allocated using the OLE IMalloc.
 * RETURNS
 *	the string representation and OLESTATUS
 */
OLESTATUS WINAPI StringFromCLSID32(
	const CLSID *id,	/* [in] the GUID to be converted */
	LPOLESTR32 *idstr	/* [out] a pointer to a to-be-allocated pointer pointing to the resulting string */
) {
	char            buf[80];
	OLESTATUS       ret;
	LPMALLOC32	mllc;

	if ((ret=CoGetMalloc32(0,&mllc)))
		return ret;

	ret=WINE_StringFromCLSID(id,buf);
	if (!ret) {
		*idstr = mllc->lpvtbl->fnAlloc(mllc,strlen(buf)*2+2);
		lstrcpyAtoW(*idstr,buf);
	}
	return ret;
}

/******************************************************************************
 *		StringFromGUID2	[COMPOBJ.76] [OLE32.152]
 *
 * Converts a global unique identifier into a string of an API-
 * specified fixed format. (The usual {.....} stuff.)
 *
 * RETURNS
 *	The (UNICODE) string representation of the GUID in 'str'
 *	The length of the resulting string, 0 if there was any problem.
 */
INT32 WINAPI
StringFromGUID2(REFGUID id, LPOLESTR32 str, INT32 cmax)
{
  char		xguid[80];

  if (WINE_StringFromCLSID(id,xguid))
  	return 0;
  if (strlen(xguid)>=cmax)
  	return 0;
  lstrcpyAtoW(str,xguid);
  return strlen(xguid);
}

/******************************************************************************
 *		CLSIDFromProgID16	[COMPOBJ.61]
 * Converts a program id into the respective GUID. (By using a registry lookup)
 * RETURNS
 *	riid associated with the progid
 */
OLESTATUS WINAPI CLSIDFromProgID16(
	LPCOLESTR16 progid,	/* [in] program id as found in registry */
	LPCLSID riid		/* [out] associated CLSID */
) {
	char	*buf,buf2[80];
	DWORD	buf2len;
	HRESULT	err;
	HKEY	xhkey;

	buf = HeapAlloc(GetProcessHeap(),0,strlen(progid)+8);
	sprintf(buf,"%s\\CLSID",progid);
	if ((err=RegOpenKey32A(HKEY_CLASSES_ROOT,buf,&xhkey))) {
		HeapFree(GetProcessHeap(),0,buf);
		return OLE_ERROR_GENERIC;
	}
	HeapFree(GetProcessHeap(),0,buf);
	buf2len = sizeof(buf2);
	if ((err=RegQueryValue32A(xhkey,NULL,buf2,&buf2len))) {
		RegCloseKey(xhkey);
		return OLE_ERROR_GENERIC;
	}
	RegCloseKey(xhkey);
	return CLSIDFromString16(buf2,riid);
}

/******************************************************************************
 *		CLSIDFromProgID32	[OLE32.2]
 * Converts a program id into the respective GUID. (By using a registry lookup)
 * RETURNS
 *	riid associated with the progid
 */
OLESTATUS WINAPI CLSIDFromProgID32(
	LPCOLESTR32 progid,	/* [in] program id as found in registry */
	LPCLSID riid		/* [out] associated CLSID */
) {
	LPOLESTR16 pid = HEAP_strdupWtoA(GetProcessHeap(),0,progid);
	OLESTATUS       ret = CLSIDFromProgID16(pid,riid);

	HeapFree(GetProcessHeap(),0,pid);
	return ret;
}

/***********************************************************************
 *           LookupETask (COMPOBJ.94)
 */
OLESTATUS WINAPI LookupETask(HTASK16 *hTask,LPVOID p) {
	FIXME(ole,"(%p,%p),stub!\n",hTask,p);
	if ((*hTask = GetCurrentTask()) == hETask) {
		memcpy(p, Table_ETask, sizeof(Table_ETask));
	}
	return 0;
}

/***********************************************************************
 *           SetETask (COMPOBJ.95)
 */
OLESTATUS WINAPI SetETask(HTASK16 hTask, LPVOID p) {
        FIXME(ole,"(%04x,%p),stub!\n",hTask,p);
	hETask = hTask;
	return 0;
}

/***********************************************************************
 *           CallObjectInWOW (COMPOBJ.201)
 */
OLESTATUS WINAPI CallObjectInWOW(LPVOID p1,LPVOID p2) {
	FIXME(ole,"(%p,%p),stub!\n",p1,p2);
	return 0;
}

/******************************************************************************
 *		CoRegisterClassObject16	[COMPOBJ.5]
 *
 * Don't know where it registers it ...
 */
OLESTATUS WINAPI CoRegisterClassObject16(
	REFCLSID rclsid,
	LPUNKNOWN pUnk,
	DWORD dwClsContext,
	DWORD flags,
	LPDWORD lpdwRegister
) {
	char	buf[80];

	WINE_StringFromCLSID(rclsid,buf);

	FIXME(ole,"(%s,%p,0x%08lx,0x%08lx,%p),stub\n",
		buf,pUnk,dwClsContext,flags,lpdwRegister
	);
	return 0;
}

/******************************************************************************
 *		CoRegisterClassObject32	[OLE32.36]
 *
 * Don't know where it registers it ...
 */
OLESTATUS WINAPI CoRegisterClassObject32(
	REFCLSID rclsid,
	LPUNKNOWN pUnk,
	DWORD dwClsContext,
	DWORD flags,
	LPDWORD lpdwRegister
) {
    char buf[80];

    WINE_StringFromCLSID(rclsid,buf);

    FIXME(ole,"(%s,%p,0x%08lx,0x%08lx,%p),stub\n",
	    buf,pUnk,dwClsContext,flags,lpdwRegister
    );
    return 0;
}

/***********************************************************************
 *           CoRevokeClassObject [OLE32.40]
 */
HRESULT WINAPI CoRevokeClassObject(DWORD dwRegister) {
    FIXME(ole,"(%08lx),stub!\n",dwRegister);
    return S_OK;
}

/***********************************************************************
 *           CoGetClassObject [COMPOBJ.7]
 */
HRESULT WINAPI CoGetClassObject(REFCLSID rclsid, DWORD dwClsContext,
                        LPVOID pvReserved, const REFIID iid, LPVOID *ppv)
{
    char xclsid[50],xiid[50];
    HRESULT hres = E_UNEXPECTED;

    char dllName[MAX_PATH+1];
    LONG dllNameLen = sizeof(dllName);
    HINSTANCE32 hLibrary;
    typedef HRESULT (CALLBACK *DllGetClassObjectFunc)(REFCLSID clsid, 
			     REFIID iid, LPVOID *ppv);
    DllGetClassObjectFunc DllGetClassObject;

    WINE_StringFromCLSID((LPCLSID)rclsid,xclsid);
    WINE_StringFromCLSID((LPCLSID)iid,xiid);
    TRACE(ole,"\n\tCLSID:\t%s,\n\tIID:\t%s\n",xclsid,xiid);

    /* out of process and remote servers not supported yet */
    if ((CLSCTX_LOCAL_SERVER|CLSCTX_REMOTE_SERVER) & dwClsContext) {
        FIXME(ole, "CLSCTX_LOCAL_SERVER and CLSCTX_REMOTE_SERVER not supported!\n");
	return E_ACCESSDENIED;
    }

    if ((CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER) & dwClsContext) {
        HKEY CLSIDkey,key;

        /* lookup CLSID in registry key HKCR/CLSID */
        hres = RegOpenKeyEx32A(HKEY_CLASSES_ROOT, "CLSID", 0, 
			       KEY_READ, &CLSIDkey);

	if (hres != ERROR_SUCCESS)
	        return REGDB_E_READREGDB;
        hres = RegOpenKeyEx32A(CLSIDkey,xclsid,0,KEY_QUERY_VALUE,&key);
	if (hres != ERROR_SUCCESS) {
	    RegCloseKey(CLSIDkey);
	    return REGDB_E_CLASSNOTREG;
	    }
	    hres = RegQueryValue32A(key, "InprocServer32", dllName, &dllNameLen);
	RegCloseKey(key);
	RegCloseKey(CLSIDkey);
	if (hres != ERROR_SUCCESS)
	        return REGDB_E_READREGDB;
	    TRACE(ole,"found InprocServer32 dll %s\n", dllName);

	/* open dll, call DllGetClassFactory */
	hLibrary = CoLoadLibrary(dllName, TRUE);
	if (hLibrary == 0) {
	    TRACE(ole,"couldn't load InprocServer32 dll %s\n", dllName);
	    return E_ACCESSDENIED; /* or should this be CO_E_DLLNOTFOUND? */
	}
	DllGetClassObject = (DllGetClassObjectFunc)GetProcAddress32(hLibrary, "DllGetClassObject");
	if (!DllGetClassObject) {
	    /* not sure if this should be called here CoFreeLibrary(hLibrary);*/
	    TRACE(ole,"couldn't find function DllGetClassObject in %s\n", dllName);
	    return E_ACCESSDENIED;
	}
	/* FIXME: Shouldn't we get a classfactory and use this to create 
	 * the required object?
	 */
	return DllGetClassObject(rclsid, iid, ppv);
    }
    return hres;
}

/******************************************************************************
 *		CoRegisterMessageFilter16	[COMPOBJ.27]
 */
OLESTATUS WINAPI CoRegisterMessageFilter16(
	LPMESSAGEFILTER lpMessageFilter,
	LPMESSAGEFILTER *lplpMessageFilter
) {
	FIXME(ole,"(%p,%p),stub!\n",lpMessageFilter,lplpMessageFilter);
	return 0;
}

/***********************************************************************
 *           CoCreateInstance [COMPOBJ.13, OLE32.7]
 */
HRESULT WINAPI CoCreateInstance(
	REFCLSID rclsid,
	LPUNKNOWN pUnkOuter,
	DWORD dwClsContext,
	REFIID iid,
	LPVOID *ppv
) {
#if 0
	char buf[80],xbuf[80];

	if (rclsid)
		WINE_StringFromCLSID(rclsid,buf);
	else
		sprintf(buf,"<rclsid-0x%08lx>",(DWORD)rclsid);
	if (iid)
		WINE_StringFromCLSID(iid,xbuf);
	else
		sprintf(xbuf,"<iid-0x%08lx>",(DWORD)iid);

	FIXME(ole,"(%s,%p,0x%08lx,%s,%p): stub !\n",buf,pUnkOuter,dwClsContext,xbuf,ppv);
	*ppv = NULL;
#else	
	HRESULT hres;
	LPCLASSFACTORY lpclf = 0;

        hres = CoGetClassObject(rclsid, dwClsContext, NULL, (const REFIID) &IID_IClassFactory, (LPVOID)&lpclf);
        if (!SUCCEEDED(hres)) return hres;
	hres = lpclf->lpvtbl->fnCreateInstance(lpclf, pUnkOuter, iid, ppv);
	lpclf->lpvtbl->fnRelease(lpclf);
	return hres;
#endif
}



/***********************************************************************
 *           CoFreeLibrary [COMPOBJ.13]
 */
void WINAPI CoFreeLibrary(HINSTANCE32 hLibrary)
{
    OpenDll *ptr, *prev;
    OpenDll *tmp;

    /* lookup library in linked list */
    prev = NULL;
    for (ptr = openDllList; ptr != NULL; ptr=ptr->next) {
	if (ptr->hLibrary == hLibrary) {
	    break;
	}
	prev = ptr;
    }

    if (ptr == NULL) {
	/* shouldn't happen if user passed in a valid hLibrary */
	return;
    }
    /* assert: ptr points to the library entry to free */

    /* free library and remove node from list */
    FreeLibrary32(hLibrary);
    if (ptr == openDllList) {
	tmp = openDllList->next;
	HeapFree(GetProcessHeap(), 0, openDllList->DllName);
	HeapFree(GetProcessHeap(), 0, openDllList);
	openDllList = tmp;
    } else {
	tmp = ptr->next;
	HeapFree(GetProcessHeap(), 0, ptr->DllName);
	HeapFree(GetProcessHeap(), 0, ptr);
	prev->next = tmp;
    }

}


/***********************************************************************
 *           CoFreeAllLibraries [COMPOBJ.12]
 */
void WINAPI CoFreeAllLibraries(void)
{
    OpenDll *ptr, *tmp;

    for (ptr = openDllList; ptr != NULL; ) {
	tmp=ptr->next;
	CoFreeLibrary(ptr->hLibrary);
	ptr = tmp;
    }
}



/***********************************************************************
 *           CoFreeUnusedLibraries [COMPOBJ.17]
 */
void WINAPI CoFreeUnusedLibraries(void)
{
    OpenDll *ptr, *tmp;
    typedef HRESULT(*DllCanUnloadNowFunc)(void);
    DllCanUnloadNowFunc DllCanUnloadNow;

    for (ptr = openDllList; ptr != NULL; ) {
	DllCanUnloadNow = (DllCanUnloadNowFunc)
	    GetProcAddress32(ptr->hLibrary, "DllCanUnloadNow");
	
	if (DllCanUnloadNow() == S_OK) {
	    tmp=ptr->next;
	    CoFreeLibrary(ptr->hLibrary);
	    ptr = tmp;
	} else {
	    ptr=ptr->next;
	}
    }
}

/***********************************************************************
 *           CoFileTimeNow [COMPOBJ.82, OLE32.10]
 * RETURNS
 *	the current system time in lpFileTime
 */
HRESULT WINAPI CoFileTimeNow(
	FILETIME *lpFileTime	/* [out] the current time */
) {
	DOSFS_UnixTimeToFileTime(time(NULL), lpFileTime, 0);
	return S_OK;
}

/***********************************************************************
 *           CoTaskMemAlloc (OLE32.43)
 * RETURNS
 * 	pointer to newly allocated block
 */
LPVOID WINAPI CoTaskMemAlloc(
	ULONG size	/* [in] size of memoryblock to be allocated */
) {
    LPMALLOC32	lpmalloc;
    HRESULT	ret = CoGetMalloc32(0,&lpmalloc);

    if (ret) 
	return NULL;
    return lpmalloc->lpvtbl->fnAlloc(lpmalloc,size);
}

/***********************************************************************
 *           CoTaskMemFree (OLE32.44)
 */
VOID WINAPI CoTaskMemFree(
	LPVOID ptr	/* [in] pointer to be freed */
) {
    LPMALLOC32	lpmalloc;
    HRESULT	ret = CoGetMalloc32(0,&lpmalloc);

    if (ret) return;
    return lpmalloc->lpvtbl->fnFree(lpmalloc,ptr);
}

/***********************************************************************
 *           CoLoadLibrary (OLE32.30)
 */
HINSTANCE32 WINAPI CoLoadLibrary(LPSTR lpszLibName, BOOL32 bAutoFree)
{
    HINSTANCE32 hLibrary;
    OpenDll *ptr;
    OpenDll *tmp;
  
    TRACE(ole,"CoLoadLibrary(%p, %d\n", lpszLibName, bAutoFree);

    hLibrary = LoadLibrary32A(lpszLibName);

    if (!bAutoFree)
	return hLibrary;

    if (openDllList == NULL) {
        /* empty list -- add first node */
        openDllList = (OpenDll*)HeapAlloc(GetProcessHeap(),0, sizeof(OpenDll));
	openDllList->DllName = HEAP_strdupA(GetProcessHeap(), 0, lpszLibName);
	openDllList->hLibrary = hLibrary;
	openDllList->next = NULL;
    } else {
        /* search for this dll */
        int found = FALSE;
        for (ptr = openDllList; ptr->next != NULL; ptr=ptr->next) {
  	    if (ptr->hLibrary == hLibrary) {
	        found = TRUE;
		break;
	    }
        }
	if (!found) {
	    /* dll not found, add it */
 	    tmp = openDllList;
	    openDllList = (OpenDll*)HeapAlloc(GetProcessHeap(),0, sizeof(OpenDll));
	    openDllList->DllName = HEAP_strdupA(GetProcessHeap(), 0, lpszLibName);
	    openDllList->hLibrary = hLibrary;
	    openDllList->next = tmp;
	}
    }
     
    return hLibrary;
}

/***********************************************************************
 *           CoInitializeWOW (OLE32.27)
 */
HRESULT WINAPI CoInitializeWOW(DWORD x,DWORD y) {
    FIXME(ole,"(0x%08lx,0x%08lx),stub!\n",x,y);
    return 0;
}

/******************************************************************************
 *		CoLockObjectExternal16	[COMPOBJ.63]
 */
HRESULT WINAPI CoLockObjectExternal16(
    LPUNKNOWN pUnk,		/* [in] object to be locked */
    BOOL16 fLock,		/* [in] do lock */
    BOOL16 fLastUnlockReleases	/* [in] ? */
) {
    FIXME(ole,"(%p,%d,%d),stub!\n",pUnk,fLock,fLastUnlockReleases);
    return S_OK;
}

/******************************************************************************
 *		CoLockObjectExternal32	[OLE32.31]
 */
HRESULT WINAPI CoLockObjectExternal32(
    LPUNKNOWN pUnk,		/* [in] object to be locked */
    BOOL32 fLock,		/* [in] do lock */
    BOOL32 fLastUnlockReleases	/* [in] ? */
) {
    FIXME(ole,"(%p,%d,%d),stub!\n",pUnk,fLock,fLastUnlockReleases);
    return S_OK;
}

/***********************************************************************
 *           CoGetState16 [COMPOBJ.115]
 */
HRESULT WINAPI CoGetState16(LPDWORD state)
{
    FIXME(ole, "(%p),stub!\n", state);
    *state = 0;
    return S_OK;
}
/***********************************************************************
 *           CoSetState32 [COM32.42]
 */
HRESULT WINAPI CoSetState32(LPDWORD state)
{
    FIXME(ole, "(%p),stub!\n", state);
    *state = 0;
    return S_OK;
}
