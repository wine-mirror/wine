#ifndef __WINE_WINE_WINSOCK16_H
#define __WINE_WINE_WINSOCK16_H

#include "windef.h"
#include "pshpack1.h"

/* ws_hostent16, ws_protoent16, ws_servent16, ws_netent16
 * are 1-byte aligned here ! */
typedef struct ws_hostent16
{
        SEGPTR  h_name;         /* official name of host */
        SEGPTR  h_aliases;      /* alias list */
        INT16   h_addrtype;     /* host address type */
        INT16   h_length;       /* length of address */
        SEGPTR  h_addr_list;    /* list of addresses from name server */
} _ws_hostent16;

typedef struct ws_protoent16
{
        SEGPTR  p_name;         /* official protocol name */
        SEGPTR  p_aliases;      /* alias list */
        INT16   p_proto;        /* protocol # */
} _ws_protoent16;

typedef struct ws_servent16
{
        SEGPTR  s_name;         /* official service name */
        SEGPTR  s_aliases;      /* alias list */
        INT16   s_port;         /* port # */
        SEGPTR  s_proto;        /* protocol to use */
} _ws_servent16;

typedef struct ws_netent16
{
        SEGPTR  n_name;         /* official name of net */
        SEGPTR  n_aliases;      /* alias list */
        INT16   n_addrtype;     /* net address type */
        INT     n_net;          /* network # */
} _ws_netent16;

#include "poppack.h"

#endif /* __WINE_WINE_WINSOCK16_H */
