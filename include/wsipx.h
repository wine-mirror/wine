/* WSIPX.H
 */

#ifndef _WINE_WSIPX_
#define _WINE_WSIPX_

#ifdef USE_WS_PREFIX
# define WS(x)    WS_##x
#else
# define WS(x)    x
#endif

typedef struct WS_sockaddr_ipx
{
    short sa_family;
    char sa_netnum[4];
    char sa_nodenum[6];
    unsigned short sa_socket;
} SOCKADDR_IPX, *PSOCKADDR_IPX, *LPSOCKADDR_IPX;

/*
 * constants
 */
#define NSPROTO_IPX                 1000
#define NSPROTO_SPX                 1256
#define NSPROTO_SPXII               1257

#undef WS
#endif /* _WINE_WSIPX_ */
