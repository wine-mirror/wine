/*
 * Copyright 2015 Nikolay Sivov for CodeWeavers
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

#ifndef __WINE_WEBSERVICES_H
#define __WINE_WEBSERVICES_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef struct _WS_ERROR WS_ERROR;

typedef enum {
    WS_ERROR_PROPERTY_STRING_COUNT,
    WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE,
    WS_ERROR_PROPERTY_LANGID
} WS_ERROR_PROPERTY_ID;

typedef struct _WS_ERROR_PROPERTY {
    WS_ERROR_PROPERTY_ID id;
    void                *value;
    ULONG                valueSize;
} WS_ERROR_PROPERTY;

HRESULT WINAPI WsCreateError(const WS_ERROR_PROPERTY*, ULONG, WS_ERROR**);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* __WINE_WEBSERVICES_H */
