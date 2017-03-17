/*
 * Task dialog control
 *
 * Copyright 2017 Fabian Maurer
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
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "commctrl.h"
#include "winerror.h"
#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(taskdialog);

/***********************************************************************
 * TaskDialogIndirect [COMCTL32.@]
 */
HRESULT WINAPI TaskDialogIndirect(const TASKDIALOGCONFIG *taskconfig, int *button,
                                  int *radio_button, BOOL *verification_flag_checked)
{
    UINT type = 0;
    INT ret;

    TRACE("%p, %p, %p, %p\n", taskconfig, button, radio_button, verification_flag_checked);

    if (taskconfig->dwCommonButtons & TDCBF_YES_BUTTON &&
        taskconfig->dwCommonButtons & TDCBF_NO_BUTTON &&
        taskconfig->dwCommonButtons & TDCBF_CANCEL_BUTTON)
        type |= MB_YESNOCANCEL;
    else
    if (taskconfig->dwCommonButtons & TDCBF_YES_BUTTON &&
        taskconfig->dwCommonButtons & TDCBF_NO_BUTTON)
        type |= MB_YESNO;
    else
    if (taskconfig->dwCommonButtons & TDCBF_RETRY_BUTTON &&
        taskconfig->dwCommonButtons & TDCBF_CANCEL_BUTTON)
        type |= MB_RETRYCANCEL;
    else
    if (taskconfig->dwCommonButtons & TDCBF_OK_BUTTON &&
        taskconfig->dwCommonButtons & TDCBF_CANCEL_BUTTON)
        type |= MB_OKCANCEL;
    else
    if (taskconfig->dwCommonButtons & TDCBF_OK_BUTTON)
        type |= MB_OK;

    ret = MessageBoxW(taskconfig->hwndParent, taskconfig->pszMainInstruction,
        taskconfig->pszWindowTitle, type);

    FIXME("dwCommonButtons=%x type=%x ret=%x\n", taskconfig->dwCommonButtons, type, ret);

    if (button) *button = ret;
    if (radio_button) *radio_button = taskconfig->nDefaultButton;
    if (verification_flag_checked) *verification_flag_checked = TRUE;

    return S_OK;
}
