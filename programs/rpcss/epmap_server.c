/*
 * RPC endpoint mapper server
 *
 * Copyright (C) 2001 Ove Kåven, TransGaming Technologies Inc,
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

#include <assert.h>
#include <string.h>

#include "rpcss.h"
#include "rpc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

struct epmap_entry
{
    struct epmap_entry *next;
    RPC_SYNTAX_IDENTIFIER iface;
    UUID object;
    char *protseq;
    char *endpoint;
};

static struct epmap_entry *epmap;

static const UUID nil_object;

static char *mystrdup(const char *str) {
    char *rval;
    rval = LocalAlloc(LPTR, strlen(str)+1);
    CopyMemory(rval, str, strlen(str)+1);
    return rval;
}

static struct epmap_entry *find_endpoint(const RPC_SYNTAX_IDENTIFIER *iface,
                                         const char *protseq, const UUID *object)
{
    struct epmap_entry *map;
    for (map=epmap; map; map=map->next) {
        if (memcmp(&map->iface, iface, sizeof(RPC_SYNTAX_IDENTIFIER))) continue;
        if (memcmp(&map->object, object, sizeof(UUID))) continue;
        if (strcmp(map->protseq, protseq)) continue;
	WINE_TRACE("found.\n");
        return map;
    }
    WINE_TRACE("not found.\n");
    return NULL;
}

static void register_endpoint(const RPC_SYNTAX_IDENTIFIER *iface, const char *protseq,
                              const char *endpoint, const UUID *objects, int objcount,
                              int no_replace)
{
    int c;

    WINE_TRACE("(protseq == %s, endpoint == %s, objcount == %i, no_replace == %i)\n",
      wine_dbgstr_a(protseq), wine_dbgstr_a(endpoint), objcount, no_replace);

    if (!objcount) {
        objects = &nil_object;
        objcount = 1;
    }

    for (c=0; c<objcount; c++) {
        struct epmap_entry *map = NULL;
        if (!no_replace)
            map = find_endpoint(iface, protseq, &objects[c]);
        if (map) {
            LocalFree(map->endpoint);
        }
        else {
            map = LocalAlloc(LPTR, sizeof(struct epmap_entry));
            memcpy(&map->iface, iface, sizeof(RPC_SYNTAX_IDENTIFIER));
            memcpy(&map->object, &objects[c], sizeof(UUID));
            map->protseq = mystrdup(protseq);
            map->next = epmap;
            epmap = map;
        }
        WINE_TRACE("  mapping endpoint (protseq == %s, endpoint == %s, uuid == %s)\n",
	  wine_dbgstr_a(protseq), wine_dbgstr_a(endpoint), wine_dbgstr_guid(&objects[c]));
        map->endpoint = mystrdup(endpoint);
    }
}

static void unregister_endpoint(const RPC_SYNTAX_IDENTIFIER *iface, const char *protseq,
                                const char *endpoint, const UUID *objects, int objcount)
{
    struct epmap_entry *map, *prev, *nprev, *next;
    int c;
    
    WINE_TRACE("(protseq == %s, endpoint == %s, objcount == %i)\n", 
      wine_dbgstr_a(protseq), wine_dbgstr_a(endpoint), objcount);
    
    if (!objcount) {
        objects = &nil_object;
        objcount = 1;
    }
    prev=NULL;
    nprev=NULL;
    map=epmap;
    while(map) {
        next = map->next;
        nprev = map;
        if (memcmp(&map->iface, iface, sizeof(RPC_SYNTAX_IDENTIFIER))) goto cont;
        for (c=0; c<objcount; c++)
            if (!memcmp(&map->object, &objects[c], sizeof(UUID))) break;
        if (c==objcount) goto cont;
        if (strcmp(map->protseq, protseq)) goto cont;
        
        WINE_TRACE("  unmapping: (protseq == %s, endpoint == %s, uuid == %s)\n",
	  wine_dbgstr_a(map->protseq), wine_dbgstr_a(map->endpoint),
	  wine_dbgstr_guid(&map->object));
        
        if (prev) prev->next = map->next;
        else epmap = map->next;
        nprev = prev;

        LocalFree(map->protseq);
        LocalFree(map->endpoint);
        LocalFree(map);

        cont:

	prev = nprev;
	map = next;
    }
}

static void resolve_endpoint(const RPC_SYNTAX_IDENTIFIER *iface, const char *protseq,
                             const UUID *object, char *rslt_ep)
{
    size_t len;
    struct epmap_entry *map;

    if (!(map = find_endpoint(iface, protseq, object))) return;

    len = min( MAX_RPCSS_NP_REPLY_STRING_LEN, strlen(map->endpoint)+1 );
    if (len) memcpy(rslt_ep, map->endpoint, len);
}

static const char *get_string(const char**ptr, const char*end)
{
    const char *str = *ptr, *nptr = str;

    while (nptr < end && *nptr) nptr++;
    if (nptr == end)
        return NULL;
    *ptr = nptr + 1;
    return str;
}

BOOL RPCSS_EpmapEmpty(void)
{
  return (!epmap);
}

void RPCSS_RegisterRpcEndpoints(RPC_SYNTAX_IDENTIFIER iface, int object_count, 
  int binding_count, int no_replace, char *vardata, long vardata_size)
{
    const char *data = vardata;
    const char *end = data + vardata_size;
    const UUID *objects = (const UUID *)data;
    int c;

    data += object_count * sizeof(UUID);
    for (c=0; c < binding_count; c++) {
        const char *protseq = get_string(&data, end);
        const char *endpoint = get_string(&data, end);
        if (protseq && endpoint)
            register_endpoint(&iface, protseq, endpoint, objects, object_count, no_replace);
    }
}

void RPCSS_UnregisterRpcEndpoints(RPC_SYNTAX_IDENTIFIER iface, int object_count,
  int binding_count, char *vardata, long vardata_size)
{
    const char *data = vardata;
    const char *end = data + vardata_size;
    const UUID *objects = (const UUID *)data;
    int c;

    data += object_count * sizeof(UUID);
    for (c=0; c < binding_count; c++) {
        const char *protseq = get_string(&data, end);
        const char *endpoint = get_string(&data, end);
        if (protseq && endpoint)
            unregister_endpoint(&iface, protseq, endpoint, objects, object_count);
    }
}

void RPCSS_ResolveRpcEndpoints(RPC_SYNTAX_IDENTIFIER iface, UUID object, char *protseq, char *rslt_ep)
{
    resolve_endpoint(&iface, protseq, &object, rslt_ep);
}
