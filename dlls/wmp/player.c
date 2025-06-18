/*
 * Copyright 2014 Jacek Caban for CodeWeavers
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

#include "wmp_private.h"

#include "wine/debug.h"
#include <nserror.h>
#include "wmpids.h"
#include "shlwapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(wmp);

static ATOM player_msg_class;
static INIT_ONCE class_init_once;
static UINT WM_WMPEVENT;

static void update_state(WindowsMediaPlayer *wmp, LONG type, LONG state)
{
    DISPPARAMS dispparams;
    VARIANTARG params[1];

    dispparams.cArgs = 1;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = params;

    V_VT(params) = VT_UI4;
    V_UI4(params) = state;

    call_sink(wmp->wmpocx, type, &dispparams);
}

static inline WMPPlaylist *impl_from_IWMPPlaylist(IWMPPlaylist *iface)
{
    return CONTAINING_RECORD(iface, WMPPlaylist, IWMPPlaylist_iface);
}

static inline WMPMedia *impl_from_IWMPMedia(IWMPMedia *iface)
{
    return CONTAINING_RECORD(iface, WMPMedia, IWMPMedia_iface);
}

static inline WindowsMediaPlayer *impl_from_IWMPNetwork(IWMPNetwork *iface)
{
    return CONTAINING_RECORD(iface, WindowsMediaPlayer, IWMPNetwork_iface);
}

static inline WindowsMediaPlayer *impl_from_IWMPPlayer4(IWMPPlayer4 *iface)
{
    return CONTAINING_RECORD(iface, WindowsMediaPlayer, IWMPPlayer4_iface);
}

static inline WindowsMediaPlayer *impl_from_IWMPPlayer(IWMPPlayer *iface)
{
    return CONTAINING_RECORD(iface, WindowsMediaPlayer, IWMPPlayer_iface);
}

static inline WindowsMediaPlayer *impl_from_IWMPControls(IWMPControls *iface)
{
    return CONTAINING_RECORD(iface, WindowsMediaPlayer, IWMPControls_iface);
}

static HRESULT WINAPI WMPPlayer4_QueryInterface(IWMPPlayer4 *iface, REFIID riid, void **ppv)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    return IOleObject_QueryInterface(&This->IOleObject_iface, riid, ppv);
}

static ULONG WINAPI WMPPlayer4_AddRef(IWMPPlayer4 *iface)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    return IOleObject_AddRef(&This->IOleObject_iface);
}

static ULONG WINAPI WMPPlayer4_Release(IWMPPlayer4 *iface)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    return IOleObject_Release(&This->IOleObject_iface);
}

static HRESULT WINAPI WMPPlayer4_GetTypeInfoCount(IWMPPlayer4 *iface, UINT *pctinfo)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_GetTypeInfo(IWMPPlayer4 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%u %ld %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_GetIDsOfNames(IWMPPlayer4 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%s %p %u %ld %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_Invoke(IWMPPlayer4 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%ld %s %ld %x %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid), lcid,
          wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_close(IWMPPlayer4 *iface)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_get_URL(IWMPPlayer4 *iface, BSTR *url)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);

    TRACE("(%p)->(%p)\n", This, url);

    if (!This->media)
        return return_bstr(L"", url);

    return return_bstr(This->media->url, url);
}

static HRESULT WINAPI WMPPlayer4_put_URL(IWMPPlayer4 *iface, BSTR url)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    IWMPMedia *media;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(url));

    hres = create_media_from_url(url, 0.0, &media);

    if (SUCCEEDED(hres)) {
        update_state(This, DISPID_WMPCOREEVENT_PLAYSTATECHANGE, wmppsTransitioning);
        hres = IWMPPlayer4_put_currentMedia(iface, media);
        IWMPMedia_Release(media); /* put will addref */
    }
    if (SUCCEEDED(hres)) {
        update_state(This, DISPID_WMPCOREEVENT_PLAYSTATECHANGE, wmppsReady);
        if (This->auto_start == VARIANT_TRUE)
            IWMPControls_play(&This->IWMPControls_iface);
    }

    return hres;
}

static HRESULT WINAPI WMPPlayer4_get_openState(IWMPPlayer4 *iface, WMPOpenState *pwmpos)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, pwmpos);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_get_playState(IWMPPlayer4 *iface, WMPPlayState *pwmpps)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, pwmpps);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_get_controls(IWMPPlayer4 *iface, IWMPControls **ppControl)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);

    TRACE("(%p)->(%p)\n", This, ppControl);

    IWMPControls_AddRef(&This->IWMPControls_iface);
    *ppControl = &This->IWMPControls_iface;
    return S_OK;
}

static HRESULT WINAPI WMPPlayer4_get_settings(IWMPPlayer4 *iface, IWMPSettings **ppSettings)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);

    TRACE("(%p)->(%p)\n", This, ppSettings);

    IWMPSettings_AddRef(&This->IWMPSettings_iface);
    *ppSettings = &This->IWMPSettings_iface;
    return S_OK;
}

static HRESULT WINAPI WMPPlayer4_get_currentMedia(IWMPPlayer4 *iface, IWMPMedia **media)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);

    TRACE("(%p)->(%p)\n", This, media);

    *media = NULL;

    if (This->media == NULL)
        return S_FALSE;

    return create_media_from_url(This->media->url, This->media->duration, media);
}

static HRESULT WINAPI WMPPlayer4_put_currentMedia(IWMPPlayer4 *iface, IWMPMedia *pMedia)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    TRACE("(%p)->(%p)\n", This, pMedia);

    if(pMedia == NULL) {
        return E_POINTER;
    }
    update_state(This, DISPID_WMPCOREEVENT_OPENSTATECHANGE, wmposPlaylistChanging);
    if(This->media != NULL) {
        IWMPControls_stop(&This->IWMPControls_iface);
        IWMPMedia_Release(&This->media->IWMPMedia_iface);
    }
    update_state(This, DISPID_WMPCOREEVENT_OPENSTATECHANGE, wmposPlaylistChanged);
    update_state(This, DISPID_WMPCOREEVENT_OPENSTATECHANGE, wmposPlaylistOpenNoMedia);

    IWMPMedia_AddRef(pMedia);
    This->media = unsafe_impl_from_IWMPMedia(pMedia);
    return S_OK;
}

static HRESULT WINAPI WMPPlayer4_get_mediaCollection(IWMPPlayer4 *iface, IWMPMediaCollection **ppMediaCollection)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, ppMediaCollection);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_get_playlistCollection(IWMPPlayer4 *iface, IWMPPlaylistCollection **ppPlaylistCollection)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, ppPlaylistCollection);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_get_versionInfo(IWMPPlayer4 *iface, BSTR *version)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);

    TRACE("(%p)->(%p)\n", This, version);

    if (!version)
        return E_POINTER;

    return return_bstr(L"12.0.7601.16982", version);
}

static HRESULT WINAPI WMPPlayer4_launchURL(IWMPPlayer4 *iface, BSTR url)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(url));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_get_network(IWMPPlayer4 *iface, IWMPNetwork **ppQNI)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    TRACE("(%p)->(%p)\n", This, ppQNI);

    IWMPNetwork_AddRef(&This->IWMPNetwork_iface);
    *ppQNI = &This->IWMPNetwork_iface;
    return S_OK;
}

static HRESULT WINAPI WMPPlayer4_get_currentPlaylist(IWMPPlayer4 *iface, IWMPPlaylist **playlist)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);

    TRACE("(%p)->(%p)\n", This, playlist);

    *playlist = NULL;

    if (This->playlist == NULL)
        return S_FALSE;

    return create_playlist(This->playlist->name, This->playlist->url, This->playlist->count, playlist);
}

static HRESULT WINAPI WMPPlayer4_put_currentPlaylist(IWMPPlayer4 *iface, IWMPPlaylist *pPL)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, pPL);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_get_cdromCollection(IWMPPlayer4 *iface, IWMPCdromCollection **ppCdromCollection)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, ppCdromCollection);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_get_closedCaption(IWMPPlayer4 *iface, IWMPClosedCaption **ppClosedCaption)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, ppClosedCaption);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_get_isOnline(IWMPPlayer4 *iface, VARIANT_BOOL *pfOnline)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, pfOnline);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_get_Error(IWMPPlayer4 *iface, IWMPError **ppError)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, ppError);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_get_Status(IWMPPlayer4 *iface, BSTR *pbstrStatus)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, pbstrStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_get_dvd(IWMPPlayer4 *iface, IWMPDVD **ppDVD)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, ppDVD);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_newPlaylist(IWMPPlayer4 *iface, BSTR name, BSTR url, IWMPPlaylist **playlist)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_w(name), debugstr_w(url), playlist);

    /* FIXME: count should be the number of items in the playlist */
    return create_playlist(name, url, 0, playlist);
}

static HRESULT WINAPI WMPPlayer4_newMedia(IWMPPlayer4 *iface, BSTR url, IWMPMedia **media)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(url), media);

    return create_media_from_url(url, 0.0, media);
}

static HRESULT WINAPI WMPPlayer4_get_enabled(IWMPPlayer4 *iface, VARIANT_BOOL *pbEnabled)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, pbEnabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_put_enabled(IWMPPlayer4 *iface, VARIANT_BOOL enabled)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%x)\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_get_fullScreen(IWMPPlayer4 *iface, VARIANT_BOOL *pbFullScreen)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, pbFullScreen);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_put_fullScreen(IWMPPlayer4 *iface, VARIANT_BOOL fullscreen)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%x)\n", This, fullscreen);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_get_enableContextMenu(IWMPPlayer4 *iface, VARIANT_BOOL *pbEnableContextMenu)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, pbEnableContextMenu);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_put_enableContextMenu(IWMPPlayer4 *iface, VARIANT_BOOL enable)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%x)\n", This, enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_put_uiMode(IWMPPlayer4 *iface, BSTR mode)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(mode));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_get_uiMode(IWMPPlayer4 *iface, BSTR *pbstrMode)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, pbstrMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_get_stretchToFit(IWMPPlayer4 *iface, VARIANT_BOOL *enabled)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_put_stretchToFit(IWMPPlayer4 *iface, VARIANT_BOOL enabled)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%x)\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_get_windowlessVideo(IWMPPlayer4 *iface, VARIANT_BOOL *enabled)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_put_windowlessVideo(IWMPPlayer4 *iface, VARIANT_BOOL enabled)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%x)\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_get_isRemote(IWMPPlayer4 *iface, VARIANT_BOOL *pvarfIsRemote)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, pvarfIsRemote);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_get_playerApplication(IWMPPlayer4 *iface, IWMPPlayerApplication **ppIWMPPlayerApplication)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%p)\n", This, ppIWMPPlayerApplication);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer4_openPlayer(IWMPPlayer4 *iface, BSTR url)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer4(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(url));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlayer_QueryInterface(IWMPPlayer *iface, REFIID riid, void **ppv)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_QueryInterface(&This->IWMPPlayer4_iface, riid, ppv);
}

static ULONG WINAPI WMPPlayer_AddRef(IWMPPlayer *iface)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_AddRef(&This->IWMPPlayer4_iface);
}

static ULONG WINAPI WMPPlayer_Release(IWMPPlayer *iface)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_Release(&This->IWMPPlayer4_iface);
}

static HRESULT WINAPI WMPPlayer_GetTypeInfoCount(IWMPPlayer *iface, UINT *pctinfo)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_GetTypeInfoCount(&This->IWMPPlayer4_iface, pctinfo);
}

static HRESULT WINAPI WMPPlayer_GetTypeInfo(IWMPPlayer *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);

    return WMPPlayer4_GetTypeInfo(&This->IWMPPlayer4_iface, iTInfo,
        lcid, ppTInfo);
}

static HRESULT WINAPI WMPPlayer_GetIDsOfNames(IWMPPlayer *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_GetIDsOfNames(&This->IWMPPlayer4_iface, riid,
        rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI WMPPlayer_Invoke(IWMPPlayer *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_Invoke(&This->IWMPPlayer4_iface, dispIdMember,
        riid, lcid, wFlags, pDispParams, pVarResult,
        pExcepInfo, puArgErr);
}

static HRESULT WINAPI WMPPlayer_close(IWMPPlayer *iface)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_close(&This->IWMPPlayer4_iface);
}

static HRESULT WINAPI WMPPlayer_get_URL(IWMPPlayer *iface, BSTR *pbstrURL)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_URL(&This->IWMPPlayer4_iface, pbstrURL);
}

static HRESULT WINAPI WMPPlayer_put_URL(IWMPPlayer *iface, BSTR url)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_put_URL(&This->IWMPPlayer4_iface, url);
}

static HRESULT WINAPI WMPPlayer_get_openState(IWMPPlayer *iface, WMPOpenState *pwmpos)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_openState(&This->IWMPPlayer4_iface, pwmpos);
}

static HRESULT WINAPI WMPPlayer_get_playState(IWMPPlayer *iface, WMPPlayState *pwmpps)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_playState(&This->IWMPPlayer4_iface, pwmpps);
}

static HRESULT WINAPI WMPPlayer_get_controls(IWMPPlayer *iface, IWMPControls **ppControl)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_controls(&This->IWMPPlayer4_iface, ppControl);
}

static HRESULT WINAPI WMPPlayer_get_settings(IWMPPlayer *iface, IWMPSettings **ppSettings)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_settings(&This->IWMPPlayer4_iface, ppSettings);
}

static HRESULT WINAPI WMPPlayer_get_currentMedia(IWMPPlayer *iface, IWMPMedia **ppMedia)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_currentMedia(&This->IWMPPlayer4_iface, ppMedia);
}

static HRESULT WINAPI WMPPlayer_put_currentMedia(IWMPPlayer *iface, IWMPMedia *pMedia)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_put_currentMedia(&This->IWMPPlayer4_iface, pMedia);
}

static HRESULT WINAPI WMPPlayer_get_mediaCollection(IWMPPlayer *iface, IWMPMediaCollection **ppMediaCollection)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_mediaCollection(&This->IWMPPlayer4_iface, ppMediaCollection);
}

static HRESULT WINAPI WMPPlayer_get_playlistCollection(IWMPPlayer *iface, IWMPPlaylistCollection **ppPlaylistCollection)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_playlistCollection(&This->IWMPPlayer4_iface, ppPlaylistCollection);
}

static HRESULT WINAPI WMPPlayer_get_versionInfo(IWMPPlayer *iface, BSTR *version)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_versionInfo(&This->IWMPPlayer4_iface, version);
}

static HRESULT WINAPI WMPPlayer_launchURL(IWMPPlayer *iface, BSTR url)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_launchURL(&This->IWMPPlayer4_iface, url);
}

static HRESULT WINAPI WMPPlayer_get_network(IWMPPlayer *iface, IWMPNetwork **ppQNI)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_network(&This->IWMPPlayer4_iface, ppQNI);
}

static HRESULT WINAPI WMPPlayer_get_currentPlaylist(IWMPPlayer *iface, IWMPPlaylist **ppPL)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_currentPlaylist(&This->IWMPPlayer4_iface, ppPL);
}

static HRESULT WINAPI WMPPlayer_put_currentPlaylist(IWMPPlayer *iface, IWMPPlaylist *pPL)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_put_currentPlaylist(&This->IWMPPlayer4_iface, pPL);
}

static HRESULT WINAPI WMPPlayer_get_cdromCollection(IWMPPlayer *iface, IWMPCdromCollection **ppCdromCollection)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_cdromCollection(&This->IWMPPlayer4_iface, ppCdromCollection);
}

static HRESULT WINAPI WMPPlayer_get_closedCaption(IWMPPlayer *iface, IWMPClosedCaption **ppClosedCaption)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_closedCaption(&This->IWMPPlayer4_iface, ppClosedCaption);
}

static HRESULT WINAPI WMPPlayer_get_isOnline(IWMPPlayer *iface, VARIANT_BOOL *pfOnline)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_isOnline(&This->IWMPPlayer4_iface, pfOnline);
}

static HRESULT WINAPI WMPPlayer_get_Error(IWMPPlayer *iface, IWMPError **ppError)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_Error(&This->IWMPPlayer4_iface, ppError);
}

static HRESULT WINAPI WMPPlayer_get_Status(IWMPPlayer *iface, BSTR *pbstrStatus)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_Status(&This->IWMPPlayer4_iface, pbstrStatus);
}

static HRESULT WINAPI WMPPlayer_get_enabled(IWMPPlayer *iface, VARIANT_BOOL *pbEnabled)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_enabled(&This->IWMPPlayer4_iface, pbEnabled);
}

static HRESULT WINAPI WMPPlayer_put_enabled(IWMPPlayer *iface, VARIANT_BOOL enabled)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_put_enabled(&This->IWMPPlayer4_iface, enabled);
}

static HRESULT WINAPI WMPPlayer_get_fullScreen(IWMPPlayer *iface, VARIANT_BOOL *pbFullScreen)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_fullScreen(&This->IWMPPlayer4_iface, pbFullScreen);
}

static HRESULT WINAPI WMPPlayer_put_fullScreen(IWMPPlayer *iface, VARIANT_BOOL fullscreen)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_put_fullScreen(&This->IWMPPlayer4_iface, fullscreen);
}

static HRESULT WINAPI WMPPlayer_get_enableContextMenu(IWMPPlayer *iface, VARIANT_BOOL *pbEnableContextMenu)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_enableContextMenu(&This->IWMPPlayer4_iface, pbEnableContextMenu);
}

static HRESULT WINAPI WMPPlayer_put_enableContextMenu(IWMPPlayer *iface, VARIANT_BOOL enable)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_put_enableContextMenu(&This->IWMPPlayer4_iface, enable);
}

static HRESULT WINAPI WMPPlayer_put_uiMode(IWMPPlayer *iface, BSTR mode)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_put_uiMode(&This->IWMPPlayer4_iface, mode);
}

static HRESULT WINAPI WMPPlayer_get_uiMode(IWMPPlayer *iface, BSTR *pbstrMode)
{
    WindowsMediaPlayer *This = impl_from_IWMPPlayer(iface);
    return WMPPlayer4_get_uiMode(&This->IWMPPlayer4_iface, pbstrMode);
}

static IWMPPlayerVtbl WMPPlayerVtbl = {
    WMPPlayer_QueryInterface,
    WMPPlayer_AddRef,
    WMPPlayer_Release,
    WMPPlayer_GetTypeInfoCount,
    WMPPlayer_GetTypeInfo,
    WMPPlayer_GetIDsOfNames,
    WMPPlayer_Invoke,
    WMPPlayer_close,
    WMPPlayer_get_URL,
    WMPPlayer_put_URL,
    WMPPlayer_get_openState,
    WMPPlayer_get_playState,
    WMPPlayer_get_controls,
    WMPPlayer_get_settings,
    WMPPlayer_get_currentMedia,
    WMPPlayer_put_currentMedia,
    WMPPlayer_get_mediaCollection,
    WMPPlayer_get_playlistCollection,
    WMPPlayer_get_versionInfo,
    WMPPlayer_launchURL,
    WMPPlayer_get_network,
    WMPPlayer_get_currentPlaylist,
    WMPPlayer_put_currentPlaylist,
    WMPPlayer_get_cdromCollection,
    WMPPlayer_get_closedCaption,
    WMPPlayer_get_isOnline,
    WMPPlayer_get_Error,
    WMPPlayer_get_Status,
    WMPPlayer_get_enabled,
    WMPPlayer_put_enabled,
    WMPPlayer_get_fullScreen,
    WMPPlayer_put_fullScreen,
    WMPPlayer_get_enableContextMenu,
    WMPPlayer_put_enableContextMenu,
    WMPPlayer_put_uiMode,
    WMPPlayer_get_uiMode,
};


static IWMPPlayer4Vtbl WMPPlayer4Vtbl = {
    WMPPlayer4_QueryInterface,
    WMPPlayer4_AddRef,
    WMPPlayer4_Release,
    WMPPlayer4_GetTypeInfoCount,
    WMPPlayer4_GetTypeInfo,
    WMPPlayer4_GetIDsOfNames,
    WMPPlayer4_Invoke,
    WMPPlayer4_close,
    WMPPlayer4_get_URL,
    WMPPlayer4_put_URL,
    WMPPlayer4_get_openState,
    WMPPlayer4_get_playState,
    WMPPlayer4_get_controls,
    WMPPlayer4_get_settings,
    WMPPlayer4_get_currentMedia,
    WMPPlayer4_put_currentMedia,
    WMPPlayer4_get_mediaCollection,
    WMPPlayer4_get_playlistCollection,
    WMPPlayer4_get_versionInfo,
    WMPPlayer4_launchURL,
    WMPPlayer4_get_network,
    WMPPlayer4_get_currentPlaylist,
    WMPPlayer4_put_currentPlaylist,
    WMPPlayer4_get_cdromCollection,
    WMPPlayer4_get_closedCaption,
    WMPPlayer4_get_isOnline,
    WMPPlayer4_get_Error,
    WMPPlayer4_get_Status,
    WMPPlayer4_get_dvd,
    WMPPlayer4_newPlaylist,
    WMPPlayer4_newMedia,
    WMPPlayer4_get_enabled,
    WMPPlayer4_put_enabled,
    WMPPlayer4_get_fullScreen,
    WMPPlayer4_put_fullScreen,
    WMPPlayer4_get_enableContextMenu,
    WMPPlayer4_put_enableContextMenu,
    WMPPlayer4_put_uiMode,
    WMPPlayer4_get_uiMode,
    WMPPlayer4_get_stretchToFit,
    WMPPlayer4_put_stretchToFit,
    WMPPlayer4_get_windowlessVideo,
    WMPPlayer4_put_windowlessVideo,
    WMPPlayer4_get_isRemote,
    WMPPlayer4_get_playerApplication,
    WMPPlayer4_openPlayer
};

static inline WindowsMediaPlayer *impl_from_IWMPSettings(IWMPSettings *iface)
{
    return CONTAINING_RECORD(iface, WindowsMediaPlayer, IWMPSettings_iface);
}

static HRESULT WINAPI WMPSettings_QueryInterface(IWMPSettings *iface, REFIID riid, void **ppv)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    return IOleObject_QueryInterface(&This->IOleObject_iface, riid, ppv);
}

static ULONG WINAPI WMPSettings_AddRef(IWMPSettings *iface)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    return IOleObject_AddRef(&This->IOleObject_iface);
}

static ULONG WINAPI WMPSettings_Release(IWMPSettings *iface)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    return IOleObject_Release(&This->IOleObject_iface);
}

static HRESULT WINAPI WMPSettings_GetTypeInfoCount(IWMPSettings *iface, UINT *pctinfo)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPSettings_GetTypeInfo(IWMPSettings *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    FIXME("(%p)->(%u %ld %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPSettings_GetIDsOfNames(IWMPSettings *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    FIXME("(%p)->(%s %p %u %ld %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPSettings_Invoke(IWMPSettings *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    FIXME("(%p)->(%ld %s %ld %x %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid), lcid,
          wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPSettings_get_isAvailable(IWMPSettings *iface, BSTR item, VARIANT_BOOL *p)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(item), p);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPSettings_get_autoStart(IWMPSettings *iface, VARIANT_BOOL *p)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    TRACE("(%p)->(%p)\n", This, p);
    if (!p)
        return E_POINTER;
    *p = This->auto_start;
    return S_OK;
}

static HRESULT WINAPI WMPSettings_put_autoStart(IWMPSettings *iface, VARIANT_BOOL v)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    TRACE("(%p)->(%x)\n", This, v);
    This->auto_start = v;
    return S_OK;
}

static HRESULT WINAPI WMPSettings_get_baseURL(IWMPSettings *iface, BSTR *p)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPSettings_put_baseURL(IWMPSettings *iface, BSTR v)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPSettings_get_defaultFrame(IWMPSettings *iface, BSTR *p)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPSettings_put_defaultFrame(IWMPSettings *iface, BSTR v)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPSettings_get_invokeURLs(IWMPSettings *iface, VARIANT_BOOL *p)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if (!p)
        return E_POINTER;
    *p = This->invoke_urls;
    return S_OK;
}

static HRESULT WINAPI WMPSettings_put_invokeURLs(IWMPSettings *iface, VARIANT_BOOL v)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    /* Leaving as FIXME as we don't currently use this */
    FIXME("(%p)->(%x)\n", This, v);
    This->invoke_urls = v;
    return S_OK;
}

static HRESULT WINAPI WMPSettings_get_mute(IWMPSettings *iface, VARIANT_BOOL *p)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPSettings_put_mute(IWMPSettings *iface, VARIANT_BOOL v)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPSettings_get_playCount(IWMPSettings *iface, LONG *p)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPSettings_put_playCount(IWMPSettings *iface, LONG v)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPSettings_get_rate(IWMPSettings *iface, double *p)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPSettings_put_rate(IWMPSettings *iface, double v)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    FIXME("(%p)->(%lf)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPSettings_get_balance(IWMPSettings *iface, LONG *p)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPSettings_put_balance(IWMPSettings *iface, LONG v)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPSettings_get_volume(IWMPSettings *iface, LONG *p)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    TRACE("(%p)->(%p)\n", This, p);
    if (!p)
        return E_POINTER;
    *p = This->volume;
    return S_OK;
}

static HRESULT WINAPI WMPSettings_put_volume(IWMPSettings *iface, LONG v)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    TRACE("(%p)->(%ld)\n", This, v);
    This->volume = v;
    if (!This->filter_graph)
        return S_OK;

    /* IBasicAudio -   [-10000, 0], wmp - [0, 100] */
    v = 10000 * v / 100 - 10000;
    if (!This->basic_audio)
        return S_FALSE;

    return IBasicAudio_put_Volume(This->basic_audio, v);
}

static HRESULT WINAPI WMPSettings_getMode(IWMPSettings *iface, BSTR mode, VARIANT_BOOL *p)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(mode), p);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPSettings_setMode(IWMPSettings *iface, BSTR mode, VARIANT_BOOL v)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    FIXME("(%p)->(%s %x)\n", This, debugstr_w(mode), v);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPSettings_get_enableErrorDialogs(IWMPSettings *iface, VARIANT_BOOL *p)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if (!p)
        return E_POINTER;
    *p = This->enable_error_dialogs;
    return S_OK;
}

static HRESULT WINAPI WMPSettings_put_enableErrorDialogs(IWMPSettings *iface, VARIANT_BOOL v)
{
    WindowsMediaPlayer *This = impl_from_IWMPSettings(iface);
    /* Leaving as FIXME as we don't currently use this */
    FIXME("(%p)->(%x)\n", This, v);
    This->enable_error_dialogs = v;
    return S_OK;
}

static const IWMPSettingsVtbl WMPSettingsVtbl = {
    WMPSettings_QueryInterface,
    WMPSettings_AddRef,
    WMPSettings_Release,
    WMPSettings_GetTypeInfoCount,
    WMPSettings_GetTypeInfo,
    WMPSettings_GetIDsOfNames,
    WMPSettings_Invoke,
    WMPSettings_get_isAvailable,
    WMPSettings_get_autoStart,
    WMPSettings_put_autoStart,
    WMPSettings_get_baseURL,
    WMPSettings_put_baseURL,
    WMPSettings_get_defaultFrame,
    WMPSettings_put_defaultFrame,
    WMPSettings_get_invokeURLs,
    WMPSettings_put_invokeURLs,
    WMPSettings_get_mute,
    WMPSettings_put_mute,
    WMPSettings_get_playCount,
    WMPSettings_put_playCount,
    WMPSettings_get_rate,
    WMPSettings_put_rate,
    WMPSettings_get_balance,
    WMPSettings_put_balance,
    WMPSettings_get_volume,
    WMPSettings_put_volume,
    WMPSettings_getMode,
    WMPSettings_setMode,
    WMPSettings_get_enableErrorDialogs,
    WMPSettings_put_enableErrorDialogs
};

static HRESULT WINAPI WMPNetwork_QueryInterface(IWMPNetwork *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IDispatch)) {
        *ppv = iface;
    }else if(IsEqualGUID(riid, &IID_IWMPNetwork)) {
        *ppv = iface;
    }else {
        WARN("Unsupported interface (%s)\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI WMPNetwork_AddRef(IWMPNetwork *iface)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    return IOleObject_AddRef(&This->IOleObject_iface);
}

static ULONG WINAPI WMPNetwork_Release(IWMPNetwork *iface)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    return IOleObject_Release(&This->IOleObject_iface);
}

static HRESULT WINAPI WMPNetwork_GetTypeInfoCount(IWMPNetwork *iface, UINT *pctinfo)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_GetTypeInfo(IWMPNetwork *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%u %ld %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_GetIDsOfNames(IWMPNetwork *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%s %p %u %ld %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_Invoke(IWMPNetwork *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%ld %s %ld %x %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid), lcid,
          wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_get_bandWidth(IWMPNetwork *iface, LONG *plBandwidth)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%p)\n", This, plBandwidth);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_get_recoveredPackets(IWMPNetwork *iface, LONG *plRecoveredPackets)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%p)\n", This, plRecoveredPackets);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_get_sourceProtocol(IWMPNetwork *iface, BSTR *pbstrSourceProtocol)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%p)\n", This, pbstrSourceProtocol);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_get_receivedPackets(IWMPNetwork *iface, LONG *plReceivedPackets)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%p)\n", This, plReceivedPackets);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_get_lostPackets(IWMPNetwork *iface, LONG *plLostPackets)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%p)\n", This, plLostPackets);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_get_receptionQuality(IWMPNetwork *iface, LONG *plReceptionQuality)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%p)\n", This, plReceptionQuality);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_get_bufferingCount(IWMPNetwork *iface, LONG *plBufferingCount)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%p)\n", This, plBufferingCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_get_bufferingProgress(IWMPNetwork *iface, LONG *plBufferingProgress)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    TRACE("(%p)->(%p)\n", This, plBufferingProgress);
    if (!This->filter_graph) {
        return S_FALSE;
    }
    /* Ideally we would use IAMOpenProgress for URL reader but we don't have it in wine (yet)
     * For file sources FileAsyncReader->Length should work
     * */
    FIXME("stub: Returning buffering progress 100\n");
    *plBufferingProgress = 100;

    return S_OK;
}

static HRESULT WINAPI WMPNetwork_get_bufferingTime(IWMPNetwork *iface, LONG *plBufferingTime)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%p)\n", This, plBufferingTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_put_bufferingTime(IWMPNetwork *iface, LONG lBufferingTime)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%ld)\n", This, lBufferingTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_get_frameRate(IWMPNetwork *iface, LONG *plFrameRate)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%p)\n", This, plFrameRate);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_get_maxBitRate(IWMPNetwork *iface, LONG *plBitRate)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%p)\n", This, plBitRate);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_get_bitRate(IWMPNetwork *iface, LONG *plBitRate)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%p)\n", This, plBitRate);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_getProxySettings(IWMPNetwork *iface, BSTR bstrProtocol, LONG *plProxySetting)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%s, %p)\n", This, debugstr_w(bstrProtocol), plProxySetting);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_setProxySettings(IWMPNetwork *iface, BSTR bstrProtocol, LONG lProxySetting)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%s, %ld)\n", This, debugstr_w(bstrProtocol), lProxySetting);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_getProxyName(IWMPNetwork *iface, BSTR bstrProtocol, BSTR *pbstrProxyName)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%s, %p)\n", This, debugstr_w(bstrProtocol), pbstrProxyName);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_setProxyName(IWMPNetwork *iface, BSTR bstrProtocol, BSTR bstrProxyName)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%s, %s)\n", This, debugstr_w(bstrProtocol), debugstr_w(bstrProxyName));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_getProxyPort(IWMPNetwork *iface, BSTR bstrProtocol, LONG *plProxyPort)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%s, %p)\n", This, debugstr_w(bstrProtocol), plProxyPort);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_setProxyPort(IWMPNetwork *iface, BSTR bstrProtocol, LONG lProxyPort)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%s, %ld)\n", This, debugstr_w(bstrProtocol), lProxyPort);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_getProxyExceptionList(IWMPNetwork *iface, BSTR bstrProtocol, BSTR *pbstrExceptionList)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%s, %p)\n", This, debugstr_w(bstrProtocol), pbstrExceptionList);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_setProxyExceptionList(IWMPNetwork *iface, BSTR bstrProtocol, BSTR bstrExceptionList)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%s, %s)\n", This, debugstr_w(bstrProtocol), debugstr_w(bstrExceptionList));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_getProxyBypassForLocal(IWMPNetwork *iface, BSTR bstrProtocol, VARIANT_BOOL *pfBypassForLocal)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%s, %p)\n", This, debugstr_w(bstrProtocol), pfBypassForLocal);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_setProxyBypassForLocal(IWMPNetwork *iface, BSTR bstrProtocol, VARIANT_BOOL fBypassForLocal)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%s, %d)\n", This, debugstr_w(bstrProtocol), fBypassForLocal);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_get_maxBandwidth(IWMPNetwork *iface, LONG *plMaxBandwidth)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%p)\n", This, plMaxBandwidth);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_put_maxBandwidth(IWMPNetwork *iface, LONG lMaxBandwidth)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%ld)\n", This, lMaxBandwidth);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_get_downloadProgress(IWMPNetwork *iface, LONG *plDownloadProgress)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    TRACE("(%p)->(%p)\n", This, plDownloadProgress);
    if (!This->filter_graph) {
        return S_FALSE;
    }
    /* Ideally we would use IAMOpenProgress for URL reader but we don't have it in wine (yet)
     * For file sources FileAsyncReader->Length could work or it should just be
     * 100
     * */
    FIXME("stub: Returning download progress 100\n");
    *plDownloadProgress = 100;

    return S_OK;
}

static HRESULT WINAPI WMPNetwork_get_encodedFrameRate(IWMPNetwork *iface, LONG *plFrameRate)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%p)\n", This, plFrameRate);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPNetwork_get_framesSkipped(IWMPNetwork *iface, LONG *plFrames)
{
    WindowsMediaPlayer *This = impl_from_IWMPNetwork(iface);
    FIXME("(%p)->(%p)\n", This, plFrames);
    return E_NOTIMPL;
}

static const IWMPNetworkVtbl WMPNetworkVtbl = {
    WMPNetwork_QueryInterface,
    WMPNetwork_AddRef,
    WMPNetwork_Release,
    WMPNetwork_GetTypeInfoCount,
    WMPNetwork_GetTypeInfo,
    WMPNetwork_GetIDsOfNames,
    WMPNetwork_Invoke,
    WMPNetwork_get_bandWidth,
    WMPNetwork_get_recoveredPackets,
    WMPNetwork_get_sourceProtocol,
    WMPNetwork_get_receivedPackets,
    WMPNetwork_get_lostPackets,
    WMPNetwork_get_receptionQuality,
    WMPNetwork_get_bufferingCount,
    WMPNetwork_get_bufferingProgress,
    WMPNetwork_get_bufferingTime,
    WMPNetwork_put_bufferingTime,
    WMPNetwork_get_frameRate,
    WMPNetwork_get_maxBitRate,
    WMPNetwork_get_bitRate,
    WMPNetwork_getProxySettings,
    WMPNetwork_setProxySettings,
    WMPNetwork_getProxyName,
    WMPNetwork_setProxyName,
    WMPNetwork_getProxyPort,
    WMPNetwork_setProxyPort,
    WMPNetwork_getProxyExceptionList,
    WMPNetwork_setProxyExceptionList,
    WMPNetwork_getProxyBypassForLocal,
    WMPNetwork_setProxyBypassForLocal,
    WMPNetwork_get_maxBandwidth,
    WMPNetwork_put_maxBandwidth,
    WMPNetwork_get_downloadProgress,
    WMPNetwork_get_encodedFrameRate,
    WMPNetwork_get_framesSkipped,
};

static HRESULT WINAPI WMPControls_QueryInterface(IWMPControls *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown)) {
        *ppv = iface;
    }else if(IsEqualGUID(riid, &IID_IDispatch)) {
        *ppv = iface;
    }else if(IsEqualGUID(riid, &IID_IWMPControls)) {
        *ppv = iface;
    }else {
        WARN("Unsupported interface (%s)\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI WMPControls_AddRef(IWMPControls *iface)
{
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    return IOleObject_AddRef(&This->IOleObject_iface);
}

static ULONG WINAPI WMPControls_Release(IWMPControls *iface)
{
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    return IOleObject_Release(&This->IOleObject_iface);
}

static HRESULT WINAPI WMPControls_GetTypeInfoCount(IWMPControls *iface, UINT *pctinfo)
{
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPControls_GetTypeInfo(IWMPControls *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    FIXME("(%p)->(%u %ld %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPControls_GetIDsOfNames(IWMPControls *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    FIXME("(%p)->(%s %p %u %ld %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPControls_Invoke(IWMPControls *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    FIXME("(%p)->(%ld %s %ld %x %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid), lcid,
          wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPControls_get_isAvailable(IWMPControls *iface, BSTR bstrItem, VARIANT_BOOL *pIsAvailable)
{
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(bstrItem), pIsAvailable);
    if (!This->filter_graph) {
        *pIsAvailable = VARIANT_FALSE;
    } else if (wcscmp(L"currentPosition", bstrItem) == 0) {
        DWORD capabilities;
        IMediaSeeking_GetCapabilities(This->media_seeking, &capabilities);
        *pIsAvailable = (capabilities & AM_SEEKING_CanSeekAbsolute) ?
            VARIANT_TRUE : VARIANT_FALSE;
    } else {
        FIXME("%s not implemented\n", debugstr_w(bstrItem));
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI WMPControls_play(IWMPControls *iface)
{
    HRESULT hres = S_OK;
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);

    TRACE("(%p)\n", This);

    if (!This->media) {
        return NS_S_WMPCORE_COMMAND_NOT_AVAILABLE;
    }

    if (!This->filter_graph) {
        hres = CoCreateInstance(&CLSID_FilterGraph,
                NULL,
                CLSCTX_INPROC_SERVER,
                &IID_IGraphBuilder,
                (void **)&This->filter_graph);
        update_state(This, DISPID_WMPCOREEVENT_OPENSTATECHANGE, wmposOpeningUnknownURL);

        if (SUCCEEDED(hres))
            hres = IGraphBuilder_RenderFile(This->filter_graph, This->media->url, NULL);
        if (SUCCEEDED(hres))
            update_state(This, DISPID_WMPCOREEVENT_OPENSTATECHANGE, wmposMediaOpen);
        if (SUCCEEDED(hres))
            hres = IGraphBuilder_QueryInterface(This->filter_graph, &IID_IMediaControl,
                    (void**)&This->media_control);
        if (SUCCEEDED(hres))
            hres = IGraphBuilder_QueryInterface(This->filter_graph, &IID_IMediaSeeking,
                    (void**)&This->media_seeking);
        if (SUCCEEDED(hres))
            hres = IMediaSeeking_SetTimeFormat(This->media_seeking, &TIME_FORMAT_MEDIA_TIME);
        if (SUCCEEDED(hres))
            hres = IGraphBuilder_QueryInterface(This->filter_graph, &IID_IMediaEvent,
                    (void**)&This->media_event);
        if (SUCCEEDED(hres))
        {
            IMediaEventEx *media_event_ex = NULL;
            hres = IGraphBuilder_QueryInterface(This->filter_graph, &IID_IMediaEventEx,
                    (void**)&media_event_ex);
            if (SUCCEEDED(hres)) {
                hres = IMediaEventEx_SetNotifyWindow(media_event_ex, (OAHWND)This->msg_window,
                        WM_WMPEVENT, (LONG_PTR)This);
                IMediaEventEx_Release(media_event_ex);
            }
        }
        if (SUCCEEDED(hres))
            hres = IGraphBuilder_QueryInterface(This->filter_graph, &IID_IBasicAudio, (void**)&This->basic_audio);
        if (SUCCEEDED(hres))
            hres = IWMPSettings_put_volume(&This->IWMPSettings_iface, This->volume);

        if (FAILED(hres))
        {
            if (This->filter_graph)
            {
                IGraphBuilder_Release(This->filter_graph);
                This->filter_graph = NULL;
            }
            if (This->media_control)
            {
                IMediaControl_Release(This->media_control);
                This->media_control = NULL;
            }
            if (This->media_seeking)
            {
                IMediaSeeking_Release(This->media_seeking);
                This->media_seeking = NULL;
            }
            if (This->media_event)
            {
                IMediaEvent_Release(This->media_event);
                This->media_event = NULL;
            }
            if (This->basic_audio)
            {
                IBasicAudio_Release(This->basic_audio);
                This->basic_audio = NULL;
            }
        }
    }

    update_state(This, DISPID_WMPCOREEVENT_PLAYSTATECHANGE, wmppsTransitioning);

    if (SUCCEEDED(hres))
        hres = IMediaControl_Run(This->media_control);

    if (hres == S_FALSE) {
        hres = S_OK; /* S_FALSE will mean that graph is transitioning and that is fine */
    }

    if (SUCCEEDED(hres)) {
        LONGLONG duration;
        update_state(This, DISPID_WMPCOREEVENT_PLAYSTATECHANGE, wmppsPlaying);
        if (SUCCEEDED(IMediaSeeking_GetDuration(This->media_seeking, &duration)))
            This->media->duration = (DOUBLE)duration / 10000000.0f;
    } else {
        update_state(This, DISPID_WMPCOREEVENT_PLAYSTATECHANGE, wmppsUndefined);
    }

    return hres;
}

static HRESULT WINAPI WMPControls_stop(IWMPControls *iface)
{
    HRESULT hres = S_OK;
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    TRACE("(%p)\n", This);
    if (!This->filter_graph) {
        return NS_S_WMPCORE_COMMAND_NOT_AVAILABLE;
    }
    if (This->media_control) {
        hres = IMediaControl_Stop(This->media_control);
        IMediaControl_Release(This->media_control);
    }
    if (This->media_event) {
        IMediaEvent_Release(This->media_event);
    }
    if (This->media_seeking) {
        IMediaSeeking_Release(This->media_seeking);
    }
    if (This->basic_audio) {
        IBasicAudio_Release(This->basic_audio);
    }
    IGraphBuilder_Release(This->filter_graph);
    This->filter_graph = NULL;
    This->media_control = NULL;
    This->media_event = NULL;
    This->media_seeking = NULL;
    This->basic_audio = NULL;

    update_state(This, DISPID_WMPCOREEVENT_OPENSTATECHANGE, wmposPlaylistOpenNoMedia);
    update_state(This, DISPID_WMPCOREEVENT_PLAYSTATECHANGE, wmppsStopped);
    return hres;
}

static HRESULT WINAPI WMPControls_pause(IWMPControls *iface)
{
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPControls_fastForward(IWMPControls *iface)
{
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPControls_fastReverse(IWMPControls *iface)
{
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPControls_get_currentPosition(IWMPControls *iface, DOUBLE *pdCurrentPosition)
{
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    HRESULT hres;
    LONGLONG currentPosition;

    TRACE("(%p)->(%p)\n", This, pdCurrentPosition);
    if (!This->media_seeking)
        return S_FALSE;

    hres = IMediaSeeking_GetCurrentPosition(This->media_seeking, &currentPosition);
    *pdCurrentPosition = (DOUBLE) currentPosition / 10000000.0f;
    TRACE("hres: %ld, pos: %f\n", hres, *pdCurrentPosition);
    return hres;
}

static HRESULT WINAPI WMPControls_put_currentPosition(IWMPControls *iface, DOUBLE dCurrentPosition)
{
    LONGLONG Current;
    HRESULT hres;
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    TRACE("(%p)->(%f)\n", This, dCurrentPosition);
    if (!This->media_seeking)
        return S_FALSE;

    Current = 10000000 * dCurrentPosition;
    hres = IMediaSeeking_SetPositions(This->media_seeking, &Current,
            AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);

    return hres;
}

static HRESULT WINAPI WMPControls_get_currentPositionString(IWMPControls *iface, BSTR *pbstrCurrentPosition)
{
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    FIXME("(%p)->(%p)\n", This, pbstrCurrentPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPControls_next(IWMPControls *iface)
{
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPControls_previous(IWMPControls *iface)
{
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPControls_get_currentItem(IWMPControls *iface, IWMPMedia **ppIWMPMedia)
{
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    FIXME("(%p)->(%p)\n", This, ppIWMPMedia);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPControls_put_currentItem(IWMPControls *iface, IWMPMedia *pIWMPMedia)
{
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    FIXME("(%p)->(%p)\n", This, pIWMPMedia);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPControls_get_currentMarker(IWMPControls *iface, LONG *plMarker)
{
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    FIXME("(%p)->(%p)\n", This, plMarker);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPControls_put_currentMarker(IWMPControls *iface, LONG lMarker)
{
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    FIXME("(%p)->(%ld)\n", This, lMarker);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPControls_playItem(IWMPControls *iface, IWMPMedia *pIWMPMedia)
{
    WindowsMediaPlayer *This = impl_from_IWMPControls(iface);
    FIXME("(%p)->(%p)\n", This, pIWMPMedia);
    return E_NOTIMPL;
}

static const IWMPControlsVtbl WMPControlsVtbl = {
    WMPControls_QueryInterface,
    WMPControls_AddRef,
    WMPControls_Release,
    WMPControls_GetTypeInfoCount,
    WMPControls_GetTypeInfo,
    WMPControls_GetIDsOfNames,
    WMPControls_Invoke,
    WMPControls_get_isAvailable,
    WMPControls_play,
    WMPControls_stop,
    WMPControls_pause,
    WMPControls_fastForward,
    WMPControls_fastReverse,
    WMPControls_get_currentPosition,
    WMPControls_put_currentPosition,
    WMPControls_get_currentPositionString,
    WMPControls_next,
    WMPControls_previous,
    WMPControls_get_currentItem,
    WMPControls_put_currentItem,
    WMPControls_get_currentMarker,
    WMPControls_put_currentMarker,
    WMPControls_playItem,
};

static HRESULT WINAPI WMPMedia_QueryInterface(IWMPMedia *iface, REFIID riid, void **ppv)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    TRACE("(%p)\n", This);
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IWMPMedia_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IWMPMedia_iface;
    }else if(IsEqualGUID(&IID_IWMPMedia, riid)) {
        TRACE("(%p)->(IID_IWMPMedia %p)\n", This, ppv);
        *ppv = &This->IWMPMedia_iface;
    }else {
        WARN("Unsupported interface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI WMPMedia_AddRef(IWMPMedia *iface)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI WMPMedia_Release(IWMPMedia *iface)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        free(This->url);
        free(This->name);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI WMPMedia_GetTypeInfoCount(IWMPMedia *iface, UINT *pctinfo)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPMedia_GetTypeInfo(IWMPMedia *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    FIXME("(%p)->(%u %ld %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPMedia_GetIDsOfNames(IWMPMedia *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    FIXME("(%p)->(%s %p %u %ld %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPMedia_Invoke(IWMPMedia *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    FIXME("(%p)->(%ld %s %ld %x %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid), lcid,
          wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPMedia_get_isIdentical(IWMPMedia *iface, IWMPMedia *other, VARIANT_BOOL *pvBool)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    FIXME("(%p)->(%p, %p)\n", This, other, pvBool);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPMedia_get_sourceURL(IWMPMedia *iface, BSTR *url)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);

    TRACE("(%p)->(%p)\n", This, url);

    return return_bstr(This->url, url);
}

static HRESULT WINAPI WMPMedia_get_name(IWMPMedia *iface, BSTR *name)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);

    TRACE("(%p)->(%p)\n", This, name);

    return return_bstr(This->name, name);
}

static HRESULT WINAPI WMPMedia_put_name(IWMPMedia *iface, BSTR name)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(name));

    if (!name) return E_POINTER;

    free(This->name);
    This->name = wcsdup(name);
    return S_OK;
}

static HRESULT WINAPI WMPMedia_get_imageSourceWidth(IWMPMedia *iface, LONG *pWidth)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    FIXME("(%p)->(%p)\n", This, pWidth);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPMedia_get_imageSourceHeight(IWMPMedia *iface, LONG *pHeight)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    FIXME("(%p)->(%p)\n", This, pHeight);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPMedia_get_markerCount(IWMPMedia *iface, LONG* pMarkerCount)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    FIXME("(%p)->(%p)\n", This, pMarkerCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPMedia_getMarkerTime(IWMPMedia *iface, LONG MarkerNum, DOUBLE *pMarkerTime)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    FIXME("(%p)->(%ld, %p)\n", This, MarkerNum, pMarkerTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPMedia_getMarkerName(IWMPMedia *iface, LONG MarkerNum, BSTR *pbstrMarkerName)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    FIXME("(%p)->(%ld, %p)\n", This, MarkerNum, pbstrMarkerName);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPMedia_get_duration(IWMPMedia *iface, DOUBLE *pDuration)
{
    /* MSDN: If this property is used with a media item other than the one
     * specified in Player.currentMedia, it may not contain a valid value. */
    WMPMedia *This = impl_from_IWMPMedia(iface);
    TRACE("(%p)->(%p)\n", This, pDuration);
    *pDuration = This->duration;
    return S_OK;
}

static HRESULT WINAPI WMPMedia_get_durationString(IWMPMedia *iface, BSTR *pbstrDuration)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    FIXME("(%p)->(%p)\n", This, pbstrDuration);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPMedia_get_attributeCount(IWMPMedia *iface, LONG *plCount)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    FIXME("(%p)->(%p)\n", This, plCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPMedia_getAttributeName(IWMPMedia *iface, LONG lIndex, BSTR *pbstrItemName)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    FIXME("(%p)->(%ld, %p)\n", This, lIndex, pbstrItemName);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPMedia_getItemInfo(IWMPMedia *iface, BSTR item_name, BSTR *value)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    FIXME("(%p)->(%s, %p)\n", This, debugstr_w(item_name), value);
    return return_bstr(NULL, value);
}

static HRESULT WINAPI WMPMedia_setItemInfo(IWMPMedia *iface, BSTR bstrItemName, BSTR bstrVal)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    FIXME("(%p)->(%s, %s)\n", This, debugstr_w(bstrItemName), debugstr_w(bstrVal));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPMedia_getItemInfoByAtom(IWMPMedia *iface, LONG lAtom, BSTR *pbstrVal)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    FIXME("(%p)->(%ld, %p)\n", This, lAtom, pbstrVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPMedia_isMemberOf(IWMPMedia *iface, IWMPPlaylist *pPlaylist, VARIANT_BOOL *pvarfIsMemberOf)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    FIXME("(%p)->(%p, %p)\n", This, pPlaylist, pvarfIsMemberOf);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPMedia_isReadOnlyItem(IWMPMedia *iface, BSTR bstrItemName, VARIANT_BOOL *pvarfIsReadOnly)
{
    WMPMedia *This = impl_from_IWMPMedia(iface);
    FIXME("(%p)->(%s, %p)\n", This, debugstr_w(bstrItemName), pvarfIsReadOnly);
    return E_NOTIMPL;
}

static const IWMPMediaVtbl WMPMediaVtbl = {
    WMPMedia_QueryInterface,
    WMPMedia_AddRef,
    WMPMedia_Release,
    WMPMedia_GetTypeInfoCount,
    WMPMedia_GetTypeInfo,
    WMPMedia_GetIDsOfNames,
    WMPMedia_Invoke,
    WMPMedia_get_isIdentical,
    WMPMedia_get_sourceURL,
    WMPMedia_get_name,
    WMPMedia_put_name,
    WMPMedia_get_imageSourceWidth,
    WMPMedia_get_imageSourceHeight,
    WMPMedia_get_markerCount,
    WMPMedia_getMarkerTime,
    WMPMedia_getMarkerName,
    WMPMedia_get_duration,
    WMPMedia_get_durationString,
    WMPMedia_get_attributeCount,
    WMPMedia_getAttributeName,
    WMPMedia_getItemInfo,
    WMPMedia_setItemInfo,
    WMPMedia_getItemInfoByAtom,
    WMPMedia_isMemberOf,
    WMPMedia_isReadOnlyItem
};

static HRESULT WINAPI WMPPlaylist_QueryInterface(IWMPPlaylist *iface, REFIID riid, void **ppv)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);
    TRACE("(%p)\n", This);
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IWMPPlaylist_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IWMPPlaylist_iface;
    }else if(IsEqualGUID(&IID_IWMPPlaylist, riid)) {
        TRACE("(%p)->(IID_IWMPPlaylist %p)\n", This, ppv);
        *ppv = &This->IWMPPlaylist_iface;
    }else {
        WARN("Unsupported interface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI WMPPlaylist_AddRef(IWMPPlaylist *iface)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI WMPPlaylist_Release(IWMPPlaylist *iface)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        free(This->url);
        free(This->name);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI WMPPlaylist_GetTypeInfoCount(IWMPPlaylist *iface, UINT *pctinfo)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlaylist_GetTypeInfo(IWMPPlaylist *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);
    FIXME("(%p)->(%u %ld %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlaylist_GetIDsOfNames(IWMPPlaylist *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);
    FIXME("(%p)->(%s %p %u %ld %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlaylist_Invoke(IWMPPlaylist *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);
    FIXME("(%p)->(%ld %s %ld %x %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid), lcid,
          wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlaylist_get_count(IWMPPlaylist *iface, LONG *count)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);

    TRACE("(%p)->(%p)\n", This, count);

    if (!count) return E_POINTER;
    *count = This->count;

    return S_OK;
}

static HRESULT WINAPI WMPPlaylist_get_name(IWMPPlaylist *iface, BSTR *name)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);

    TRACE("(%p)->(%p)\n", This, name);

    return return_bstr(This->name, name);
}

static HRESULT WINAPI WMPPlaylist_put_name(IWMPPlaylist *iface, BSTR name)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(name));

    if (!name) return E_POINTER;

    free(This->name);
    This->name = wcsdup(name);
    return S_OK;
}

static HRESULT WINAPI WMPPlaylist_get_attributeCount(IWMPPlaylist *iface, LONG *count)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);
    FIXME("(%p)->(%p)\n", This, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlaylist_get_attributeName(IWMPPlaylist *iface, LONG index, BSTR *attribute_name)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);
    FIXME("(%p)->(%ld %p)\n", This, index, attribute_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlaylist_get_Item(IWMPPlaylist *iface, LONG index, IWMPMedia **media)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);
    FIXME("(%p)->(%ld %p)\n", This, index, media);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlaylist_getItemInfo(IWMPPlaylist *iface, BSTR name, BSTR *value)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(name), value);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlaylist_setItemInfo(IWMPPlaylist *iface, BSTR name, BSTR value)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(name), debugstr_w(value));
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlaylist_get_isIdentical(IWMPPlaylist *iface, IWMPPlaylist *playlist, VARIANT_BOOL *var)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);
    FIXME("(%p)->(%p %p)\n", This, playlist, var);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlaylist_clear(IWMPPlaylist *iface)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlaylist_insertItem(IWMPPlaylist *iface, LONG index, IWMPMedia *media)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);
    FIXME("(%p)->(%ld %p)\n", This, index, media);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlaylist_appendItem(IWMPPlaylist *iface, IWMPMedia *media)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);
    FIXME("(%p)->(%p)\n", This, media);
    return S_OK;
}

static HRESULT WINAPI WMPPlaylist_removeItem(IWMPPlaylist *iface, IWMPMedia *media)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);
    FIXME("(%p)->(%p)\n", This, media);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMPPlaylist_moveItem(IWMPPlaylist *iface, LONG old_index, LONG new_index)
{
    WMPPlaylist *This = impl_from_IWMPPlaylist(iface);
    FIXME("(%p)->(%ld %ld)\n", This, old_index, new_index);
    return E_NOTIMPL;
}

static const IWMPPlaylistVtbl WMPPlaylistVtbl = {
    WMPPlaylist_QueryInterface,
    WMPPlaylist_AddRef,
    WMPPlaylist_Release,
    WMPPlaylist_GetTypeInfoCount,
    WMPPlaylist_GetTypeInfo,
    WMPPlaylist_GetIDsOfNames,
    WMPPlaylist_Invoke,
    WMPPlaylist_get_count,
    WMPPlaylist_get_name,
    WMPPlaylist_put_name,
    WMPPlaylist_get_attributeCount,
    WMPPlaylist_get_attributeName,
    WMPPlaylist_get_Item,
    WMPPlaylist_getItemInfo,
    WMPPlaylist_setItemInfo,
    WMPPlaylist_get_isIdentical,
    WMPPlaylist_clear,
    WMPPlaylist_insertItem,
    WMPPlaylist_appendItem,
    WMPPlaylist_removeItem,
    WMPPlaylist_moveItem
};

static LRESULT WINAPI player_wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_WMPEVENT && wParam == 0) {
        WindowsMediaPlayer *wmp = (WindowsMediaPlayer*)lParam;
        LONG event_code;
        LONG_PTR p1, p2;
        HRESULT hr;
        if (wmp->media_event) {
            do {
                hr = IMediaEvent_GetEvent(wmp->media_event, &event_code, &p1, &p2, 0);
                if (SUCCEEDED(hr)) {
                    TRACE("got event_code = 0x%02lx\n", event_code);
                    IMediaEvent_FreeEventParams(wmp->media_event, event_code, p1, p2);
                    /* For now we only handle EC_COMPLETE */
                    if (event_code == EC_COMPLETE) {
                        update_state(wmp, DISPID_WMPCOREEVENT_PLAYSTATECHANGE, wmppsMediaEnded);
                        update_state(wmp, DISPID_WMPCOREEVENT_PLAYSTATECHANGE, wmppsTransitioning);
                        update_state(wmp, DISPID_WMPCOREEVENT_PLAYSTATECHANGE, wmppsStopped);
                    }
                }
            } while (hr == S_OK);
        } else {
            FIXME("Got event from quartz when interfaces are already released\n");
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static BOOL WINAPI register_player_msg_class(INIT_ONCE *once, void *param, void **context) {
    static WNDCLASSEXW wndclass = {
        sizeof(wndclass), CS_DBLCLKS, player_wnd_proc, 0, 0,
        NULL, NULL, NULL, NULL, NULL,
        L"_WMPMessage", NULL
    };

    wndclass.hInstance = wmp_instance;
    player_msg_class = RegisterClassExW(&wndclass);
    WM_WMPEVENT= RegisterWindowMessageW(L"_WMPMessage");
    return TRUE;
}

void unregister_player_msg_class(void) {
    if(player_msg_class)
        UnregisterClassW(MAKEINTRESOURCEW(player_msg_class), wmp_instance);
}

BOOL init_player(WindowsMediaPlayer *wmp)
{
    IWMPPlaylist *playlist;
    BSTR name;

    InitOnceExecuteOnce(&class_init_once, register_player_msg_class, NULL, NULL);
    wmp->msg_window = CreateWindowW( MAKEINTRESOURCEW(player_msg_class), NULL, 0, 0,
            0, 0, 0, HWND_MESSAGE, 0, wmp_instance, wmp );
    if (!wmp->msg_window) {
        ERR("Failed to create message window, GetLastError: %ld\n", GetLastError());
        return FALSE;
    }
    if (!WM_WMPEVENT) {
        ERR("Failed to register window message, GetLastError: %ld\n", GetLastError());
        return FALSE;
    }

    wmp->IWMPPlayer4_iface.lpVtbl = &WMPPlayer4Vtbl;
    wmp->IWMPPlayer_iface.lpVtbl = &WMPPlayerVtbl;
    wmp->IWMPSettings_iface.lpVtbl = &WMPSettingsVtbl;
    wmp->IWMPControls_iface.lpVtbl = &WMPControlsVtbl;
    wmp->IWMPNetwork_iface.lpVtbl = &WMPNetworkVtbl;

    name = SysAllocString(L"Playlist1");
    if (SUCCEEDED(create_playlist(name, NULL, 0, &playlist)))
        wmp->playlist = unsafe_impl_from_IWMPPlaylist(playlist);
    else
        wmp->playlist = NULL;
    SysFreeString(name);

    wmp->invoke_urls = VARIANT_TRUE;
    wmp->auto_start = VARIANT_TRUE;
    wmp->volume = 100;
    return TRUE;
}

void destroy_player(WindowsMediaPlayer *wmp)
{
    IWMPControls_stop(&wmp->IWMPControls_iface);
    if (wmp->media)
        IWMPMedia_Release(&wmp->media->IWMPMedia_iface);
    if (wmp->playlist)
        IWMPPlaylist_Release(&wmp->playlist->IWMPPlaylist_iface);
    DestroyWindow(wmp->msg_window);
}

WMPMedia *unsafe_impl_from_IWMPMedia(IWMPMedia *iface)
{
    if (iface->lpVtbl == &WMPMediaVtbl) {
        return CONTAINING_RECORD(iface, WMPMedia, IWMPMedia_iface);
    }
    return NULL;
}

WMPPlaylist *unsafe_impl_from_IWMPPlaylist(IWMPPlaylist *iface)
{
    if (iface->lpVtbl == &WMPPlaylistVtbl) {
        return CONTAINING_RECORD(iface, WMPPlaylist, IWMPPlaylist_iface);
    }
    return NULL;
}

HRESULT create_media_from_url(BSTR url, double duration, IWMPMedia **ppMedia)
{
    WMPMedia *media;
    IUri *uri;
    BSTR path;
    HRESULT hr;
    WCHAR *name_dup;

    media = calloc(1, sizeof(*media));
    if (!media)
        return E_OUTOFMEMORY;

    media->IWMPMedia_iface.lpVtbl = &WMPMediaVtbl;

    if (url)
    {
        media->url = wcsdup(url);
        name_dup = wcsdup(url);

        hr = CreateUri(name_dup, Uri_CREATE_ALLOW_RELATIVE | Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME, 0, &uri);
        if (FAILED(hr))
        {
            free(name_dup);
            IWMPMedia_Release(&media->IWMPMedia_iface);
            return hr;
        }
        hr = IUri_GetPath(uri, &path);
        if (hr != S_OK)
        {
            free(name_dup);
            IUri_Release(uri);
            IWMPMedia_Release(&media->IWMPMedia_iface);
            return hr;
        }

        /* GetPath() will return "/" for invalid uri's
         * only strip extension when uri is valid
         */
        if (wcscmp(path, L"/") != 0)
            PathRemoveExtensionW(name_dup);
        PathStripPathW(name_dup);

        media->name = name_dup;

        SysFreeString(path);
        IUri_Release(uri);
    }
    else
    {
        media->url = wcsdup(L"");
        media->name = wcsdup(L"");
    }

    media->duration = duration;
    media->ref = 1;

    if (media->url) {
        *ppMedia = &media->IWMPMedia_iface;

        return S_OK;
    }
    IWMPMedia_Release(&media->IWMPMedia_iface);
    return E_OUTOFMEMORY;
}

HRESULT create_playlist(BSTR name, BSTR url, LONG count, IWMPPlaylist **ppPlaylist)
{
    WMPPlaylist *playlist;

    playlist = calloc(1, sizeof(*playlist));
    if (!playlist)
        return E_OUTOFMEMORY;

    playlist->IWMPPlaylist_iface.lpVtbl = &WMPPlaylistVtbl;
    playlist->url = wcsdup(url ? url : L"");
    playlist->name = wcsdup(name ? name : L"");
    playlist->ref = 1;
    playlist->count = count;

    if (playlist->url)
    {
        *ppPlaylist = &playlist->IWMPPlaylist_iface;
        return S_OK;
    }

    IWMPPlaylist_Release(&playlist->IWMPPlaylist_iface);
    return E_OUTOFMEMORY;
}
