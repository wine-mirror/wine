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
#include <stdlib.h>
#include <string.h>

#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commctrl.h"
#include "winerror.h"
#include "comctl32.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(taskdialog);

#define ALIGNED_LENGTH(_Len, _Align) (((_Len)+(_Align))&~(_Align))
#define ALIGNED_POINTER(_Ptr, _Align) ((LPVOID)ALIGNED_LENGTH((ULONG_PTR)(_Ptr), _Align))
#define ALIGN_LENGTH(_Len, _Align) _Len = ALIGNED_LENGTH(_Len, _Align)
#define ALIGN_POINTER(_Ptr, _Align) _Ptr = ALIGNED_POINTER(_Ptr, _Align)

static const UINT DIALOG_MIN_WIDTH = 240;
static const UINT DIALOG_SPACING = 5;
static const UINT DIALOG_BUTTON_WIDTH = 50;
static const UINT DIALOG_BUTTON_HEIGHT = 14;
static const UINT DIALOG_TIMER_MS = 200;

static const UINT ID_CONTENT          = 0xf001;

static const UINT ID_TIMER = 1;

struct taskdialog_control
{
    struct list entry;
    DLGITEMTEMPLATE *template;
    unsigned int template_size;
};

struct taskdialog_button_desc
{
    int id;
    const WCHAR *text;
    HINSTANCE hinst;
};

struct taskdialog_template_desc
{
    const TASKDIALOGCONFIG *taskconfig;
    struct list controls;
    WORD control_count;
    struct taskdialog_button_desc *default_button;
};

struct taskdialog_info
{
    HWND hwnd;
    const TASKDIALOGCONFIG *taskconfig;
    DWORD last_timer_tick;
    HFONT font;
    HFONT main_instruction_font;
    /* Control handles */
    HWND main_instruction;
    /* Dialog metrics */
    struct
    {
        LONG x_baseunit;
        LONG y_baseunit;
        LONG h_spacing;
        LONG v_spacing;
    } m;
};

struct button_layout_info
{
    INT id;
    HWND hwnd;
    LONG width;
    LONG line;
};

static void taskdialog_du_to_px(struct taskdialog_info *dialog_info, LONG *width, LONG *height)
{
    if (width) *width = MulDiv(*width, dialog_info->m.x_baseunit, 4);
    if (height) *height = MulDiv(*height, dialog_info->m.y_baseunit, 8);
}

static void template_write_data(char **ptr, const void *src, unsigned int size)
{
    memcpy(*ptr, src, size);
    *ptr += size;
}

static unsigned int taskdialog_add_control(struct taskdialog_template_desc *desc, WORD id, const WCHAR *class,
        HINSTANCE hInstance, const WCHAR *text, DWORD style)
{
    struct taskdialog_control *control = Alloc(sizeof(*control));
    unsigned int size, class_size, text_size;
    DLGITEMTEMPLATE *template;
    static const WCHAR nulW;
    const WCHAR *textW;
    char *ptr;

    class_size = (strlenW(class) + 1) * sizeof(WCHAR);

    if (IS_INTRESOURCE(text))
        text_size = LoadStringW(hInstance, (UINT_PTR)text, (WCHAR *)&textW, 0) * sizeof(WCHAR);
    else
    {
        textW = text;
        text_size = strlenW(textW) * sizeof(WCHAR);
    }

    size = sizeof(DLGITEMTEMPLATE);
    size += class_size;
    size += text_size + sizeof(WCHAR);
    size += sizeof(WORD); /* creation data */

    control->template = template = Alloc(size);
    control->template_size = size;

    template->style = WS_VISIBLE | style;
    template->dwExtendedStyle = 0;
    template->id = id;
    ptr = (char *)(template + 1);
    template_write_data(&ptr, class, class_size);
    template_write_data(&ptr, textW, text_size);
    template_write_data(&ptr, &nulW, sizeof(nulW));

    list_add_tail(&desc->controls, &control->entry);
    desc->control_count++;
    return ALIGNED_LENGTH(size, 3);
}

static unsigned int taskdialog_add_static_label(struct taskdialog_template_desc *desc, WORD id, const WCHAR *str)
{
    unsigned int size;

    if (!str)
        return 0;

    size = taskdialog_add_control(desc, id, WC_STATICW, desc->taskconfig->hInstance, str, 0);
    return size;
}

static unsigned int taskdialog_add_content(struct taskdialog_template_desc *desc)
{
    return taskdialog_add_static_label(desc, ID_CONTENT, desc->taskconfig->pszContent);
}

static void taskdialog_init_button(struct taskdialog_button_desc *button, struct taskdialog_template_desc *desc,
        int id, const WCHAR *text, BOOL custom_button)
{
    button->id = id;
    button->text = text;
    button->hinst = custom_button ? desc->taskconfig->hInstance : COMCTL32_hModule;

    if (id == desc->taskconfig->nDefaultButton)
        desc->default_button = button;
}

static void taskdialog_init_common_buttons(struct taskdialog_template_desc *desc, struct taskdialog_button_desc *buttons,
    unsigned int *button_count)
{
    DWORD flags = desc->taskconfig->dwCommonButtons;

#define TASKDIALOG_INIT_COMMON_BUTTON(id) \
    do { \
        taskdialog_init_button(&buttons[(*button_count)++], desc, ID##id, MAKEINTRESOURCEW(IDS_BUTTON_##id), FALSE); \
    } while(0)

    if (flags & TDCBF_OK_BUTTON)
        TASKDIALOG_INIT_COMMON_BUTTON(OK);
    if (flags & TDCBF_YES_BUTTON)
        TASKDIALOG_INIT_COMMON_BUTTON(YES);
    if (flags & TDCBF_NO_BUTTON)
        TASKDIALOG_INIT_COMMON_BUTTON(NO);
    if (flags & TDCBF_RETRY_BUTTON)
        TASKDIALOG_INIT_COMMON_BUTTON(RETRY);
    if (flags & TDCBF_CANCEL_BUTTON)
        TASKDIALOG_INIT_COMMON_BUTTON(CANCEL);
    if (flags & TDCBF_CLOSE_BUTTON)
        TASKDIALOG_INIT_COMMON_BUTTON(CLOSE);

#undef TASKDIALOG_INIT_COMMON_BUTTON
}

static unsigned int taskdialog_add_buttons(struct taskdialog_template_desc *desc)
{
    unsigned int count = 0, buttons_size, i, size = 0;
    const TASKDIALOGCONFIG *taskconfig = desc->taskconfig;
    struct taskdialog_button_desc *buttons;

    /* Allocate enough memory for the custom and the default buttons. Maximum 6 default buttons possible. */
    buttons_size = 6;
    if (taskconfig->cButtons && taskconfig->pButtons)
        buttons_size += taskconfig->cButtons;

    if (!(buttons = Alloc(buttons_size * sizeof(*buttons))))
        return 0;

    /* Custom buttons */
    if (taskconfig->cButtons && taskconfig->pButtons)
        for (i = 0; i < taskconfig->cButtons; i++)
            taskdialog_init_button(&buttons[count++], desc, taskconfig->pButtons[i].nButtonID,
                    taskconfig->pButtons[i].pszButtonText, TRUE);

    /* Common buttons */
    taskdialog_init_common_buttons(desc, buttons, &count);

    /* There must be at least one button */
    if (count == 0)
        taskdialog_init_button(&buttons[count++], desc, IDOK, MAKEINTRESOURCEW(IDS_BUTTON_OK), FALSE);

    if (!desc->default_button)
        desc->default_button = &buttons[0];

    /* create all buttons */
    for (i = 0; i < count; i++)
    {
        DWORD style = &buttons[i] == desc->default_button ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON;
        size += taskdialog_add_control(desc, buttons[i].id, WC_BUTTONW, buttons[i].hinst, buttons[i].text, style);
    }

    Free(buttons);

    return size;
}

static void taskdialog_clear_controls(struct list *controls)
{
    struct taskdialog_control *control, *control2;

    LIST_FOR_EACH_ENTRY_SAFE(control, control2, controls, struct taskdialog_control, entry)
    {
        list_remove(&control->entry);
        Free(control->template);
        Free(control);
    }
}

static unsigned int taskdialog_get_reference_rect(const TASKDIALOGCONFIG *taskconfig, RECT *ret)
{
    HMONITOR monitor = MonitorFromWindow(taskconfig->hwndParent ? taskconfig->hwndParent : GetActiveWindow(),
                                         MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO info;

    info.cbSize = sizeof(info);
    GetMonitorInfoW(monitor, &info);

    if (taskconfig->dwFlags & TDF_POSITION_RELATIVE_TO_WINDOW && taskconfig->hwndParent)
        GetWindowRect(taskconfig->hwndParent, ret);
    else
        *ret = info.rcWork;

    return info.rcWork.right - info.rcWork.left;
}

static WCHAR *taskdialog_get_exe_name(WCHAR *name, DWORD length)
{
    DWORD len = GetModuleFileNameW(NULL, name, length);
    if (len && len < length)
    {
        WCHAR *p;
        if ((p = strrchrW(name, '/'))) name = p + 1;
        if ((p = strrchrW(name, '\\'))) name = p + 1;
        return name;
    }
    else
        return NULL;
}

static DLGTEMPLATE *create_taskdialog_template(const TASKDIALOGCONFIG *taskconfig)
{
    struct taskdialog_control *control, *control2;
    unsigned int size, title_size;
    struct taskdialog_template_desc desc;
    static const WORD fontsize = 0x7fff;
    static const WCHAR emptyW[] = { 0 };
    const WCHAR *titleW = NULL;
    DLGTEMPLATE *template;
    WCHAR pathW[MAX_PATH];
    char *ptr;

    /* Window title */
    if (!taskconfig->pszWindowTitle)
        titleW = taskdialog_get_exe_name(pathW, ARRAY_SIZE(pathW));
    else if (IS_INTRESOURCE(taskconfig->pszWindowTitle))
    {
        if (!LoadStringW(taskconfig->hInstance, LOWORD(taskconfig->pszWindowTitle), (WCHAR *)&titleW, 0))
            titleW = taskdialog_get_exe_name(pathW, ARRAY_SIZE(pathW));
    }
    else
        titleW = taskconfig->pszWindowTitle;
    if (!titleW)
        titleW = emptyW;
    title_size = (strlenW(titleW) + 1) * sizeof(WCHAR);

    size = sizeof(DLGTEMPLATE) + 2 * sizeof(WORD);
    size += title_size;
    size += 2; /* font size */

    list_init(&desc.controls);
    desc.taskconfig = taskconfig;
    desc.control_count = 0;
    desc.default_button = NULL;

    size += taskdialog_add_content(&desc);
    size += taskdialog_add_buttons(&desc);

    template = Alloc(size);
    if (!template)
    {
        taskdialog_clear_controls(&desc.controls);
        return NULL;
    }

    template->style = DS_MODALFRAME | DS_SETFONT | WS_CAPTION | WS_VISIBLE | WS_SYSMENU;
    if (taskconfig->dwFlags & TDF_CAN_BE_MINIMIZED) template->style |= WS_MINIMIZEBOX;
    if (!(taskconfig->dwFlags & TDF_NO_SET_FOREGROUND)) template->style |= DS_SETFOREGROUND;
    if (taskconfig->dwFlags & TDF_RTL_LAYOUT) template->dwExtendedStyle = WS_EX_LAYOUTRTL | WS_EX_RIGHT | WS_EX_RTLREADING;
    template->cdit = desc.control_count;

    ptr = (char *)(template + 1);
    ptr += 2; /* menu */
    ptr += 2; /* class */
    template_write_data(&ptr, titleW, title_size);
    template_write_data(&ptr, &fontsize, sizeof(fontsize));

    /* write control entries */
    LIST_FOR_EACH_ENTRY_SAFE(control, control2, &desc.controls, struct taskdialog_control, entry)
    {
        ALIGN_POINTER(ptr, 3);

        template_write_data(&ptr, control->template, control->template_size);

        /* list item won't be needed later */
        list_remove(&control->entry);
        Free(control->template);
        Free(control);
    }

    return template;
}

static HRESULT taskdialog_notify(struct taskdialog_info *dialog_info, UINT notification, WPARAM wparam, LPARAM lparam)
{
    const TASKDIALOGCONFIG *taskconfig = dialog_info->taskconfig;
    return taskconfig->pfCallback
               ? taskconfig->pfCallback(dialog_info->hwnd, notification, wparam, lparam, taskconfig->lpCallbackData)
               : S_OK;
}

static void taskdialog_on_button_click(struct taskdialog_info *dialog_info, WORD command_id)
{
    if (taskdialog_notify(dialog_info, TDN_BUTTON_CLICKED, command_id, 0) == S_OK)
        EndDialog(dialog_info->hwnd, command_id);
}

static WCHAR *taskdialog_gettext(struct taskdialog_info *dialog_info, BOOL user_resource, const WCHAR *text)
{
    const WCHAR *textW = NULL;
    INT length;
    WCHAR *ret;

    if (IS_INTRESOURCE(text))
    {
        if (!(length = LoadStringW(user_resource ? dialog_info->taskconfig->hInstance : COMCTL32_hModule,
                                   (UINT_PTR)text, (WCHAR *)&textW, 0)))
            return NULL;
    }
    else
    {
        textW = text;
        length = strlenW(textW);
    }

    ret = Alloc((length + 1) * sizeof(WCHAR));
    if (ret) memcpy(ret, textW, length * sizeof(WCHAR));

    return ret;
}

static void taskdialog_get_label_size(struct taskdialog_info *dialog_info, HWND hwnd, LONG max_width, SIZE *size)
{
    DWORD style = DT_EXPANDTABS | DT_CALCRECT | DT_WORDBREAK;
    HFONT hfont, old_hfont;
    HDC hdc;
    RECT rect = {0};
    WCHAR text[1024];
    INT text_length;

    if (dialog_info->taskconfig->dwFlags & TDF_RTL_LAYOUT)
        style |= DT_RIGHT | DT_RTLREADING;
    else
        style |= DT_LEFT;

    hfont = (HFONT)SendMessageW(hwnd, WM_GETFONT, 0, 0);
    text_length = GetWindowTextW(hwnd, text, ARRAY_SIZE(text));
    hdc = GetDC(hwnd);
    old_hfont = SelectObject(hdc, hfont);
    rect.right = max_width;
    size->cy = DrawTextW(hdc, text, text_length, &rect, style);
    size->cx = min(max_width, rect.right - rect.left);
    if (old_hfont) SelectObject(hdc, old_hfont);
    ReleaseDC(hwnd, hdc);
}

static HWND taskdialog_create_label(struct taskdialog_info *dialog_info, const WCHAR *text, HFONT font)
{
    WCHAR *textW;
    HWND hwnd;

    if (!text) return NULL;

    textW = taskdialog_gettext(dialog_info, TRUE, text);
    hwnd = CreateWindowW(WC_STATICW, textW, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, dialog_info->hwnd, NULL, 0, NULL);
    if (textW) Free(textW);

    SendMessageW(hwnd, WM_SETFONT, (WPARAM)font, 0);
    return hwnd;
}

static void taskdialog_add_main_instruction(struct taskdialog_info *dialog_info)
{
    const TASKDIALOGCONFIG *taskconfig = dialog_info->taskconfig;
    NONCLIENTMETRICSW ncm;

    if (!taskconfig->pszMainInstruction) return;

    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
    /* 1.25 times the height */
    ncm.lfMessageFont.lfHeight = ncm.lfMessageFont.lfHeight * 5 / 4;
    ncm.lfMessageFont.lfWeight = FW_BOLD;
    dialog_info->main_instruction_font = CreateFontIndirectW(&ncm.lfMessageFont);

    dialog_info->main_instruction =
        taskdialog_create_label(dialog_info, taskconfig->pszMainInstruction, dialog_info->main_instruction_font);
}

static void taskdialog_label_layout(struct taskdialog_info *dialog_info, HWND hwnd, INT start_x, LONG dialog_width,
                                    LONG *dialog_height)
{
    LONG x, y, max_width;
    SIZE size;

    if (!hwnd) return;

    x = start_x + dialog_info->m.h_spacing;
    y = *dialog_height + dialog_info->m.v_spacing;
    max_width = dialog_width - x - dialog_info->m.h_spacing;
    taskdialog_get_label_size(dialog_info, hwnd, max_width, &size);
    SetWindowPos(hwnd, 0, x, y, size.cx, size.cy, SWP_NOZORDER);
    *dialog_height = y + size.cy;
}

static void taskdialog_layout(struct taskdialog_info *dialog_info)
{
    const TASKDIALOGCONFIG *taskconfig = dialog_info->taskconfig;
    DWORD flags = taskconfig->dwCommonButtons;
    static BOOL first_time = TRUE;
    HWND hwnd;
    RECT ref_rect;
    LONG screen_width, dialog_width, dialog_height = 0;
    LONG h_spacing, v_spacing;
    INT button_max_count, button_count = 0;
    struct button_layout_info *button_layout_infos;
    LONG button_min_width, button_height;
    LONG *line_widths, line_count, align;
    LONG x, y;
    SIZE size;
    INT i;

    screen_width = taskdialog_get_reference_rect(dialog_info->taskconfig, &ref_rect);
    dialog_width = max(taskconfig->cxWidth, DIALOG_MIN_WIDTH);
    taskdialog_du_to_px(dialog_info, &dialog_width, 0);
    dialog_width = min(dialog_width, screen_width);

    h_spacing = dialog_info->m.h_spacing;
    v_spacing = dialog_info->m.v_spacing;

    /* Main instruction */
    taskdialog_label_layout(dialog_info, dialog_info->main_instruction, 0, dialog_width, &dialog_height);

    /* Content */
    hwnd = GetDlgItem(dialog_info->hwnd, ID_CONTENT);
    taskdialog_label_layout(dialog_info, hwnd, 0, dialog_width, &dialog_height);

    /* Common and custom buttons */
    /* Allocate enough memory for the custom and the default buttons. Maximum 6 default buttons possible. */
    button_max_count = 6;
    /* Custom buttons */
    if (taskconfig->cButtons && taskconfig->pButtons) button_max_count += taskconfig->cButtons;

    button_layout_infos = Alloc(button_max_count * sizeof(*button_layout_infos));
    line_widths = Alloc(button_max_count * sizeof(*line_widths));

    if (taskconfig->cButtons && taskconfig->pButtons)
        for (button_count = 0; button_count < taskconfig->cButtons; button_count++)
            button_layout_infos[button_count].id = taskconfig->pButtons[button_count].nButtonID;

    /* Ok button may be added if no button is specified in taskconfig */
    if (GetDlgItem(dialog_info->hwnd, IDOK)) button_layout_infos[button_count++].id = IDOK;
    if (flags & TDCBF_YES_BUTTON) button_layout_infos[button_count++].id = IDYES;
    if (flags & TDCBF_NO_BUTTON) button_layout_infos[button_count++].id = IDNO;
    if (flags & TDCBF_RETRY_BUTTON) button_layout_infos[button_count++].id = IDRETRY;
    if (flags & TDCBF_CANCEL_BUTTON) button_layout_infos[button_count++].id = IDCANCEL;
    if (flags & TDCBF_CLOSE_BUTTON) button_layout_infos[button_count++].id = IDCLOSE;

    button_min_width = DIALOG_BUTTON_WIDTH;
    button_height = DIALOG_BUTTON_HEIGHT;
    taskdialog_du_to_px(dialog_info, &button_min_width, &button_height);
    for (i = 0; i < button_count; i++)
    {
        button_layout_infos[i].hwnd = GetDlgItem(dialog_info->hwnd, button_layout_infos[i].id);
        taskdialog_get_label_size(dialog_info, button_layout_infos[i].hwnd, dialog_width - h_spacing * 2, &size);
        button_layout_infos[i].width = max(size.cx, button_min_width);
    }

    /* Separate buttons into lines */
    x = h_spacing;
    for (i = 0, line_count = 0; i < button_count; i++)
    {
        if (x + button_layout_infos[i].width + h_spacing >= dialog_width)
        {
            x = h_spacing;
            line_count++;
        }

        button_layout_infos[i].line = line_count;

        x += button_layout_infos[i].width + h_spacing;
        line_widths[line_count] += button_layout_infos[i].width + h_spacing;
    }
    line_count++;

    /* Try to balance lines so they are about the same size */
    for (i = 1; i < line_count - 1; i++)
    {
        int diff_now = abs(line_widths[i] - line_widths[i - 1]);
        unsigned int j, last_button = 0;
        int diff_changed;

        for (j = 0; j < button_count; j++)
            if (button_layout_infos[j].line == i - 1) last_button = j;

        /* Difference in length of both lines if we wrapped the last button from the last line into this one */
        diff_changed = abs(2 * button_layout_infos[last_button].width + line_widths[i] - line_widths[i - 1]);

        if (diff_changed < diff_now)
        {
            button_layout_infos[last_button].line = i;
            line_widths[i] += button_layout_infos[last_button].width;
            line_widths[i - 1] -= button_layout_infos[last_button].width;
        }
    }

    /* Calculate left alignment so all lines are as far right as possible. */
    align = dialog_width - h_spacing;
    for (i = 0; i < line_count; i++)
    {
        int new_alignment = dialog_width - line_widths[i];
        if (new_alignment < align) align = new_alignment;
    }

    /* Now that we got them all positioned, move all buttons */
    x = align;
    size.cy = button_height;
    for (i = 0; i < button_count; i++)
    {
        /* New line */
        if (i > 0 && button_layout_infos[i].line != button_layout_infos[i - 1].line)
        {
            x = align;
            dialog_height += size.cy + v_spacing;
        }

        y = dialog_height + v_spacing;
        size.cx = button_layout_infos[i].width;
        SetWindowPos(button_layout_infos[i].hwnd, 0, x, y, size.cx, size.cy, SWP_NOZORDER);
        x += button_layout_infos[i].width + h_spacing;
    }

    /* Add height for last row button and spacing */
    dialog_height += size.cy + v_spacing;

    Free(button_layout_infos);
    Free(line_widths);

    /* Add height for spacing, title height and frame height */
    dialog_height += v_spacing;
    dialog_height += GetSystemMetrics(SM_CYCAPTION);
    dialog_height += GetSystemMetrics(SM_CXDLGFRAME);

    if (first_time)
    {
        x = (ref_rect.left + ref_rect.right + dialog_width) / 2;
        y = (ref_rect.top + ref_rect.bottom + dialog_height) / 2;
        SetWindowPos(dialog_info->hwnd, 0, x, y, dialog_width, dialog_height, SWP_NOZORDER);
        first_time = FALSE;
    }
    else
        SetWindowPos(dialog_info->hwnd, 0, 0, 0, dialog_width, dialog_height, SWP_NOMOVE | SWP_NOZORDER);
}

static void taskdialog_init(struct taskdialog_info *dialog_info, HWND hwnd)
{
    const TASKDIALOGCONFIG *taskconfig = dialog_info->taskconfig;
    NONCLIENTMETRICSW ncm;
    HDC hdc;

    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);

    memset(dialog_info, 0, sizeof(*dialog_info));
    dialog_info->taskconfig = taskconfig;
    dialog_info->hwnd = hwnd;
    dialog_info->font = CreateFontIndirectW(&ncm.lfMessageFont);

    hdc = GetDC(dialog_info->hwnd);
    SelectObject(hdc, dialog_info->font);
    dialog_info->m.x_baseunit = GdiGetCharDimensions(hdc, NULL, &dialog_info->m.y_baseunit);
    ReleaseDC(dialog_info->hwnd, hdc);

    dialog_info->m.h_spacing = DIALOG_SPACING;
    dialog_info->m.v_spacing = DIALOG_SPACING;
    taskdialog_du_to_px(dialog_info, &dialog_info->m.h_spacing, &dialog_info->m.v_spacing);

    if (taskconfig->dwFlags & TDF_CALLBACK_TIMER)
    {
        SetTimer(hwnd, ID_TIMER, DIALOG_TIMER_MS, NULL);
        dialog_info->last_timer_tick = GetTickCount();
    }

    taskdialog_add_main_instruction(dialog_info);

    taskdialog_layout(dialog_info);
}

static void taskdialog_destroy(struct taskdialog_info *dialog_info)
{
    if (dialog_info->taskconfig->dwFlags & TDF_CALLBACK_TIMER) KillTimer(dialog_info->hwnd, ID_TIMER);
    if (dialog_info->font) DeleteObject(dialog_info->font);
    if (dialog_info->main_instruction_font) DeleteObject(dialog_info->main_instruction_font);
}

static INT_PTR CALLBACK taskdialog_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static const WCHAR taskdialog_info_propnameW[] = {'T','a','s','k','D','i','a','l','o','g','I','n','f','o',0};
    struct taskdialog_info *dialog_info;

    TRACE("hwnd=%p msg=0x%04x wparam=%lx lparam=%lx\n", hwnd, msg, wParam, lParam);

    if (msg != WM_INITDIALOG)
        dialog_info = GetPropW(hwnd, taskdialog_info_propnameW);

    switch (msg)
    {
        case TDM_CLICK_BUTTON:
            taskdialog_on_button_click(dialog_info, LOWORD(wParam));
            break;
        case WM_INITDIALOG:
            dialog_info = (struct taskdialog_info *)lParam;

            taskdialog_init(dialog_info, hwnd);

            SetPropW(hwnd, taskdialog_info_propnameW, dialog_info);
            taskdialog_notify(dialog_info, TDN_DIALOG_CONSTRUCTED, 0, 0);
            taskdialog_notify(dialog_info, TDN_CREATED, 0, 0);
            break;
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                taskdialog_on_button_click(dialog_info, LOWORD(wParam));
                break;
            }
            return FALSE;
        case WM_HELP:
            taskdialog_notify(dialog_info, TDN_HELP, 0, 0);
            break;
        case WM_TIMER:
            if (ID_TIMER == wParam)
            {
                DWORD elapsed = GetTickCount() - dialog_info->last_timer_tick;
                if (taskdialog_notify(dialog_info, TDN_TIMER, elapsed, 0) == S_FALSE)
                    dialog_info->last_timer_tick = GetTickCount();
            }
            break;
        case WM_DESTROY:
            taskdialog_notify(dialog_info, TDN_DESTROYED, 0, 0);
            RemovePropW(hwnd, taskdialog_info_propnameW);
            taskdialog_destroy(dialog_info);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 * TaskDialogIndirect [COMCTL32.@]
 */
HRESULT WINAPI TaskDialogIndirect(const TASKDIALOGCONFIG *taskconfig, int *button,
                                  int *radio_button, BOOL *verification_flag_checked)
{
    struct taskdialog_info dialog_info;
    DLGTEMPLATE *template;
    INT ret;

    TRACE("%p, %p, %p, %p\n", taskconfig, button, radio_button, verification_flag_checked);

    if (!taskconfig || taskconfig->cbSize != sizeof(TASKDIALOGCONFIG))
        return E_INVALIDARG;

    dialog_info.taskconfig = taskconfig;

    template = create_taskdialog_template(taskconfig);
    ret = (short)DialogBoxIndirectParamW(taskconfig->hInstance, template, taskconfig->hwndParent,
            taskdialog_proc, (LPARAM)&dialog_info);
    Free(template);

    if (button) *button = ret;
    if (radio_button) *radio_button = taskconfig->nDefaultButton;
    if (verification_flag_checked) *verification_flag_checked = TRUE;

    return S_OK;
}

/***********************************************************************
 * TaskDialog [COMCTL32.@]
 */
HRESULT WINAPI TaskDialog(HWND owner, HINSTANCE hinst, const WCHAR *title, const WCHAR *main_instruction,
    const WCHAR *content, TASKDIALOG_COMMON_BUTTON_FLAGS common_buttons, const WCHAR *icon, int *button)
{
    TASKDIALOGCONFIG taskconfig;

    TRACE("%p, %p, %s, %s, %s, %#x, %s, %p\n", owner, hinst, debugstr_w(title), debugstr_w(main_instruction),
        debugstr_w(content), common_buttons, debugstr_w(icon), button);

    memset(&taskconfig, 0, sizeof(taskconfig));
    taskconfig.cbSize = sizeof(taskconfig);
    taskconfig.hwndParent = owner;
    taskconfig.hInstance = hinst;
    taskconfig.dwCommonButtons = common_buttons;
    taskconfig.pszWindowTitle = title;
    taskconfig.u.pszMainIcon = icon;
    taskconfig.pszMainInstruction = main_instruction;
    taskconfig.pszContent = content;
    return TaskDialogIndirect(&taskconfig, button, NULL, NULL);
}
