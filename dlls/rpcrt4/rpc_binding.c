/*
 * RPC binding API
 *
 * Copyright 2001 Ove Kåven, TransGaming Technologies
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
 *
 * TODO:
 *  - a whole lot
 */

#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winerror.h"
#include "winreg.h"
#include "wine/unicode.h"

#include "rpc.h"

#include "wine/debug.h"

#include "rpc_binding.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

LPSTR RPCRT4_strndupA(LPSTR src, INT slen)
{
  DWORD len;
  LPSTR s;
  if (!src) return NULL;
  if (slen == -1) slen = strlen(src);
  len = slen;
  s = HeapAlloc(GetProcessHeap(), 0, len+1);
  memcpy(s, src, len);
  s[len] = 0;
  return s;
}

LPSTR RPCRT4_strdupWtoA(LPWSTR src)
{
  DWORD len;
  LPSTR s;
  if (!src) return NULL;
  len = WideCharToMultiByte(CP_ACP, 0, src, -1, NULL, 0, NULL, NULL);
  s = HeapAlloc(GetProcessHeap(), 0, len);
  WideCharToMultiByte(CP_ACP, 0, src, -1, s, len, NULL, NULL);
  return s;
}

LPWSTR RPCRT4_strdupAtoW(LPSTR src)
{
  DWORD len;
  LPWSTR s;
  if (!src) return NULL;
  len = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
  s = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
  MultiByteToWideChar(CP_ACP, 0, src, -1, s, len);
  return s;
}

LPWSTR RPCRT4_strndupW(LPWSTR src, INT slen)
{
  DWORD len;
  LPWSTR s;
  if (!src) return NULL;
  if (slen == -1) slen = strlenW(src);
  len = slen;
  s = HeapAlloc(GetProcessHeap(), 0, (len+1)*sizeof(WCHAR));
  memcpy(s, src, len*sizeof(WCHAR));
  s[len] = 0;
  return s;
}

void RPCRT4_strfree(LPSTR src)
{
  if (src) HeapFree(GetProcessHeap(), 0, src);
}

RPC_STATUS RPCRT4_CreateBindingA(RpcBinding** Binding, BOOL server, LPSTR Protseq)
{
  RpcBinding* NewBinding;

  NewBinding = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RpcBinding));
  NewBinding->refs = 1;
  NewBinding->server = server;
  NewBinding->Protseq = RPCRT4_strdupA(Protseq);

  TRACE("binding: %p\n", NewBinding);
  *Binding = NewBinding;

  return RPC_S_OK;
}

RPC_STATUS RPCRT4_CreateBindingW(RpcBinding** Binding, BOOL server, LPWSTR Protseq)
{
  RpcBinding* NewBinding;
  if (Binding)
    TRACE("  (*Binding == ^%p, server == %s, Protseq == \"%s\")\n", *Binding, server ? "Yes" : "No", debugstr_w(Protseq));
  else {
    ERR("!RpcBinding?\n"); 
    *((char *)0) = 0; /* we will crash below anyhow... */
  }

  NewBinding = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RpcBinding));
  NewBinding->refs = 1;
  NewBinding->server = server;
  NewBinding->Protseq = RPCRT4_strdupWtoA(Protseq);

  TRACE("binding: %p\n", NewBinding);
  *Binding = NewBinding;

  return RPC_S_OK;
}

RPC_STATUS RPCRT4_CompleteBindingA(RpcBinding* Binding, LPSTR NetworkAddr,  LPSTR Endpoint,  LPSTR NetworkOptions)
{
  
  TRACE("  (RpcBinding == ^%p, NetworkAddr == \"%s\", EndPoint == \"%s\", NetworkOptions == \"%s\")\n", Binding, NetworkAddr, Endpoint, NetworkOptions);

  RPCRT4_strfree(Binding->NetworkAddr);
  Binding->NetworkAddr = RPCRT4_strdupA(NetworkAddr);
  RPCRT4_strfree(Binding->Endpoint);
  Binding->Endpoint = RPCRT4_strdupA(Endpoint);

  return RPC_S_OK;
}

RPC_STATUS RPCRT4_CompleteBindingW(RpcBinding* Binding, LPWSTR NetworkAddr, LPWSTR Endpoint, LPWSTR NetworkOptions)
{

  TRACE("  (RpcBinding == ^%p, NetworkAddr == \"%s\", EndPoint == \"%s\", NetworkOptions == \"%s\")\n", Binding, 
   debugstr_w(NetworkAddr), debugstr_w(Endpoint), debugstr_w(NetworkOptions));

  RPCRT4_strfree(Binding->NetworkAddr);
  Binding->NetworkAddr = RPCRT4_strdupWtoA(NetworkAddr);
  RPCRT4_strfree(Binding->Endpoint);
  Binding->Endpoint = RPCRT4_strdupWtoA(Endpoint);

  return RPC_S_OK;
}

RPC_STATUS RPCRT4_ResolveBinding(RpcBinding* Binding, LPSTR Endpoint)
{
  RPCRT4_strfree(Binding->Endpoint);
  Binding->Endpoint = RPCRT4_strdupA(Endpoint);

  return RPC_S_OK;
}

RPC_STATUS RPCRT4_SetBindingObject(RpcBinding* Binding, UUID* ObjectUuid)
{
  TRACE("  (*RpcBinding == ^%p, UUID == %s)\n", Binding, debugstr_guid(ObjectUuid)); 
  if (ObjectUuid) memcpy(&Binding->ObjectUuid, ObjectUuid, sizeof(UUID));
  else UuidCreateNil(&Binding->ObjectUuid);
  return RPC_S_OK;
}

RPC_STATUS RPCRT4_SpawnBinding(RpcBinding** Binding, RpcBinding* OldBinding)
{
  RpcBinding* NewBinding;
  if (Binding)
    TRACE("  (*RpcBinding == ^%p, OldBinding == ^%p)\n", *Binding, OldBinding);
  else {
    ERR("!RpcBinding?"); 
    /* we will crash below anyhow... */
    *((char *)0) = 0;
  }

  NewBinding = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RpcBinding));
  NewBinding->refs = 1;
  NewBinding->server = OldBinding->server;
  NewBinding->Protseq = RPCRT4_strdupA(OldBinding->Protseq);
  NewBinding->NetworkAddr = RPCRT4_strdupA(OldBinding->NetworkAddr);
  NewBinding->Endpoint = RPCRT4_strdupA(OldBinding->Endpoint);
  /* because of the way named pipes work, we'll transfer the connected pipe
   * to the child, then reopen the server binding to continue listening */
  NewBinding->conn = OldBinding->conn;
  NewBinding->ovl = OldBinding->ovl;
  OldBinding->conn = 0;
  memset(&OldBinding->ovl, 0, sizeof(OldBinding->ovl));
  *Binding = NewBinding;
  RPCRT4_OpenBinding(OldBinding);

  return RPC_S_OK;
}

RPC_STATUS RPCRT4_ExportBinding(RpcBinding** Binding, RpcBinding* OldBinding)
{
  InterlockedIncrement(&OldBinding->refs);
  *Binding = OldBinding;
  return RPC_S_OK;
}

RPC_STATUS RPCRT4_DestroyBinding(RpcBinding* Binding)
{
  if (InterlockedDecrement(&Binding->refs))
    return RPC_S_OK;

  TRACE("binding: %p\n", Binding);
  RPCRT4_CloseBinding(Binding);
  RPCRT4_strfree(Binding->Endpoint);
  RPCRT4_strfree(Binding->NetworkAddr);
  RPCRT4_strfree(Binding->Protseq);
  HeapFree(GetProcessHeap(), 0, Binding);
  return RPC_S_OK;
}

RPC_STATUS RPCRT4_OpenBinding(RpcBinding* Binding)
{
  TRACE("  (Binding == ^%p)\n", Binding);
  if (!Binding->conn) {
    if (Binding->server) { /* server */
      /* protseq=ncalrpc: supposed to use NT LPC ports,
       * but we'll implement it with named pipes for now */
      if (strcmp(Binding->Protseq, "ncalrpc") == 0) {
        static LPSTR prefix = "\\\\.\\pipe\\lrpc\\";
        LPSTR pname;
        pname = HeapAlloc(GetProcessHeap(), 0, strlen(prefix) + strlen(Binding->Endpoint) + 1);
        strcat(strcpy(pname, prefix), Binding->Endpoint);
       TRACE("listening on %s\n", pname);
        Binding->conn = CreateNamedPipeA(pname, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                         0, PIPE_UNLIMITED_INSTANCES, 0, 0, 5000, NULL);
        HeapFree(GetProcessHeap(), 0, pname);
        memset(&Binding->ovl, 0, sizeof(Binding->ovl));
        Binding->ovl.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
        if (!ConnectNamedPipe(Binding->conn, &Binding->ovl)) {
          DWORD err = GetLastError();
          if (err == ERROR_PIPE_CONNECTED) {
            SetEvent(Binding->ovl.hEvent);
            return RPC_S_OK;
          }
          return err;
        }
      }
      /* protseq=ncacn_np: named pipes */
      else if (strcmp(Binding->Protseq, "ncacn_np") == 0) {
        static LPSTR prefix = "\\\\.";
        LPSTR pname;
        pname = HeapAlloc(GetProcessHeap(), 0, strlen(prefix) + strlen(Binding->Endpoint) + 1);
        strcat(strcpy(pname, prefix), Binding->Endpoint);
       TRACE("listening on %s\n", pname);
        Binding->conn = CreateNamedPipeA(pname, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                         0, PIPE_UNLIMITED_INSTANCES, 0, 0, 5000, NULL);
        HeapFree(GetProcessHeap(), 0, pname);
        memset(&Binding->ovl, 0, sizeof(Binding->ovl));
        Binding->ovl.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
        if (!ConnectNamedPipe(Binding->conn, &Binding->ovl)) {
          DWORD err = GetLastError();
          if (err == ERROR_PIPE_CONNECTED) {
            SetEvent(Binding->ovl.hEvent);
            return RPC_S_OK;
          }
          return err;
        }
      }
      else {
       ERR("protseq %s not supported\n", Binding->Protseq);
       return RPC_S_PROTSEQ_NOT_SUPPORTED;
      }
    }
    else { /* client */
      /* protseq=ncalrpc: supposed to use NT LPC ports,
       * but we'll implement it with named pipes for now */
      if (strcmp(Binding->Protseq, "ncalrpc") == 0) {
        static LPSTR prefix = "\\\\.\\pipe\\lrpc\\";
        LPSTR pname;
       HANDLE conn;
       DWORD err;

        pname = HeapAlloc(GetProcessHeap(), 0, strlen(prefix) + strlen(Binding->Endpoint) + 1);
        strcat(strcpy(pname, prefix), Binding->Endpoint);
       TRACE("connecting to %s\n", pname);
       while (TRUE) {
         if (WaitNamedPipeA(pname, NMPWAIT_WAIT_FOREVER)) {
           conn = CreateFileA(pname, GENERIC_READ|GENERIC_WRITE, 0, NULL,
                              OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
           if (conn != INVALID_HANDLE_VALUE) break;
           err = GetLastError();
           if (err == ERROR_PIPE_BUSY) continue;
           TRACE("connection failed, error=%lx\n", err);
           HeapFree(GetProcessHeap(), 0, pname);
           return err;
         }
         else {
           err = GetLastError();
           TRACE("connection failed, error=%lx\n", err);
           HeapFree(GetProcessHeap(), 0, pname);
           return err;
         }
       }

       /* success */
       HeapFree(GetProcessHeap(), 0, pname);
        memset(&Binding->ovl, 0, sizeof(Binding->ovl));
        Binding->ovl.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
       Binding->conn = conn;
      }
      /* protseq=ncacn_np: named pipes */
      else if (strcmp(Binding->Protseq, "ncacn_np") == 0) {
        static LPSTR prefix = "\\\\.";
        LPSTR pname;
       HANDLE conn;
       DWORD err;

        pname = HeapAlloc(GetProcessHeap(), 0, strlen(prefix) + strlen(Binding->Endpoint) + 1);
        strcat(strcpy(pname, prefix), Binding->Endpoint);
       TRACE("connecting to %s\n", pname);
       conn = CreateFileA(pname, GENERIC_READ|GENERIC_WRITE, 0, NULL,
                          OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
       if (conn == INVALID_HANDLE_VALUE) {
         err = GetLastError();
         /* we don't need to handle ERROR_PIPE_BUSY here,
          * the doc says that it is returned to the app */
         TRACE("connection failed, error=%lx\n", err);
         HeapFree(GetProcessHeap(), 0, pname);
         return err;
       }

       /* success */
       HeapFree(GetProcessHeap(), 0, pname);
        memset(&Binding->ovl, 0, sizeof(Binding->ovl));
        Binding->ovl.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
       Binding->conn = conn;
      }
      else {
       ERR("protseq %s not supported\n", Binding->Protseq);
       return RPC_S_PROTSEQ_NOT_SUPPORTED;
      }
    }
  }
  return RPC_S_OK;
}

RPC_STATUS RPCRT4_CloseBinding(RpcBinding* Binding)
{
  TRACE("  (Binding == ^%p)\n", Binding);
  if (Binding->conn) {
    CancelIo(Binding->conn);
    CloseHandle(Binding->conn);
    Binding->conn = 0;
  }
  if (Binding->ovl.hEvent) {
    CloseHandle(Binding->ovl.hEvent);
    Binding->ovl.hEvent = 0;
  }
  return RPC_S_OK;
}

/* utility functions for string composing and parsing */
static unsigned RPCRT4_strcopyA(LPSTR data, LPCSTR src)
{
  unsigned len = strlen(src);
  memcpy(data, src, len*sizeof(CHAR));
  return len;
}

static unsigned RPCRT4_strcopyW(LPWSTR data, LPCWSTR src)
{
  unsigned len = strlenW(src);
  memcpy(data, src, len*sizeof(WCHAR));
  return len;
}

static LPSTR RPCRT4_strconcatA(LPSTR dst, LPCSTR src)
{
  DWORD len = strlen(dst), slen = strlen(src);
  LPSTR ndst = HeapReAlloc(GetProcessHeap(), 0, dst, (len+slen+2)*sizeof(CHAR));
  if (!ndst) HeapFree(GetProcessHeap(), 0, dst);
  ndst[len] = ',';
  memcpy(ndst+len+1, src, slen*sizeof(CHAR));
  ndst[len+slen+1] = 0;
  return ndst;
}

static LPWSTR RPCRT4_strconcatW(LPWSTR dst, LPCWSTR src)
{
  DWORD len = strlenW(dst), slen = strlenW(src);
  LPWSTR ndst = HeapReAlloc(GetProcessHeap(), 0, dst, (len+slen+2)*sizeof(WCHAR));
  if (!ndst) HeapFree(GetProcessHeap(), 0, dst);
  ndst[len] = ',';
  memcpy(ndst+len+1, src, slen*sizeof(WCHAR));
  ndst[len+slen+1] = 0;
  return ndst;
}


/***********************************************************************
 *             RpcStringBindingComposeA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcStringBindingComposeA( LPSTR ObjUuid, LPSTR Protseq,
                                           LPSTR NetworkAddr, LPSTR Endpoint,
                                           LPSTR Options, LPSTR* StringBinding )
{
  DWORD len = 1;
  LPSTR data;

  TRACE( "(%s,%s,%s,%s,%s,%p)\n",
        debugstr_a( ObjUuid ), debugstr_a( Protseq ),
        debugstr_a( NetworkAddr ), debugstr_a( Endpoint ),
        debugstr_a( Options ), StringBinding );

  if (ObjUuid && *ObjUuid) len += strlen(ObjUuid) + 1;
  if (Protseq && *Protseq) len += strlen(Protseq) + 1;
  if (NetworkAddr && *NetworkAddr) len += strlen(NetworkAddr);
  if (Endpoint && *Endpoint) len += strlen(Endpoint) + 2;
  if (Options && *Options) len += strlen(Options) + 2;

  data = HeapAlloc(GetProcessHeap(), 0, len);
  *StringBinding = data;

  if (ObjUuid && *ObjUuid) {
    data += RPCRT4_strcopyA(data, ObjUuid);
    *data++ = '@';
  }
  if (Protseq && *Protseq) {
    data += RPCRT4_strcopyA(data, Protseq);
    *data++ = ':';
  }
  if (NetworkAddr && *NetworkAddr) {
    data += RPCRT4_strcopyA(data, NetworkAddr);
  }
  if ((Endpoint && *Endpoint) ||
      (Options && *Options)) {
    *data++ = '[';
    if (Endpoint && *Endpoint) {
      data += RPCRT4_strcopyA(data, Endpoint);
      if (Options && *Options) *data++ = ',';
    }
    if (Options && *Options) {
      data += RPCRT4_strcopyA(data, Options);
    }
    *data++ = ']';
  }
  *data = 0;

  return RPC_S_OK;
}

/***********************************************************************
 *             RpcStringBindingComposeW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcStringBindingComposeW( LPWSTR ObjUuid, LPWSTR Protseq,
                                            LPWSTR NetworkAddr, LPWSTR Endpoint,
                                            LPWSTR Options, LPWSTR* StringBinding )
{
  DWORD len = 1;
  LPWSTR data;

  TRACE("(%s,%s,%s,%s,%s,%p)\n",
       debugstr_w( ObjUuid ), debugstr_w( Protseq ),
       debugstr_w( NetworkAddr ), debugstr_w( Endpoint ),
       debugstr_w( Options ), StringBinding);

  if (ObjUuid && *ObjUuid) len += strlenW(ObjUuid) + 1;
  if (Protseq && *Protseq) len += strlenW(Protseq) + 1;
  if (NetworkAddr && *NetworkAddr) len += strlenW(NetworkAddr);
  if (Endpoint && *Endpoint) len += strlenW(Endpoint) + 2;
  if (Options && *Options) len += strlenW(Options) + 2;

  data = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
  *StringBinding = data;

  if (ObjUuid && *ObjUuid) {
    data += RPCRT4_strcopyW(data, ObjUuid);
    *data++ = '@';
  }
  if (Protseq && *Protseq) {
    data += RPCRT4_strcopyW(data, Protseq);
    *data++ = ':';
  }
  if (NetworkAddr && *NetworkAddr) {
    data += RPCRT4_strcopyW(data, NetworkAddr);
  }
  if ((Endpoint && *Endpoint) ||
      (Options && *Options)) {
    *data++ = '[';
    if (Endpoint && *Endpoint) {
      data += RPCRT4_strcopyW(data, Endpoint);
      if (Options && *Options) *data++ = ',';
    }
    if (Options && *Options) {
      data += RPCRT4_strcopyW(data, Options);
    }
    *data++ = ']';
  }
  *data = 0;

  return RPC_S_OK;
}


/***********************************************************************
 *             RpcStringBindingParseA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcStringBindingParseA( LPSTR StringBinding, LPSTR *ObjUuid,
                                          LPSTR *Protseq, LPSTR *NetworkAddr,
                                          LPSTR *Endpoint, LPSTR *Options)
{
  CHAR *data, *next;
  static const char ep_opt[] = "endpoint=";

  TRACE("(%s,%p,%p,%p,%p,%p)\n", debugstr_a(StringBinding),
       ObjUuid, Protseq, NetworkAddr, Endpoint, Options);

  if (ObjUuid) *ObjUuid = NULL;
  if (Protseq) *Protseq = NULL;
  if (NetworkAddr) *NetworkAddr = NULL;
  if (Endpoint) *Endpoint = NULL;
  if (Options) *Options = NULL;

  data = StringBinding;

  next = strchr(data, '@');
  if (next) {
    if (ObjUuid) *ObjUuid = RPCRT4_strndupA(data, next - data);
    data = next+1;
  }

  next = strchr(data, ':');
  if (next) {
    if (Protseq) *Protseq = RPCRT4_strndupA(data, next - data);
    data = next+1;
  }

  next = strchr(data, '[');
  if (next) {
    CHAR *close, *opt;

    if (NetworkAddr) *NetworkAddr = RPCRT4_strndupA(data, next - data);
    data = next+1;
    close = strchr(data, ']');
    if (!close) goto fail;

    /* tokenize options */
    while (data < close) {
      next = strchr(data, ',');
      if (!next || next > close) next = close;
      /* FIXME: this is kind of inefficient */
      opt = RPCRT4_strndupA(data, next - data);
      data = next+1;

      /* parse option */
      next = strchr(opt, '=');
      if (!next) {
       /* not an option, must be an endpoint */
       if (*Endpoint) goto fail;
       *Endpoint = opt;
      }
      else {
       if (strncmp(opt, ep_opt, strlen(ep_opt)) == 0) {
         /* endpoint option */
         if (*Endpoint) goto fail;
         *Endpoint = RPCRT4_strdupA(next+1);
         HeapFree(GetProcessHeap(), 0, opt);
       }
       else {
         /* network option */
         if (*Options) {
            /* FIXME: this is kind of inefficient */
           *Options = RPCRT4_strconcatA(*Options, opt);
           HeapFree(GetProcessHeap(), 0, opt);
         }
         else *Options = opt;
       }
      }
    }

    data = close+1;
    if (*data) goto fail;
  }
  else if (NetworkAddr) *NetworkAddr = RPCRT4_strdupA(data);
  return RPC_S_OK;

fail:
  if (ObjUuid) RpcStringFreeA(ObjUuid);
  if (Protseq) RpcStringFreeA(Protseq);
  if (NetworkAddr) RpcStringFreeA(NetworkAddr);
  if (Endpoint) RpcStringFreeA(Endpoint);
  if (Options) RpcStringFreeA(Options);
  return RPC_S_INVALID_STRING_BINDING;
}

/***********************************************************************
 *             RpcStringBindingParseW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcStringBindingParseW( LPWSTR StringBinding, LPWSTR *ObjUuid,
                                          LPWSTR *Protseq, LPWSTR *NetworkAddr,
                                          LPWSTR *Endpoint, LPWSTR *Options)
{
  WCHAR *data, *next;
  static const WCHAR ep_opt[] = {'e','n','d','p','o','i','n','t','=',0};

  TRACE("(%s,%p,%p,%p,%p,%p)\n", debugstr_w(StringBinding),
       ObjUuid, Protseq, NetworkAddr, Endpoint, Options);

  if (ObjUuid) *ObjUuid = NULL;
  if (Protseq) *Protseq = NULL;
  if (NetworkAddr) *NetworkAddr = NULL;
  if (Endpoint) *Endpoint = NULL;
  if (Options) *Options = NULL;

  data = StringBinding;

  next = strchrW(data, '@');
  if (next) {
    if (ObjUuid) *ObjUuid = RPCRT4_strndupW(data, next - data);
    data = next+1;
  }

  next = strchrW(data, ':');
  if (next) {
    if (Protseq) *Protseq = RPCRT4_strndupW(data, next - data);
    data = next+1;
  }

  next = strchrW(data, '[');
  if (next) {
    WCHAR *close, *opt;

    if (NetworkAddr) *NetworkAddr = RPCRT4_strndupW(data, next - data);
    data = next+1;
    close = strchrW(data, ']');
    if (!close) goto fail;

    /* tokenize options */
    while (data < close) {
      next = strchrW(data, ',');
      if (!next || next > close) next = close;
      /* FIXME: this is kind of inefficient */
      opt = RPCRT4_strndupW(data, next - data);
      data = next+1;

      /* parse option */
      next = strchrW(opt, '=');
      if (!next) {
       /* not an option, must be an endpoint */
       if (*Endpoint) goto fail;
       *Endpoint = opt;
      }
      else {
       if (strncmpW(opt, ep_opt, strlenW(ep_opt)) == 0) {
         /* endpoint option */
         if (*Endpoint) goto fail;
         *Endpoint = RPCRT4_strdupW(next+1);
         HeapFree(GetProcessHeap(), 0, opt);
       }
       else {
         /* network option */
         if (*Options) {
            /* FIXME: this is kind of inefficient */
           *Options = RPCRT4_strconcatW(*Options, opt);
           HeapFree(GetProcessHeap(), 0, opt);
         }
         else *Options = opt;
       }
      }
    }

    data = close+1;
    if (*data) goto fail;
  }
  else if (NetworkAddr) *NetworkAddr = RPCRT4_strdupW(data);

  return RPC_S_OK;
fail:
  if (ObjUuid) RpcStringFreeW(ObjUuid);
  if (Protseq) RpcStringFreeW(Protseq);
  if (NetworkAddr) RpcStringFreeW(NetworkAddr);
  if (Endpoint) RpcStringFreeW(Endpoint);
  if (Options) RpcStringFreeW(Options);
  return RPC_S_INVALID_STRING_BINDING;
}

/***********************************************************************
 *             RpcBindingFree (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcBindingFree( RPC_BINDING_HANDLE* Binding )
{
  RPC_STATUS status;
  TRACE("(%p) = %p\n", Binding, *Binding);
  status = RPCRT4_DestroyBinding(*Binding);
  if (status == RPC_S_OK) *Binding = 0;
  return status;
}
  
/***********************************************************************
 *             RpcBindingVectorFree (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcBindingVectorFree( RPC_BINDING_VECTOR** BindingVector )
{
  RPC_STATUS status;
  unsigned long c;

  TRACE("(%p)\n", BindingVector);
  for (c=0; c<(*BindingVector)->Count; c++) {
    status = RpcBindingFree(&(*BindingVector)->BindingH[c]);
  }
  HeapFree(GetProcessHeap(), 0, *BindingVector);
  *BindingVector = NULL;
  return RPC_S_OK;
}
  
/***********************************************************************
 *             RpcBindingInqObject (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcBindingInqObject( RPC_BINDING_HANDLE Binding, UUID* ObjectUuid )
{
  RpcBinding* bind = (RpcBinding*)Binding;

  TRACE("(%p,%p) = %s\n", Binding, ObjectUuid, debugstr_guid(&bind->ObjectUuid));
  memcpy(ObjectUuid, &bind->ObjectUuid, sizeof(UUID));
  return RPC_S_OK;
}
  
/***********************************************************************
 *             RpcBindingSetObject (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcBindingSetObject( RPC_BINDING_HANDLE Binding, UUID* ObjectUuid )
{
  RpcBinding* bind = (RpcBinding*)Binding;

  TRACE("(%p,%s)\n", Binding, debugstr_guid(ObjectUuid));
  if (bind->server) return RPC_S_WRONG_KIND_OF_BINDING;
  return RPCRT4_SetBindingObject(Binding, ObjectUuid);
}

/***********************************************************************
 *             RpcBindingFromStringBindingA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcBindingFromStringBindingA( LPSTR StringBinding, RPC_BINDING_HANDLE* Binding )
{
  RPC_STATUS ret;
  RpcBinding* bind = NULL;
  LPSTR ObjectUuid, Protseq, NetworkAddr, Endpoint, Options;
  UUID Uuid;

  TRACE("(%s,%p)\n", debugstr_a(StringBinding), Binding);

  ret = RpcStringBindingParseA(StringBinding, &ObjectUuid, &Protseq,
                              &NetworkAddr, &Endpoint, &Options);
  if (ret != RPC_S_OK) return ret;

  ret = UuidFromStringA(ObjectUuid, &Uuid);

  if (ret == RPC_S_OK)
    ret = RPCRT4_CreateBindingA(&bind, FALSE, Protseq);
  if (ret == RPC_S_OK)
    ret = RPCRT4_SetBindingObject(bind, &Uuid);
  if (ret == RPC_S_OK)
    ret = RPCRT4_CompleteBindingA(bind, NetworkAddr, Endpoint, Options);

  RpcStringFreeA(&Options);
  RpcStringFreeA(&Endpoint);
  RpcStringFreeA(&NetworkAddr);
  RpcStringFreeA(&Protseq);
  RpcStringFreeA(&ObjectUuid);

  if (ret == RPC_S_OK) *Binding = (RPC_BINDING_HANDLE)bind;
  else RPCRT4_DestroyBinding(bind);

  return ret;
}

/***********************************************************************
 *             RpcBindingFromStringBindingW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcBindingFromStringBindingW( LPWSTR StringBinding, RPC_BINDING_HANDLE* Binding )
{
  RPC_STATUS ret;
  RpcBinding* bind = NULL;
  LPWSTR ObjectUuid, Protseq, NetworkAddr, Endpoint, Options;
  UUID Uuid;

  TRACE("(%s,%p)\n", debugstr_w(StringBinding), Binding);

  ret = RpcStringBindingParseW(StringBinding, &ObjectUuid, &Protseq,
                              &NetworkAddr, &Endpoint, &Options);
  if (ret != RPC_S_OK) return ret;

  ret = UuidFromStringW(ObjectUuid, &Uuid);

  if (ret == RPC_S_OK)
    ret = RPCRT4_CreateBindingW(&bind, FALSE, Protseq);
  if (ret == RPC_S_OK)
    ret = RPCRT4_SetBindingObject(bind, &Uuid);
  if (ret == RPC_S_OK)
    ret = RPCRT4_CompleteBindingW(bind, NetworkAddr, Endpoint, Options);

  RpcStringFreeW(&Options);
  RpcStringFreeW(&Endpoint);
  RpcStringFreeW(&NetworkAddr);
  RpcStringFreeW(&Protseq);
  RpcStringFreeW(&ObjectUuid);

  if (ret == RPC_S_OK) *Binding = (RPC_BINDING_HANDLE)bind;
  else RPCRT4_DestroyBinding(bind);

  return ret;
}
  
/***********************************************************************
 *             RpcBindingToStringBindingA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcBindingToStringBindingA( RPC_BINDING_HANDLE Binding, LPSTR* StringBinding )
{
  RPC_STATUS ret;
  RpcBinding* bind = (RpcBinding*)Binding;
  LPSTR ObjectUuid;

  TRACE("(%p,%p)\n", Binding, StringBinding);

  ret = UuidToStringA(&bind->ObjectUuid, &ObjectUuid);
  if (ret != RPC_S_OK) return ret;

  ret = RpcStringBindingComposeA(ObjectUuid, bind->Protseq, bind->NetworkAddr,
                                 bind->Endpoint, NULL, StringBinding);

  RpcStringFreeA(&ObjectUuid);

  return ret;
}
  
/***********************************************************************
 *             RpcBindingToStringBindingW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcBindingToStringBindingW( RPC_BINDING_HANDLE Binding, LPWSTR* StringBinding )
{
  RPC_STATUS ret;
  LPSTR str = NULL;
  TRACE("(%p,%p)\n", Binding, StringBinding);
  ret = RpcBindingToStringBindingA(Binding, &str);
  *StringBinding = RPCRT4_strdupAtoW(str);
  RpcStringFreeA(&str);
  return ret;
}
  
/***********************************************************************
 *             I_RpcBindingSetAsync (RPCRT4.@)
 * NOTES
 *  Exists in win9x and winNT, but with different number of arguments
 *  (9x version has 3 arguments, NT has 2).
 */
RPC_STATUS WINAPI I_RpcBindingSetAsync( RPC_BINDING_HANDLE Binding, RPC_BLOCKING_FN BlockingFn, unsigned long ServerTid )
{
  RpcBinding* bind = (RpcBinding*)Binding;

  TRACE( "(%p,%p,%ld): stub\n", Binding, BlockingFn, ServerTid );

  bind->BlockingFn = BlockingFn;
  bind->ServerTid = ServerTid;

  return RPC_S_OK;
}
