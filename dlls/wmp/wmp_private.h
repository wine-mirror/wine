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

#define COBJMACROS

#include "windows.h"
#include "ole2.h"
#include "dshow.h"
#include "wmp.h"

#define TID_LIST \
    XIID(NULL) \
    CTID(WindowsMediaPlayer)

typedef enum {
#define XIID(iface) iface ## _tid,
#define CTID(name) name ## _tid,
TID_LIST
#undef XIID
#undef CTID
    LAST_tid
} typeinfo_id;

HRESULT get_typeinfo(typeinfo_id tid, ITypeInfo **typeinfo) DECLSPEC_HIDDEN;

typedef struct {
    IConnectionPoint IConnectionPoint_iface;

    IConnectionPointContainer *container;

    IDispatch **sinks;
    DWORD sinks_size;

    IID iid;
} ConnectionPoint;

typedef struct {
    IWMPMedia IWMPMedia_iface;

    LONG ref;

    WCHAR *url;
    WCHAR *name;

    DOUBLE duration;
} WMPMedia;

typedef struct {
    IWMPPlaylist IWMPPlaylist_iface;

    LONG ref;
    LONG count;

    WCHAR *url;
    WCHAR *name;
} WMPPlaylist;

struct WindowsMediaPlayer {
    IOleObject IOleObject_iface;
    IProvideClassInfo2 IProvideClassInfo2_iface;
    IPersistStreamInit IPersistStreamInit_iface;
    IOleInPlaceObjectWindowless IOleInPlaceObjectWindowless_iface;
    IConnectionPointContainer IConnectionPointContainer_iface;
    IOleControl IOleControl_iface;
    IWMPPlayer4 IWMPPlayer4_iface;
    IWMPPlayer IWMPPlayer_iface;
    IWMPSettings IWMPSettings_iface;
    IWMPControls IWMPControls_iface;
    IWMPNetwork IWMPNetwork_iface;

    LONG ref;

    IOleClientSite *client_site;
    HWND hwnd;
    SIZEL extent;

    /* Settings */
    VARIANT_BOOL auto_start;
    VARIANT_BOOL invoke_urls;
    VARIANT_BOOL enable_error_dialogs;
    LONG volume;

    ConnectionPoint *wmpocx;

    WMPMedia *media;
    WMPPlaylist *playlist;

    /* DirectShow stuff */
    IGraphBuilder* filter_graph;
    IMediaControl* media_control;
    IMediaEvent* media_event;
    IMediaSeeking* media_seeking;
    IBasicAudio* basic_audio;

    /* Async event notification */
    HWND msg_window;
};

BOOL init_player(WindowsMediaPlayer*) DECLSPEC_HIDDEN;
void destroy_player(WindowsMediaPlayer*) DECLSPEC_HIDDEN;
WMPMedia *unsafe_impl_from_IWMPMedia(IWMPMedia *iface) DECLSPEC_HIDDEN;
WMPPlaylist *unsafe_impl_from_IWMPPlaylist(IWMPPlaylist *iface) DECLSPEC_HIDDEN;
HRESULT create_media_from_url(BSTR url, double duration, IWMPMedia **ppMedia) DECLSPEC_HIDDEN;
HRESULT create_playlist(BSTR name, BSTR url, LONG count, IWMPPlaylist **ppPlaylist) DECLSPEC_HIDDEN;
void ConnectionPointContainer_Init(WindowsMediaPlayer *wmp) DECLSPEC_HIDDEN;
void ConnectionPointContainer_Destroy(WindowsMediaPlayer *wmp) DECLSPEC_HIDDEN;
void call_sink(ConnectionPoint *This, DISPID dispid, DISPPARAMS *dispparams) DECLSPEC_HIDDEN;

HRESULT WINAPI WMPFactory_CreateInstance(IClassFactory*,IUnknown*,REFIID,void**) DECLSPEC_HIDDEN;

void unregister_wmp_class(void) DECLSPEC_HIDDEN;
void unregister_player_msg_class(void) DECLSPEC_HIDDEN;

extern HINSTANCE wmp_instance DECLSPEC_HIDDEN;

static inline HRESULT return_bstr(const WCHAR *value, BSTR *p)
{
    if(!p)
        return E_POINTER;

    if(value) {
        *p = SysAllocString(value);
        if(!*p)
            return E_OUTOFMEMORY;
    }else {
        *p = NULL;
    }

    return S_OK;
}
