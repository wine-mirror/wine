/*
 *	COMPOBJ library
 *
 *	Copyright 1995	Martin von Loewis
 *	Copyright 1998	Justin Bradford
 *      Copyright 1999  Francis Beaudet
 *  Copyright 1999  Sylvain St-Germain
 */

#include "config.h"
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_FILE_H
# include <sys/file.h>
#endif
#include <sys/ioctl.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_SOCKIO_H
# include <sys/sockio.h>
#endif
#ifdef HAVE_NET_IF_H
# include <net/if.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "windef.h"
#include "wine/winbase16.h"
#include "wine/winestring.h"
#include "winerror.h"
#include "ole.h"
#include "ole2ver.h"
#include "debug.h"
#include "file.h"
#include "heap.h"
#include "ldt.h"
#include "winreg.h"

#include "wine/obj_base.h"
#include "wine/obj_misc.h"
#include "wine/obj_clientserver.h"

#include "ifs.h"

/****************************************************************************
 *  COM External Lock structures and methods declaration
 *
 *  This api provides a linked list to managed external references to 
 *  COM objects.  
 *
 *  The public interface consists of three calls: 
 *      COM_ExternalLockAddRef
 *      COM_ExternalLockRelease
 *      COM_ExternalLockFreeList
 */

#define EL_END_OF_LIST 0
#define EL_NOT_FOUND   0

/*
 * Declaration of the static structure that manage the 
 * external lock to COM  objects.
 */
typedef struct COM_ExternalLock     COM_ExternalLock;
typedef struct COM_ExternalLockList COM_ExternalLockList;

struct COM_ExternalLock
{
  IUnknown         *pUnk;     /* IUnknown referenced */
  ULONG            uRefCount; /* external lock counter to IUnknown object*/
  COM_ExternalLock *next;     /* Pointer to next element in list */
};

struct COM_ExternalLockList
{
  COM_ExternalLock *head;     /* head of list */
};

/*
 * Declaration and initialization of the static structure that manages
 * the external lock to COM objects.
 */
static COM_ExternalLockList elList = { EL_END_OF_LIST };

/*
 * Public Interface to the external lock list   
 */
static void COM_ExternalLockFreeList();
static void COM_ExternalLockAddRef(IUnknown *pUnk);
static void COM_ExternalLockRelease(IUnknown *pUnk, BOOL bRelAll);
void COM_ExternalLockDump(); /* testing purposes, not static to avoid warning */

/*
 * Private methods used to managed the linked list   
 */
static BOOL COM_ExternalLockInsert(
  IUnknown *pUnk);

static void COM_ExternalLockDelete(
  COM_ExternalLock *element);

static COM_ExternalLock* COM_ExternalLockFind(
  IUnknown *pUnk);

static COM_ExternalLock* COM_ExternalLockLocate(
  COM_ExternalLock *element,
  IUnknown         *pUnk);

/****************************************************************************
 * This section defines variables internal to the COM module.
 *
 * TODO: Most of these things will have to be made thread-safe.
 */
LPMALLOC16 currentMalloc16=NULL;
LPMALLOC currentMalloc32=NULL;

HTASK16 hETask = 0;
WORD Table_ETask[62];

/*
 * This lock count counts the number of times CoInitialize is called. It is
 * decreased every time CoUninitialize is called. When it hits 0, the COM
 * libraries are freed
 */
static ULONG s_COMLockCount = 0;

/*
 * This linked list contains the list of registered class objects. These
 * are mostly used to register the factories for out-of-proc servers of OLE
 * objects.
 *
 * TODO: Make this data structure aware of inter-process communication. This
 *       means that parts of this will be exported to the Wine Server.
 */
typedef struct tagRegisteredClass
{
  CLSID     classIdentifier;
  LPUNKNOWN classObject;
  DWORD     runContext;
  DWORD     connectFlags;
  DWORD     dwCookie;
  struct tagRegisteredClass* nextClass;
} RegisteredClass;

static RegisteredClass* firstRegisteredClass = NULL;

/* this open DLL table belongs in a per process table, but my guess is that
 * it shouldn't live in the kernel, so I'll put them out here in DLL
 * space assuming that there is one OLE32 per process.
 */
typedef struct tagOpenDll {
  char *DllName;                /* really only needed for debugging */
    HINSTANCE hLibrary;       
    struct tagOpenDll *next;
} OpenDll;

static OpenDll *openDllList = NULL; /* linked list of open dlls */

/*****************************************************************************
 * This section contains prototypes to internal methods for this
 * module
 */
static HRESULT COM_GetRegisteredClassObject(REFCLSID    rclsid,
					    DWORD       dwClsContext,
					    LPUNKNOWN*  ppUnk);

static void COM_RevokeAllClasses();


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
	LPVOID lpReserved	/* [in] pointer to win16 malloc interface */
) {
    currentMalloc16 = (LPMALLOC16)lpReserved;
    return S_OK;
}

/******************************************************************************
 *		CoInitialize32	[OLE32.26]
 *
 * Initializes the COM libraries.
 *
 * See CoInitializeEx32
 */
HRESULT WINAPI CoInitialize(
	LPVOID lpReserved	/* [in] pointer to win32 malloc interface
                                   (obsolete, should be NULL) */
) 
{
  /*
   * Just delegate to the newer method.
   */
  return CoInitializeEx(lpReserved, COINIT_APARTMENTTHREADED);
}

/******************************************************************************
 *		CoInitializeEx32	[OLE32.163]
 *
 * Initializes the COM libraries. The behavior used to set the win32 IMalloc
 * used for memory management is obsolete.
 *
 * RETURNS
 *  S_OK               if successful,
 *  S_FALSE            if this function was called already.
 *  RPC_E_CHANGED_MODE if a previous call to CoInitialize specified another
 *                      threading model.
 *
 * BUGS
 * Only the single threaded model is supported. As a result RPC_E_CHANGED_MODE 
 * is never returned.
 *
 * See the windows documentation for more details.
 */
HRESULT WINAPI CoInitializeEx(
	LPVOID lpReserved,	/* [in] pointer to win32 malloc interface
                                   (obsolete, should be NULL) */
	DWORD dwCoInit		/* [in] A value from COINIT specifies the threading model */
) 
{
  HRESULT hr;

  TRACE(ole, "(%p, %x)\n", lpReserved, (int)dwCoInit);

  if (lpReserved!=NULL)
  {
    ERR(ole,"(%p, %x) - Bad parameter passed-in %p, must be an old Windows Application\n", lpReserved, (int)dwCoInit, lpReserved);
  }

  /*
   * Check for unsupported features.
   */
  if (dwCoInit!=COINIT_APARTMENTTHREADED) 
  {
    FIXME(ole, ":(%p,%x): unsupported flag %x\n", lpReserved, (int)dwCoInit, (int)dwCoInit);
    /* Hope for the best and continue anyway */
  }

  /*
   * Check the lock count. If this is the first time going through the initialize
   * process, we have to initialize the libraries.
   */
  if (s_COMLockCount==0)
  {
    /*
     * Initialize the various COM libraries and data structures.
     */
    TRACE(ole, "() - Initializing the COM libraries\n");

    hr = S_OK;
  }
  else
    hr = S_FALSE;

  /*
   * Crank-up that lock count.
   */
  s_COMLockCount++;

  return hr;
}

/***********************************************************************
 *           CoUninitialize16   [COMPOBJ.3]
 * Don't know what it does. 
 * 3-Nov-98 -- this was originally misspelled, I changed it to what I
 *   believe is the correct spelling
 */
void WINAPI CoUninitialize16(void)
{
  TRACE(ole,"()\n");
  CoFreeAllLibraries();
}

/***********************************************************************
 *           CoUninitialize32   [OLE32.47]
 *
 * This method will release the COM libraries.
 *
 * See the windows documentation for more details.
 */
void WINAPI CoUninitialize(void)
{
  TRACE(ole,"()\n");
  
  /*
   * Decrease the reference count.
   */
  s_COMLockCount--;
  
  /*
   * If we are back to 0 locks on the COM library, make sure we free
   * all the associated data structures.
   */
  if (s_COMLockCount==0)
  {
    /*
     * Release the various COM libraries and data structures.
     */
    TRACE(ole, "() - Releasing the COM libraries\n");

    /*
     * Release the references to the registered class objects.
     */
    COM_RevokeAllClasses();

    /*
     * This will free the loaded COM Dlls.
     */
    CoFreeAllLibraries();

    /*
     * This will free list of external references to COM objects.
     */
    COM_ExternalLockFreeList();
}
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
HRESULT WINAPI CoGetMalloc(
	DWORD dwMemContext,	/* [in] unknown */
	LPMALLOC *lpMalloc	/* [out] current win32 malloc interface */
) {
    if(!currentMalloc32)
	currentMalloc32 = IMalloc_Constructor();
    *lpMalloc = currentMalloc32;
    return S_OK;
}

/***********************************************************************
 *           CoCreateStandardMalloc16 [COMPOBJ.71]
 */
HRESULT WINAPI CoCreateStandardMalloc16(DWORD dwMemContext,
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
HRESULT WINAPI CoDisconnectObject( LPUNKNOWN lpUnk, DWORD reserved )
{
    TRACE(ole,"%p %lx\n",lpUnk,reserved);
    return S_OK;
}

/***********************************************************************
 *           IsEqualGUID16 [COMPOBJ.18]
 *
 * Compares two Unique Identifiers.
 *
 * RETURNS
 *	TRUE if equal
 */
BOOL16 WINAPI IsEqualGUID16(
	GUID* g1,	/* [in] unique id 1 */
	GUID* g2	/**/
) {
    return !memcmp( g1, g2, sizeof(GUID) );
}

/***********************************************************************
 *           IsEqualGUID32 [OLE32.76]
 *
 * Compares two Unique Identifiers.
 *
 * RETURNS
 *	TRUE if equal
 */
BOOL WINAPI IsEqualGUID32(
     REFGUID rguid1, /* [in] unique id 1 */
     REFGUID rguid2  /* [in] unique id 2 */
     )
{
    return !memcmp(rguid1,rguid2,sizeof(GUID));
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
HRESULT WINAPI CLSIDFromString16(
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
 *
 * Creates a 128bit GUID.
 * Implemented according the DCE specification for UUID generation.
 * Code is based upon uuid library in e2fsprogs by Theodore Ts'o.
 * Copyright (C) 1996, 1997 Theodore Ts'o.
 *
 * RETURNS
 *
 *  S_OK if successful.
 */
HRESULT WINAPI CoCreateGuid(
	GUID *pguid /* [out] points to the GUID to initialize */
) {
   static char has_init = 0;
   unsigned char a[6];
   static int                      adjustment = 0;
   static struct timeval           last = {0, 0};
   static UINT16                   clock_seq;
   struct timeval                  tv;
   unsigned long long              clock_reg;
   UINT clock_high, clock_low;
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
   
   return S_OK;
}

/******************************************************************************
 *		CLSIDFromString32	[OLE32.3]
 * Converts a unique identifier from it's string representation into 
 * the GUID struct.
 * RETURNS
 *	the converted GUID
 */
HRESULT WINAPI CLSIDFromString(
	LPCOLESTR idstr,	/* [in] string representation of GUID */
	CLSID *id		/* [out] GUID represented by above string */
) {
    LPOLESTR16      xid = HEAP_strdupWtoA(GetProcessHeap(),0,idstr);
    OLESTATUS       ret = CLSIDFromString16(xid,id);

    HeapFree(GetProcessHeap(),0,xid);
    return ret;
}

/******************************************************************************
 *		WINE_StringFromCLSID	[Internal]
 * Converts a GUID into the respective string representation.
 *
 * NOTES
 *
 * RETURNS
 *	the string representation and OLESTATUS
 */
HRESULT WINE_StringFromCLSID(
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
HRESULT WINAPI StringFromCLSID16(
        REFCLSID id,            /* [in] the GUID to be converted */
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
    	(FARPROC16)((ICOM_VTABLE(IMalloc16)*)PTR_SEG_TO_LIN(
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
HRESULT WINAPI StringFromCLSID(
        REFCLSID id,            /* [in] the GUID to be converted */
	LPOLESTR *idstr	/* [out] a pointer to a to-be-allocated pointer pointing to the resulting string */
) {
	char            buf[80];
	OLESTATUS       ret;
	LPMALLOC	mllc;

	if ((ret=CoGetMalloc(0,&mllc)))
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
INT WINAPI
StringFromGUID2(REFGUID id, LPOLESTR str, INT cmax)
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
HRESULT WINAPI CLSIDFromProgID16(
	LPCOLESTR16 progid,	/* [in] program id as found in registry */
	LPCLSID riid		/* [out] associated CLSID */
) {
	char	*buf,buf2[80];
	DWORD	buf2len;
	HRESULT	err;
	HKEY	xhkey;

	buf = HeapAlloc(GetProcessHeap(),0,strlen(progid)+8);
	sprintf(buf,"%s\\CLSID",progid);
	if ((err=RegOpenKeyA(HKEY_CLASSES_ROOT,buf,&xhkey))) {
		HeapFree(GetProcessHeap(),0,buf);
		return OLE_ERROR_GENERIC;
	}
	HeapFree(GetProcessHeap(),0,buf);
	buf2len = sizeof(buf2);
	if ((err=RegQueryValueA(xhkey,NULL,buf2,&buf2len))) {
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
HRESULT WINAPI CLSIDFromProgID(
	LPCOLESTR progid,	/* [in] program id as found in registry */
	LPCLSID riid		/* [out] associated CLSID */
) {
	LPOLESTR16 pid = HEAP_strdupWtoA(GetProcessHeap(),0,progid);
	OLESTATUS       ret = CLSIDFromProgID16(pid,riid);

	HeapFree(GetProcessHeap(),0,pid);
	return ret;
}

/* FIXME: this function is not declared in the WINELIB headers. But where should it go ? */
/***********************************************************************
 *           LookupETask (COMPOBJ.94)
 */
OLESTATUS WINAPI LookupETask16(HTASK16 *hTask,LPVOID p) {
	FIXME(ole,"(%p,%p),stub!\n",hTask,p);
	if ((*hTask = GetCurrentTask()) == hETask) {
		memcpy(p, Table_ETask, sizeof(Table_ETask));
	}
	return 0;
}

/* FIXME: this function is not declared in the WINELIB headers. But where should it go ? */
/***********************************************************************
 *           SetETask (COMPOBJ.95)
 */
OLESTATUS WINAPI SetETask16(HTASK16 hTask, LPVOID p) {
        FIXME(ole,"(%04x,%p),stub!\n",hTask,p);
	hETask = hTask;
	return 0;
}

/* FIXME: this function is not declared in the WINELIB headers. But where should it go ? */
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
HRESULT WINAPI CoRegisterClassObject16(
	REFCLSID rclsid,
	LPUNKNOWN pUnk,
	DWORD dwClsContext, /* [in] CLSCTX flags indicating the context in which to run the executable */
	DWORD flags,        /* [in] REGCLS flags indicating how connections are made */
	LPDWORD lpdwRegister
) {
	char	buf[80];

	WINE_StringFromCLSID(rclsid,buf);

	FIXME(ole,"(%s,%p,0x%08lx,0x%08lx,%p),stub\n",
		buf,pUnk,dwClsContext,flags,lpdwRegister
	);
	return 0;
}

/***
 * COM_GetRegisteredClassObject
 *
 * This internal method is used to scan the registered class list to 
 * find a class object.
 *
 * Params: 
 *   rclsid        Class ID of the class to find.
 *   dwClsContext  Class context to match.
 *   ppv           [out] returns a pointer to the class object. Complying
 *                 to normal COM usage, this method will increase the
 *                 reference count on this object.
 */
static HRESULT COM_GetRegisteredClassObject(
	REFCLSID    rclsid,
	DWORD       dwClsContext,
	LPUNKNOWN*  ppUnk)
{
  RegisteredClass* curClass;

  /*
   * Sanity check
   */
  assert(ppUnk!=0);

  /*
   * Iterate through the whole list and try to match the class ID.
   */
  curClass = firstRegisteredClass;

  while (curClass != 0)
  {
    /*
     * Check if we have a match on the class ID.
     */
    if (IsEqualGUID32(&(curClass->classIdentifier), rclsid))
    {
      /*
       * Since we don't do out-of process or DCOM just right away, let's ignore the
       * class context.
       */

      /*
       * We have a match, return the pointer to the class object.
       */
      *ppUnk = curClass->classObject;

      IUnknown_AddRef(curClass->classObject);

      return S_OK;
    }

    /*
     * Step to the next class in the list.
     */
    curClass = curClass->nextClass;
  }

  /*
   * If we get to here, we haven't found our class.
   */
  return S_FALSE;
}

/******************************************************************************
 *		CoRegisterClassObject32	[OLE32.36]
 *
 * This method will register the class object for a given class ID.
 *
 * See the Windows documentation for more details.
 */
HRESULT WINAPI CoRegisterClassObject(
	REFCLSID rclsid,
	LPUNKNOWN pUnk,
	DWORD dwClsContext, /* [in] CLSCTX flags indicating the context in which to run the executable */
	DWORD flags,        /* [in] REGCLS flags indicating how connections are made */
	LPDWORD lpdwRegister
) 
{
  RegisteredClass* newClass;
  LPUNKNOWN        foundObject;
  HRESULT          hr;
    char buf[80];

    WINE_StringFromCLSID(rclsid,buf);

  TRACE(ole,"(%s,%p,0x%08lx,0x%08lx,%p)\n",
	buf,pUnk,dwClsContext,flags,lpdwRegister);

  /*
   * Perform a sanity check on the parameters
   */
  if ( (lpdwRegister==0) || (pUnk==0) )
  {
    return E_INVALIDARG;
}

  /*
   * Initialize the cookie (out parameter)
   */
  *lpdwRegister = 0;

  /*
   * First, check if the class is already registered.
   * If it is, this should cause an error.
   */
  hr = COM_GetRegisteredClassObject(rclsid, dwClsContext, &foundObject);

  if (hr == S_OK)
  {
    /*
     * The COM_GetRegisteredClassObject increased the reference count on the
     * object so it has to be released.
     */
    IUnknown_Release(foundObject);

    return CO_E_OBJISREG;
  }
    
  /*
   * If it is not registered, we must create a new entry for this class and
   * append it to the registered class list.
   * We use the address of the chain node as the cookie since we are sure it's
   * unique.
   */
  newClass = HeapAlloc(GetProcessHeap(), 0, sizeof(RegisteredClass));

  /*
   * Initialize the node.
   */
  newClass->classIdentifier = *rclsid;
  newClass->runContext      = dwClsContext;
  newClass->connectFlags    = flags;
  newClass->dwCookie        = (DWORD)newClass;
  newClass->nextClass       = firstRegisteredClass;

  /*
   * Since we're making a copy of the object pointer, we have to increase it's
   * reference count.
   */
  newClass->classObject     = pUnk;
  IUnknown_AddRef(newClass->classObject);

  firstRegisteredClass = newClass;
    
  /*
   * We're successfyl Yippee!
   */
  return S_OK;
}

/***********************************************************************
 *           CoRevokeClassObject32 [OLE32.40]
 *
 * This method will remove a class object from the class registry
 *
 * See the Windows documentation for more details.
 */
HRESULT WINAPI CoRevokeClassObject(
        DWORD dwRegister) 
{
  RegisteredClass** prevClassLink;
  RegisteredClass*  curClass;

  TRACE(ole,"(%08lx)\n",dwRegister);

  /*
   * Iterate through the whole list and try to match the cookie.
   */
  curClass      = firstRegisteredClass;
  prevClassLink = &firstRegisteredClass;

  while (curClass != 0)
  {
    /*
     * Check if we have a match on the cookie.
     */
    if (curClass->dwCookie == dwRegister)
    {
      /*
       * Remove the class from the chain.
       */
      *prevClassLink = curClass->nextClass;

      /*
       * Release the reference to the class object.
       */
      IUnknown_Release(curClass->classObject);

      /*
       * Free the memory used by the chain node.
 */
      HeapFree(GetProcessHeap(), 0, curClass);

    return S_OK;
}

    /*
     * Step to the next class in the list.
     */
    prevClassLink = &(curClass->nextClass);
    curClass      = curClass->nextClass;
  }

  /*
   * If we get to here, we haven't found our class.
   */
  return E_INVALIDARG;
}

/***********************************************************************
 *           CoGetClassObject [COMPOBJ.7]
 */
HRESULT WINAPI CoGetClassObject(REFCLSID rclsid, DWORD dwClsContext,
                        LPVOID pvReserved, REFIID iid, LPVOID *ppv)
{
    LPUNKNOWN regClassObject;
    char xclsid[50],xiid[50];
    HRESULT hres = E_UNEXPECTED;

    char dllName[MAX_PATH+1];
    DWORD dllNameLen = sizeof(dllName);
    HINSTANCE hLibrary;
    typedef HRESULT (CALLBACK *DllGetClassObjectFunc)(REFCLSID clsid, 
			     REFIID iid, LPVOID *ppv);
    DllGetClassObjectFunc DllGetClassObject;

    WINE_StringFromCLSID((LPCLSID)rclsid,xclsid);
    WINE_StringFromCLSID((LPCLSID)iid,xiid);
    TRACE(ole,"\n\tCLSID:\t%s,\n\tIID:\t%s\n",xclsid,xiid);

    /*
     * First, try and see if we can't match the class ID with one of the 
     * registered classes.
     */
    if (S_OK == COM_GetRegisteredClassObject(rclsid, dwClsContext, &regClassObject))
    {
      /*
       * Get the required interface from the retrieved pointer.
       */
      hres = IUnknown_QueryInterface(regClassObject, iid, ppv);

      /*
       * Since QI got another reference on the pointer, we want to release the
       * one we already have. If QI was unsuccessful, this will release the object. This
       * is good since we are not returning it in the "out" parameter.
       */
      IUnknown_Release(regClassObject);

      return hres;
    }

    /* out of process and remote servers not supported yet */
    if ((CLSCTX_LOCAL_SERVER|CLSCTX_REMOTE_SERVER) & dwClsContext) {
        FIXME(ole, "CLSCTX_LOCAL_SERVER and CLSCTX_REMOTE_SERVER not supported!\n");
	return E_ACCESSDENIED;
    }

    if ((CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER) & dwClsContext) {
        HKEY CLSIDkey,key;

        /* lookup CLSID in registry key HKCR/CLSID */
        hres = RegOpenKeyExA(HKEY_CLASSES_ROOT, "CLSID", 0, 
			       KEY_READ, &CLSIDkey);

	if (hres != ERROR_SUCCESS)
	        return REGDB_E_READREGDB;
        hres = RegOpenKeyExA(CLSIDkey,xclsid,0,KEY_QUERY_VALUE,&key);
	if (hres != ERROR_SUCCESS) {
	    RegCloseKey(CLSIDkey);
	    return REGDB_E_CLASSNOTREG;
	}
	hres = RegQueryValueA(key, "InprocServer32", dllName, &dllNameLen);
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
	DllGetClassObject = (DllGetClassObjectFunc)GetProcAddress(hLibrary, "DllGetClassObject");
	if (!DllGetClassObject) {
	    /* not sure if this should be called here CoFreeLibrary(hLibrary);*/
	    TRACE(ole,"couldn't find function DllGetClassObject in %s\n", dllName);
	    return E_ACCESSDENIED;
	}

	/*
	 * Ask the DLL for it's class object. (there was a note here about class
	 * factories but this is good.
	 */
	return DllGetClassObject(rclsid, iid, ppv);
    }
    return hres;
}

/******************************************************************************
 *		CoRegisterMessageFilter16	[COMPOBJ.27]
 */
HRESULT WINAPI CoRegisterMessageFilter16(
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
	LPVOID *ppv) 
{
	HRESULT hres;
	LPCLASSFACTORY lpclf = 0;

  /*
   * Sanity check
   */
  if (ppv==0)
    return E_POINTER;

  /*
   * Initialize the "out" parameter
   */
  *ppv = 0;
  
  /*
   * Get a class factory to construct the object we want.
   */
  hres = CoGetClassObject(rclsid,
			  dwClsContext,
			  NULL,
			  &IID_IClassFactory,
			  (LPVOID)&lpclf);

  if (FAILED(hres))
    return hres;

  /*
   * Create the object and don't forget to release the factory
   */
	hres = IClassFactory_CreateInstance(lpclf, pUnkOuter, iid, ppv);
	IClassFactory_Release(lpclf);

	return hres;
}



/***********************************************************************
 *           CoFreeLibrary [COMPOBJ.13]
 */
void WINAPI CoFreeLibrary(HINSTANCE hLibrary)
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
    FreeLibrary(hLibrary);
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
	    GetProcAddress(ptr->hLibrary, "DllCanUnloadNow");
	
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
    LPMALLOC	lpmalloc;
    HRESULT	ret = CoGetMalloc(0,&lpmalloc);

    if (FAILED(ret)) 
	return NULL;

    return IMalloc_Alloc(lpmalloc,size);
}

/***********************************************************************
 *           CoTaskMemFree (OLE32.44)
 */
VOID WINAPI CoTaskMemFree(
	LPVOID ptr	/* [in] pointer to be freed */
) {
    LPMALLOC	lpmalloc;
    HRESULT	ret = CoGetMalloc(0,&lpmalloc);

    if (FAILED(ret)) 
      return;

    IMalloc_Free(lpmalloc, ptr);
}

/***********************************************************************
 *           CoTaskMemRealloc (OLE32.45)
 * RETURNS
 * 	pointer to newly allocated block
 */
LPVOID WINAPI CoTaskMemRealloc(
  LPVOID pvOld,
  ULONG  size)	/* [in] size of memoryblock to be allocated */
{
  LPMALLOC lpmalloc;
  HRESULT  ret = CoGetMalloc(0,&lpmalloc);
  
  if (FAILED(ret)) 
    return NULL;

  return IMalloc_Realloc(lpmalloc, pvOld, size);
}

/***********************************************************************
 *           CoLoadLibrary (OLE32.30)
 */
HINSTANCE WINAPI CoLoadLibrary(LPOLESTR16 lpszLibName, BOOL bAutoFree)
{
    HINSTANCE hLibrary;
    OpenDll *ptr;
    OpenDll *tmp;
  
    TRACE(ole,"CoLoadLibrary(%p, %d\n", lpszLibName, bAutoFree);

    hLibrary = LoadLibraryA(lpszLibName);

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
HRESULT WINAPI CoLockObjectExternal(
    LPUNKNOWN pUnk,		/* [in] object to be locked */
    BOOL fLock,		/* [in] do lock */
    BOOL fLastUnlockReleases) /* [in] unlock all */
{

  if (fLock) 
  {
    /* 
     * Increment the external lock coutner, COM_ExternalLockAddRef also
     * increment the object's internal lock counter.
     */
    COM_ExternalLockAddRef( pUnk); 
  }
  else
  {
    /* 
     * Decrement the external lock coutner, COM_ExternalLockRelease also
     * decrement the object's internal lock counter.
     */
    COM_ExternalLockRelease( pUnk, fLastUnlockReleases);
  }

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
HRESULT WINAPI CoSetState(LPDWORD state)
{
    FIXME(ole, "(%p),stub!\n", state);
    *state = 0;
    return S_OK;
}

/***
 * COM_RevokeAllClasses
 *
 * This method is called when the COM libraries are uninitialized to 
 * release all the references to the class objects registered with
 * the library
 */
static void COM_RevokeAllClasses()
{
  while (firstRegisteredClass!=0)
  {
    CoRevokeClassObject(firstRegisteredClass->dwCookie);
  }
}

/****************************************************************************
 *  COM External Lock methods implementation
 */

/****************************************************************************
 * Public - Method that increments the count for a IUnknown* in the linked 
 * list.  The item is inserted if not already in the list.
 */
static void COM_ExternalLockAddRef(
  IUnknown *pUnk)
{
  COM_ExternalLock *externalLock = COM_ExternalLockFind(pUnk);

  /*
   * Add an external lock to the object. If it was already externally
   * locked, just increase the reference count. If it was not.
   * add the item to the list.
   */
  if ( externalLock == EL_NOT_FOUND )
    COM_ExternalLockInsert(pUnk);
  else
    externalLock->uRefCount++;

  /*
   * Add an internal lock to the object
   */
  IUnknown_AddRef(pUnk); 
}

/****************************************************************************
 * Public - Method that decrements the count for a IUnknown* in the linked 
 * list.  The item is removed from the list if its count end up at zero or if
 * bRelAll is TRUE.
 */
static void COM_ExternalLockRelease(
  IUnknown *pUnk,
  BOOL   bRelAll)
{
  COM_ExternalLock *externalLock = COM_ExternalLockFind(pUnk);

  if ( externalLock != EL_NOT_FOUND )
  {
    do
    {
      externalLock->uRefCount--;  /* release external locks      */
      IUnknown_Release(pUnk);     /* release local locks as well */

      if ( bRelAll == FALSE ) 
        break;  /* perform single release */

    } while ( externalLock->uRefCount > 0 );  

    if ( externalLock->uRefCount == 0 )  /* get rid of the list entry */
      COM_ExternalLockDelete(externalLock);
  }
}
/****************************************************************************
 * Public - Method that frees the content of the list.
 */
static void COM_ExternalLockFreeList()
{
  COM_ExternalLock *head;

  head = elList.head;                 /* grab it by the head             */
  while ( head != EL_END_OF_LIST )
  {
    COM_ExternalLockDelete(head);     /* get rid of the head stuff       */

    head = elList.head;               /* get the new head...             */ 
  }
}

/****************************************************************************
 * Public - Method that dump the content of the list.
 */
void COM_ExternalLockDump()
{
  COM_ExternalLock *current = elList.head;

  printf("\nExternal lock list contains:\n");

  while ( current != EL_END_OF_LIST )
  {
    printf( "\t%p with %lu references count.\n", 
      current->pUnk, 
      current->uRefCount);
 
    /* Skip to the next item */ 
    current = current->next;
  } 

}

/****************************************************************************
 * Internal - Find a IUnknown* in the linked list
 */
static COM_ExternalLock* COM_ExternalLockFind(
  IUnknown *pUnk)
{
  return COM_ExternalLockLocate(elList.head, pUnk);
}

/****************************************************************************
 * Internal - Recursivity agent for IUnknownExternalLockList_Find
 */
static COM_ExternalLock* COM_ExternalLockLocate(
  COM_ExternalLock *element,
  IUnknown         *pUnk)
{
  if ( element == EL_END_OF_LIST )  
    return EL_NOT_FOUND;

  else if ( element->pUnk == pUnk )    /* We found it */
    return element;

  else                                 /* Not the right guy, keep on looking */ 
    return COM_ExternalLockLocate( element->next, pUnk);
}

/****************************************************************************
 * Internal - Insert a new IUnknown* to the linked list
 */
static BOOL COM_ExternalLockInsert(
  IUnknown *pUnk)
{
  COM_ExternalLock *newLock      = NULL;
  COM_ExternalLock *previousHead = NULL;

  /*
   * Allocate space for the new storage object
   */
  newLock = HeapAlloc(GetProcessHeap(), 0, sizeof(COM_ExternalLock));

  if (newLock!=NULL)
  {
    if ( elList.head == EL_END_OF_LIST ) 
    {
      elList.head = newLock;    /* The list is empty */
    }
    else 
    {
      /* 
       * insert does it at the head
       */
      previousHead  = elList.head;
      elList.head = newLock;
    }

    /*
     * Set new list item data member 
     */
    newLock->pUnk      = pUnk;
    newLock->uRefCount = 1;
    newLock->next      = previousHead;
    
    return TRUE;
  }
  else
    return FALSE;
}

/****************************************************************************
 * Internal - Method that removes an item from the linked list.
 */
static void COM_ExternalLockDelete(
  COM_ExternalLock *itemList)
{
  COM_ExternalLock *current = elList.head;

  if ( current == itemList )
  {
    /* 
     * this section handles the deletion of the first node 
     */
    elList.head = itemList->next;
    HeapFree( GetProcessHeap(), 0, itemList);  
  }
  else
  {
    do 
    {
      if ( current->next == itemList )   /* We found the item to free  */
      {
        current->next = itemList->next;  /* readjust the list pointers */
  
        HeapFree( GetProcessHeap(), 0, itemList);  
        break; 
      }
 
      /* Skip to the next item */ 
      current = current->next;
  
    } while ( current != EL_END_OF_LIST );
  }
}
