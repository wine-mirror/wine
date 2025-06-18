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

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct navigator
{
    struct strmbase_filter filter;
    IDvdControl2 IDvdControl2_iface;
    IDvdInfo2 IDvdInfo2_iface;
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
    else if (IsEqualGUID(iid, &IID_IDvdInfo2))
        *out = &filter->IDvdInfo2_iface;
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

    FIXME("filter %p, title %lu, flags %#lx, cmd %p.\n", filter, title, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_PlayChapterInTitle(IDvdControl2 *iface, ULONG title, ULONG chapter,
        DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, title %lu, chapter %lu, flags %#lx, cmd %p.\n", filter, title, chapter, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_PlayTimeInTitle(IDvdControl2 *iface, ULONG title, DVD_HMSF_TIMECODE *time,
        DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, title %lu, time %p, flags %#lx, cmd %p.\n", filter, title, time, flags, cmd);

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

    FIXME("filter %p, flags %#lx, cmd %p.\n", filter, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_PlayAtTime(IDvdControl2 *iface, DVD_HMSF_TIMECODE *time, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, time %p, flags %#lx, cmd %p.\n", filter, time, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_PlayChapter(IDvdControl2 *iface, ULONG chapter, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, chapter %lu, flags %#lx, cmd %p.\n", filter, chapter, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_PlayPrevChapter(IDvdControl2 *iface, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, flags %#lx, cmd %p.\n", filter, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_ReplayChapter(IDvdControl2 *iface, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, flags %#lx, cmd %p.\n", filter, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_PlayNextChapter(IDvdControl2 *iface, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, flags %#lx, cmd %p.\n", filter, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_PlayForwards(IDvdControl2 *iface, double speed, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, speed %.16e, flags %#lx, cmd %p.\n", filter, speed, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_PlayBackwards(IDvdControl2 *iface, double speed, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, speed %.16e, flags %#lx, cmd %p.\n", filter, speed, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_ShowMenu(IDvdControl2 *iface, DVD_MENU_ID id, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, id %d, flags %#lx, cmd %p.\n", filter, id, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_Resume(IDvdControl2 *iface, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, flags %#lx, cmd %p.\n", filter, flags, cmd);

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

    FIXME("filter %p, button %lu.\n", filter, button);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectAndActivateButton(IDvdControl2 *iface, ULONG button)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, button %lu.\n", filter, button);

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

    FIXME("filter %p, stream %lu, flags %#lx, cmd %p.\n", filter, stream, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectSubpictureStream(IDvdControl2 *iface, ULONG stream, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, stream %lu, flags %#lx, cmd %p.\n", filter, stream, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SetSubpictureState(IDvdControl2 *iface, BOOL enable, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, enable %d, flags %#lx, cmd %p.\n", filter, enable, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectAngle(IDvdControl2 *iface, ULONG angle, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, angle %lu, flags %#lx, cmd %p.\n", filter, angle, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectParentalLevel(IDvdControl2 *iface, ULONG level)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, level %lu.\n", filter, level);

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

    FIXME("filter %p, mode %lu.\n", filter, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectVideoModePreference(IDvdControl2 *iface, ULONG mode)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, mode %lu.\n", filter, mode);

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

    FIXME("filter %p, title %lu, chapter %lu, count %lu, flags %#lx, cmd %p.\n", filter, title, chapter, count, flags, cmd);

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

    FIXME("filter %p, state %p, flags %#lx, cmd %p.\n", filter, state, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_PlayPeriodInTitleAutoStop(IDvdControl2 *iface, ULONG title,
        DVD_HMSF_TIMECODE *start_time, DVD_HMSF_TIMECODE *end_time, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, title %lu, start_time %p, end_time %p, flags %#lx, cmd %p.\n",
            filter, title, start_time, end_time, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SetGRPM(IDvdControl2 *iface, ULONG index, WORD value, DWORD flags, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, index %lu, value %d, flags %#lx, cmd %p.\n", filter, index, value, flags, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectDefaultMenuLanguage(IDvdControl2 *iface, LCID language)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, language %#lx.\n", filter, language);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectDefaultAudioLanguage(IDvdControl2 *iface, LCID language,
        DVD_AUDIO_LANG_EXT extension)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, language %#lx, extension %d.\n", filter, language, extension);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_control2_SelectDefaultSubpictureLanguage(IDvdControl2 *iface, LCID language,
        DVD_SUBPICTURE_LANG_EXT extension)
{
    struct navigator *filter = impl_from_IDvdControl2(iface);

    FIXME("filter %p, language %#lx, extension %d.\n", filter, language, extension);

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

static struct navigator *impl_from_IDvdInfo2(IDvdInfo2 *iface)
{
    return CONTAINING_RECORD(iface, struct navigator, IDvdInfo2_iface);
}

static HRESULT WINAPI dvd_info2_QueryInterface(IDvdInfo2 *iface, REFIID iid, void **out)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);
    return IUnknown_QueryInterface(filter->filter.outer_unk, iid, out);
}

static ULONG WINAPI dvd_info2_AddRef(IDvdInfo2 *iface)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);
    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI dvd_info2_Release(IDvdInfo2 *iface)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);
    return IUnknown_Release(filter->filter.outer_unk);
}

static HRESULT WINAPI dvd_info2_GetCurrentDomain(IDvdInfo2 *iface, DVD_DOMAIN *domain)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, domain %p.\n", filter, domain);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetCurrentLocation(IDvdInfo2 *iface, DVD_PLAYBACK_LOCATION2 *location)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, location %p.\n", filter, location);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetTotalTitleTime(IDvdInfo2 *iface, DVD_HMSF_TIMECODE *time, ULONG *flags)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, time %p, flags %p.\n", filter, time, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetCurrentButton(IDvdInfo2 *iface, ULONG *count, ULONG *current)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, count %p, current %p.\n", filter, count, current);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetCurrentAngle(IDvdInfo2 *iface, ULONG *count, ULONG *current)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, count %p, current %p.\n", filter, count, current);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetCurrentAudio(IDvdInfo2 *iface, ULONG *count, ULONG *current)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, count %p, current %p.\n", filter, count, current);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetCurrentSubpicture(IDvdInfo2 *iface, ULONG *count, ULONG *current, BOOL *enable)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, count %p, current %p, enable %p.\n", filter, count, current, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetCurrentUOPS(IDvdInfo2 *iface, ULONG *uops)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, uops %p.\n", filter, uops);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetAllSPRMs(IDvdInfo2 *iface, SPRMARRAY *regs)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, regs %p.\n", filter, regs);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetAllGPRMs(IDvdInfo2 *iface, GPRMARRAY *regs)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, regs %p.\n", filter, regs);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetAudioLanguage(IDvdInfo2 *iface, ULONG stream, LCID *language)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, stream %lu, language %p.\n", filter, stream, language);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetSubpictureLanguage(IDvdInfo2 *iface, ULONG stream, LCID *language)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, stream %lu, language %p.\n", filter, stream, language);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetTitleAttributes(IDvdInfo2 *iface, ULONG index,
        DVD_MenuAttributes *menu, DVD_TitleAttributes *title)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, index %lu, menu %p, title %p.\n", filter, index, menu, title);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetVMGAttributes(IDvdInfo2 *iface, DVD_MenuAttributes *attr)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, attr %p.\n", filter, attr);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetVideoAttributes(IDvdInfo2 *iface, DVD_VideoAttributes *attr)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, attr %p.\n", filter, attr);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetAudioAttributes(IDvdInfo2 *iface, ULONG stream, DVD_AudioAttributes *attr)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, stream %lu, attr %p.\n", filter, stream, attr);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetKaraokeAttributes(IDvdInfo2 *iface, ULONG stream, DVD_KaraokeAttributes *attr)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, stream %lu, attr %p.\n", filter, stream, attr);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetSubpictureAttributes(IDvdInfo2 *iface, ULONG stream,
        DVD_SubpictureAttributes *attr)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, stream %lu, attr %p.\n", filter, stream, attr);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetCurrentVolumeInfo(IDvdInfo2 *iface, ULONG *volume_count, ULONG *current,
        DVD_DISC_SIDE *side, ULONG *title_count)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, volume_count %p, current %p, side %p, title_count %p.\n",
            filter, volume_count, current, side, title_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetDVDTextNumberOfLanguages(IDvdInfo2 *iface, ULONG *count)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, count %p.\n", filter, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetDVDTextLanguageInfo(IDvdInfo2 *iface, ULONG index, ULONG *string_count,
        LCID *language, enum DVD_TextCharSet *character_set)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, index %lu, string_count %p, language %p, character_set %p.\n",
            filter, index, string_count, language, character_set);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetDVDTextStringAsNative(IDvdInfo2 *iface, ULONG lang_index, ULONG string_index,
        BYTE *string, ULONG size, ULONG *ret_size, enum DVD_TextStringType *type)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, lang_index %lu, string_index %lu, string %p, size %lu, ret_size %p, type %p.\n",
            filter, lang_index, string_index, string, size, ret_size, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetDVDTextStringAsUnicode(IDvdInfo2 *iface, ULONG lang_index, ULONG string_index,
        WCHAR *string, ULONG size, ULONG *ret_size, enum DVD_TextStringType *type)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, lang_index %lu, string_index %lu, string %p, size %lu, ret_size %p, type %p.\n",
            filter, lang_index, string_index, string, size, ret_size, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetPlayerParentalLevel(IDvdInfo2 *iface, ULONG *level, BYTE country_code[2])
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, level %p, country_code %p.\n", filter, level, country_code);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetNumberOfChapters(IDvdInfo2 *iface, ULONG title, ULONG *count)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, title %lu, count %p.\n", filter, title, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetTitleParentalLevels(IDvdInfo2 *iface, ULONG title, ULONG *levels)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, title %lu, levels %p.\n", filter, title, levels);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetDVDDirectory(IDvdInfo2 *iface, WCHAR *path, ULONG size, ULONG *ret_size)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, path %p, size %lu, ret_size %p.\n", filter, path, size, ret_size);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_IsAudioStreamEnabled(IDvdInfo2 *iface, ULONG stream, BOOL *enable)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, stream %lu, enable %p.\n", filter, stream, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetDiscID(IDvdInfo2 *iface, const WCHAR *path, ULONGLONG *id)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, path %s, id %p.\n", filter, debugstr_w(path), id);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetState(IDvdInfo2 *iface, IDvdState **state)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, state %p.\n", filter, state);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetMenuLanguages(IDvdInfo2 *iface, LCID *languages, ULONG count, ULONG *ret_count)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, languages %p, count %lu, ret_count %p.\n", filter, languages, count, ret_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetButtonAtPosition(IDvdInfo2 *iface, POINT point, ULONG *button)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, point %s, button %p.\n", filter, wine_dbgstr_point(&point), button);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetCmdFromEvent(IDvdInfo2 *iface, LONG_PTR param, IDvdCmd **cmd)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, param %#Ix, cmd %p.\n", filter, param, cmd);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetDefaultMenuLanguage(IDvdInfo2 *iface, LCID *language)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, language %p.\n", filter, language);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetDefaultAudioLanguage(IDvdInfo2 *iface, LCID *language,
        DVD_AUDIO_LANG_EXT *extension)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, language %p, extension %p.\n", filter, language, extension);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_SelectDefaultSubpictureLanguage(IDvdInfo2 *iface, LCID *language,
        DVD_SUBPICTURE_LANG_EXT *extension)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, language %p, extension %p.\n", filter, language, extension);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetDecoderCaps(IDvdInfo2 *iface, DVD_DECODER_CAPS *caps)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, caps %p.\n", filter, caps);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_GetButtonRect(IDvdInfo2 *iface, ULONG button, RECT *rect)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, button %lu, rect %p.\n", filter, button, rect);

    return E_NOTIMPL;
}

static HRESULT WINAPI dvd_info2_IsSubpictureStreamEnabled(IDvdInfo2 *iface, ULONG stream, BOOL *enable)
{
    struct navigator *filter = impl_from_IDvdInfo2(iface);

    FIXME("filter %p, stream %lu, enable %p.\n", filter, stream, enable);

    return E_NOTIMPL;
}

static const struct IDvdInfo2Vtbl dvd_info2_vtbl =
{
    dvd_info2_QueryInterface,
    dvd_info2_AddRef,
    dvd_info2_Release,
    dvd_info2_GetCurrentDomain,
    dvd_info2_GetCurrentLocation,
    dvd_info2_GetTotalTitleTime,
    dvd_info2_GetCurrentButton,
    dvd_info2_GetCurrentAngle,
    dvd_info2_GetCurrentAudio,
    dvd_info2_GetCurrentSubpicture,
    dvd_info2_GetCurrentUOPS,
    dvd_info2_GetAllSPRMs,
    dvd_info2_GetAllGPRMs,
    dvd_info2_GetAudioLanguage,
    dvd_info2_GetSubpictureLanguage,
    dvd_info2_GetTitleAttributes,
    dvd_info2_GetVMGAttributes,
    dvd_info2_GetVideoAttributes,
    dvd_info2_GetAudioAttributes,
    dvd_info2_GetKaraokeAttributes,
    dvd_info2_GetSubpictureAttributes,
    dvd_info2_GetCurrentVolumeInfo,
    dvd_info2_GetDVDTextNumberOfLanguages,
    dvd_info2_GetDVDTextLanguageInfo,
    dvd_info2_GetDVDTextStringAsNative,
    dvd_info2_GetDVDTextStringAsUnicode,
    dvd_info2_GetPlayerParentalLevel,
    dvd_info2_GetNumberOfChapters,
    dvd_info2_GetTitleParentalLevels,
    dvd_info2_GetDVDDirectory,
    dvd_info2_IsAudioStreamEnabled,
    dvd_info2_GetDiscID,
    dvd_info2_GetState,
    dvd_info2_GetMenuLanguages,
    dvd_info2_GetButtonAtPosition,
    dvd_info2_GetCmdFromEvent,
    dvd_info2_GetDefaultMenuLanguage,
    dvd_info2_GetDefaultAudioLanguage,
    dvd_info2_SelectDefaultSubpictureLanguage,
    dvd_info2_GetDecoderCaps,
    dvd_info2_GetButtonRect,
    dvd_info2_IsSubpictureStreamEnabled
};

HRESULT navigator_create(IUnknown *outer, IUnknown **out)
{
    struct navigator *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_DVDNavigator, &filter_ops);
    object->IDvdControl2_iface.lpVtbl = &dvd_control2_vtbl;
    object->IDvdInfo2_iface.lpVtbl = &dvd_info2_vtbl;

    TRACE("Created DVD Navigator filter %p.\n", object);
    *out = &object->filter.IUnknown_inner;
    return S_OK;
}
