/* DirectMusicScript Private Include
 *
 * Copyright (C) 2003 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __WINE_DMSCRIPT_PRIVATE_H
#define __WINE_DMSCRIPT_PRIVATE_H

#include "windef.h"
#include "wine/debug.h"
#include "winbase.h"
#include "winnt.h"
#include "dmusicc.h"
#include "dmusici.h"
#include "dmusics.h"
#include "dmplugin.h"
#include "dmusicf.h"
#include "dsound.h"

/*****************************************************************************
 * Interfaces
 */
typedef struct IDirectMusicScriptImpl IDirectMusicScriptImpl;

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirectMusicScript) DirectMusicScript_Vtbl;

/*****************************************************************************
 * ClassFactory
 *
 * can support IID_IDirectMusicScript
 * return always an IDirectMusicScriptImpl
 */
extern HRESULT WINAPI DMUSIC_CreateDirectMusicScript (LPCGUID lpcGUID, LPDIRECTMUSICSCRIPT* ppDMScript, LPUNKNOWN pUnkOuter);

/*****************************************************************************
 * IDirectMusicScriptImpl implementation structure
 */
struct IDirectMusicScriptImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectMusicScript);
  DWORD          ref;

  /* IDirectMusicScriptImpl fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectMusicScriptImpl_QueryInterface (LPDIRECTMUSICSCRIPT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI   IDirectMusicScriptImpl_AddRef (LPDIRECTMUSICSCRIPT iface);
extern ULONG WINAPI   IDirectMusicScriptImpl_Release (LPDIRECTMUSICSCRIPT iface);
/* IDirectMusicScript: */
extern HRESULT WINAPI IDirectMusicScriptImpl_Init (LPDIRECTMUSICSCRIPT iface, IDirectMusicPerformance* pPerformance, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_CallRoutine (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszRoutineName, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_SetVariableVariant (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, VARIANT varValue, BOOL fSetRef, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_GetVariableVariant (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, VARIANT* pvarValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_SetVariableNumber (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, LONG lValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_GetVariableNumber (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, LONG* plValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_SetVariableObject (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, IUnknown* punkValue, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_GetVariableObject (LPDIRECTMUSICSCRIPT iface, WCHAR* pwszVariableName, REFIID riid, LPVOID* ppv, DMUS_SCRIPT_ERRORINFO* pErrorInfo);
extern HRESULT WINAPI IDirectMusicScriptImpl_EnumRoutine (LPDIRECTMUSICSCRIPT iface, DWORD dwIndex, WCHAR* pwszName);
extern HRESULT WINAPI IDirectMusicScriptImpl_EnumVariable (LPDIRECTMUSICSCRIPT iface, DWORD dwIndex, WCHAR* pwszName);

#endif	/* __WINE_DMSCRIPT_PRIVATE_H */
