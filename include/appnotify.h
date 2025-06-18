/*
 * Copyright (C) 2023 Mohamad Al-Jaf
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

#ifndef _WINE_APISET_PSMAPPNOTIFY_H_
#define _WINE_APISET_PSMAPPNOTIFY_H_

#include <windows.h>

#ifdef _CONTRACT_GEN
#define PSM_APP_API_HOST
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(PSM_APP_API_HOST)
#define APICONTRACT
#else
#define APICONTRACT DECLSPEC_IMPORT
#endif

typedef void (__cdecl *PAPPSTATE_CHANGE_ROUTINE)(BOOLEAN quiesced, void *context);
typedef void (__cdecl *PAPPCONSTRAIN_CHANGE_ROUTINE)(BOOLEAN constrained, void *context);

typedef struct _APPSTATE_REGISTRATION *PAPPSTATE_REGISTRATION;
typedef struct _APPCONSTRAIN_REGISTRATION *PAPPCONSTRAIN_REGISTRATION;

APICONTRACT ULONG NTAPI RegisterAppConstrainedChangeNotification(PAPPCONSTRAIN_CHANGE_ROUTINE,void *,PAPPCONSTRAIN_REGISTRATION *);
APICONTRACT ULONG NTAPI RegisterAppStateChangeNotification(PAPPSTATE_CHANGE_ROUTINE,void *,PAPPSTATE_REGISTRATION *);
APICONTRACT void  NTAPI UnregisterAppConstrainedChangeNotification(PAPPCONSTRAIN_REGISTRATION);
APICONTRACT void  NTAPI UnregisterAppStateChangeNotification(PAPPSTATE_REGISTRATION);

#ifdef __cplusplus
}
#endif

#endif /* _WINE_APISET_PSMAPPNOTIFY_H_ */
