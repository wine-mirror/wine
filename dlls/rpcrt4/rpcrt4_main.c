/*
 *  RPCRT4
 *
 * Copyright 2000 Huw D M Davies for Codeweavers
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "windef.h"
#include "wine/windef16.h"
#include "winerror.h"
#include "winbase.h"
#include "wine/unicode.h"
#include "rpc.h"

#include "ole2.h"
#include "rpcndr.h"
#include "rpcproxy.h"

#ifdef HAVE_SYS_FILE_H
# include <sys/file.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
# include <sys/sockio.h>
#endif
#ifdef HAVE_NET_IF_H
# include <net/if.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static UUID uuid_nil;

/***********************************************************************
 * RPCRT4_LibMain
 *
 * PARAMS
 *     hinstDLL    [I] handle to the DLL's instance
 *     fdwReason   [I]
 *     lpvReserved [I] reserved, must be NULL
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
RPCRT4_LibMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
	break;

    case DLL_PROCESS_DETACH:
	break;
    }

    return TRUE;
}

/*************************************************************************
 * UuidEqual [RPCRT4.@]
 *
 * PARAMS
 *     UUID *Uuid1        [I] Uuid to compare
 *     UUID *Uuid2        [I] Uuid to compare
 *     RPC_STATUS *Status [O] returns RPC_S_OK
 *
 * RETURNS
 *     TRUE/FALSE
 */
int WINAPI UuidEqual(UUID *uuid1, UUID *uuid2, RPC_STATUS *Status)
{
  TRACE("(%s,%s)\n", debugstr_guid(uuid1), debugstr_guid(uuid2));
  *Status = RPC_S_OK;
  if (!uuid1) uuid1 = &uuid_nil;
  if (!uuid2) uuid2 = &uuid_nil;
  if (uuid1 == uuid2) return TRUE;
  return !memcmp(uuid1, uuid2, sizeof(UUID));
}

/*************************************************************************
 * UuidIsNil [RPCRT4.@]
 *
 * PARAMS
 *     UUID *Uuid         [I] Uuid to compare
 *     RPC_STATUS *Status [O] retuns RPC_S_OK
 *
 * RETURNS
 *     TRUE/FALSE
 */
int WINAPI UuidIsNil(UUID *uuid, RPC_STATUS *Status)
{
  TRACE("(%s)\n", debugstr_guid(uuid));
  *Status = RPC_S_OK;
  if (!uuid) return TRUE;
  return !memcmp(uuid, &uuid_nil, sizeof(UUID));
}

 /*************************************************************************
 * UuidCreateNil [RPCRT4.@]
 *
 * PARAMS
 *     UUID *Uuid [O] returns a nil UUID
 *
 * RETURNS
 *     RPC_S_OK
 */
RPC_STATUS WINAPI UuidCreateNil(UUID *Uuid)
{
  *Uuid = uuid_nil;
  return RPC_S_OK;
}

/*************************************************************************
 *           UuidCreate   [RPCRT4.@]
 *
 * Creates a 128bit UUID.
 * Implemented according the DCE specification for UUID generation.
 * Code is based upon uuid library in e2fsprogs by Theodore Ts'o.
 * Copyright (C) 1996, 1997 Theodore Ts'o.
 *
 * RETURNS
 *
 *  S_OK if successful.
 */
RPC_STATUS WINAPI UuidCreate(UUID *Uuid)
{
   static char has_init = 0;
   static unsigned char a[6];
   static int                      adjustment = 0;
   static struct timeval           last = {0, 0};
   static WORD                     clock_seq;
   struct timeval                  tv;
   unsigned long long              clock_reg;
   DWORD clock_high, clock_low;
   WORD temp_clock_seq, temp_clock_mid, temp_clock_hi_and_version;
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
#ifdef HAVE_SOCKADDR_SA_LEN
#  ifndef max
#   define max(a,b) ((a) > (b) ? (a) : (b))
#  endif
#  define ifreq_size(i) max(sizeof(struct ifreq),\
sizeof((i).ifr_name)+(i).ifr_addr.sa_len)
# else
#  define ifreq_size(i) sizeof(struct ifreq)
# endif /* defined(HAVE_SOCKADDR_SA_LEN) */

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
	    for (i = 0; i < n; i+= ifreq_size(ifr) ) {
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
   temp_clock_mid = (WORD)clock_high;
   temp_clock_hi_and_version = (clock_high >> 16) | 0x1000;

   /* pack the information into the GUID structure */

   ((unsigned char*)&Uuid->Data1)[3] = (unsigned char)clock_low;
   clock_low >>= 8;
   ((unsigned char*)&Uuid->Data1)[2] = (unsigned char)clock_low;
   clock_low >>= 8;
   ((unsigned char*)&Uuid->Data1)[1] = (unsigned char)clock_low;
   clock_low >>= 8;
   ((unsigned char*)&Uuid->Data1)[0] = (unsigned char)clock_low;

   ((unsigned char*)&Uuid->Data2)[1] = (unsigned char)temp_clock_mid;
   temp_clock_mid >>= 8;
   ((unsigned char*)&Uuid->Data2)[0] = (unsigned char)temp_clock_mid;

   ((unsigned char*)&Uuid->Data3)[1] = (unsigned char)temp_clock_hi_and_version;
   temp_clock_hi_and_version >>= 8;
   ((unsigned char*)&Uuid->Data3)[0] = (unsigned char)temp_clock_hi_and_version;

   ((unsigned char*)Uuid->Data4)[1] = (unsigned char)temp_clock_seq;
   temp_clock_seq >>= 8;
   ((unsigned char*)Uuid->Data4)[0] = (unsigned char)temp_clock_seq;

   ((unsigned char*)Uuid->Data4)[2] = a[0];
   ((unsigned char*)Uuid->Data4)[3] = a[1];
   ((unsigned char*)Uuid->Data4)[4] = a[2];
   ((unsigned char*)Uuid->Data4)[5] = a[3];
   ((unsigned char*)Uuid->Data4)[6] = a[4];
   ((unsigned char*)Uuid->Data4)[7] = a[5];

   TRACE("%s\n", debugstr_guid(Uuid));

   return RPC_S_OK;
}


/*************************************************************************
 *           UuidCreateSequential   [RPCRT4.@]
 *
 * Creates a 128bit UUID by calling UuidCreate.
 * New API in Win 2000
 */

RPC_STATUS WINAPI UuidCreateSequential(UUID *Uuid)
{
   return UuidCreate (Uuid);
}


/*************************************************************************
 *           RpcStringFreeA   [RPCRT4.@]
 *
 * Frees a character string allocated by the RPC run-time library.
 *
 * RETURNS
 *
 *  S_OK if successful.
 */
RPC_STATUS WINAPI RpcStringFreeA(LPSTR* String)
{
  HeapFree( GetProcessHeap(), 0, *String);

  return RPC_S_OK;
}


/*************************************************************************
 *           UuidHash   [RPCRT4.@]
 *
 * Generates a hash value for a given UUID
 *
 */
unsigned short WINAPI UuidHash(UUID *uuid, RPC_STATUS *Status)
{
  FIXME("stub:\n");
  *Status = RPC_S_OK;
  return 0xabcd;
}

/*************************************************************************
 *           UuidToStringA   [RPCRT4.@]
 *
 * Converts a UUID to a string.
 *
 * UUID format is 8 hex digits, followed by a hyphen then three groups of
 * 4 hex digits each followed by a hyphen and then 12 hex digits
 *
 * RETURNS
 *
 *  S_OK if successful.
 *  S_OUT_OF_MEMORY if unsucessful.
 */
RPC_STATUS WINAPI UuidToStringA(UUID *Uuid, LPSTR* StringUuid)
{
  *StringUuid = HeapAlloc( GetProcessHeap(), 0, sizeof(char) * 37);

  if(!(*StringUuid))
    return RPC_S_OUT_OF_MEMORY;

  sprintf(*StringUuid, "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                 Uuid->Data1, Uuid->Data2, Uuid->Data3,
                 Uuid->Data4[0], Uuid->Data4[1], Uuid->Data4[2],
                 Uuid->Data4[3], Uuid->Data4[4], Uuid->Data4[5],
                 Uuid->Data4[6], Uuid->Data4[7] );

  return RPC_S_OK;
}

/***********************************************************************
 *		UuidFromStringA (RPCRT4.@)
 */
RPC_STATUS WINAPI UuidFromStringA(LPSTR str, UUID *uuid)
{
    FIXME("%s %p\n",debugstr_a(str),uuid);
    return RPC_S_INVALID_STRING_UUID;
}

/***********************************************************************
 *		UuidFromStringW (RPCRT4.@)
 */
RPC_STATUS WINAPI UuidFromStringW(LPWSTR str, UUID *uuid)
{
    FIXME("%s %p\n",debugstr_w(str),uuid);
    return RPC_S_INVALID_STRING_UUID;
}

/***********************************************************************
 *		NdrDllRegisterProxy (RPCRT4.@)
 */
HRESULT WINAPI NdrDllRegisterProxy(
  HMODULE hDll,          /* [in] */
  const ProxyFileInfo **pProxyFileList, /* [in] */
  const CLSID *pclsid    /* [in] */
)
{
  FIXME("(%x,%p,%s), stub!\n",hDll,pProxyFileList,debugstr_guid(pclsid));
  return S_OK;
}

/***********************************************************************
 *		RpcServerUseProtseqEpA (RPCRT4.@)
 */

RPC_STATUS WINAPI RpcServerUseProtseqEpA( LPSTR Protseq, UINT MaxCalls, LPSTR Endpoint, LPVOID SecurityDescriptor )
{
  RPC_POLICY policy;

  TRACE( "(%s,%u,%s,%p)\n", Protseq, MaxCalls, Endpoint, SecurityDescriptor );

  /* This should provide the default behaviour */
  policy.Length        = sizeof( policy );
  policy.EndpointFlags = 0;
  policy.NICFlags      = 0;

  return RpcServerUseProtseqEpExA( Protseq, MaxCalls, Endpoint, SecurityDescriptor, &policy );
}

/***********************************************************************
 *		RpcServerUseProtseqEpW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUseProtseqEpW( LPWSTR Protseq, UINT MaxCalls, LPWSTR Endpoint, LPVOID SecurityDescriptor )
{
  RPC_POLICY policy;

  TRACE( "(%s,%u,%s,%p)\n", debugstr_w( Protseq ), MaxCalls, debugstr_w( Endpoint ), SecurityDescriptor );

  /* This should provide the default behaviour */
  policy.Length        = sizeof( policy );
  policy.EndpointFlags = 0;
  policy.NICFlags      = 0;

  return RpcServerUseProtseqEpExW( Protseq, MaxCalls, Endpoint, SecurityDescriptor, &policy );
}

/***********************************************************************
 *		RpcServerUseProtseqEpExA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUseProtseqEpExA( LPSTR Protseq, UINT MaxCalls, LPSTR Endpoint, LPVOID SecurityDescriptor,
                          PRPC_POLICY lpPolicy )
{
  FIXME( "(%s,%u,%s,%p,{%u,%lu,%lu}): stub\n", Protseq, MaxCalls, Endpoint, SecurityDescriptor,
                                               lpPolicy->Length, lpPolicy->EndpointFlags, lpPolicy->NICFlags );

  return RPC_S_PROTSEQ_NOT_SUPPORTED; /* We don't support anything at this point */
}

/***********************************************************************
 *		RpcServerUseProtseqEpExW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUseProtseqEpExW( LPWSTR Protseq, UINT MaxCalls, LPWSTR Endpoint, LPVOID SecurityDescriptor,
                          PRPC_POLICY lpPolicy )
{
  FIXME( "(%s,%u,%s,%p,{%u,%lu,%lu}): stub\n", debugstr_w( Protseq ), MaxCalls, debugstr_w( Endpoint ),
                                               SecurityDescriptor,
                                               lpPolicy->Length, lpPolicy->EndpointFlags, lpPolicy->NICFlags );

  return RPC_S_PROTSEQ_NOT_SUPPORTED; /* We don't support anything at this point */
}

/***********************************************************************
 *		RpcServerRegisterIf (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerRegisterIf( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, RPC_MGR_EPV* MgrEpv )
{
  /* FIXME: Dump UUID using UuidToStringA */
  TRACE( "(%p,%p,%p)\n", IfSpec, MgrTypeUuid, MgrEpv );

  return RpcServerRegisterIf2( IfSpec, MgrTypeUuid, MgrEpv, 0, RPC_C_LISTEN_MAX_CALLS_DEFAULT, (UINT)-1, NULL );
}

/***********************************************************************
 *		RpcServerRegisterIfEx (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerRegisterIfEx( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, RPC_MGR_EPV* MgrEpv,
                       UINT Flags, UINT MaxCalls, RPC_IF_CALLBACK_FN* IfCallbackFn )
{
  /* FIXME: Dump UUID using UuidToStringA */
  TRACE( "(%p,%p,%p,%u,%u,%p)\n", IfSpec, MgrTypeUuid, MgrEpv, Flags, MaxCalls, IfCallbackFn );

  return RpcServerRegisterIf2( IfSpec, MgrTypeUuid, MgrEpv, Flags, MaxCalls, (UINT)-1, IfCallbackFn );
}

/***********************************************************************
 *		RpcServerRegisterIf2 (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerRegisterIf2( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, RPC_MGR_EPV* MgrEpv,
                      UINT Flags, UINT MaxCalls, UINT MaxRpcSize, RPC_IF_CALLBACK_FN* IfCallbackFn )
{
  /* FIXME: Dump UUID using UuidToStringA */
  FIXME( "(%p,%p,%p,%u,%u,%u,%p): stub\n", IfSpec, MgrTypeUuid, MgrEpv, Flags, MaxCalls, MaxRpcSize, IfCallbackFn );

  return RPC_S_UNKNOWN_IF; /* I guess this return code is as good as any failure */
}


/***********************************************************************
 *		RpcServerRegisterAuthInfoA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerRegisterAuthInfoA( LPSTR ServerPrincName, ULONG AuthnSvc, RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn,
                            LPVOID Arg )
{
  FIXME( "(%s,%lu,%p,%p): stub\n", ServerPrincName, AuthnSvc, GetKeyFn, Arg );

  return RPC_S_UNKNOWN_AUTHN_SERVICE; /* We don't know any authentication services */
}

/***********************************************************************
 *		RpcServerRegisterAuthInfoW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerRegisterAuthInfoW( LPWSTR ServerPrincName, ULONG AuthnSvc, RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn,
                            LPVOID Arg )
{
  FIXME( "(%s,%lu,%p,%p): stub\n", debugstr_w( ServerPrincName ), AuthnSvc, GetKeyFn, Arg );

  return RPC_S_UNKNOWN_AUTHN_SERVICE; /* We don't know any authentication services */
}

/***********************************************************************
 *		RpcServerListen (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerListen( UINT MinimumCallThreads, UINT MaxCalls, UINT DontWait )
{
  FIXME( "(%u,%u,%u): stub\n", MinimumCallThreads, MaxCalls, DontWait );

  return RPC_S_NO_PROTSEQS_REGISTERED; /* Since we don't allow registration this seems reasonable */
}

/***********************************************************************
 *		RpcStringBindingComposeA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcStringBindingComposeA( LPSTR ObjUuid, LPSTR Protseq, LPSTR NetworkAddr, LPSTR Endpoint,
                          LPSTR Options, LPSTR* StringBinding )
{
  FIXME( "(%s,%s,%s,%s,%s,%p): stub\n", ObjUuid, Protseq, NetworkAddr, Endpoint, Options, StringBinding );
  *StringBinding = NULL;

  return RPC_S_INVALID_STRING_UUID; /* Failure */
}

/***********************************************************************
 *		RpcStringBindingComposeW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcStringBindingComposeW( LPWSTR ObjUuid, LPWSTR Protseq, LPWSTR NetworkAddr, LPWSTR Endpoint,
                          LPWSTR Options, LPWSTR* StringBinding )
{
  FIXME( "(%s,%s,%s,%s,%s,%p): stub\n", debugstr_w( ObjUuid ), debugstr_w( Protseq ), debugstr_w( NetworkAddr ),
                                        debugstr_w( Endpoint ), debugstr_w( Options ), StringBinding );
  *StringBinding = NULL;

  return RPC_S_INVALID_STRING_UUID; /* Failure */
}

/***********************************************************************
 *		RpcBindingFree (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcBindingFree(RPC_BINDING_HANDLE* Binding)
{
  FIXME("(%p): stub\n", Binding);
  return RPC_S_OK;
}
/***********************************************************************
 *		RpcBindingFromStringBindingA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcBindingFromStringBindingA( LPSTR StringBinding, RPC_BINDING_HANDLE* Binding )
{
  FIXME( "(%s,%p): stub\n", StringBinding, Binding );

  return RPC_S_INVALID_STRING_BINDING; /* As good as any failure code */
}

/***********************************************************************
 *		RpcBindingFromStringBindingW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcBindingFromStringBindingW( LPWSTR StringBinding, RPC_BINDING_HANDLE* Binding )
{
  FIXME( "(%s,%p): stub\n", debugstr_w( StringBinding ), Binding );

  return RPC_S_INVALID_STRING_BINDING; /* As good as any failure code */
}

/***********************************************************************
 *		NdrDllCanUnloadNow (RPCRT4.@)
 */
HRESULT WINAPI NdrDllCanUnloadNow(CStdPSFactoryBuffer *pPSFactoryBuffer)
{
    FIXME("%p\n",pPSFactoryBuffer);
    return FALSE;
}

/***********************************************************************
 *		NdrDllGetClassObject (RPCRT4.@)
 */
HRESULT WINAPI NdrDllGetClassObject(
    REFCLSID rclsid, REFIID riid , LPVOID *ppv,
    const ProxyFileInfo **   pProxyFileList,
    const CLSID *            pclsid,
    CStdPSFactoryBuffer *    pPSFactoryBuffer)
{
    if(ppv)
        *ppv = NULL;
    return RPC_S_UNKNOWN_IF;
}

/***********************************************************************
 *              DllRegisterServer (RPCRT4.@)
 */

HRESULT WINAPI RPCRT4_DllRegisterServer( void )
{
        FIXME( "(): stub\n" );
        return S_OK;
}
