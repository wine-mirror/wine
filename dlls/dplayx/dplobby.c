/* Direct Play Lobby 2 & 3 Implementation
 *
 * Copyright 1998,1999,2000 - Peter Hunnisett
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include <stdarg.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "winnls.h"
#include "wine/debug.h"

#include "dplayx_global.h"
#include "dplayx_messages.h"
#include "dplayx_queue.h"
#include "dplobby.h"

WINE_DEFAULT_DEBUG_CHANNEL(dplay);

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
typedef struct IDirectPlayLobbyImpl  IDirectPlayLobbyAImpl;
typedef struct IDirectPlayLobbyImpl  IDirectPlayLobbyWImpl;
typedef struct IDirectPlayLobbyImpl  IDirectPlayLobby2AImpl;
typedef struct IDirectPlayLobbyImpl  IDirectPlayLobby2WImpl;
typedef struct IDirectPlayLobbyImpl  IDirectPlayLobby3AImpl;
typedef struct IDirectPlayLobbyImpl  IDirectPlayLobby3WImpl;

/* Forward declarations for this module helper methods */
HRESULT DPL_CreateCompoundAddress ( LPCDPCOMPOUNDADDRESSELEMENT lpElements, DWORD dwElementCount,
                                    LPVOID lpAddress, LPDWORD lpdwAddressSize, BOOL bAnsiInterface )DECLSPEC_HIDDEN;

static HRESULT DPL_CreateAddress( REFGUID guidSP, REFGUID guidDataType, LPCVOID lpData, DWORD dwDataSize,
                           LPVOID lpAddress, LPDWORD lpdwAddressSize, BOOL bAnsiInterface );


/*****************************************************************************
 * IDirectPlayLobby {1,2,3} implementation structure
 *
 * The philosophy behind this extra pointer dereference is that I wanted to
 * have the same structure for all types of objects without having to do
 * a lot of casting. I also only wanted to implement an interface in the
 * object it was "released" with IUnknown interface being implemented in the 1 version.
 * Of course, with these new interfaces comes the data required to keep the state required
 * by these interfaces. So, basically, the pointers contain the data associated with
 * a release. If you use the data associated with release 3 in a release 2 object, you'll
 * get a run time trap, as that won't have any data.
 *
 */
struct DPLMSG
{
  DPQ_ENTRY( DPLMSG ) msgs;  /* Link to next queued message */
};
typedef struct DPLMSG* LPDPLMSG;

typedef struct tagDirectPlayLobbyData
{
  HKEY  hkCallbackKeyHack;
  DWORD dwMsgThread;
  DPQ_HEAD( DPLMSG ) msgs;  /* List of messages received */
} DirectPlayLobbyData;

typedef struct IDirectPlayLobbyImpl
{
    IDirectPlayLobby3 IDirectPlayLobby3_iface;
    IDirectPlayLobby3A IDirectPlayLobby3A_iface;
    LONG ulInterfaceRef;
    CRITICAL_SECTION lock;
    DirectPlayLobbyData*          dpl;
} IDirectPlayLobbyImpl;

static inline IDirectPlayLobbyImpl *impl_from_IDirectPlayLobby3( IDirectPlayLobby3 *iface )
{
    return CONTAINING_RECORD( iface, IDirectPlayLobbyImpl, IDirectPlayLobby3_iface );
}

static inline IDirectPlayLobbyImpl *impl_from_IDirectPlayLobby3A( IDirectPlayLobby3A *iface )
{
    return CONTAINING_RECORD( iface, IDirectPlayLobbyImpl, IDirectPlayLobby3A_iface );
}

static BOOL DPL_CreateLobby1( LPVOID lpDPL )
{
  IDirectPlayLobbyAImpl *This = lpDPL;

  This->dpl = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *(This->dpl) ) );
  if ( This->dpl == NULL )
  {
    return FALSE;
  }

  DPQ_INIT( This->dpl->msgs );

  return TRUE;
}

static BOOL DPL_DestroyLobby1( LPVOID lpDPL )
{
  IDirectPlayLobbyAImpl *This = lpDPL;

  if( This->dpl->dwMsgThread )
  {
    FIXME( "Should kill the msg thread\n" );
  }

  DPQ_DELETEQ( This->dpl->msgs, msgs, LPDPLMSG, cbDeleteElemFromHeap );

  /* Delete the contents */
  HeapFree( GetProcessHeap(), 0, This->dpl );

  return TRUE;
}

static HRESULT WINAPI DPL_QueryInterface( IDirectPlayLobbyImpl *This, REFIID riid, void **ppvObj )
{
  TRACE("(%p)->(%s,%p)\n", This, debugstr_guid( riid ), ppvObj );

  if ( IsEqualGUID( &IID_IUnknown, riid ) || IsEqualGUID( &IID_IDirectPlayLobby, riid ) ||
      IsEqualGUID( &IID_IDirectPlayLobby2, riid ) || IsEqualGUID( &IID_IDirectPlayLobby3, riid ) )
    *ppvObj = &This->IDirectPlayLobby3_iface;
  else if ( IsEqualGUID( &IID_IDirectPlayLobbyA, riid ) ||
      IsEqualGUID( &IID_IDirectPlayLobby2A, riid ) || IsEqualGUID( &IID_IDirectPlayLobby3A, riid ) )
    *ppvObj = &This->IDirectPlayLobby3A_iface;
  else
  {
    /* Unsupported interface */
    *ppvObj = NULL;
    return E_NOINTERFACE;
  }

  IDirectPlayLobby_AddRef( (LPDIRECTPLAYLOBBY)*ppvObj );

  return S_OK;
}

/*
 * Simple procedure. Just increment the reference count to this
 * structure and return the new reference count.
 */
static ULONG WINAPI DPL_AddRef( IDirectPlayLobbyImpl *This )
{
  ULONG ulInterfaceRefCount = InterlockedIncrement( &This->ulInterfaceRef );

  TRACE( "ref count incremented to %u for %p\n", ulInterfaceRefCount, This );

  return ulInterfaceRefCount;
}

/*
 * Simple COM procedure. Decrease the reference count to this object.
 * If the object no longer has any reference counts, free up the associated
 * memory.
 */
static ULONG WINAPI DPL_Release( IDirectPlayLobbyImpl *This )
{
  ULONG ulInterfaceRefCount = InterlockedDecrement( &This->ulInterfaceRef );

  TRACE( "ref count decremented to %u for %p\n", ulInterfaceRefCount, This );

  /* Deallocate if this is the last reference to the object */
  if( ulInterfaceRefCount == 0 )
  {
     DPL_DestroyLobby1( This );
     This->lock.DebugInfo->Spare[0] = 0;
     DeleteCriticalSection( &This->lock );
     HeapFree( GetProcessHeap(), 0, This );
  }

  return ulInterfaceRefCount;
}

static HRESULT WINAPI IDirectPlayLobby3AImpl_QueryInterface( IDirectPlayLobby3A *iface, REFIID riid,
        void **ppv )
{
    IDirectPlayLobbyImpl *This = impl_from_IDirectPlayLobby3A( iface );
    return DPL_QueryInterface( This, riid, ppv );
}

static HRESULT WINAPI IDirectPlayLobby3Impl_QueryInterface( IDirectPlayLobby3 *iface, REFIID riid,
        void **ppv )
{
    IDirectPlayLobbyImpl *This = impl_from_IDirectPlayLobby3( iface );
    return DPL_QueryInterface( This, riid, ppv );
}

static ULONG WINAPI IDirectPlayLobby3AImpl_AddRef(IDirectPlayLobby3A *iface)
{
    IDirectPlayLobbyImpl *This = impl_from_IDirectPlayLobby3A( iface );
    return DPL_AddRef( This );
}

static ULONG WINAPI IDirectPlayLobby3Impl_AddRef(IDirectPlayLobby3 *iface)
{
    IDirectPlayLobbyImpl *This = impl_from_IDirectPlayLobby3( iface );
    return DPL_AddRef( This );
}

static ULONG WINAPI IDirectPlayLobby3AImpl_Release(IDirectPlayLobby3A *iface)
{
    IDirectPlayLobbyImpl *This = impl_from_IDirectPlayLobby3A( iface );
    return DPL_Release( This );
}

static ULONG WINAPI IDirectPlayLobby3Impl_Release(IDirectPlayLobby3 *iface)
{
    IDirectPlayLobbyImpl *This = impl_from_IDirectPlayLobby3( iface );
    return DPL_Release( This );
}


/********************************************************************
 *
 * Connects an application to the session specified by the DPLCONNECTION
 * structure currently stored with the DirectPlayLobby object.
 *
 * Returns an IDirectPlay interface.
 *
 */
static HRESULT DPL_ConnectEx
( IDirectPlayLobbyAImpl* This,
  DWORD     dwFlags,
  REFIID    riid,
  LPVOID*   lplpDP,
  IUnknown* pUnk)
{
  HRESULT         hr;
  DWORD           dwOpenFlags = 0;
  DWORD           dwConnSize = 0;
  LPDPLCONNECTION lpConn;

  FIXME("(%p)->(0x%08x,%p,%p): semi stub\n", This, dwFlags, lplpDP, pUnk );

  if( pUnk )
  {
     return DPERR_INVALIDPARAMS;
  }

  /* Backwards compatibility */
  if( dwFlags == 0 )
  {
    dwFlags = DPCONNECT_RETURNSTATUS;
  }

  if ( ( hr = dplay_create( riid, lplpDP ) ) != DP_OK )
  {
     ERR( "error creating interface for %s:%s.\n",
          debugstr_guid( riid ), DPLAYX_HresultToString( hr ) );
     return hr;
  }

  /* FIXME: Is it safe/correct to use appID of 0? */
  hr = IDirectPlayLobby_GetConnectionSettings( &This->IDirectPlayLobby3_iface,
                                               0, NULL, &dwConnSize );
  if( hr != DPERR_BUFFERTOOSMALL )
  {
    return hr;
  }

  lpConn = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwConnSize );

  if( lpConn == NULL )
  {
    return DPERR_NOMEMORY;
  }

  /* FIXME: Is it safe/correct to use appID of 0? */
  hr = IDirectPlayLobby_GetConnectionSettings( &This->IDirectPlayLobby3_iface,
                                               0, lpConn, &dwConnSize );
  if( FAILED( hr ) )
  {
    HeapFree( GetProcessHeap(), 0, lpConn );
    return hr;
  }

#if 0
  /* - Need to call IDirectPlay::EnumConnections with the service provider to get that good information
   * - Need to call CreateAddress to create the lpConnection param for IDirectPlay::InitializeConnection
   * - Call IDirectPlay::InitializeConnection
   */

  /* Now initialize the Service Provider */
  hr = IDirectPlayX_InitializeConnection( (*(LPDIRECTPLAY2*)lplpDP),
#endif


  /* Setup flags to pass into DirectPlay::Open */
  if( dwFlags & DPCONNECT_RETURNSTATUS )
  {
    dwOpenFlags |= DPOPEN_RETURNSTATUS;
  }
  dwOpenFlags |= lpConn->dwFlags;

  hr = IDirectPlayX_Open( (*(LPDIRECTPLAY2*)lplpDP), lpConn->lpSessionDesc,
                          dwOpenFlags );

  HeapFree( GetProcessHeap(), 0, lpConn );

  return hr;
}

static HRESULT WINAPI IDirectPlayLobby3AImpl_Connect( IDirectPlayLobby3A *iface, DWORD flags,
    IDirectPlay2A **dp, IUnknown *unk)
{
    return IDirectPlayLobby_ConnectEx( iface, flags, &IID_IDirectPlay2A, (void**)dp, unk );
}

static HRESULT WINAPI IDirectPlayLobby3Impl_Connect( IDirectPlayLobby3 *iface, DWORD flags,
        IDirectPlay2 **dp, IUnknown *unk)
{
    return IDirectPlayLobby_ConnectEx( iface, flags, &IID_IDirectPlay2A, (void**)dp, unk );
}

/********************************************************************
 *
 * Creates a DirectPlay Address, given a service provider-specific network
 * address.
 * Returns an address contains the globally unique identifier
 * (GUID) of the service provider and data that the service provider can
 * interpret as a network address.
 *
 * NOTE: It appears that this method is supposed to be really really stupid
 *       with no error checking on the contents.
 */
static HRESULT WINAPI IDirectPlayLobby3AImpl_CreateAddress( IDirectPlayLobby3A *iface,
        REFGUID guidSP, REFGUID guidDataType, const void *lpData, DWORD dwDataSize, void *lpAddress,
        DWORD *lpdwAddressSize )
{
  return DPL_CreateAddress( guidSP, guidDataType, lpData, dwDataSize,
                            lpAddress, lpdwAddressSize, TRUE );
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_CreateAddress
( LPDIRECTPLAYLOBBY iface,
  REFGUID guidSP,
  REFGUID guidDataType,
  LPCVOID lpData,
  DWORD dwDataSize,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize )
{
  return DPL_CreateAddress( guidSP, guidDataType, lpData, dwDataSize,
                            lpAddress, lpdwAddressSize, FALSE );
}

static HRESULT DPL_CreateAddress(
  REFGUID guidSP,
  REFGUID guidDataType,
  LPCVOID lpData,
  DWORD dwDataSize,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize,
  BOOL bAnsiInterface )
{
  const DWORD dwNumAddElements = 2; /* Service Provide & address data type */
  DPCOMPOUNDADDRESSELEMENT addressElements[ 2 /* dwNumAddElements */ ];

  TRACE( "(%p)->(%p,%p,0x%08x,%p,%p,%d)\n", guidSP, guidDataType, lpData, dwDataSize,
                                             lpAddress, lpdwAddressSize, bAnsiInterface );

  addressElements[ 0 ].guidDataType = DPAID_ServiceProvider;
  addressElements[ 0 ].dwDataSize = sizeof( GUID );
  addressElements[ 0 ].lpData = (LPVOID)guidSP;

  addressElements[ 1 ].guidDataType = *guidDataType;
  addressElements[ 1 ].dwDataSize = dwDataSize;
  addressElements[ 1 ].lpData = (LPVOID)lpData;

  /* Call CreateCompoundAddress to cut down on code.
     NOTE: We can do this because we don't support DPL 1 interfaces! */
  return DPL_CreateCompoundAddress( addressElements, dwNumAddElements,
                                    lpAddress, lpdwAddressSize, bAnsiInterface );
}



/********************************************************************
 *
 * Parses out chunks from the DirectPlay Address buffer by calling the
 * given callback function, with lpContext, for each of the chunks.
 *
 */
static HRESULT WINAPI IDirectPlayLobby3AImpl_EnumAddress( IDirectPlayLobby3A *iface,
        LPDPENUMADDRESSCALLBACK lpEnumAddressCallback, const void *lpAddress, DWORD dwAddressSize,
        void *lpContext )
{
  IDirectPlayLobbyImpl *This = impl_from_IDirectPlayLobby3A( iface );

  TRACE("(%p)->(%p,%p,0x%08x,%p)\n", This, lpEnumAddressCallback, lpAddress,
                                      dwAddressSize, lpContext );

  return DPL_EnumAddress( lpEnumAddressCallback, lpAddress, dwAddressSize, lpContext );
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_EnumAddress
( LPDIRECTPLAYLOBBY iface,
  LPDPENUMADDRESSCALLBACK lpEnumAddressCallback,
  LPCVOID lpAddress,
  DWORD dwAddressSize,
  LPVOID lpContext )
{
  IDirectPlayLobbyWImpl *This = (IDirectPlayLobbyWImpl *)iface;

  TRACE("(%p)->(%p,%p,0x%08x,%p)\n", This, lpEnumAddressCallback, lpAddress,
                                      dwAddressSize, lpContext );

  return DPL_EnumAddress( lpEnumAddressCallback, lpAddress, dwAddressSize, lpContext );
}

HRESULT DPL_EnumAddress( LPDPENUMADDRESSCALLBACK lpEnumAddressCallback, LPCVOID lpAddress,
                         DWORD dwAddressSize, LPVOID lpContext )
{
  DWORD dwTotalSizeEnumerated = 0;

  /* FIXME: First chunk is always the total size chunk - Should we report it? */

  while ( dwTotalSizeEnumerated < dwAddressSize )
  {
    const DPADDRESS* lpElements = lpAddress;
    DWORD dwSizeThisEnumeration;

    /* Invoke the enum method. If false is returned, stop enumeration */
    if ( !lpEnumAddressCallback( &lpElements->guidDataType,
                                 lpElements->dwDataSize,
                                 (const BYTE *)lpElements + sizeof( DPADDRESS ),
                                 lpContext ) )
    {
      break;
    }

    dwSizeThisEnumeration  = sizeof( DPADDRESS ) + lpElements->dwDataSize;
    lpAddress = (const BYTE*) lpAddress + dwSizeThisEnumeration;
    dwTotalSizeEnumerated += dwSizeThisEnumeration;
  }

  return DP_OK;
}

/********************************************************************
 *
 * Enumerates all the address types that a given service provider needs to
 * build the DirectPlay Address.
 *
 */
static HRESULT WINAPI IDirectPlayLobby3AImpl_EnumAddressTypes( IDirectPlayLobby3A *iface,
        LPDPLENUMADDRESSTYPESCALLBACK lpEnumAddressTypeCallback, REFGUID guidSP, void *lpContext,
        DWORD dwFlags )
{
  IDirectPlayLobbyImpl *This = impl_from_IDirectPlayLobby3A( iface );

  HKEY   hkResult;
  LPCSTR searchSubKey    = "SOFTWARE\\Microsoft\\DirectPlay\\Service Providers";
  DWORD  dwIndex, sizeOfSubKeyName=50;
  char   subKeyName[51];
  FILETIME filetime;

  TRACE(" (%p)->(%p,%p,%p,0x%08x)\n", This, lpEnumAddressTypeCallback, guidSP, lpContext, dwFlags );

  if( dwFlags != 0 )
  {
    return DPERR_INVALIDPARAMS;
  }

  if( !lpEnumAddressTypeCallback )
  {
     return DPERR_INVALIDPARAMS;
  }

  if( guidSP == NULL )
  {
    return DPERR_INVALIDOBJECT;
  }

    /* Need to loop over the service providers in the registry */
    if( RegOpenKeyExA( HKEY_LOCAL_MACHINE, searchSubKey,
                         0, KEY_READ, &hkResult ) != ERROR_SUCCESS )
    {
      /* Hmmm. Does this mean that there are no service providers? */
      ERR(": no service providers?\n");
      return DP_OK;
    }

    /* Traverse all the service providers we have available */
    for( dwIndex=0;
         RegEnumKeyExA( hkResult, dwIndex, subKeyName, &sizeOfSubKeyName,
                        NULL, NULL, NULL, &filetime ) != ERROR_NO_MORE_ITEMS;
         ++dwIndex, sizeOfSubKeyName=50 )
    {

      HKEY     hkServiceProvider, hkServiceProviderAt;
      GUID     serviceProviderGUID;
      DWORD    returnTypeGUID, sizeOfReturnBuffer = 50;
      char     atSubKey[51];
      char     returnBuffer[51];
      WCHAR    buff[51];
      DWORD    dwAtIndex;
      LPCSTR   atKey = "Address Types";
      LPCSTR   guidDataSubKey   = "Guid";
      FILETIME filetime;


      TRACE(" this time through: %s\n", subKeyName );

      /* Get a handle for this particular service provider */
      if( RegOpenKeyExA( hkResult, subKeyName, 0, KEY_READ,
                         &hkServiceProvider ) != ERROR_SUCCESS )
      {
         ERR(": what the heck is going on?\n" );
         continue;
      }

      if( RegQueryValueExA( hkServiceProvider, guidDataSubKey,
                            NULL, &returnTypeGUID, (LPBYTE)returnBuffer,
                            &sizeOfReturnBuffer ) != ERROR_SUCCESS )
      {
        ERR(": missing GUID registry data members\n" );
        continue;
      }

      /* FIXME: Check return types to ensure we're interpreting data right */
      MultiByteToWideChar( CP_ACP, 0, returnBuffer, -1, buff, sizeof(buff)/sizeof(WCHAR) );
      CLSIDFromString( buff, &serviceProviderGUID );
      /* FIXME: Have I got a memory leak on the serviceProviderGUID? */

      /* Determine if this is the Service Provider that the user asked for */
      if( !IsEqualGUID( &serviceProviderGUID, guidSP ) )
      {
        continue;
      }

      /* Get a handle for this particular service provider */
      if( RegOpenKeyExA( hkServiceProvider, atKey, 0, KEY_READ,
                         &hkServiceProviderAt ) != ERROR_SUCCESS )
      {
        TRACE(": No Address Types registry data sub key/members\n" );
        break;
      }

      /* Traverse all the address type we have available */
      for( dwAtIndex=0;
           RegEnumKeyExA( hkServiceProviderAt, dwAtIndex, atSubKey, &sizeOfSubKeyName,
                          NULL, NULL, NULL, &filetime ) != ERROR_NO_MORE_ITEMS;
           ++dwAtIndex, sizeOfSubKeyName=50 )
      {
        TRACE( "Found Address Type GUID %s\n", atSubKey );

        /* FIXME: Check return types to ensure we're interpreting data right */
        MultiByteToWideChar( CP_ACP, 0, atSubKey, -1, buff, sizeof(buff)/sizeof(WCHAR) );
        CLSIDFromString( buff, &serviceProviderGUID );
        /* FIXME: Have I got a memory leak on the serviceProviderGUID? */

        /* The enumeration will return FALSE if we are not to continue */
        if( !lpEnumAddressTypeCallback( &serviceProviderGUID, lpContext, 0 ) )
        {
           WARN("lpEnumCallback returning FALSE\n" );
           break; /* FIXME: This most likely has to break from the procedure...*/
        }

      }

      /* We only enumerate address types for 1 GUID. We've found it, so quit looking */
      break;
    }

  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_EnumAddressTypes
( LPDIRECTPLAYLOBBY iface,
  LPDPLENUMADDRESSTYPESCALLBACK lpEnumAddressTypeCallback,
  REFGUID guidSP,
  LPVOID lpContext,
  DWORD dwFlags )
{
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
}

/********************************************************************
 *
 * Enumerates what applications are registered with DirectPlay by
 * invoking the callback function with lpContext.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyWImpl_EnumLocalApplications
( LPDIRECTPLAYLOBBY iface,
  LPDPLENUMLOCALAPPLICATIONSCALLBACK lpEnumLocalAppCallback,
  LPVOID lpContext,
  DWORD dwFlags )
{
  IDirectPlayLobbyWImpl *This = (IDirectPlayLobbyWImpl *)iface;

  FIXME("(%p)->(%p,%p,0x%08x):stub\n", This, lpEnumLocalAppCallback, lpContext, dwFlags );

  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobby3AImpl_EnumLocalApplications( IDirectPlayLobby3A *iface,
        LPDPLENUMLOCALAPPLICATIONSCALLBACK lpEnumLocalAppCallback, void *lpContext, DWORD dwFlags )
{
  IDirectPlayLobbyImpl *This = impl_from_IDirectPlayLobby3A( iface );

  HKEY hkResult;
  LPCSTR searchSubKey    = "SOFTWARE\\Microsoft\\DirectPlay\\Applications";
  LPCSTR guidDataSubKey  = "Guid";
  DWORD dwIndex, sizeOfSubKeyName=50;
  char subKeyName[51];
  FILETIME filetime;

  TRACE("(%p)->(%p,%p,0x%08x)\n", This, lpEnumLocalAppCallback, lpContext, dwFlags );

  if( dwFlags != 0 )
  {
    return DPERR_INVALIDPARAMS;
  }

  if( !lpEnumLocalAppCallback )
  {
     return DPERR_INVALIDPARAMS;
  }

  /* Need to loop over the service providers in the registry */
  if( RegOpenKeyExA( HKEY_LOCAL_MACHINE, searchSubKey,
                     0, KEY_READ, &hkResult ) != ERROR_SUCCESS )
  {
    /* Hmmm. Does this mean that there are no service providers? */
    ERR(": no service providers?\n");
    return DP_OK;
  }

  /* Traverse all registered applications */
  for( dwIndex=0;
       RegEnumKeyExA( hkResult, dwIndex, subKeyName, &sizeOfSubKeyName, NULL, NULL, NULL, &filetime ) != ERROR_NO_MORE_ITEMS;
       ++dwIndex, sizeOfSubKeyName=50 )
  {

    HKEY       hkServiceProvider;
    GUID       serviceProviderGUID;
    DWORD      returnTypeGUID, sizeOfReturnBuffer = 50;
    char       returnBuffer[51];
    WCHAR      buff[51];
    DPLAPPINFO dplAppInfo;

    TRACE(" this time through: %s\n", subKeyName );

    /* Get a handle for this particular service provider */
    if( RegOpenKeyExA( hkResult, subKeyName, 0, KEY_READ,
                       &hkServiceProvider ) != ERROR_SUCCESS )
    {
       ERR(": what the heck is going on?\n" );
       continue;
    }

    if( RegQueryValueExA( hkServiceProvider, guidDataSubKey,
                          NULL, &returnTypeGUID, (LPBYTE)returnBuffer,
                          &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
      ERR(": missing GUID registry data members\n" );
      continue;
    }

    /* FIXME: Check return types to ensure we're interpreting data right */
    MultiByteToWideChar( CP_ACP, 0, returnBuffer, -1, buff, sizeof(buff)/sizeof(WCHAR) );
    CLSIDFromString( buff, &serviceProviderGUID );
    /* FIXME: Have I got a memory leak on the serviceProviderGUID? */

    dplAppInfo.dwSize               = sizeof( dplAppInfo );
    dplAppInfo.guidApplication      = serviceProviderGUID;
    dplAppInfo.u.lpszAppNameA = subKeyName;

    EnterCriticalSection( &This->lock );

    memcpy( &This->dpl->hkCallbackKeyHack, &hkServiceProvider, sizeof( hkServiceProvider ) );

    if( !lpEnumLocalAppCallback( &dplAppInfo, lpContext, dwFlags ) )
    {
       LeaveCriticalSection( &This->lock );
       break;
    }

    LeaveCriticalSection( &This->lock );
  }

  return DP_OK;
}

/********************************************************************
 *
 * Retrieves the DPLCONNECTION structure that contains all the information
 * needed to start and connect an application. This was generated using
 * either the RunApplication or SetConnectionSettings methods.
 *
 * NOTES: If lpData is NULL then just return lpdwDataSize. This allows
 *        the data structure to be allocated by our caller which can then
 *        call this procedure/method again with a valid data pointer.
 */
static HRESULT WINAPI IDirectPlayLobby3AImpl_GetConnectionSettings( IDirectPlayLobby3A *iface,
        DWORD dwAppID, void *lpData, DWORD *lpdwDataSize )
{
  IDirectPlayLobbyImpl *This = impl_from_IDirectPlayLobby3A( iface );
  HRESULT hr;

  TRACE("(%p)->(0x%08x,%p,%p)\n", This, dwAppID, lpData, lpdwDataSize );

  EnterCriticalSection( &This->lock );

  hr = DPLAYX_GetConnectionSettingsA( dwAppID,
                                      lpData,
                                      lpdwDataSize
                                    );

  LeaveCriticalSection( &This->lock );

  return hr;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_GetConnectionSettings
( LPDIRECTPLAYLOBBY iface,
  DWORD dwAppID,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  IDirectPlayLobbyWImpl *This = (IDirectPlayLobbyWImpl *)iface;
  HRESULT hr;

  TRACE("(%p)->(0x%08x,%p,%p)\n", This, dwAppID, lpData, lpdwDataSize );

  EnterCriticalSection( &This->lock );

  hr = DPLAYX_GetConnectionSettingsW( dwAppID,
                                      lpData,
                                      lpdwDataSize
                                    );

  LeaveCriticalSection( &This->lock );

  return hr;
}

/********************************************************************
 *
 * Retrieves the message sent between a lobby client and a DirectPlay
 * application. All messages are queued until received.
 *
 */
static HRESULT WINAPI IDirectPlayLobby3AImpl_ReceiveLobbyMessage( IDirectPlayLobby3A *iface,
        DWORD dwFlags, DWORD dwAppID, DWORD *lpdwMessageFlags, void *lpData,
        DWORD *lpdwDataSize )
{
  IDirectPlayLobbyImpl *This = impl_from_IDirectPlayLobby3A( iface );
  FIXME(":stub %p %08x %08x %p %p %p\n", This, dwFlags, dwAppID, lpdwMessageFlags, lpData,
         lpdwDataSize );
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_ReceiveLobbyMessage
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDWORD lpdwMessageFlags,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  IDirectPlayLobbyWImpl *This = (IDirectPlayLobbyWImpl *)iface;
  FIXME(":stub %p %08x %08x %p %p %p\n", This, dwFlags, dwAppID, lpdwMessageFlags, lpData,
         lpdwDataSize );
  return DPERR_OUTOFMEMORY;
}

typedef struct tagRunApplicationEnumStruct
{
  IDirectPlayLobbyAImpl* This;

  GUID  appGUID;
  LPSTR lpszPath;
  LPSTR lpszFileName;
  LPSTR lpszCommandLine;
  LPSTR lpszCurrentDirectory;
} RunApplicationEnumStruct, *lpRunApplicationEnumStruct;

/* To be called by RunApplication to find how to invoke the function */
static BOOL CALLBACK RunApplicationA_EnumLocalApplications
( LPCDPLAPPINFO   lpAppInfo,
  LPVOID          lpContext,
  DWORD           dwFlags )
{
  lpRunApplicationEnumStruct lpData = (lpRunApplicationEnumStruct)lpContext;

  if( IsEqualGUID( &lpAppInfo->guidApplication, &lpData->appGUID ) )
  {
    char  returnBuffer[200];
    DWORD returnType, sizeOfReturnBuffer;
    LPCSTR clSubKey   = "CommandLine";
    LPCSTR cdSubKey   = "CurrentDirectory";
    LPCSTR fileSubKey = "File";
    LPCSTR pathSubKey = "Path";

    /* FIXME: Lazy man hack - dplay struct has the present reg key saved */

    sizeOfReturnBuffer = 200;

    /* Get all the appropriate data from the registry */
    if( RegQueryValueExA( lpData->This->dpl->hkCallbackKeyHack, clSubKey,
                          NULL, &returnType, (LPBYTE)returnBuffer,
                          &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
       ERR( ": missing CommandLine registry data member\n" );
    }
    else
    {
        if ((lpData->lpszCommandLine = HeapAlloc( GetProcessHeap(), 0, strlen(returnBuffer)+1 )))
            strcpy( lpData->lpszCommandLine, returnBuffer );
    }

    sizeOfReturnBuffer = 200;

    if( RegQueryValueExA( lpData->This->dpl->hkCallbackKeyHack, cdSubKey,
                          NULL, &returnType, (LPBYTE)returnBuffer,
                          &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
       ERR( ": missing CurrentDirectory registry data member\n" );
    }
    else
    {
        if ((lpData->lpszCurrentDirectory = HeapAlloc( GetProcessHeap(), 0, strlen(returnBuffer)+1 )))
            strcpy( lpData->lpszCurrentDirectory, returnBuffer );
    }

    sizeOfReturnBuffer = 200;

    if( RegQueryValueExA( lpData->This->dpl->hkCallbackKeyHack, fileSubKey,
                          NULL, &returnType, (LPBYTE)returnBuffer,
                          &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
       ERR( ": missing File registry data member\n" );
    }
    else
    {
        if ((lpData->lpszFileName = HeapAlloc( GetProcessHeap(), 0, strlen(returnBuffer)+1 )))
            strcpy( lpData->lpszFileName, returnBuffer );
    }

    sizeOfReturnBuffer = 200;

    if( RegQueryValueExA( lpData->This->dpl->hkCallbackKeyHack, pathSubKey,
                          NULL, &returnType, (LPBYTE)returnBuffer,
                          &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
       ERR( ": missing Path registry data member\n" );
    }
    else
    {
        if ((lpData->lpszPath = HeapAlloc( GetProcessHeap(), 0, strlen(returnBuffer)+1 )))
            strcpy( lpData->lpszPath, returnBuffer );
    }

    return FALSE; /* No need to keep going as we found what we wanted */
  }

  return TRUE; /* Keep enumerating, haven't found the application yet */
}

static BOOL DPL_CreateAndSetLobbyHandles( DWORD dwDestProcessId, HANDLE hDestProcess,
                                   LPHANDLE lphStart, LPHANDLE lphDeath,
                                   LPHANDLE lphRead )
{
  /* These are the handles for the created process */
  HANDLE hAppStart = 0, hAppDeath = 0, hAppRead  = 0;
  SECURITY_ATTRIBUTES s_attrib;

  s_attrib.nLength              = sizeof( s_attrib );
  s_attrib.lpSecurityDescriptor = NULL;
  s_attrib.bInheritHandle       = TRUE;

  *lphStart = CreateEventW( &s_attrib, TRUE, FALSE, NULL );
  *lphDeath = CreateEventW( &s_attrib, TRUE, FALSE, NULL );
  *lphRead  = CreateEventW( &s_attrib, TRUE, FALSE, NULL );

  if( ( !DuplicateHandle( GetCurrentProcess(), *lphStart,
                          hDestProcess, &hAppStart,
                          0, FALSE, DUPLICATE_SAME_ACCESS ) ) ||
      ( !DuplicateHandle( GetCurrentProcess(), *lphDeath,
                          hDestProcess, &hAppDeath,
                          0, FALSE, DUPLICATE_SAME_ACCESS ) ) ||
      ( !DuplicateHandle( GetCurrentProcess(), *lphRead,
                          hDestProcess, &hAppRead,
                          0, FALSE, DUPLICATE_SAME_ACCESS ) )
    )
  {
    if (*lphStart) { CloseHandle(*lphStart); *lphStart = 0; }
    if (*lphDeath) { CloseHandle(*lphDeath); *lphDeath = 0; }
    if (*lphRead)  { CloseHandle(*lphRead);  *lphRead  = 0; }
    /* FIXME: Handle leak... */
    ERR( "Unable to dup handles\n" );
    return FALSE;
  }

  if( !DPLAYX_SetLobbyHandles( dwDestProcessId,
                               hAppStart, hAppDeath, hAppRead ) )
  {
    /* FIXME: Handle leak... */
    return FALSE;
  }

  return TRUE;
}


/********************************************************************
 *
 * Starts an application and passes to it all the information to
 * connect to a session.
 *
 */
static HRESULT WINAPI IDirectPlayLobby3AImpl_RunApplication( IDirectPlayLobby3A *iface,
        DWORD dwFlags, DWORD *lpdwAppID, DPLCONNECTION *lpConn, HANDLE hReceiveEvent )
{
  IDirectPlayLobbyImpl *This = impl_from_IDirectPlayLobby3A( iface );
  HRESULT hr;
  RunApplicationEnumStruct enumData;
  char temp[200];
  STARTUPINFOA startupInfo;
  PROCESS_INFORMATION newProcessInfo;
  LPSTR appName;
  DWORD dwSuspendCount;
  HANDLE hStart, hDeath, hSettingRead;

  TRACE( "(%p)->(0x%08x,%p,%p,%p)\n",
         This, dwFlags, lpdwAppID, lpConn, hReceiveEvent );

  if( dwFlags != 0 )
  {
    return DPERR_INVALIDPARAMS;
  }

  if( DPLAYX_AnyLobbiesWaitingForConnSettings() )
  {
    FIXME( "Waiting lobby not being handled correctly\n" );
  }

  EnterCriticalSection( &This->lock );

  ZeroMemory( &enumData, sizeof( enumData ) );
  enumData.This    = This;
  enumData.appGUID = lpConn->lpSessionDesc->guidApplication;

  /* Our callback function will fill up the enumData structure with all the information
     required to start a new process */
  IDirectPlayLobby_EnumLocalApplications( iface, RunApplicationA_EnumLocalApplications,
                                          (&enumData), 0 );

  /* First the application name */
  strcpy( temp, enumData.lpszPath );
  strcat( temp, "\\" );
  strcat( temp, enumData.lpszFileName );
  HeapFree( GetProcessHeap(), 0, enumData.lpszPath );
  HeapFree( GetProcessHeap(), 0, enumData.lpszFileName );
  if ((appName = HeapAlloc( GetProcessHeap(), 0, strlen(temp)+1 ))) strcpy( appName, temp );

  /* Now the command line */
  strcat( temp, " " );
  strcat( temp, enumData.lpszCommandLine );
  HeapFree( GetProcessHeap(), 0, enumData.lpszCommandLine );
  if ((enumData.lpszCommandLine = HeapAlloc( GetProcessHeap(), 0, strlen(temp)+1 )))
      strcpy( enumData.lpszCommandLine, temp );

  ZeroMemory( &startupInfo, sizeof( startupInfo ) );
  startupInfo.cb = sizeof( startupInfo );
  /* FIXME: Should any fields be filled in? */

  ZeroMemory( &newProcessInfo, sizeof( newProcessInfo ) );

  if( !CreateProcessA( appName,
                       enumData.lpszCommandLine,
                       NULL,
                       NULL,
                       FALSE,
                       CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_CONSOLE | CREATE_SUSPENDED, /* Creation Flags */
                       NULL,
                       enumData.lpszCurrentDirectory,
                       &startupInfo,
                       &newProcessInfo
                     )
    )
  {
    ERR( "Failed to create process for app %s\n", appName );

    HeapFree( GetProcessHeap(), 0, appName );
    HeapFree( GetProcessHeap(), 0, enumData.lpszCommandLine );
    HeapFree( GetProcessHeap(), 0, enumData.lpszCurrentDirectory );

    LeaveCriticalSection( &This->lock );
    return DPERR_CANTCREATEPROCESS;
  }

  HeapFree( GetProcessHeap(), 0, appName );
  HeapFree( GetProcessHeap(), 0, enumData.lpszCommandLine );
  HeapFree( GetProcessHeap(), 0, enumData.lpszCurrentDirectory );

  /* Reserve this global application id! */
  if( !DPLAYX_CreateLobbyApplication( newProcessInfo.dwProcessId ) )
  {
    ERR( "Unable to create global application data for 0x%08x\n",
           newProcessInfo.dwProcessId );
  }

  hr = IDirectPlayLobby_SetConnectionSettings( iface, 0, newProcessInfo.dwProcessId, lpConn );

  if( hr != DP_OK )
  {
    ERR( "SetConnectionSettings failure %s\n", DPLAYX_HresultToString( hr ) );
    LeaveCriticalSection( &This->lock );
    return hr;
  }

  /* Setup the handles for application notification */
  DPL_CreateAndSetLobbyHandles( newProcessInfo.dwProcessId,
                                newProcessInfo.hProcess,
                                &hStart, &hDeath, &hSettingRead );

  /* Setup the message thread ID */
  This->dpl->dwMsgThread =
    CreateLobbyMessageReceptionThread( hReceiveEvent, hStart, hDeath, hSettingRead );

  DPLAYX_SetLobbyMsgThreadId( newProcessInfo.dwProcessId, This->dpl->dwMsgThread );

  LeaveCriticalSection( &This->lock );

  /* Everything seems to have been set correctly, update the dwAppID */
  *lpdwAppID = newProcessInfo.dwProcessId;

  /* Unsuspend the process - should return the prev suspension count */
  if( ( dwSuspendCount = ResumeThread( newProcessInfo.hThread ) ) != 1 )
  {
    ERR( "ResumeThread failed with 0x%08x\n", dwSuspendCount );
  }

  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_RunApplication
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  LPDWORD lpdwAppID,
  LPDPLCONNECTION lpConn,
  HANDLE hReceiveEvent )
{
  IDirectPlayLobbyWImpl *This = (IDirectPlayLobbyWImpl *)iface;
  FIXME( "(%p)->(0x%08x,%p,%p,%p):stub\n", This, dwFlags, lpdwAppID, lpConn, hReceiveEvent );
  return DPERR_OUTOFMEMORY;
}

/********************************************************************
 *
 * Sends a message between the application and the lobby client.
 * All messages are queued until received.
 *
 */
static HRESULT WINAPI IDirectPlayLobby3AImpl_SendLobbyMessage( IDirectPlayLobby3A *iface,
        DWORD flags, DWORD appid, void *data, DWORD size )
{
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_SendLobbyMessage
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPVOID lpData,
  DWORD dwDataSize )
{
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
}

/********************************************************************
 *
 * Modifies the DPLCONNECTION structure to contain all information
 * needed to start and connect an application.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyWImpl_SetConnectionSettings
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  IDirectPlayLobbyWImpl *This = (IDirectPlayLobbyWImpl *)iface;
  HRESULT hr;

  TRACE("(%p)->(0x%08x,0x%08x,%p)\n", This, dwFlags, dwAppID, lpConn );

  EnterCriticalSection( &This->lock );

  hr = DPLAYX_SetConnectionSettingsW( dwFlags, dwAppID, lpConn );

  /* FIXME: Don't think that this is supposed to fail, but the documentation
            is somewhat sketchy. I'll try creating a lobby application
            for this... */
  if( hr == DPERR_NOTLOBBIED )
  {
    FIXME( "Unlobbied app setting connections. Is this correct behavior?\n" );
    if( dwAppID == 0 )
    {
       dwAppID = GetCurrentProcessId();
    }
    DPLAYX_CreateLobbyApplication( dwAppID );
    hr = DPLAYX_SetConnectionSettingsW( dwFlags, dwAppID, lpConn );
  }

  LeaveCriticalSection( &This->lock );

  return hr;
}

static HRESULT WINAPI IDirectPlayLobby3AImpl_SetConnectionSettings( IDirectPlayLobby3A *iface,
        DWORD dwFlags, DWORD dwAppID, DPLCONNECTION *lpConn )
{
  IDirectPlayLobbyImpl *This = impl_from_IDirectPlayLobby3A( iface );
  HRESULT hr;

  TRACE("(%p)->(0x%08x,0x%08x,%p)\n", This, dwFlags, dwAppID, lpConn );

  EnterCriticalSection( &This->lock );

  hr = DPLAYX_SetConnectionSettingsA( dwFlags, dwAppID, lpConn );

  /* FIXME: Don't think that this is supposed to fail, but the documentation
            is somewhat sketchy. I'll try creating a lobby application
            for this... */
  if( hr == DPERR_NOTLOBBIED )
  {
    FIXME( "Unlobbied app setting connections. Is this correct behavior?\n" );
    dwAppID = GetCurrentProcessId();
    DPLAYX_CreateLobbyApplication( dwAppID );
    hr = DPLAYX_SetConnectionSettingsA( dwFlags, dwAppID, lpConn );
  }

  LeaveCriticalSection( &This->lock );

  return hr;
}

/********************************************************************
 *
 * Registers an event that will be set when a lobby message is received.
 *
 */
static HRESULT WINAPI IDirectPlayLobby3AImpl_SetLobbyMessageEvent( IDirectPlayLobby3A *iface,
        DWORD flags, DWORD appid, HANDLE event )
{
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_SetLobbyMessageEvent
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  DWORD dwAppID,
  HANDLE hReceiveEvent )
{
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
}


/* DPL 2 methods */
static HRESULT WINAPI IDirectPlayLobby2WImpl_CreateCompoundAddress
( LPDIRECTPLAYLOBBY2 iface,
  LPCDPCOMPOUNDADDRESSELEMENT lpElements,
  DWORD dwElementCount,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize )
{
  return DPL_CreateCompoundAddress( lpElements, dwElementCount, lpAddress, lpdwAddressSize, FALSE );
}

static HRESULT WINAPI IDirectPlayLobby3AImpl_CreateCompoundAddress( IDirectPlayLobby3A *iface,
        const DPCOMPOUNDADDRESSELEMENT *lpElements, DWORD dwElementCount, void *lpAddress,
        DWORD *lpdwAddressSize )
{
  return DPL_CreateCompoundAddress( lpElements, dwElementCount, lpAddress, lpdwAddressSize, TRUE );
}

HRESULT DPL_CreateCompoundAddress
( LPCDPCOMPOUNDADDRESSELEMENT lpElements,
  DWORD dwElementCount,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize,
  BOOL bAnsiInterface )
{
  DWORD dwSizeRequired = 0;
  DWORD dwElements;
  LPCDPCOMPOUNDADDRESSELEMENT lpOrigElements = lpElements;

  TRACE("(%p,0x%08x,%p,%p)\n", lpElements, dwElementCount, lpAddress, lpdwAddressSize );

  /* Parameter check */
  if( ( lpElements == NULL ) ||
      ( dwElementCount == 0 )   /* FIXME: Not sure if this is a failure case */
    )
  {
    return DPERR_INVALIDPARAMS;
  }

  /* Add the total size chunk */
  dwSizeRequired += sizeof( DPADDRESS ) + sizeof( DWORD );

  /* Calculate the size of the buffer required */
  for ( dwElements = dwElementCount; dwElements > 0; --dwElements, ++lpElements )
  {
    if ( ( IsEqualGUID( &lpElements->guidDataType, &DPAID_ServiceProvider ) ) ||
         ( IsEqualGUID( &lpElements->guidDataType, &DPAID_LobbyProvider ) )
       )
    {
      dwSizeRequired += sizeof( DPADDRESS ) + sizeof( GUID );
    }
    else if ( ( IsEqualGUID( &lpElements->guidDataType, &DPAID_Phone ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_Modem ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_INet ) )
            )
    {
      if( !bAnsiInterface )
      {
        ERR( "Ansi GUIDs used for unicode interface\n" );
        return DPERR_INVALIDFLAGS;
      }

      dwSizeRequired += sizeof( DPADDRESS ) + lpElements->dwDataSize;
    }
    else if ( ( IsEqualGUID( &lpElements->guidDataType, &DPAID_PhoneW ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_ModemW ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_INetW ) )
            )
    {
      if( bAnsiInterface )
      {
        ERR( "Unicode GUIDs used for ansi interface\n" );
        return DPERR_INVALIDFLAGS;
      }

      FIXME( "Right size for unicode interface?\n" );
      dwSizeRequired += sizeof( DPADDRESS ) + lpElements->dwDataSize * sizeof( WCHAR );
    }
    else if ( IsEqualGUID( &lpElements->guidDataType, &DPAID_INetPort ) )
    {
      dwSizeRequired += sizeof( DPADDRESS ) + sizeof( WORD );
    }
    else if ( IsEqualGUID( &lpElements->guidDataType, &DPAID_ComPort ) )
    {
      FIXME( "Right size for unicode interface?\n" );
      dwSizeRequired += sizeof( DPADDRESS ) + sizeof( DPCOMPORTADDRESS ); /* FIXME: Right size? */
    }
    else
    {
      ERR( "Unknown GUID %s\n", debugstr_guid(&lpElements->guidDataType) );
      return DPERR_INVALIDFLAGS;
    }
  }

  /* The user wants to know how big a buffer to allocate for us */
  if( ( lpAddress == NULL ) ||
      ( *lpdwAddressSize < dwSizeRequired )
    )
  {
    *lpdwAddressSize = dwSizeRequired;
    return DPERR_BUFFERTOOSMALL;
  }

  /* Add the total size chunk */
  {
    LPDPADDRESS lpdpAddress = lpAddress;

    lpdpAddress->guidDataType = DPAID_TotalSize;
    lpdpAddress->dwDataSize = sizeof( DWORD );
    lpAddress = (char *) lpAddress + sizeof( DPADDRESS );

    *(LPDWORD)lpAddress = dwSizeRequired;
    lpAddress = (char *) lpAddress + sizeof( DWORD );
  }

  /* Calculate the size of the buffer required */
  for( dwElements = dwElementCount, lpElements = lpOrigElements;
       dwElements > 0;
       --dwElements, ++lpElements )
  {
    if ( ( IsEqualGUID( &lpElements->guidDataType, &DPAID_ServiceProvider ) ) ||
         ( IsEqualGUID( &lpElements->guidDataType, &DPAID_LobbyProvider ) )
       )
    {
      LPDPADDRESS lpdpAddress = lpAddress;

      lpdpAddress->guidDataType = lpElements->guidDataType;
      lpdpAddress->dwDataSize = sizeof( GUID );
      lpAddress = (char *) lpAddress + sizeof( DPADDRESS );

      CopyMemory( lpAddress, lpElements->lpData, sizeof( GUID ) );
      lpAddress = (char *) lpAddress + sizeof( GUID );
    }
    else if ( ( IsEqualGUID( &lpElements->guidDataType, &DPAID_Phone ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_Modem ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_INet ) )
            )
    {
      LPDPADDRESS lpdpAddress = lpAddress;

      lpdpAddress->guidDataType = lpElements->guidDataType;
      lpdpAddress->dwDataSize = lpElements->dwDataSize;
      lpAddress = (char *) lpAddress + sizeof( DPADDRESS );

      lstrcpynA( lpAddress, lpElements->lpData, lpElements->dwDataSize );
      lpAddress = (char *) lpAddress + lpElements->dwDataSize;
    }
    else if ( ( IsEqualGUID( &lpElements->guidDataType, &DPAID_PhoneW ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_ModemW ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_INetW ) )
            )
    {
      LPDPADDRESS lpdpAddress = lpAddress;

      lpdpAddress->guidDataType = lpElements->guidDataType;
      lpdpAddress->dwDataSize = lpElements->dwDataSize;
      lpAddress = (char *) lpAddress + sizeof( DPADDRESS );

      lstrcpynW( lpAddress, lpElements->lpData, lpElements->dwDataSize );
      lpAddress = (char *) lpAddress + lpElements->dwDataSize * sizeof( WCHAR );
    }
    else if ( IsEqualGUID( &lpElements->guidDataType, &DPAID_INetPort ) )
    {
      LPDPADDRESS lpdpAddress = lpAddress;

      lpdpAddress->guidDataType = lpElements->guidDataType;
      lpdpAddress->dwDataSize = lpElements->dwDataSize;
      lpAddress = (char *) lpAddress + sizeof( DPADDRESS );

      *((LPWORD)lpAddress) = *((LPWORD)lpElements->lpData);
      lpAddress = (char *) lpAddress + sizeof( WORD );
    }
    else if ( IsEqualGUID( &lpElements->guidDataType, &DPAID_ComPort ) )
    {
      LPDPADDRESS lpdpAddress = lpAddress;

      lpdpAddress->guidDataType = lpElements->guidDataType;
      lpdpAddress->dwDataSize = lpElements->dwDataSize;
      lpAddress = (char *) lpAddress + sizeof( DPADDRESS );

      CopyMemory( lpAddress, lpElements->lpData, sizeof( DPADDRESS ) );
      lpAddress = (char *) lpAddress + sizeof( DPADDRESS );
    }
  }

  return DP_OK;
}

/* DPL 3 methods */

static HRESULT WINAPI IDirectPlayLobby3WImpl_ConnectEx
( LPDIRECTPLAYLOBBY3 iface, DWORD dwFlags, REFIID riid,
  LPVOID* lplpDP, IUnknown* pUnk )
{
  IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)iface ;
  return DPL_ConnectEx( This, dwFlags, riid, lplpDP, pUnk );
}

static HRESULT WINAPI IDirectPlayLobby3AImpl_ConnectEx
( LPDIRECTPLAYLOBBY3A iface, DWORD dwFlags, REFIID riid,
  LPVOID* lplpDP, IUnknown* pUnk )
{
  IDirectPlayLobbyImpl *This = impl_from_IDirectPlayLobby3A( iface );
  return DPL_ConnectEx( This, dwFlags, riid, lplpDP, pUnk );
}

static HRESULT WINAPI IDirectPlayLobby3WImpl_RegisterApplication
( LPDIRECTPLAYLOBBY3 iface, DWORD dwFlags, LPDPAPPLICATIONDESC lpAppDesc )
{
  FIXME(":stub\n");
  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobby3AImpl_RegisterApplication
( LPDIRECTPLAYLOBBY3A iface, DWORD dwFlags, LPDPAPPLICATIONDESC lpAppDesc )
{
  FIXME(":stub\n");
  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobby3WImpl_UnregisterApplication
( LPDIRECTPLAYLOBBY3 iface, DWORD dwFlags, REFGUID lpAppDesc )
{
  FIXME(":stub\n");
  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobby3AImpl_UnregisterApplication
( LPDIRECTPLAYLOBBY3A iface, DWORD dwFlags, REFGUID lpAppDesc )
{
  FIXME(":stub\n");
  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobby3WImpl_WaitForConnectionSettings
( LPDIRECTPLAYLOBBY3 iface, DWORD dwFlags )
{
  HRESULT hr         = DP_OK;
  BOOL    bStartWait = !(dwFlags & DPLWAIT_CANCEL);

  TRACE( "(%p)->(0x%08x)\n", iface, dwFlags );

  if( DPLAYX_WaitForConnectionSettings( bStartWait ) )
  {
    /* FIXME: What is the correct error return code? */
    hr = DPERR_NOTLOBBIED;
  }

  return hr;
}

static HRESULT WINAPI IDirectPlayLobby3AImpl_WaitForConnectionSettings
( LPDIRECTPLAYLOBBY3A iface, DWORD dwFlags )
{
  HRESULT hr         = DP_OK;
  BOOL    bStartWait = !(dwFlags & DPLWAIT_CANCEL);

  TRACE( "(%p)->(0x%08x)\n", iface, dwFlags );

  if( DPLAYX_WaitForConnectionSettings( bStartWait ) )
  {
    /* FIXME: What is the correct error return code? */
    hr = DPERR_NOTLOBBIED;
  }

  return hr;
}


static const IDirectPlayLobby3Vtbl dpl3A_vt =
{
    IDirectPlayLobby3AImpl_QueryInterface,
    IDirectPlayLobby3AImpl_AddRef,
    IDirectPlayLobby3AImpl_Release,
    IDirectPlayLobby3AImpl_Connect,
    IDirectPlayLobby3AImpl_CreateAddress,
    IDirectPlayLobby3AImpl_EnumAddress,
    IDirectPlayLobby3AImpl_EnumAddressTypes,
    IDirectPlayLobby3AImpl_EnumLocalApplications,
    IDirectPlayLobby3AImpl_GetConnectionSettings,
    IDirectPlayLobby3AImpl_ReceiveLobbyMessage,
    IDirectPlayLobby3AImpl_RunApplication,
    IDirectPlayLobby3AImpl_SendLobbyMessage,
    IDirectPlayLobby3AImpl_SetConnectionSettings,
    IDirectPlayLobby3AImpl_SetLobbyMessageEvent,
    IDirectPlayLobby3AImpl_CreateCompoundAddress,
    IDirectPlayLobby3AImpl_ConnectEx,
    IDirectPlayLobby3AImpl_RegisterApplication,
    IDirectPlayLobby3AImpl_UnregisterApplication,
    IDirectPlayLobby3AImpl_WaitForConnectionSettings
};

/* Direct Play Lobby 3 (unicode) Virtual Table for methods */

/* Note: Hack so we can reuse the old functions without compiler warnings */
# define XCAST(fun)     (void*)

static const IDirectPlayLobby3Vtbl dpl3_vt =
{
    IDirectPlayLobby3Impl_QueryInterface,
    IDirectPlayLobby3Impl_AddRef,
    IDirectPlayLobby3Impl_Release,
    IDirectPlayLobby3Impl_Connect,
  XCAST(CreateAddress)IDirectPlayLobbyWImpl_CreateAddress,
  XCAST(EnumAddress)IDirectPlayLobbyWImpl_EnumAddress,
  XCAST(EnumAddressTypes)IDirectPlayLobbyWImpl_EnumAddressTypes,
  XCAST(EnumLocalApplications)IDirectPlayLobbyWImpl_EnumLocalApplications,
  XCAST(GetConnectionSettings)IDirectPlayLobbyWImpl_GetConnectionSettings,
  XCAST(ReceiveLobbyMessage)IDirectPlayLobbyWImpl_ReceiveLobbyMessage,
  XCAST(RunApplication)IDirectPlayLobbyWImpl_RunApplication,
  XCAST(SendLobbyMessage)IDirectPlayLobbyWImpl_SendLobbyMessage,
  XCAST(SetConnectionSettings)IDirectPlayLobbyWImpl_SetConnectionSettings,
  XCAST(SetLobbyMessageEvent)IDirectPlayLobbyWImpl_SetLobbyMessageEvent,

  XCAST(CreateCompoundAddress)IDirectPlayLobby2WImpl_CreateCompoundAddress,

  IDirectPlayLobby3WImpl_ConnectEx,
  IDirectPlayLobby3WImpl_RegisterApplication,
  IDirectPlayLobby3WImpl_UnregisterApplication,
  IDirectPlayLobby3WImpl_WaitForConnectionSettings
};
#undef XCAST

HRESULT dplobby_create( REFIID riid, void **ppv )
{
    IDirectPlayLobbyImpl *obj;
    HRESULT hr;

    TRACE( "(%s, %p)\n", debugstr_guid( riid ), ppv );

    *ppv = NULL;
    obj = HeapAlloc( GetProcessHeap(), 0, sizeof( *obj ) );
    if ( !obj )
        return DPERR_OUTOFMEMORY;

    obj->IDirectPlayLobby3_iface.lpVtbl = &dpl3_vt;
    obj->IDirectPlayLobby3A_iface.lpVtbl = &dpl3A_vt;
    obj->ulInterfaceRef = 1;

    InitializeCriticalSection( &obj->lock );
    obj->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": IDirectPlayLobbyImpl.lock");

    if ( DPL_CreateLobby1( obj ) )
        hr = IDirectPlayLobby_QueryInterface( &obj->IDirectPlayLobby3_iface, riid, ppv );
    else
        hr = DPERR_NOMEMORY;
    IDirectPlayLobby_Release( &obj->IDirectPlayLobby3_iface );

    return hr;
}



/***************************************************************************
 *  DirectPlayLobbyCreateA   (DPLAYX.4)
 *
 */
HRESULT WINAPI DirectPlayLobbyCreateA( LPGUID lpGUIDDSP,
                                       LPDIRECTPLAYLOBBYA *lplpDPL,
                                       IUnknown *lpUnk,
                                       LPVOID lpData,
                                       DWORD dwDataSize )
{
  TRACE("lpGUIDDSP=%p lplpDPL=%p lpUnk=%p lpData=%p dwDataSize=%08x\n",
        lpGUIDDSP,lplpDPL,lpUnk,lpData,dwDataSize);

  /* Parameter Check: lpGUIDSP, lpUnk & lpData must be NULL. dwDataSize must
   * equal 0. These fields are mostly for future expansion.
   */
  if ( lpGUIDDSP || lpData || dwDataSize )
  {
     *lplpDPL = NULL;
     return DPERR_INVALIDPARAMS;
  }

  if( lpUnk )
  {
     *lplpDPL = NULL;
     ERR("Bad parameters!\n" );
     return CLASS_E_NOAGGREGATION;
  }

  return dplobby_create( &IID_IDirectPlayLobbyA, (void**)lplpDPL );
}

/***************************************************************************
 *  DirectPlayLobbyCreateW   (DPLAYX.5)
 *
 */
HRESULT WINAPI DirectPlayLobbyCreateW( LPGUID lpGUIDDSP,
                                       LPDIRECTPLAYLOBBY *lplpDPL,
                                       IUnknown *lpUnk,
                                       LPVOID lpData,
                                       DWORD dwDataSize )
{
  TRACE("lpGUIDDSP=%p lplpDPL=%p lpUnk=%p lpData=%p dwDataSize=%08x\n",
        lpGUIDDSP,lplpDPL,lpUnk,lpData,dwDataSize);

  /* Parameter Check: lpGUIDSP, lpUnk & lpData must be NULL. dwDataSize must
   * equal 0. These fields are mostly for future expansion.
   */
  if ( lpGUIDDSP || lpData || dwDataSize )
  {
     *lplpDPL = NULL;
     ERR("Bad parameters!\n" );
     return DPERR_INVALIDPARAMS;
  }

  if( lpUnk )
  {
     *lplpDPL = NULL;
     ERR("Bad parameters!\n" );
     return CLASS_E_NOAGGREGATION;
  }

  return dplobby_create( &IID_IDirectPlayLobby, (void**)lplpDPL );
}
