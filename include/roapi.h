/*
 * Copyright (C) 2014 Martin Storsjo
 * Copyright (C) 2016 Michael Müller
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

#ifndef __WINE_ROAPI_H
#define __WINE_ROAPI_H

#include <windef.h>
#include <activation.h>

typedef enum
{
    RO_INIT_SINGLETHREADED = 0,
    RO_INIT_MULTITHREADED  = 1,
} RO_INIT_TYPE;

DECLARE_HANDLE(APARTMENT_SHUTDOWN_REGISTRATION_COOKIE);

#ifdef __cplusplus
typedef struct {} *RO_REGISTRATION_COOKIE;
#else
typedef struct _RO_REGISTRATION_COOKIE *RO_REGISTRATION_COOKIE;
#endif
typedef HRESULT (WINAPI *PFNGETACTIVATIONFACTORY)(HSTRING, IActivationFactory **);

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI RoActivateInstance(HSTRING classid, IInspectable **instance);
HRESULT WINAPI RoGetActivationFactory(HSTRING classid, REFIID iid, void **class_factory);
HRESULT WINAPI RoInitialize(RO_INIT_TYPE type);
void WINAPI RoUninitialize(void);

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_ROAPI_H */
