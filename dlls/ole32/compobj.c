/*
 *	COMPOBJ library
 *
 *	Copyright 1995	Martin von Loewis
 *	Copyright 1998	Justin Bradford
 *      Copyright 1999  Francis Beaudet
 *  Copyright 1999  Sylvain St-Germain
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "objbase.h"
#include "ole2.h"
#include "ole2ver.h"
#include "rpc.h"
#include "wingdi.h"
#include "winerror.h"
#include "winreg.h"
#include "wownt32.h"
#include "wtypes.h"

#include "wine/obj_base.h"
#include "wine/obj_clientserver.h"
#include "wine/obj_misc.h"
#include "wine/obj_marshal.h"
#include "wine/obj_storage.h"
#include "wine/winbase16.h"
#include "compobj_private.h"
#include "ifs.h"
#include "heap.h"

#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ole);

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
HINSTANCE16     COMPOBJ_hInstance = 0;
HINSTANCE       COMPOBJ_hInstance32 = 0;
static int      COMPOBJ_Attach = 0;

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
 *           CoBuildVersion [OLE32.4]
 *
 * RETURNS
 *	Current build version, hiword is majornumber, loword is minornumber
 */
DWORD WINAPI CoBuildVersion(void)
{
    TRACE("Returning version %d, build %d.\n", rmm, rup);
    return (rmm<<16)+rup;
}

/******************************************************************************
 *		CoInitialize	[COMPOBJ.2]
 * Set the win16 IMalloc used for memory management
 */
HRESULT WINAPI CoInitialize16(
	LPVOID lpReserved	/* [in] pointer to win16 malloc interface */
) {
    currentMalloc16 = (LPMALLOC16)lpReserved;
    return S_OK;
}

/******************************************************************************
 *		CoInitialize	[OLE32.26]
 *
 * Initializes the COM libraries.
 *
 * See CoInitializeEx
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
 *		CoInitializeEx	[OLE32.163]
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

  TRACE("(%p, %x)\n", lpReserved, (int)dwCoInit);

  if (lpReserved!=NULL)
  {
    ERR("(%p, %x) - Bad parameter passed-in %p, must be an old Windows Application\n", lpReserved, (int)dwCoInit, lpReserved);
  }

  /*
   * Check for unsupported features.
   */
  if (dwCoInit!=COINIT_APARTMENTTHREADED) 
  {
    FIXME(":(%p,%x): unsupported flag %x\n", lpReserved, (int)dwCoInit, (int)dwCoInit);
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
    TRACE("() - Initializing the COM libraries\n");

    RunningObjectTableImpl_Initialize();

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
 *           CoUninitialize   [COMPOBJ.3]
 * Don't know what it does. 
 * 3-Nov-98 -- this was originally misspelled, I changed it to what I
 *   believe is the correct spelling
 */
void WINAPI CoUninitialize16(void)
{
  TRACE("()\n");
  CoFreeAllLibraries();
}

/***********************************************************************
 *           CoUninitialize   [OLE32.47]
 *
 * This method will release the COM libraries.
 *
 * See the windows documentation for more details.
 */
void WINAPI CoUninitialize(void)
{
  TRACE("()\n");
  
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
    TRACE("() - Releasing the COM libraries\n");

    RunningObjectTableImpl_UnInitialize();
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
 *           CoGetMalloc    [COMPOBJ.4]
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
 *		CoGetMalloc	[OLE32.20]
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
 *           CoCreateStandardMalloc [COMPOBJ.71]
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
 *		CoDisconnectObject	[OLE32.8]
 */
HRESULT WINAPI CoDisconnectObject( LPUNKNOWN lpUnk, DWORD reserved )
{
    TRACE("(%p, %lx)\n",lpUnk,reserved);
    return S_OK;
}

/***********************************************************************
 *           IsEqualGUID [COMPOBJ.18]
 *
 * Compares two Unique Identifiers.
 *
 * RETURNS
 *	TRUE if equal
 */
BOOL16 WINAPI IsEqualGUID16(
	GUID* g1,	/* [in] unique id 1 */
	GUID* g2	/* [in] unique id 2 */
) {
    return !memcmp( g1, g2, sizeof(GUID) );
}

/******************************************************************************
 *		CLSIDFromString	[COMPOBJ.20]
 * Converts a unique identifier from its string representation into 
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

  if (!s)
	  s = "{00000000-0000-0000-0000-000000000000}";
  else {  /* validate the CLSID string */

      if (strlen(s) != 38)
          return CO_E_CLASSSTRING;

      if ((s[0]!='{') || (s[9]!='-') || (s[14]!='-') || (s[19]!='-') || (s[24]!='-') || (s[37]!='}'))
          return CO_E_CLASSSTRING;

      for (i=1; i<37; i++)
      {
          if ((i == 9)||(i == 14)||(i == 19)||(i == 24)) continue;
          if (!(((s[i] >= '0') && (s[i] <= '9'))  ||
                ((s[i] >= 'a') && (s[i] <= 'f'))  ||
                ((s[i] >= 'A') && (s[i] <= 'F')))
             )
              return CO_E_CLASSSTRING;
      }
  }

  TRACE("%s -> %p\n", s, id);

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
 */
HRESULT WINAPI CoCreateGuid(
	GUID *pguid /* [out] points to the GUID to initialize */
) {
    return UuidCreate(pguid);
}

/******************************************************************************
 *		CLSIDFromString	[OLE32.3]
 *		IIDFromString   [OLE32.74]
 * Converts a unique identifier from its string representation into 
 * the GUID struct.
 *
 * UNDOCUMENTED
 *      If idstr is not a valid CLSID string then it gets treated as a ProgID
 *
 * RETURNS
 *	the converted GUID
 */
HRESULT WINAPI CLSIDFromString(
	LPCOLESTR idstr,	/* [in] string representation of GUID */
	CLSID *id		/* [out] GUID represented by above string */
) {
    LPOLESTR16      xid = HEAP_strdupWtoA(GetProcessHeap(),0,idstr);
    HRESULT       ret = CLSIDFromString16(xid,id);

    HeapFree(GetProcessHeap(),0,xid);
    if(ret != S_OK) { /* It appears a ProgID is also valid */
        ret = CLSIDFromProgID(idstr, id);
    }
    return ret;
}

/******************************************************************************
 *		WINE_StringFromCLSID	[Internal]
 * Converts a GUID into the respective string representation.
 *
 * NOTES
 *
 * RETURNS
 *	the string representation and HRESULT
 */
static HRESULT WINE_StringFromCLSID(
	const CLSID *id,	/* [in] GUID to be converted */
	LPSTR idstr		/* [out] pointer to buffer to contain converted guid */
) {
  static const char *hex = "0123456789ABCDEF";
  char *s;
  int	i;

  if (!id)
	{ ERR("called with id=Null\n");
	  *idstr = 0x00;
	  return E_FAIL;
	}
	
  sprintf(idstr, "{%08lX-%04X-%04X-%02X%02X-",
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

  TRACE("%p->%s\n", id, idstr);

  return S_OK;
}

/******************************************************************************
 *		StringFromCLSID	[COMPOBJ.19]
 * Converts a GUID into the respective string representation.
 * The target string is allocated using the OLE IMalloc.
 * RETURNS
 *	the string representation and HRESULT
 */
HRESULT WINAPI StringFromCLSID16(
        REFCLSID id,            /* [in] the GUID to be converted */
	LPOLESTR16 *idstr	/* [out] a pointer to a to-be-allocated segmented pointer pointing to the resulting string */

) {
    extern BOOL WINAPI K32WOWCallback16Ex( DWORD vpfn16, DWORD dwFlags,
                                           DWORD cbArgs, LPVOID pArgs, LPDWORD pdwRetCode );
    LPMALLOC16	mllc;
    HRESULT	ret;
    DWORD	args[2];

    ret = CoGetMalloc16(0,&mllc);
    if (ret) return ret;

    args[0] = (DWORD)mllc;
    args[1] = 40;

    /* No need for a Callback entry, we have WOWCallback16Ex which does
     * everything we need.
     */
    if (!K32WOWCallback16Ex(
    	(DWORD)((ICOM_VTABLE(IMalloc16)*)MapSL(
            (SEGPTR)ICOM_VTBL(((LPMALLOC16)MapSL((SEGPTR)mllc))))
	)->Alloc,
	WCB16_CDECL,
	2*sizeof(DWORD),
	(LPVOID)args,
	(LPDWORD)idstr
    )) {
    	WARN("CallTo16 IMalloc16 failed\n");
    	return E_FAIL;
    }
    return WINE_StringFromCLSID(id,MapSL((SEGPTR)*idstr));
}

/******************************************************************************
 *		StringFromCLSID	[OLE32.151]
 *		StringFromIID   [OLE32.153]
 * Converts a GUID into the respective string representation.
 * The target string is allocated using the OLE IMalloc.
 * RETURNS
 *	the string representation and HRESULT
 */
HRESULT WINAPI StringFromCLSID(
        REFCLSID id,            /* [in] the GUID to be converted */
	LPOLESTR *idstr	/* [out] a pointer to a to-be-allocated pointer pointing to the resulting string */
) {
	char            buf[80];
	HRESULT       ret;
	LPMALLOC	mllc;

	if ((ret=CoGetMalloc(0,&mllc)))
		return ret;

	ret=WINE_StringFromCLSID(id,buf);
	if (!ret) {
            DWORD len = MultiByteToWideChar( CP_ACP, 0, buf, -1, NULL, 0 );
            *idstr = IMalloc_Alloc( mllc, len * sizeof(WCHAR) );
            MultiByteToWideChar( CP_ACP, 0, buf, -1, *idstr, len );
	}
	return ret;
}

/******************************************************************************
 *		StringFromGUID2	[COMPOBJ.76]
 *		StringFromGUID2	[OLE32.152]
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
  return MultiByteToWideChar( CP_ACP, 0, xguid, -1, str, cmax );
}

/******************************************************************************
 * ProgIDFromCLSID [OLE32.133]
 * Converts a class id into the respective Program ID. (By using a registry lookup)
 * RETURNS S_OK on success
 * riid associated with the progid
 */

HRESULT WINAPI ProgIDFromCLSID(
  REFCLSID clsid, /* [in] class id as found in registry */
  LPOLESTR *lplpszProgID/* [out] associated Prog ID */
)
{
  char     strCLSID[50], *buf, *buf2;
  DWORD    buf2len;
  HKEY     xhkey;
  LPMALLOC mllc;
  HRESULT  ret = S_OK;

  WINE_StringFromCLSID(clsid, strCLSID);

  buf = HeapAlloc(GetProcessHeap(), 0, strlen(strCLSID)+14);
  sprintf(buf,"CLSID\\%s\\ProgID", strCLSID);
  if (RegOpenKeyA(HKEY_CLASSES_ROOT, buf, &xhkey))
    ret = REGDB_E_CLASSNOTREG;

  HeapFree(GetProcessHeap(), 0, buf);

  if (ret == S_OK)
  {
    buf2 = HeapAlloc(GetProcessHeap(), 0, 255);
    buf2len = 255;
    if (RegQueryValueA(xhkey, NULL, buf2, &buf2len))
      ret = REGDB_E_CLASSNOTREG;

    if (ret == S_OK)
    {
      if (CoGetMalloc(0,&mllc))
        ret = E_OUTOFMEMORY;
      else
      {
          DWORD len = MultiByteToWideChar( CP_ACP, 0, buf2, -1, NULL, 0 );
          *lplpszProgID = IMalloc_Alloc(mllc, len * sizeof(WCHAR) );
          MultiByteToWideChar( CP_ACP, 0, buf2, -1, *lplpszProgID, len );
      }
    }
    HeapFree(GetProcessHeap(), 0, buf2);
  }

  RegCloseKey(xhkey);
  return ret;
}

/******************************************************************************
 *		CLSIDFromProgID	[COMPOBJ.61]
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
                return CO_E_CLASSSTRING;
	}
	HeapFree(GetProcessHeap(),0,buf);
	buf2len = sizeof(buf2);
	if ((err=RegQueryValueA(xhkey,NULL,buf2,&buf2len))) {
		RegCloseKey(xhkey);
                return CO_E_CLASSSTRING;
	}
	RegCloseKey(xhkey);
	return CLSIDFromString16(buf2,riid);
}

/******************************************************************************
 *		CLSIDFromProgID	[OLE32.2]
 * Converts a program id into the respective GUID. (By using a registry lookup)
 * RETURNS
 *	riid associated with the progid
 */
HRESULT WINAPI CLSIDFromProgID(
	LPCOLESTR progid,	/* [in] program id as found in registry */
	LPCLSID riid		/* [out] associated CLSID */
) {
	LPOLESTR16 pid = HEAP_strdupWtoA(GetProcessHeap(),0,progid);
	HRESULT       ret = CLSIDFromProgID16(pid,riid);

	HeapFree(GetProcessHeap(),0,pid);
	return ret;
}



/*****************************************************************************
 *             CoGetPSClsid [OLE32.22]
 *
 * This function returns the CLSID of the DLL that implements the proxy and stub
 * for the specified interface. 
 *
 * It determines this by searching the 
 * HKEY_CLASSES_ROOT\Interface\{string form of riid}\ProxyStubClsid32 in the registry
 * and any interface id registered by CoRegisterPSClsid within the current process.
 * 
 * FIXME: We only search the registry, not ids registered with CoRegisterPSClsid.
 */
HRESULT WINAPI CoGetPSClsid(
          REFIID riid,     /* [in]  Interface whose proxy/stub CLSID is to be returned */
          CLSID *pclsid )    /* [out] Where to store returned proxy/stub CLSID */
{
    char *buf, buf2[40];
    DWORD buf2len;
    HKEY xhkey;

    TRACE("() riid=%s, pclsid=%p\n", debugstr_guid(riid), pclsid);

    /* Get the input iid as a string */
    WINE_StringFromCLSID(riid, buf2);
    /* Allocate memory for the registry key we will construct.
       (length of iid string plus constant length of static text */
    buf = HeapAlloc(GetProcessHeap(), 0, strlen(buf2)+27);
    if (buf == NULL)
    {
       return (E_OUTOFMEMORY);
    }

    /* Construct the registry key we want */
    sprintf(buf,"Interface\\%s\\ProxyStubClsid32", buf2);

    /* Open the key.. */
    if (RegOpenKeyA(HKEY_CLASSES_ROOT, buf, &xhkey))
    {
       HeapFree(GetProcessHeap(),0,buf);
       return (E_INVALIDARG);
    }
    HeapFree(GetProcessHeap(),0,buf);

    /* ... Once we have the key, query the registry to get the
       value of CLSID as a string, and convert it into a 
       proper CLSID structure to be passed back to the app */
    buf2len = sizeof(buf2);
    if ( (RegQueryValueA(xhkey,NULL,buf2,&buf2len)) )
    {
       RegCloseKey(xhkey);
       return E_INVALIDARG;
    }
    RegCloseKey(xhkey);

    /* We have the CLSid we want back from the registry as a string, so
       lets convert it into a CLSID structure */
    if ( (CLSIDFromString16(buf2,pclsid)) != NOERROR)
    {
       return E_INVALIDARG;
    }

    TRACE ("() Returning CLSID=%s\n", debugstr_guid(pclsid));
    return (S_OK);
}



/***********************************************************************
 *		WriteClassStm (OLE32.159)
 *
 * This function write a CLSID on stream
 */
HRESULT WINAPI WriteClassStm(IStream *pStm,REFCLSID rclsid)
{
    TRACE("(%p,%p)\n",pStm,rclsid);

    if (rclsid==NULL)
        return E_INVALIDARG;

    return IStream_Write(pStm,rclsid,sizeof(CLSID),NULL);
}

/***********************************************************************
 *		ReadClassStm (OLE32.135)
 *
 * This function read a CLSID from a stream
 */
HRESULT WINAPI ReadClassStm(IStream *pStm,CLSID *pclsid)
{
    ULONG nbByte;
    HRESULT res;
    
    TRACE("(%p,%p)\n",pStm,pclsid);

    if (pclsid==NULL)
        return E_INVALIDARG;
    
    res = IStream_Read(pStm,(void*)pclsid,sizeof(CLSID),&nbByte);

    if (FAILED(res))
        return res;
    
    if (nbByte != sizeof(CLSID))
        return S_FALSE;
    else
        return S_OK;
}

/* FIXME: this function is not declared in the WINELIB headers. But where should it go ? */
/***********************************************************************
 *           LookupETask (COMPOBJ.94)
 */
HRESULT WINAPI LookupETask16(HTASK16 *hTask,LPVOID p) {
	FIXME("(%p,%p),stub!\n",hTask,p);
	if ((*hTask = GetCurrentTask()) == hETask) {
		memcpy(p, Table_ETask, sizeof(Table_ETask));
	}
	return 0;
}

/* FIXME: this function is not declared in the WINELIB headers. But where should it go ? */
/***********************************************************************
 *           SetETask (COMPOBJ.95)
 */
HRESULT WINAPI SetETask16(HTASK16 hTask, LPVOID p) {
        FIXME("(%04x,%p),stub!\n",hTask,p);
	hETask = hTask;
	return 0;
}

/* FIXME: this function is not declared in the WINELIB headers. But where should it go ? */
/***********************************************************************
 *           CALLOBJECTINWOW (COMPOBJ.201)
 */
HRESULT WINAPI CallObjectInWOW(LPVOID p1,LPVOID p2) {
	FIXME("(%p,%p),stub!\n",p1,p2);
	return 0;
}

/******************************************************************************
 *		CoRegisterClassObject	[COMPOBJ.5]
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

	FIXME("(%s,%p,0x%08lx,0x%08lx,%p),stub\n",
		buf,pUnk,dwClsContext,flags,lpdwRegister
	);
	return 0;
}


/******************************************************************************
 *      CoRevokeClassObject [COMPOBJ.6]
 *
 */
HRESULT WINAPI CoRevokeClassObject16(DWORD dwRegister) /* [in] token on class obj */
{
    FIXME("(0x%08lx),stub!\n", dwRegister);
    return 0;
}

/******************************************************************************
 *      CoFileTimeToDosDateTime [COMPOBJ.30]
 */
BOOL16 WINAPI CoFileTimeToDosDateTime16(const FILETIME *ft, LPWORD lpDosDate, LPWORD lpDosTime)
{
    return FileTimeToDosDateTime(ft, lpDosDate, lpDosTime);
}

/******************************************************************************
 *      CoDosDateTimeToFileTime [COMPOBJ.31]
 */
BOOL16 WINAPI CoDosDateTimeToFileTime16(WORD wDosDate, WORD wDosTime, FILETIME *ft)
{
    return DosDateTimeToFileTime(wDosDate, wDosTime, ft);
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
    if (IsEqualGUID(&(curClass->classIdentifier), rclsid))
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
 *		CoRegisterClassObject	[OLE32.36]
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

  TRACE("(%s,%p,0x%08lx,0x%08lx,%p)\n",
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
   * Since we're making a copy of the object pointer, we have to increase its
   * reference count.
   */
  newClass->classObject     = pUnk;
  IUnknown_AddRef(newClass->classObject);

  firstRegisteredClass = newClass;

  /*
   * Assign the out parameter (cookie)
   */
  *lpdwRegister = newClass->dwCookie;
    
  /*
   * We're successful Yippee!
   */
  return S_OK;
}

/***********************************************************************
 *           CoRevokeClassObject [OLE32.40]
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

  TRACE("(%08lx)\n",dwRegister);

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
 *           CoGetClassObject [OLE32.16]
 */
HRESULT WINAPI CoGetClassObject(
    REFCLSID rclsid, DWORD dwClsContext, COSERVERINFO *pServerInfo,
    REFIID iid, LPVOID *ppv
) {
    LPUNKNOWN	regClassObject;
    HRESULT	hres = E_UNEXPECTED;
    char	xclsid[80];
    WCHAR dllName[MAX_PATH+1];
    DWORD dllNameLen = sizeof(dllName);
    HINSTANCE hLibrary;
    typedef HRESULT CALLBACK (*DllGetClassObjectFunc)(REFCLSID clsid, 
			     REFIID iid, LPVOID *ppv);
    DllGetClassObjectFunc DllGetClassObject;

    WINE_StringFromCLSID((LPCLSID)rclsid,xclsid);

    TRACE("\n\tCLSID:\t%s,\n\tIID:\t%s\n",
	debugstr_guid(rclsid),
	debugstr_guid(iid)
    );

    if (pServerInfo) {
	FIXME("\tpServerInfo: name=%s\n",debugstr_w(pServerInfo->pwszName));
	FIXME("\t\tpAuthInfo=%p\n",pServerInfo->pAuthInfo);
    }

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
    if (     ((CLSCTX_LOCAL_SERVER|CLSCTX_REMOTE_SERVER) & dwClsContext)
        && !((CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER) & dwClsContext)
    ){
        FIXME("%s %s not supported!\n",
		(dwClsContext&CLSCTX_LOCAL_SERVER)?"CLSCTX_LOCAL_SERVER":"",
		(dwClsContext&CLSCTX_REMOTE_SERVER)?"CLSCTX_REMOTE_SERVER":""
	);
	return E_ACCESSDENIED;
    }

    if ((CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER) & dwClsContext) {
        HKEY key;
	char buf[200];

	sprintf(buf,"CLSID\\%s\\InprocServer32",xclsid);
        hres = RegOpenKeyExA(HKEY_CLASSES_ROOT, buf, 0, KEY_READ, &key);

	if (hres != ERROR_SUCCESS) {
	    return REGDB_E_CLASSNOTREG;
	}

	memset(dllName,0,sizeof(dllName));
	hres= RegQueryValueExW(key,NULL,NULL,NULL,(LPBYTE)dllName,&dllNameLen);
	if (hres)
		return REGDB_E_CLASSNOTREG; /* FIXME: check retval */
	RegCloseKey(key);
	TRACE("found InprocServer32 dll %s\n", debugstr_w(dllName));

	/* open dll, call DllGetClassObject */
	hLibrary = CoLoadLibrary(dllName, TRUE);
	if (hLibrary == 0) {
	    FIXME("couldn't load InprocServer32 dll %s\n", debugstr_w(dllName));
	    return E_ACCESSDENIED; /* or should this be CO_E_DLLNOTFOUND? */
	}
	DllGetClassObject = (DllGetClassObjectFunc)GetProcAddress(hLibrary, "DllGetClassObject");
	if (!DllGetClassObject) {
	    /* not sure if this should be called here CoFreeLibrary(hLibrary);*/
	    FIXME("couldn't find function DllGetClassObject in %s\n", debugstr_w(dllName));
	    return E_ACCESSDENIED;
	}

	/*
	 * Ask the DLL for its class object. (there was a note here about class
	 * factories but this is good.
	 */
	return DllGetClassObject(rclsid, iid, ppv);
    }
    return hres;
}

/***********************************************************************
 *        CoResumeClassObjects (OLE32.173)
 *
 * Resumes classobjects registered with REGCLS suspended
 */
HRESULT WINAPI CoResumeClassObjects(void)
{
	FIXME("\n");
	return S_OK;
}

/***********************************************************************
 *        GetClassFile (OLE32.67)
 *
 * This function supplies the CLSID associated with the given filename.
 */
HRESULT WINAPI GetClassFile(LPCOLESTR filePathName,CLSID *pclsid)
{
    IStorage *pstg=0;
    HRESULT res;
    int nbElm=0,length=0,i=0;
    LONG sizeProgId=20;
    LPOLESTR *pathDec=0,absFile=0,progId=0;
    WCHAR extention[100]={0};

    TRACE("()\n");

    /* if the file contain a storage object the return the CLSID writen by IStorage_SetClass method*/
    if((StgIsStorageFile(filePathName))==S_OK){

        res=StgOpenStorage(filePathName,NULL,STGM_READ | STGM_SHARE_DENY_WRITE,NULL,0,&pstg);

        if (SUCCEEDED(res))
            res=ReadClassStg(pstg,pclsid);

        IStorage_Release(pstg);

        return res;
    }
    /* if the file is not a storage object then attemps to match various bits in the file against a
       pattern in the registry. this case is not frequently used ! so I present only the psodocode for
       this case
       
     for(i=0;i<nFileTypes;i++)

        for(i=0;j<nPatternsForType;j++){

            PATTERN pat;
            HANDLE  hFile;

            pat=ReadPatternFromRegistry(i,j);
            hFile=CreateFileW(filePathName,,,,,,hFile);
            SetFilePosition(hFile,pat.offset);
            ReadFile(hFile,buf,pat.size,NULL,NULL);
            if (memcmp(buf&pat.mask,pat.pattern.pat.size)==0){

                *pclsid=ReadCLSIDFromRegistry(i);
                return S_OK;
            }
        }
     */

    /* if the obove strategies fail then search for the extension key in the registry */

    /* get the last element (absolute file) in the path name */
    nbElm=FileMonikerImpl_DecomposePath(filePathName,&pathDec);
    absFile=pathDec[nbElm-1];

    /* failed if the path represente a directory and not an absolute file name*/
    if (lstrcmpW(absFile,(LPOLESTR)"\\"))
        return MK_E_INVALIDEXTENSION;

    /* get the extension of the file */
    length=lstrlenW(absFile);
    for(i=length-1; ( (i>=0) && (extention[i]=absFile[i]) );i--);
        
    /* get the progId associated to the extension */
    progId=CoTaskMemAlloc(sizeProgId);

    res=RegQueryValueW(HKEY_CLASSES_ROOT,extention,progId,&sizeProgId);

    if (res==ERROR_MORE_DATA){

        progId = CoTaskMemRealloc(progId,sizeProgId);
        res=RegQueryValueW(HKEY_CLASSES_ROOT,extention,progId,&sizeProgId);
    }
    if (res==ERROR_SUCCESS)
        /* return the clsid associated to the progId */
        res= CLSIDFromProgID(progId,pclsid);

    for(i=0; pathDec[i]!=NULL;i++)
        CoTaskMemFree(pathDec[i]);
    CoTaskMemFree(pathDec);

    CoTaskMemFree(progId);

    if (res==ERROR_SUCCESS)
        return res;

    return MK_E_INVALIDEXTENSION;
}
/******************************************************************************
 *		CoRegisterMessageFilter	[COMPOBJ.27]
 */
HRESULT WINAPI CoRegisterMessageFilter16(
	LPMESSAGEFILTER lpMessageFilter,
	LPMESSAGEFILTER *lplpMessageFilter
) {
	FIXME("(%p,%p),stub!\n",lpMessageFilter,lplpMessageFilter);
	return 0;
}

/***********************************************************************
 *           CoCreateInstance [COMPOBJ.13]
 *           CoCreateInstance [OLE32.7]
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

  if (FAILED(hres)) {
    FIXME("no instance created for %s, hres is 0x%08lx\n",debugstr_guid(iid),hres);
    return hres;
  }

  /*
   * Create the object and don't forget to release the factory
   */
	hres = IClassFactory_CreateInstance(lpclf, pUnkOuter, iid, ppv);
	IClassFactory_Release(lpclf);

	return hres;
}

/***********************************************************************
 *           CoCreateInstanceEx [OLE32.165]
 */
HRESULT WINAPI CoCreateInstanceEx(
  REFCLSID      rclsid, 
  LPUNKNOWN     pUnkOuter,
  DWORD         dwClsContext, 
  COSERVERINFO* pServerInfo,
  ULONG         cmq,
  MULTI_QI*     pResults)
{
  IUnknown* pUnk = NULL;
  HRESULT   hr;
  ULONG     index;
  int       successCount = 0;

  /*
   * Sanity check
   */
  if ( (cmq==0) || (pResults==NULL))
    return E_INVALIDARG;

  if (pServerInfo!=NULL)
    FIXME("() non-NULL pServerInfo not supported!\n");

  /*
   * Initialize all the "out" parameters.
   */
  for (index = 0; index < cmq; index++)
  {
    pResults[index].pItf = NULL;
    pResults[index].hr   = E_NOINTERFACE;
  }

  /*
   * Get the object and get its IUnknown pointer.
   */
  hr = CoCreateInstance(rclsid, 
			pUnkOuter,
			dwClsContext,
			&IID_IUnknown,
			(VOID**)&pUnk);

  if (hr)
    return hr;

  /*
   * Then, query for all the interfaces requested.
   */
  for (index = 0; index < cmq; index++)
  {
    pResults[index].hr = IUnknown_QueryInterface(pUnk,
						 pResults[index].pIID,
						 (VOID**)&(pResults[index].pItf));

    if (pResults[index].hr == S_OK)
      successCount++;
  }

  /*
   * Release our temporary unknown pointer.
   */
  IUnknown_Release(pUnk);

  if (successCount == 0)
    return E_NOINTERFACE;

  if (successCount!=cmq)
    return CO_S_NOTALLINTERFACES;

  return S_OK;
}

/***********************************************************************
 *           CoFreeLibrary [OLE32.13]
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
	HeapFree(GetProcessHeap(), 0, openDllList);
	openDllList = tmp;
    } else {
	tmp = ptr->next;
	HeapFree(GetProcessHeap(), 0, ptr);
	prev->next = tmp;
    }

}


/***********************************************************************
 *           CoFreeAllLibraries [OLE32.12]
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
 *           CoFreeUnusedLibraries [OLE32.14]
 */
void WINAPI CoFreeUnusedLibraries(void)
{
    OpenDll *ptr, *tmp;
    typedef HRESULT(*DllCanUnloadNowFunc)(void);
    DllCanUnloadNowFunc DllCanUnloadNow;

    for (ptr = openDllList; ptr != NULL; ) {
	DllCanUnloadNow = (DllCanUnloadNowFunc)
	    GetProcAddress(ptr->hLibrary, "DllCanUnloadNow");
	
	if ( (DllCanUnloadNow != NULL) &&
	     (DllCanUnloadNow() == S_OK) ) {
	    tmp=ptr->next;
	    CoFreeLibrary(ptr->hLibrary);
	    ptr = tmp;
	} else {
	    ptr=ptr->next;
	}
    }
}

/***********************************************************************
 *           CoFileTimeNow [COMPOBJ.82]
 *           CoFileTimeNow [OLE32.10]
 *
 * RETURNS
 *	the current system time in lpFileTime
 */
HRESULT WINAPI CoFileTimeNow( FILETIME *lpFileTime ) /* [out] the current time */
{
    GetSystemTimeAsFileTime( lpFileTime );
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
HINSTANCE WINAPI CoLoadLibrary(LPOLESTR lpszLibName, BOOL bAutoFree)
{
    HINSTANCE hLibrary;
    OpenDll *ptr;
    OpenDll *tmp;
  
    TRACE("(%s, %d)\n", debugstr_w(lpszLibName), bAutoFree);

    hLibrary = LoadLibraryExW(lpszLibName, 0, LOAD_WITH_ALTERED_SEARCH_PATH);

    if (!bAutoFree)
	return hLibrary;

    if (openDllList == NULL) {
        /* empty list -- add first node */
        openDllList = (OpenDll*)HeapAlloc(GetProcessHeap(),0, sizeof(OpenDll));
	openDllList->hLibrary=hLibrary;
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
    FIXME("(0x%08lx,0x%08lx),stub!\n",x,y);
    return 0;
}

/******************************************************************************
 *		CoLockObjectExternal	[COMPOBJ.63]
 */
HRESULT WINAPI CoLockObjectExternal16(
    LPUNKNOWN pUnk,		/* [in] object to be locked */
    BOOL16 fLock,		/* [in] do lock */
    BOOL16 fLastUnlockReleases	/* [in] ? */
) {
    FIXME("(%p,%d,%d),stub!\n",pUnk,fLock,fLastUnlockReleases);
    return S_OK;
}

/******************************************************************************
 *		CoLockObjectExternal	[OLE32.31]
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
 *           CoGetState [COMPOBJ.115]
 */
HRESULT WINAPI CoGetState16(LPDWORD state)
{
    FIXME("(%p),stub!\n", state);
    *state = 0;
    return S_OK;
}
/***********************************************************************
 *           CoSetState [OLE32.42]
 */
HRESULT WINAPI CoSetState(LPDWORD state)
{
    FIXME("(%p),stub!\n", state);
    if (state) *state = 0;
    return S_OK;
}
/***********************************************************************
 *          CoCreateFreeThreadedMarshaler [OLE32.5]
 */
HRESULT WINAPI CoCreateFreeThreadedMarshaler (LPUNKNOWN punkOuter, LPUNKNOWN* ppunkMarshal)
{
   FIXME ("(%p %p): stub\n", punkOuter, ppunkMarshal);
    
   return S_OK;
}


/***********************************************************************
 *           DllGetClassObject [OLE32.63]
 */
HRESULT WINAPI OLE32_DllGetClassObject(REFCLSID rclsid, REFIID iid,LPVOID *ppv)
{	
	FIXME("\n\tCLSID:\t%s,\n\tIID:\t%s\n",debugstr_guid(rclsid),debugstr_guid(iid));
	*ppv = NULL;
	return CLASS_E_CLASSNOTAVAILABLE;
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

  DPRINTF("\nExternal lock list contains:\n");

  while ( current != EL_END_OF_LIST )
  {
      DPRINTF( "\t%p with %lu references count.\n", current->pUnk, current->uRefCount);
 
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

/***********************************************************************
 *      DllEntryPoint                   [COMPOBJ.116]
 *
 *    Initialization code for the COMPOBJ DLL
 *
 * RETURNS:
 */
BOOL WINAPI COMPOBJ_DllEntryPoint(DWORD Reason, HINSTANCE16 hInst, WORD ds, WORD HeapSize, DWORD res1, WORD res2)
{
        TRACE("(%08lx, %04x, %04x, %04x, %08lx, %04x)\n", Reason, hInst, ds, HeapSize,
 res1, res2);
        switch(Reason)
        {
        case DLL_PROCESS_ATTACH:
                if (!COMPOBJ_Attach++) COMPOBJ_hInstance = hInst;
                break;

        case DLL_PROCESS_DETACH:
                if(!--COMPOBJ_Attach)
                        COMPOBJ_hInstance = 0;
                break;
        }
        return TRUE;
}

/******************************************************************************
 *              OleGetAutoConvert        [OLE32.104]
 */
HRESULT WINAPI OleGetAutoConvert(REFCLSID clsidOld, LPCLSID pClsidNew)
{
    HKEY hkey = 0;
    char buf[200];
    WCHAR wbuf[200];
    DWORD len;
    HRESULT res = S_OK;

    sprintf(buf,"CLSID\\");WINE_StringFromCLSID(clsidOld,&buf[6]);
    if (RegOpenKeyA(HKEY_CLASSES_ROOT,buf,&hkey))
    {
        res = REGDB_E_CLASSNOTREG;
	goto done;
    }
    len = 200;
    /* we can just query for the default value of AutoConvertTo key like that,
       without opening the AutoConvertTo key and querying for NULL (default) */
    if (RegQueryValueA(hkey,"AutoConvertTo",buf,&len))
    {
        res = REGDB_E_KEYMISSING;
	goto done;
    }
    MultiByteToWideChar( CP_ACP, 0, buf, -1, wbuf, sizeof(wbuf)/sizeof(WCHAR) );
    CLSIDFromString(wbuf,pClsidNew);
done:
  if (hkey) RegCloseKey(hkey);

  return res;
}

/******************************************************************************
 *              OleSetAutoConvert        [OLE32.126]
 */
HRESULT WINAPI OleSetAutoConvert(REFCLSID clsidOld, REFCLSID clsidNew)
{
    HKEY hkey = 0, hkeyConvert = 0;
    char buf[200], szClsidNew[200];
    HRESULT res = S_OK;

    TRACE("(%p,%p);\n", clsidOld, clsidNew);
    sprintf(buf,"CLSID\\");WINE_StringFromCLSID(clsidOld,&buf[6]);
    WINE_StringFromCLSID(clsidNew, szClsidNew);
    if (RegOpenKeyA(HKEY_CLASSES_ROOT,buf,&hkey))
    {
        res = REGDB_E_CLASSNOTREG;
	goto done;
    }
    if (RegCreateKeyA(hkey, "AutoConvertTo", &hkeyConvert))
    {
        res = REGDB_E_WRITEREGDB;
	goto done;
    }
    if (RegSetValueExA(hkeyConvert, NULL, 0,
                            REG_SZ, (LPBYTE)szClsidNew, strlen(szClsidNew)+1))
    {
        res = REGDB_E_WRITEREGDB;
	goto done;
    }

done:
    if (hkeyConvert) RegCloseKey(hkeyConvert);
    if (hkey) RegCloseKey(hkey);

    return res;
}

/***********************************************************************
 *           IsEqualGUID [OLE32.76]
 *
 * Compares two Unique Identifiers.
 *
 * RETURNS
 *	TRUE if equal
 */
#undef IsEqualGUID
BOOL WINAPI IsEqualGUID(
     REFGUID rguid1, /* [in] unique id 1 */
     REFGUID rguid2  /* [in] unique id 2 */
     )
{
    return !memcmp(rguid1,rguid2,sizeof(GUID));
}
