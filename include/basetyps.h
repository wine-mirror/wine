/*
 * Copyright (C) 1998 Anders Norlander
 * Copyright (C) 2005 Steven Edwards
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

#ifndef _BASETYPS_H_
#define _BASETYPS_H_

#ifdef __cplusplus
# define EXTERN_C extern "C"
#else
# define EXTERN_C extern
#endif

#define STDMETHODCALLTYPE     WINAPI
#define STDMETHODVCALLTYPE    WINAPIV
#define STDAPICALLTYPE        WINAPI
#define STDAPIVCALLTYPE       WINAPIV
#define STDAPI                EXTERN_C HRESULT STDAPICALLTYPE
#define STDAPI_(type)         EXTERN_C type STDAPICALLTYPE
#define STDMETHODIMP          HRESULT STDMETHODCALLTYPE
#define STDMETHODIMP_(type)   type STDMETHODCALLTYPE
#define STDAPIV               EXTERN_C HRESULT STDAPIVCALLTYPE
#define STDAPIV_(type)        EXTERN_C type STDAPIVCALLTYPE
#define STDMETHODIMPV         HRESULT STDMETHODVCALLTYPE
#define STDMETHODIMPV_(type)  type STDMETHODVCALLTYPE

#undef STDMETHOD
#undef STDMETHOD_
#undef PURE
#undef THIS_
#undef THIS
#undef DECLARE_INTERFACE
#undef DECLARE_INTERFACE_

#if defined(__cplusplus) && !defined(CINTERFACE)

#ifdef COM_STDMETHOD_CAN_THROW
# define COM_DECLSPEC_NOTHROW
#else
# define COM_DECLSPEC_NOTHROW DECLSPEC_NOTHROW
#endif

# define interface struct
# define STDMETHOD(m)    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE m
# define STDMETHOD_(t,m) virtual COM_DECLSPEC_NOTHROW t STDMETHODCALLTYPE m
# define PURE =0
# define THIS_
# define THIS void
# define DECLARE_INTERFACE(i)    interface i
# define DECLARE_INTERFACE_(i,b) interface i : public b
#else
# define STDMETHOD(m) HRESULT (STDMETHODCALLTYPE *m)
# define STDMETHOD_(t,m) t (STDMETHODCALLTYPE *m)
# define PURE
# define THIS_ INTERFACE *,
# define THIS INTERFACE *
# ifdef CONST_VTABLE
#  define DECLARE_INTERFACE(i) \
     typedef interface i { const struct i##Vtbl *lpVtbl; } i; \
     typedef struct i##Vtbl i##Vtbl; \
     struct i##Vtbl
# else
#  define DECLARE_INTERFACE(i) \
     typedef interface i { struct i##Vtbl *lpVtbl; } i; \
     typedef struct i##Vtbl i##Vtbl; \
     struct i##Vtbl
# endif
# define DECLARE_INTERFACE_(i,b) DECLARE_INTERFACE(i)
#endif

#include <guiddef.h>

#ifndef _ERROR_STATUS_T_DEFINED
typedef unsigned long error_status_t;
#define _ERROR_STATUS_T_DEFINED
#endif

#ifndef _WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short wchar_t;
#endif
#define _WCHAR_T_DEFINED
#endif

#endif /* _BASETYPS_H_ */
