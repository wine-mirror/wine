/*
 * Defines undocumented Microsoft COM interfaces and APIs seemingly related to some 'channel' notion.
 *
 * Depends on 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_CHANNEL_H
#define __WINE_WINE_OBJ_CHANNEL_H


/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID   (IID_IChannelHook,	0x1008c4a0L, 0x7613, 0x11cf, 0x9a, 0xf1, 0x00, 0x20, 0xaf, 0x6e, 0x72, 0xf4);
typedef struct IChannelHook IChannelHook,*LPCHANNELHOOK;

DEFINE_GUID   (IID_IPSFactoryBuffer,	0xd5f569d0L, 0x593b, 0x101a, 0xb5, 0x69, 0x08, 0x00, 0x2b, 0x2d, 0xbf, 0x7a);
typedef struct IPSFactoryBuffer IPSFactoryBuffer,*LPPSFACTORYBUFFER;

DEFINE_GUID   (IID_IRpcChannelBuffer,	0xd5f56b60L, 0x593b, 0x101a, 0xb5, 0x69, 0x08, 0x00, 0x2b, 0x2d, 0xbf, 0x7a);
typedef struct IRpcChannelBuffer IRpcChannelBuffer,*LPRPCCHANNELBUFFER;

DEFINE_GUID   (IID_IRpcProxyBuffer,	0xd5f56a34L, 0x593b, 0x101a, 0xb5, 0x69, 0x08, 0x00, 0x2b, 0x2d, 0xbf, 0x7a);
typedef struct IRpcProxyBuffer IRpcProxyBuffer,*LPRPCPROXYBUFFER;

DEFINE_GUID   (IID_IRpcStubBuffer,	0xd5f56afcL, 0x593b, 0x101a, 0xb5, 0x69, 0x08, 0x00, 0x2b, 0x2d, 0xbf, 0x7a);
typedef struct IRpcStubBuffer IRpcStubBuffer,*LPRPCSTUBBUFFER;


/*****************************************************************************
 * IChannelHook interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IPSFactoryBuffer interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IRpcChannelBuffer interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IRpcProxyBuffer interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IRpcStubBuffer interface
 */
/* FIXME: not implemented */


#endif /* __WINE_WINE_OBJ_CHANNEL_H */
