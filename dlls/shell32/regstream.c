/*
 *	SHRegOpenStream
 */
#include <string.h>

#include "wine/obj_storage.h"

#include "debug.h"
#include "heap.h"
#include "winerror.h"
#include "winreg.h"

#include "shell32_main.h"

DEFAULT_DEBUG_CHANNEL(shell)

typedef struct 
{	ICOM_VTABLE(IStream)* lpvtbl;
	DWORD		ref;
	HKEY		hKey;
	LPSTR		pszSubKey;
	LPSTR		pszValue;
	LPBYTE		pbBuffer;
	DWORD		dwLength;
	DWORD		dwPos;
} ISHRegStream;

static struct ICOM_VTABLE(IStream) rstvt;

/**************************************************************************
*   IStream_Constructor()
*/
IStream *IStream_Constructor(HKEY hKey, LPCSTR pszSubKey, LPCSTR pszValue, DWORD grfMode)
{	ISHRegStream*	rstr;
	DWORD		dwType;
	
	rstr = (ISHRegStream*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ISHRegStream));
	rstr->lpvtbl=&rstvt;
	rstr->ref = 1;

	if ( ERROR_SUCCESS == RegOpenKeyExA (hKey, pszSubKey, 0, KEY_READ, &(rstr->hKey)))
	{ if ( ERROR_SUCCESS == RegQueryValueExA(rstr->hKey, (LPSTR)pszValue,0,0,0,&(rstr->dwLength)))
	  { 
	    /* read the binary data into the buffer */
	    rstr->pbBuffer = HeapAlloc(GetProcessHeap(),0,rstr->dwLength);
	    if (rstr->pbBuffer)
	    { if ( ERROR_SUCCESS == RegQueryValueExA(rstr->hKey, (LPSTR)pszValue,0,&dwType,rstr->pbBuffer,&(rstr->dwLength)))
	      { if (dwType == REG_BINARY )
	        { rstr->pszSubKey = HEAP_strdupA (GetProcessHeap(),0, pszSubKey);
	          rstr->pszValue = HEAP_strdupA (GetProcessHeap(),0, pszValue);		
	          TRACE(shell,"(%p)->0x%08x,%s,%s,0x%08lx\n", rstr, hKey, pszSubKey, pszValue, grfMode);
	          shell32_ObjCount++;
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

	char    xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);

	TRACE(shell,"(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

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
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/**************************************************************************
*  IStream_fnAddRef
*/
static ULONG WINAPI IStream_fnAddRef(IStream *iface)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE(shell,"(%p)->(count=%lu)\n",This, This->ref);

	shell32_ObjCount++;
	return ++(This->ref);
}

/**************************************************************************
*  IStream_fnRelease
*/
static ULONG WINAPI IStream_fnRelease(IStream *iface)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE(shell,"(%p)->()\n",This);

	shell32_ObjCount--;

	if (!--(This->ref)) 
	{ TRACE(shell," destroying SHReg IStream (%p)\n",This);

	  if (This->pszSubKey)
	    HeapFree(GetProcessHeap(),0,This->pszSubKey);

	  if (This->pszValue)
	    HeapFree(GetProcessHeap(),0,This->pszValue);

	  if (This->pbBuffer)
	    HeapFree(GetProcessHeap(),0,This->pbBuffer);

	  if (This->hKey)
	    RegCloseKey(This->hKey);

	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}

HRESULT WINAPI IStream_fnRead (IStream * iface, void* pv, ULONG cb, ULONG* pcbRead)
{
	ICOM_THIS(ISHRegStream, iface);

	DWORD dwBytesToRead, dwBytesLeft;
	
	TRACE(shell,"(%p)->(%p,0x%08lx,%p)\n",This, pv, cb, pcbRead);
	
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
HRESULT WINAPI IStream_fnWrite (IStream * iface, const void* pv, ULONG cb, ULONG* pcbWritten)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE(shell,"(%p)\n",This);

	return E_NOTIMPL;
}
HRESULT WINAPI IStream_fnSeek (IStream * iface, LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE(shell,"(%p)\n",This);

	return E_NOTIMPL;
}
HRESULT WINAPI IStream_fnSetSize (IStream * iface, ULARGE_INTEGER libNewSize)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE(shell,"(%p)\n",This);

	return E_NOTIMPL;
}
HRESULT WINAPI IStream_fnCopyTo (IStream * iface, IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE(shell,"(%p)\n",This);

	return E_NOTIMPL;
}
HRESULT WINAPI IStream_fnCommit (IStream * iface, DWORD grfCommitFlags)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE(shell,"(%p)\n",This);

	return E_NOTIMPL;
}
HRESULT WINAPI IStream_fnRevert (IStream * iface)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE(shell,"(%p)\n",This);

	return E_NOTIMPL;
}
HRESULT WINAPI IStream_fnLockRegion (IStream * iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE(shell,"(%p)\n",This);

	return E_NOTIMPL;
}
HRESULT WINAPI IStream_fnUnlockRegion (IStream * iface, ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE(shell,"(%p)\n",This);

	return E_NOTIMPL;
}
HRESULT WINAPI IStream_fnStat (IStream * iface, STATSTG*   pstatstg, DWORD grfStatFlag)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE(shell,"(%p)\n",This);

	return E_NOTIMPL;
}
HRESULT WINAPI IStream_fnClone (IStream * iface, IStream** ppstm)
{
	ICOM_THIS(ISHRegStream, iface);

	TRACE(shell,"(%p)\n",This);

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
 * OpenRegStream				[SHELL32.85]
 *
 * NOTES
 *     exported by ordinal
 */
IStream * WINAPI OpenRegStream(HKEY hkey, LPCSTR pszSubkey, LPCSTR pszValue, DWORD grfMode)
{
	TRACE(shell,"(0x%08x,%s,%s,0x%08lx)\n",hkey, pszSubkey, pszValue, grfMode);
	return IStream_Constructor(hkey, pszSubkey, pszValue, grfMode);
}
