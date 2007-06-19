/*
 * RPCSS definitions
 *
 * Copyright (C) 2002 Greg Turner
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

#ifndef __WINE_RPCSS_H
#define __WINE_RPCSS_H

#include "wine/rpcss_shared.h"
#include "windows.h"

/* in seconds */
#define RPCSS_DEFAULT_MAX_LAZY_TIMEOUT 30

/* rpcss_main.c */
HANDLE RPCSS_GetMasterMutex(void);

/* epmap_server.c */
BOOL RPCSS_EpmapEmpty(void);
BOOL RPCSS_NPDoWork(HANDLE exit_event);
void RPCSS_RegisterRpcEndpoints(RPC_SYNTAX_IDENTIFIER iface, int object_count,
  int binding_count, int no_replace, char *vardata, long vardata_size);
void RPCSS_UnregisterRpcEndpoints(RPC_SYNTAX_IDENTIFIER iface, int object_count,
  int binding_count, char *vardata, long vardata_size);
void RPCSS_ResolveRpcEndpoints(RPC_SYNTAX_IDENTIFIER iface, UUID object,
  char *protseq, char *rslt_ep);

/* named_pipe_kludge.c */
BOOL RPCSS_BecomePipeServer(void);
BOOL RPCSS_UnBecomePipeServer(void);
LONG RPCSS_SrvThreadCount(void);

#endif /* __WINE_RPCSS_H */
