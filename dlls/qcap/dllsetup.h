/*
 * DirectX DLL registration and unregistration
 *
 * Copyright (C) 2005 Rolf Kalbermatter
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

#ifndef _QCAP_DLLSETUP_H_DEFINED
#define _QCAP_DLLSETUP_H_DEFINED

#define COBJMACROS
#include "strmif.h"

/* Filter Setup data structures not defined in axextend.idl
    They are not part of the standard SDK headers but come from the combase.h header
    file in the DirectX Samples/Multimedia/BaseClasses */
typedef REGPINTYPES AMOVIESETUP_MEDIATYPE,
    * PAMOVIESETUP_MEDIATYPE,
    * LPAMOVIESETUP_MEDIATYPE;

typedef REGFILTERPINS AMOVIESETUP_PIN,
    * PAMOVIESETUP_PIN, 
    * LPAMOVIESETUP_PIN;

typedef struct _AMOVIESETUP_FILTER
{
    const CLSID           *clsID;
    const WCHAR           *strName;
    DWORD                 dwMerit;
    UINT                  nPins;
    const AMOVIESETUP_PIN *lpPin;
} AMOVIESETUP_FILTER, * PAMOVIESETUP_FILTER, * LPAMOVIESETUP_FILTER;

/* This needs to go into Combase.h */
typedef IUnknown *(CALLBACK *LPFNNewCOMObject)(LPUNKNOWN pUnkOuter, HRESULT *phr);
typedef void (CALLBACK *LPFNInitRoutine)(BOOL bLoading, const CLSID *rclsid);

typedef struct tagCFactoryTemplate {
    const WCHAR *m_Name;
    const CLSID *m_ClsID;
    LPFNNewCOMObject m_lpfnNew;
    LPFNInitRoutine m_lpfnInit;
    const AMOVIESETUP_FILTER *m_pAMovieSetup_Filter;
} CFactoryTemplate;

/****************************************************************************
 * SetupRegisterServers
 *
 * This function is table driven using the static members of the
 * CFactoryTemplate class defined in the Dll.
 *
 * It registers the Dll as the InprocServer32 for all the classes in
 * CFactoryTemplate
 *
 ****************************************************************************/
extern HRESULT SetupRegisterServers(const CFactoryTemplate * pList, int num, BOOL bRegister);

/****************************************************************************
 * SetupInitializeServers
 *
 * This function is table driven using the static members of the
 * CFactoryTemplate class defined in the Dll.
 *
 * It calls the initialize function for any class in CFactoryTemplate with
 * one defined.
 *
 ****************************************************************************/
extern void SetupInitializeServers(const CFactoryTemplate * pList, int num,
                                   BOOL bLoading);

#endif /* _QCAP_DLLSETUP_H_DEFINED */
