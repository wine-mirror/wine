/*
 * Copyright 2020 Dmitry Timoshkov
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

#ifndef __DSCLIENT_H_
#define __DSCLIENT_H_

DEFINE_GUID(CLSID_DsDisplaySpecifier,0x1ab4a8c0,0x6a0b,0x11d2,0xad,0x49,0x00,0xc0,0x4f,0xa3,0x1a,0x86);
#define IID_IDsDisplaySpecifier CLSID_DsDisplaySpecifier

#define DSECAF_NOTLISTED            0x00000001

typedef HRESULT (CALLBACK *LPDSENUMATTRIBUTES)(LPARAM, LPCWSTR, LPCWSTR, DWORD);

#define DSCCIF_HASWIZARDDIALOG      0x00000001
#define DSCCIF_HASWIZARDPRIMARYPAGE 0x00000002

typedef struct
{
    DWORD dwFlags;
    CLSID clsidWizardDialog;
    CLSID clsidWizardPrimaryPage;
    DWORD cWizardExtensions;
    CLSID aWizardExtensions[1];
} DSCLASSCREATIONINFO, *LPDSCLASSCREATIONINFO;

#undef INTERFACE
#define INTERFACE IDsDisplaySpecifier
DECLARE_INTERFACE_IID_(IDsDisplaySpecifier, IUnknown, "1ab4a8c0-6a0b-11d2-ad49-00c04fa31a86")
{
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, void **obj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD(SetServer)(THIS_ LPCWSTR server, LPCWSTR user, LPCWSTR password, DWORD flags) PURE;
    STDMETHOD(SetLanguageID)(THIS_ LANGID lang) PURE;
    STDMETHOD(GetDisplaySpecifier)(THIS_ LPCWSTR object, REFIID iid, void **obj) PURE;
    STDMETHOD(GetIconLocation)(THIS_ LPCWSTR object, DWORD flags, LPWSTR buffer, INT size, INT *id) PURE;
    STDMETHOD_(HICON,GetIcon)(THIS_ LPCWSTR object, DWORD flags, INT cx, INT cy) PURE;
    STDMETHOD(GetFriendlyClassName)(THIS_ LPCWSTR object, LPWSTR buffer, INT size) PURE;
    STDMETHOD(GetFriendlyAttributeName)(THIS_ LPCWSTR object, LPCWSTR name, LPWSTR buffer, UINT size) PURE;
    STDMETHOD_(BOOL,IsClassContainer)(THIS_ LPCWSTR object, LPCWSTR path, DWORD flags) PURE;
    STDMETHOD(GetClassCreationInfo)(THIS_ LPCWSTR object, LPDSCLASSCREATIONINFO *info) PURE;
    STDMETHOD(EnumClassAttributes)(THIS_ LPCWSTR object, LPDSENUMATTRIBUTES cb, LPARAM param) PURE;
    STDMETHOD_(ADSTYPE,GetAttributeADsType)(THIS_ LPCWSTR name) PURE;
};

#endif /* __DSCLIENT_H_ */
