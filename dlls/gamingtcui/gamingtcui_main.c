/*
 * Copyright 2021 Paul Gofman for CodeWeavers
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
 *
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "gamingtcui.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gamingtcui);

HRESULT WINAPI ProcessPendingGameUI(BOOL wait_for_completion)
{
    FIXME("wait_for_completion %#x stub.\n", wait_for_completion);

    return S_OK;
}

HRESULT WINAPI ShowPlayerPickerUI(HSTRING prompt_display_text, const HSTRING *xuids, size_t xuid_count,
        const HSTRING *preselected_xuids, size_t preselected_xuid_count, size_t min_selection_count,
        size_t max_selection_count, PlayerPickerUICompletionRoutine completion_routine, void *context)
{
    FIXME("prompt_display_text %p, xuids %p, xuid_count %Iu, preselected_xuids %p, preselected_xuid_count %Iu,"
            " min_selection_count %Iu, max_selection_count %Iu, completion_routine %p, context %p semi-stub.\n",
            prompt_display_text, xuids, xuid_count, preselected_xuids, preselected_xuid_count,
            min_selection_count, max_selection_count, completion_routine, context);

    if (completion_routine)
        completion_routine(S_OK, context, preselected_xuids, preselected_xuid_count);

    return S_OK;
}

HRESULT WINAPI ShowProfileCardUI(HSTRING target_user_xuid, GameUICompletionRoutine completion_routine, void *context)
{
    FIXME("target_user_xuid %p, completion_routine %p, context %p stub.\n",
            target_user_xuid, completion_routine, context);

    if (completion_routine)
        completion_routine(S_OK, context);

    return S_OK;
}
