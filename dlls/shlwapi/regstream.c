/*
 *	SHRegOpenStream
 */
#include <string.h>

#include "winerror.h"
#include "winreg.h"
#include "wine/obj_storage.h"

#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(shell);

typedef struct 
{	ICOM_VFIELD(IStream);
	DWORD		ref;
	HKEY		hKey;
	LPBYTE		pbBuffer;
	DWORD		dwLength;
	DWORD		dwPos;
} ISHRegStream;

static struct ICOM_VTABLE(IStream) rstvt;

/**************************************************************************
*   IStream_ConstructorA	[internal]
*/
static IStream *IStream_ConstructorA(HKEY hKey, LPCSTR pszSubKey, LPCSTR pszValue, DWORD grfMode)
{
	ISHRegStream*	rstr;
	DWORD		dwType;
	
	rstr = (ISHRegStream*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ISHRegStream));

	ICOM_VTBL(rstr)=&rstvt;
	rstr->ref = 1;

	if (!(RegOpenKeyExA (hKey, pszSubKey, 0, KEY_READ, &(rstr->hKey))))
	{
	  if (!(RegQueryValueExA(rstr->hKey, pszValue,0,0,0,&(rstr->dwLength))))
	  { 
	    /* read the binary data into the buffer */
	    if((rstr->pbBuffer = HeapAlloc(GetProcessHeap(),0,rstr->dwLength)))
	    {
	      if (!(RegQueryValueExA(rstr->hKey, pszValue,0,&dwType,rstr->pbBuffer,&(rstr->dwLength))))
	      {
	        if (dwType == REG_BINARY )
	        {
	          TRACE ("%p\n", rstr);
	          return (IStream*)rstr;
	        }
	      }
	      HeapFree (GetProcessHeap(),0,rstr->pbBuffer);
	    }
	  }
	  RegCloseKey(rstr->hKey);
	}
	HeapFree (GetProcessHeap(),0,rstr);
	return NULL;
}

/**************************************************************************
*   IStream_ConstructorW	[internal]
*/
static IStream *IStream_ConstructorW(HKEY hKey, LPCWSTR pszSubKey, LPCWSTR pszValue, DWORD grfMode)
{
	ISHRegStream*	rstr;
	DWORD		dwType;
	
	rstr = (ISHRegStream*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ISHRegStream));

	ICOM_VTBL(rstr)=&rstvt;
	rstr->ref = 1;

	if (!(RegOpenKeyExW (hKey, pszSubKey, 0, KEY_READ, &(rstr->hKey))))
	{
	  if (!(RegQueryValueExW(rstr->hKey, pszValue,0,0,0,&(rstr->dwLength))))
	  { 
	    /* read the binary data into the buffer */
	    if((rstr->pbBuffer = HeapAlloc(GetProcessHeap(),0,rstr->dwLength)))
	    {
	      if (!(RegQueryValueExW(rstr->hKey, pszValue,0,&dwType,rstr->pbBuffer,&(rstr->dwLength))))
	      {
	        if (dwType == REG_BINARY )
	        {
	          TRACE ("%p\n", rstr);
	          return (IStream*)rstr;
	        }
	      }
	      HeapFree (GetProcessHeap(),0,rstr->pbBuffer);
	    }
	  }
	  RegCloseKey(rstr->hKey);
	}
	HeapFree (GetProcessHeap(),0,rstr);
	return NULL;
}

/**************************************************************************
*  IStream_fnQueryInterface
*/
static HRESULT WINAPI IStream_fnQueryInterface(IStream *iface, REFIID riid, LPVOID *ppvObj)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))	/*IUnknown*/
	{ *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IStream))	/*IStream*/
	{ *ppvObj = This;
	}   

	if(*ppvObj)
	{ 
	  IStream_AddRef((IStream*)*ppvObj);      
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/**************************************************************************
*  IStream_fnAddRef
*/
static ULONG WINAPI IStream_fnAddRef(IStream *iface)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE("(%p)->(count=%lu)\n",This, This->ref);

	return ++(This->ref);
}

/**************************************************************************
*  IStream_fnRelease
*/
static ULONG WINAPI IStream_fnRelease(IStream *iface)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE("(%p)->()\n",This);

	if (!--(This->ref)) 
	{ TRACE(" destroying SHReg IStream (%p)\n",This);

	  if (This->pbBuffer)
	    HeapFree(GetProcessHeap(),0,This->pbBuffer);

	  if (This->hKey)
	    RegCloseKey(This->hKey);

	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}

static HRESULT WINAPI IStream_fnRead (IStream * iface, void* pv, ULONG cb, ULONG* pcbRead)
{
	ICOM_THIS(ISHRegStream, iface);

	DWORD dwBytesToRead, dwBytesLeft;
	
	TRACE("(%p)->(%p,0x%08lx,%p)\n",This, pv, cb, pcbRead);
	
	if ( !pv )
	  return STG_E_INVALIDPOINTER;
	  
	dwBytesLeft = This->dwLength - This->dwPos;

	if ( 0 >= dwBytesLeft )						/* end of buffer */
	  return S_FALSE;
	
	dwBytesToRead = ( cb > dwBytesLeft) ? dwBytesLeft : cb;

	memmove ( pv, (This->pbBuffer) + (This->dwPos), dwBytesToRead);
	
	This->dwPos += dwBytesToRead;					/* adjust pointer */

	if (pcbRead)
	  *pcbRead = dwBytesToRead;

	return S_OK;
}
static HRESULT WINAPI IStream_fnWrite (IStream * iface, const void* pv, ULONG cb, ULONG* pcbWritten)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE("(%p)\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI IStream_fnSeek (IStream * iface, LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE("(%p)\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI IStream_fnSetSize (IStream * iface, ULARGE_INTEGER libNewSize)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE("(%p)\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI IStream_fnCopyTo (IStream * iface, IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE("(%p)\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI IStream_fnCommit (IStream * iface, DWORD grfCommitFlags)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE("(%p)\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI IStream_fnRevert (IStream * iface)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE("(%p)\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI IStream_fnLockRegion (IStream * iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE("(%p)\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI IStream_fnUnlockRegion (IStream * iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE("(%p)\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI IStream_fnStat (IStream * iface, STATSTG*   pstatstg, DWORD grfStatFlag)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE("(%p)\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI IStream_fnClone (IStream * iface, IStream** ppstm)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE("(%p)\n",This);

	return E_NOTIMPL;
}

static struct ICOM_VTABLE(IStream) rstvt = 
{	
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IStream_fnQueryInterface,
	IStream_fnAddRef,
	IStream_fnRelease,
	IStream_fnRead,
	IStream_fnWrite,
	IStream_fnSeek,
	IStream_fnSetSize,
	IStream_fnCopyTo,
	IStream_fnCommit,
	IStream_fnRevert,
	IStream_fnLockRegion,
	IStream_fnUnlockRegion,
	IStream_fnStat,
	IStream_fnClone
	
};

/*************************************************************************
 * SHOpenRegStreamA				[SHLWAPI.@]
 */
IStream * WINAPI SHOpenRegStreamA(
	HKEY hkey,
	LPCSTR pszSubkey,
	LPCSTR pszValue,
	DWORD grfMode)
{
	TRACE("(0x%08x,%s,%s,0x%08lx)\n",
	hkey, pszSubkey, pszValue, grfMode);

	return IStream_ConstructorA(hkey, pszSubkey, pszValue, grfMode);
}

/*************************************************************************
 * SHOpenRegStreamW				[SHLWAPI.@]
 */
IStream * WINAPI SHOpenRegStreamW(
	HKEY hkey,
	LPCWSTR pszSubkey,
	LPCWSTR pszValue,
	DWORD grfMode)
{
	TRACE("(0x%08x,%s,%s,0x%08lx)\n",
	hkey, debugstr_w(pszSubkey), debugstr_w(pszValue), grfMode);

	return IStream_ConstructorW(hkey, pszSubkey, pszValue, grfMode);
}
