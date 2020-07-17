/*
 * Navigator filter
 *
 * Copyright 2020 Gijs Vermeulen
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

#include "qdvd_private.h"
#include "wine/strmbase.h"

WINE_DEFAULT_DEBUG_CHANNEL(qdvd);

struct navigator
{
    struct strmbase_filter filter;
    IDvdControl2 IDvdControl2_iface;
};

static inline struct navigator *impl_from_strmbase_filter(struct strmbase_filter *filter)
{
    return CONTAINING_RECORD(filter, struct navigator, filter);
}

static HRESULT navigator_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct navigator *filter = impl_from_strmbase_filter(iface);

    if (IsEqualGUID(iid, &IID_IDvdControl2))
        *out = &filter->IDvdControl2_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static struct strmbase_pin *navigator_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    return NULL;
}

static void navigator_destroy(struct strmbase_filter *iface)
{
    struct navigator *filter = impl_from_strmbase_filter(iface);

    strmbase_filter_cleanup(&filter->filter);
    free(filter);
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_query_interface = navigator_query_interface,
    .filter_get_pin = navigator_get_pin,
    .filter_destroy = navigator_destroy,
};

static struct navigator *impl_from_IDvdControl2(IDvdControl2 *iface)
{
    return CONTAINING_RECORD(iface, struct navigator, IDvdControl2_iface);
}

static HRESULT WINAPI dvd_control2_QueryInterface(IDvdControl2 *iface, REFIID iid, void **out)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);
    return IUnknown_QueryInterface(filter->filter.outer_unk, iid, out);
}

static ULONG WINAPI dvd_control2_AddRef(IDvdControl2 *iface)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);
    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI dvd_control2_Release(IDvdControl2 *iface)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);
    return IUnknown_Release(filter->filter.outer_unk);
}

static HRESULT WINAPI dvd_control2_PlayTitle(IDvdControl2 *iface, ULONG title, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, title %u, flags %#x, cmd %p.\n", filter, title, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_PlayChapterInTitle(IDvdControl2 *iface, ULONG title, ULONG chapter,
        DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, title %u, chapter %u, flags %#x, cmd %p.\n", filter, title, chapter, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_PlayTimeInTitle(IDvdControl2 *iface, ULONG title, DVD_HMSF_TIMECODE *time,
        DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, title %u, time %p, flags %#x, cmd %p.\n", filter, title, time, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_Stop(IDvdControl2 *iface)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p.\n", filter);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_ReturnFromSubmenu(IDvdControl2 *iface, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, flags %#x, cmd %p.\n", filter, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_PlayAtTime(IDvdControl2 *iface, DVD_HMSF_TIMECODE *time, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, time %p, flags %#x, cmd %p.\n", filter, time, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_PlayChapter(IDvdControl2 *iface, ULONG chapter, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, chapter %u, flags %#x, cmd %p.\n", filter, chapter, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_PlayPrevChapter(IDvdControl2 *iface, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, flags %#x, cmd %p.\n", filter, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_ReplayChapter(IDvdControl2 *iface, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, flags %#x, cmd %p.\n", filter, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_PlayNextChapter(IDvdControl2 *iface, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, flags %#x, cmd %p.\n", filter, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_PlayForwards(IDvdControl2 *iface, double speed, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, speed %f, flags %#x, cmd %p.\n", filter, speed, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_PlayBackwards(IDvdControl2 *iface, double speed, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, speed %f, flags %#x, cmd %p.\n", filter, speed, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_ShowMenu(IDvdControl2 *iface, DVD_MENU_ID id, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, id %d, flags %#x, cmd %p.\n", filter, id, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_Resume(IDvdControl2 *iface, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, flags %#x, cmd %p.\n", filter, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectRelativeButton(IDvdControl2 *iface, DVD_RELATIVE_BUTTON button)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, button %d.\n", filter, button);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_ActivateButton(IDvdControl2 *iface)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p.\n", filter);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectButton(IDvdControl2 *iface, ULONG button)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, button %u.\n", filter, button);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectAndActivateButton(IDvdControl2 *iface, ULONG button)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, button %u.\n", filter, button);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_StillOff(IDvdControl2 *iface)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p.\n", filter);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_Pause(IDvdControl2 *iface, BOOL enable)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, enable %d.\n", filter, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectAudioStream(IDvdControl2 *iface, ULONG stream, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, stream %u, flags %#x, cmd %p.\n", filter, stream, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectSubpictureStream(IDvdControl2 *iface, ULONG stream, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, stream %u, flags %#x, cmd %p.\n", filter, stream, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SetSubpictureState(IDvdControl2 *iface, BOOL enable, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, enable %d, flags %#x, cmd %p.\n", filter, enable, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectAngle(IDvdControl2 *iface, ULONG angle, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, angle %u, flags %#x, cmd %p.\n", filter, angle, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectParentalLevel(IDvdControl2 *iface, ULONG level)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, level %u.\n", filter, level);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectParentalCountry(IDvdControl2 *iface, BYTE country[2])
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, country %p.\n", filter, country);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectKaraokeAudioPresentationMode(IDvdControl2 *iface, ULONG mode)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, mode %u.\n", filter, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectVideoModePreference(IDvdControl2 *iface, ULONG mode)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, mode %u.\n", filter, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SetDVDDirectory(IDvdControl2 *iface, const WCHAR *path)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, path %s.\n", filter, debugstr_w(path));

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_ActivateAtPosition(IDvdControl2 *iface, POINT point)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, point %s.\n", filter, wine_dbgstr_point(&point));

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectAtPosition(IDvdControl2 *iface, POINT point)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, point %s.\n", filter, wine_dbgstr_point(&point));

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_PlayChaptersAutoStop(IDvdControl2 *iface, ULONG title, ULONG chapter, ULONG count,
        DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, title %u, chapter %u, count %u, flags %#x, cmd %p.\n", filter, title, chapter, count, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_AcceptParentalLevelChange(IDvdControl2 *iface, BOOL accept)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, accept %d.\n", filter, accept);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SetOption(IDvdControl2 *iface, DVD_OPTION_FLAG flag, BOOL option)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, flag %d, option %d.\n", filter, flag, option);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SetState(IDvdControl2 *iface, IDvdState *state, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, state %p, flags %#x, cmd %p.\n", filter, state, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_PlayPeriodInTitleAutoStop(IDvdControl2 *iface, ULONG title,
        DVD_HMSF_TIMECODE *start_time, DVD_HMSF_TIMECODE *end_time, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, title %u, start_time %p, end_time %p, flags %#x, cmd %p.\n",
            filter, title, start_time, end_time, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SetGRPM(IDvdControl2 *iface, ULONG index, WORD value, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, index %u, value %i, flags %#x, cmd %p.\n", filter, index, value, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectDefaultMenuLanguage(IDvdControl2 *iface, LCID language)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, language %#x.\n", filter, language);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectDefaultAudioLanguage(IDvdControl2 *iface, LCID language,
        DVD_AUDIO_LANG_EXT extension)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, language %#x, extension %d.\n", filter, language, extension);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectDefaultSubpictureLanguage(IDvdControl2 *iface, LCID language,
        DVD_SUBPICTURE_LANG_EXT extension)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, language %#x, extension %d.\n", filter, language, extension);

    return E_NOTIMPL;
}

static const struct IDvdControl2Vtbl dvd_control2_vtbl =
{
    dvd_control2_QueryInterface,
    dvd_control2_AddRef,
    dvd_control2_Release,
    dvd_control2_PlayTitle,
    dvd_control2_PlayChapterInTitle,
    dvd_control2_PlayTimeInTitle,
    dvd_control2_Stop,
    dvd_control2_ReturnFromSubmenu,
    dvd_control2_PlayAtTime,
    dvd_control2_PlayChapter,
    dvd_control2_PlayPrevChapter,
    dvd_control2_ReplayChapter,
    dvd_control2_PlayNextChapter,
    dvd_control2_PlayForwards,
    dvd_control2_PlayBackwards,
    dvd_control2_ShowMenu,
    dvd_control2_Resume,
    dvd_control2_SelectRelativeButton,
    dvd_control2_ActivateButton,
    dvd_control2_SelectButton,
    dvd_control2_SelectAndActivateButton,
    dvd_control2_StillOff,
    dvd_control2_Pause,
    dvd_control2_SelectAudioStream,
    dvd_control2_SelectSubpictureStream,
    dvd_control2_SetSubpictureState,
    dvd_control2_SelectAngle,
    dvd_control2_SelectParentalLevel,
    dvd_control2_SelectParentalCountry,
    dvd_control2_SelectKaraokeAudioPresentationMode,
    dvd_control2_SelectVideoModePreference,
    dvd_control2_SetDVDDirectory,
    dvd_control2_ActivateAtPosition,
    dvd_control2_SelectAtPosition,
    dvd_control2_PlayChaptersAutoStop,
    dvd_control2_AcceptParentalLevelChange,
    dvd_control2_SetOption,
    dvd_control2_SetState,
    dvd_control2_PlayPeriodInTitleAutoStop,
    dvd_control2_SetGRPM,
    dvd_control2_SelectDefaultMenuLanguage,
    dvd_control2_SelectDefaultAudioLanguage,
    dvd_control2_SelectDefaultSubpictureLanguage,
};

HRESULT navigator_create(IUnknown *outer, IUnknown **out)
{
    struct navigator *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_DVDNavigator, &filter_ops);
    object->IDvdControl2_iface.lpVtbl = &dvd_control2_vtbl;

    TRACE("Created DVD Navigator filter %p.\n", object);
    *out = &object->filter.IUnknown_inner;
    return S_OK;
}
