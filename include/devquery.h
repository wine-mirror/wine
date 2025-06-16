/*
 * Copyright 2025 Vibhav Pant
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

#ifndef __DEVQUERY_H__
#define __DEVQUERY_H__

#include <devquerydef.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_HANDLE(HDEVQUERY);
typedef HDEVQUERY *PHDEVQUERY;

typedef void (WINAPI *PDEV_QUERY_RESULT_CALLBACK)( HDEVQUERY devquery, void *user_data, const DEV_QUERY_RESULT_ACTION_DATA *action_data );

HRESULT WINAPI DevCreateObjectQuery( DEV_OBJECT_TYPE type, ULONG flags, ULONG props_len, const DEVPROPCOMPKEY *props, ULONG filters_len,
                                     const DEVPROP_FILTER_EXPRESSION *filters, PDEV_QUERY_RESULT_CALLBACK callback, void *user_data, HDEVQUERY *devquery );
HRESULT WINAPI DevCreateObjectQueryEx( DEV_OBJECT_TYPE type, ULONG flags, ULONG props_len, const DEVPROPCOMPKEY *props, ULONG filters_len,
                                       const DEVPROP_FILTER_EXPRESSION *filters, ULONG params_len, const DEV_QUERY_PARAMETER *params,
                                       PDEV_QUERY_RESULT_CALLBACK callback, void *user_data, HDEVQUERY *devquery );

HRESULT WINAPI DevCreateObjectQueryFromId( DEV_OBJECT_TYPE type, const WCHAR *id, ULONG flags, ULONG props_len, const DEVPROPCOMPKEY *props, ULONG filters_len,
                                           const DEVPROP_FILTER_EXPRESSION *filters, PDEV_QUERY_RESULT_CALLBACK callback, void *user_data, HDEVQUERY *devquery );
HRESULT WINAPI DevCreateObjectQueryFromIdEx( DEV_OBJECT_TYPE type, const WCHAR *id, ULONG flags, ULONG props_len, const DEVPROPCOMPKEY *props,
                                             ULONG filters_len, const DEVPROP_FILTER_EXPRESSION *filters, ULONG params_len, const DEV_QUERY_PARAMETER *params,
                                             PDEV_QUERY_RESULT_CALLBACK callback, void *user_data, HDEVQUERY *devquery );
HRESULT WINAPI DevCreateObjectQueryFromIds( DEV_OBJECT_TYPE type, const WCHAR *id_sz, ULONG flags, ULONG props_len, const DEVPROPCOMPKEY *props, ULONG filters_len,
                                            const DEVPROP_FILTER_EXPRESSION *filters, PDEV_QUERY_RESULT_CALLBACK callback, void *user_data, HDEVQUERY *devquery );
HRESULT WINAPI DevCreateObjectQueryFromIdsEx( DEV_OBJECT_TYPE type, const WCHAR *id_sz, ULONG flags, ULONG props_len, const DEVPROPCOMPKEY *props,
                                              ULONG filters_len, const DEVPROP_FILTER_EXPRESSION *filters, ULONG params_len, const DEV_QUERY_PARAMETER *params,
                                              PDEV_QUERY_RESULT_CALLBACK callback, void *user_data, HDEVQUERY *devquery );
void WINAPI DevCloseObjectQuery( HDEVQUERY devquery );

HRESULT WINAPI DevGetObjects( DEV_OBJECT_TYPE type, ULONG flags, ULONG props_len, const DEVPROPCOMPKEY *props, ULONG filters_len,
                              const DEVPROP_FILTER_EXPRESSION *filters, ULONG *objs_len, const DEV_OBJECT **objs );
HRESULT WINAPI DevGetObjectsEx( DEV_OBJECT_TYPE type, ULONG flags, ULONG props_len, const DEVPROPCOMPKEY *props, ULONG filters_len,
                                const DEVPROP_FILTER_EXPRESSION *filters, ULONG params_len, const DEV_QUERY_PARAMETER *params, ULONG *objs_len,
                                const DEV_OBJECT **objs );
void WINAPI DevFreeObjects( ULONG len, const DEV_OBJECT *objs );

HRESULT WINAPI DevGetObjectProperties( DEV_OBJECT_TYPE type, const WCHAR *id, ULONG flags, ULONG props_len, const DEVPROPCOMPKEY *props, ULONG *buf_len,
                                       const DEVPROPERTY **buf );
HRESULT WINAPI DevGetObjectPropertiesEx( DEV_OBJECT_TYPE type, const WCHAR *id, ULONG flags, ULONG props_len, const DEVPROPCOMPKEY *props, ULONG params_len,
                                         const DEV_QUERY_PARAMETER *params, ULONG *buf_len, const DEVPROPERTY **buf );
const DEVPROPERTY *WINAPI DevFindProperty( const DEVPROPKEY *key, DEVPROPSTORE store, const WCHAR *locale, ULONG props_len, const DEVPROPERTY *props );
void WINAPI DevFreeObjectProperties( ULONG len, const DEVPROPERTY *props );

#ifdef __cplusplus
}
#endif

#endif /* __DEVQUERY_H__ */
