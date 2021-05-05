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

#ifndef __WINE_GAMINGTCUI_H
#define __WINE_GAMINGTCUI_H

#include <windows.h>
#include <hstring.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (WINAPI *GameUICompletionRoutine)(HRESULT return_code, void *context);
typedef void (WINAPI *PlayerPickerUICompletionRoutine)(HRESULT return_code, void *context,
        const HSTRING *selected_xuids, size_t count);

HRESULT WINAPI ProcessPendingGameUI(BOOL wait_for_completion);
HRESULT WINAPI ShowPlayerPickerUI(HSTRING prompt_display_text, const HSTRING *xuids, size_t xuid_count,
        const HSTRING *preselected_xuids, size_t preselected_xuid_count, size_t min_selection_count,
        size_t max_selection_count, PlayerPickerUICompletionRoutine completion_routine, void *context);
HRESULT WINAPI ShowProfileCardUI(HSTRING target_user_xuid, GameUICompletionRoutine completion_routine, void *context);

#ifdef __cplusplus
}
#endif
#endif
