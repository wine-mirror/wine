/*
 * Defines the COM interfaces and APIs related to client/server aspects.
 *
 * Depends on 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_CLIENTSERVER_H
#define __WINE_WINE_OBJ_CLIENTSERVER_H


/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IClientSecurity,	0x0000013dL, 0, 0);
typedef struct IClientSecurity IClientSecurity,*LPCLIENTSECURITY;

DEFINE_OLEGUID(IID_IExternalConnection,	0x00000019L, 0, 0);
typedef struct IExternalConnection IExternalConnection,*LPEXTERNALCONNECTION;

DEFINE_OLEGUID(IID_IMessageFilter,	0x00000016L, 0, 0);
typedef struct IMessageFilter IMessageFilter,*LPMESSAGEFILTER;

DEFINE_OLEGUID(IID_IServerSecurity,	0x0000013eL, 0, 0);
typedef struct IServerSecurity IServerSecurity,*LPSERVERSECURITY;


/*****************************************************************************
 * IClientSecurity interface
 */
/* FIXME: not implemented */


/*****************************************************************************
 * IExternalConnection interface
 */
/* FIXME: not implemented */

HRESULT WINAPI CoDisconnectObject(LPUNKNOWN lpUnk, DWORD reserved);


/*****************************************************************************
 * IMessageFilter interface
 */
/* FIXME: not implemented */
#define ICOM_INTERFACE IMessageFilter
ICOM_BEGIN(IMessageFilter, IUnknown)
ICOM_END(IMessageFilter)
#undef ICOM_INTERFACE



HRESULT WINAPI CoRegisterMessageFilter16(LPMESSAGEFILTER lpMessageFilter,LPMESSAGEFILTER *lplpMessageFilter);
HRESULT WINAPI CoRegisterMessageFilter32(LPMESSAGEFILTER lpMessageFilter,LPMESSAGEFILTER *lplpMessageFilter);
#define CoRegisterMessageFilter WINELIB_NAME(CoRegisterMessageFilter)


/*****************************************************************************
 * IServerSecurity interface
 */
/* FIXME: not implemented */


#endif /* __WINE_WINE_OBJ_CLIENTSERVER_H */
