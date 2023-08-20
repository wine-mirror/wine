/*
 *	AutoComplete interfaces implementation.
 *
 *	Copyright 2004	Maxime Bellengé <maxime.bellenge@laposte.net>
 *	Copyright 2018	Gabriel Ivăncescu <gabrielopcode@gmail.com>
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

/*
  TODO:
  - implement ACO_SEARCH style
  - implement ACO_RTLREADING style
  - implement ACO_WORD_FILTER style
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define COBJMACROS

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shlwapi.h"
#include "winerror.h"
#include "objbase.h"

#include "pidl.h"
#include "shlobj.h"
#include "shldisp.h"
#include "debughlp.h"
#include "shell32_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

typedef struct
{
    IAutoComplete2 IAutoComplete2_iface;
    IAutoCompleteDropDown IAutoCompleteDropDown_iface;
    LONG ref;
    BOOL initialized;
    BOOL enabled;
    UINT enum_strs_num;
    WCHAR **enum_strs;
    WCHAR **listbox_strs;
    HWND hwndEdit;
    HWND hwndListBox;
    HWND hwndListBoxOwner;
    WNDPROC wpOrigEditProc;
    WNDPROC wpOrigLBoxProc;
    WNDPROC wpOrigLBoxOwnerProc;
    WCHAR *txtbackup;
    WCHAR *quickComplete;
    IEnumString *enumstr;
    IACList *aclist;
    AUTOCOMPLETEOPTIONS options;
    WCHAR no_fwd_char;
} IAutoCompleteImpl;

enum autoappend_flag
{
    autoappend_flag_yes,
    autoappend_flag_no,
    autoappend_flag_displayempty
};

enum prefix_filtering
{
    prefix_filtering_none = 0,  /* no prefix filtering (raw search) */
    prefix_filtering_protocol,  /* filter common protocol (e.g. http://) */
    prefix_filtering_all        /* filter all common prefixes (protocol & www. ) */
};

static const WCHAR autocomplete_propertyW[] = L"Wine Autocomplete control";

static inline IAutoCompleteImpl *impl_from_IAutoComplete2(IAutoComplete2 *iface)
{
    return CONTAINING_RECORD(iface, IAutoCompleteImpl, IAutoComplete2_iface);
}

static inline IAutoCompleteImpl *impl_from_IAutoCompleteDropDown(IAutoCompleteDropDown *iface)
{
    return CONTAINING_RECORD(iface, IAutoCompleteImpl, IAutoCompleteDropDown_iface);
}

static void set_text_and_selection(IAutoCompleteImpl *ac, HWND hwnd, WCHAR *text, WPARAM start, LPARAM end)
{
    /* Send it directly to the edit control to match Windows behavior */
    WNDPROC proc = ac->wpOrigEditProc;
    if (CallWindowProcW(proc, hwnd, WM_SETTEXT, 0, (LPARAM)text))
        CallWindowProcW(proc, hwnd, EM_SETSEL, start, end);
}

static inline WCHAR *filter_protocol(WCHAR *str)
{
    if (!wcsncmp(str, L"http", 4))
    {
        str += 4;
        str += (*str == 's');    /* https */
        if (str[0] == ':' && str[1] == '/' && str[2] == '/')
            return str + 3;
    }
    return NULL;
}

static inline WCHAR *filter_www(WCHAR *str)
{
    if (!wcsncmp(str, L"www.", 4)) return str + 4;
    return NULL;
}

/*
   Get the prefix filtering based on text, for example if text's prefix
   is a protocol, then we return none because we actually filter nothing
*/
static enum prefix_filtering get_text_prefix_filtering(const WCHAR *text)
{
    /* Convert to lowercase to perform case insensitive filtering,
       using the longest possible prefix as the size of the buffer */
    WCHAR buf[sizeof("https://")];
    UINT i;

    for (i = 0; i < ARRAY_SIZE(buf) - 1 && text[i]; i++)
        buf[i] = towlower(text[i]);
    buf[i] = '\0';

    if (filter_protocol(buf)) return prefix_filtering_none;
    if (filter_www(buf)) return prefix_filtering_protocol;
    return prefix_filtering_all;
}

/*
   Filter the prefix of str based on the value of pfx_filter
   This is used in sorting, so it's more performance sensitive
*/
static WCHAR *filter_str_prefix(WCHAR *str, enum prefix_filtering pfx_filter)
{
    WCHAR *p = str;

    if (pfx_filter == prefix_filtering_none) return str;
    if ((p = filter_protocol(str))) str = p;

    if (pfx_filter == prefix_filtering_protocol) return str;
    if ((p = filter_www(str))) str = p;

    return str;
}

static inline int sort_strs_cmpfn_impl(WCHAR *a, WCHAR *b, enum prefix_filtering pfx_filter)
{
    WCHAR *str1 = filter_str_prefix(a, pfx_filter);
    WCHAR *str2 = filter_str_prefix(b, pfx_filter);
    return wcsicmp(str1, str2);
}

static int __cdecl sort_strs_cmpfn_none(const void *a, const void *b)
{
    return sort_strs_cmpfn_impl(*(WCHAR* const*)a, *(WCHAR* const*)b, prefix_filtering_none);
}

static int __cdecl sort_strs_cmpfn_protocol(const void *a, const void *b)
{
    return sort_strs_cmpfn_impl(*(WCHAR* const*)a, *(WCHAR* const*)b, prefix_filtering_protocol);
}

static int __cdecl sort_strs_cmpfn_all(const void *a, const void *b)
{
    return sort_strs_cmpfn_impl(*(WCHAR* const*)a, *(WCHAR* const*)b, prefix_filtering_all);
}

static int (* __cdecl sort_strs_cmpfn[])(const void*, const void*) =
{
    sort_strs_cmpfn_none,
    sort_strs_cmpfn_protocol,
    sort_strs_cmpfn_all
};

static void sort_strs(WCHAR **strs, UINT numstrs, enum prefix_filtering pfx_filter)
{
    qsort(strs, numstrs, sizeof(*strs), sort_strs_cmpfn[pfx_filter]);
}

/*
   Enumerate all of the strings and sort them in the internal list.

   We don't free the enumerated strings (except on error) to avoid needless
   copies, until the next reset (or the object itself is destroyed)
*/
static void enumerate_strings(IAutoCompleteImpl *ac, enum prefix_filtering pfx_filter)
{
    UINT cur = 0, array_size = 1024;
    LPOLESTR *strs = NULL, *tmp;
    ULONG read;

    do
    {
        if ((tmp = realloc(strs, array_size * sizeof(*strs))) == NULL)
            goto fail;
        strs = tmp;

        do
        {
            if (FAILED(IEnumString_Next(ac->enumstr, array_size - cur, &strs[cur], &read)))
                read = 0;
        } while (read != 0 && (cur += read) < array_size);

        array_size *= 2;
    } while (read != 0);

    /* Allocate even if there were zero strings enumerated, to mark it non-NULL */
    if ((tmp = realloc(strs, cur * sizeof(*strs))))
    {
        strs = tmp;
        if (cur > 0)
            sort_strs(strs, cur, pfx_filter);

        ac->enum_strs = strs;
        ac->enum_strs_num = cur;
        return;
    }

fail:
    while (cur--)
        CoTaskMemFree(strs[cur]);
    free(strs);
}

static UINT find_matching_enum_str(IAutoCompleteImpl *ac, UINT start, WCHAR *text,
                                   UINT len, enum prefix_filtering pfx_filter, int direction)
{
    WCHAR **strs = ac->enum_strs;
    UINT index = ~0, a = start, b = ac->enum_strs_num;
    while (a < b)
    {
        UINT i = (a + b - 1) / 2;
        int cmp = wcsnicmp(text, filter_str_prefix(strs[i], pfx_filter), len);
        if (cmp == 0)
        {
            index = i;
            cmp   = direction;
        }
        if (cmp <= 0) b = i;
        else          a = i + 1;
    }
    return index;
}

static void free_enum_strs(IAutoCompleteImpl *ac)
{
    WCHAR **strs = ac->enum_strs;
    if (strs)
    {
        UINT i = ac->enum_strs_num;
        ac->enum_strs = NULL;
        while (i--)
            CoTaskMemFree(strs[i]);
        free(strs);
    }
}

static void hide_listbox(IAutoCompleteImpl *ac, HWND hwnd, BOOL reset)
{
    ShowWindow(ac->hwndListBoxOwner, SW_HIDE);
    SendMessageW(hwnd, LB_RESETCONTENT, 0, 0);
    if (reset) free_enum_strs(ac);
}

static void show_listbox(IAutoCompleteImpl *ac)
{
    RECT r;
    UINT cnt, width, height;

    GetWindowRect(ac->hwndEdit, &r);

    /* Windows XP displays 7 lines at most, then it uses a scroll bar */
    cnt    = SendMessageW(ac->hwndListBox, LB_GETCOUNT, 0, 0);
    height = SendMessageW(ac->hwndListBox, LB_GETITEMHEIGHT, 0, 0) * min(cnt + 1, 7);
    width = r.right - r.left;

    SetWindowPos(ac->hwndListBoxOwner, HWND_TOP, r.left, r.bottom + 1, width, height,
                 SWP_SHOWWINDOW | SWP_NOACTIVATE);
}

static void set_listbox_font(IAutoCompleteImpl *ac, HFONT font)
{
    /* We have to calculate the item height manually due to owner-drawn */
    HFONT old_font = NULL;
    UINT height = 16;
    HDC hdc;

    if ((hdc = GetDCEx(ac->hwndListBox, 0, DCX_CACHE)))
    {
        TEXTMETRICW metrics;
        if (font) old_font = SelectObject(hdc, font);
        if (GetTextMetricsW(hdc, &metrics))
            height = metrics.tmHeight;
        if (old_font) SelectObject(hdc, old_font);
        ReleaseDC(ac->hwndListBox, hdc);
    }
    SendMessageW(ac->hwndListBox, WM_SETFONT, (WPARAM)font, FALSE);
    SendMessageW(ac->hwndListBox, LB_SETITEMHEIGHT, 0, height);
}

static BOOL draw_listbox_item(IAutoCompleteImpl *ac, DRAWITEMSTRUCT *info, UINT id)
{
    COLORREF old_text, old_bk;
    HDC hdc = info->hDC;
    UINT state;
    WCHAR *str;

    if (info->CtlType != ODT_LISTBOX || info->CtlID != id ||
        id != (UINT)GetWindowLongPtrW(ac->hwndListBox, GWLP_ID))
        return FALSE;

    if ((INT)info->itemID < 0 || info->itemAction == ODA_FOCUS)
        return TRUE;

    state = info->itemState;
    if (state & ODS_SELECTED)
    {
        old_bk = SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
        old_text = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
    }

    str = ac->listbox_strs[info->itemID];
    ExtTextOutW(hdc, info->rcItem.left + 1, info->rcItem.top,
                ETO_OPAQUE | ETO_CLIPPED, &info->rcItem, str,
                lstrlenW(str), NULL);

    if (state & ODS_SELECTED)
    {
        SetBkColor(hdc, old_bk);
        SetTextColor(hdc, old_text);
    }
    return TRUE;
}

static size_t format_quick_complete(WCHAR *dst, const WCHAR *qc, const WCHAR *str, size_t str_len)
{
    /* Replace the first %s directly without using snprintf, to avoid
       exploits since the format string can be retrieved from the registry */
    WCHAR *base = dst;
    UINT args = 0;
    while (*qc != '\0')
    {
        if (qc[0] == '%')
        {
            if (args < 1 && qc[1] == 's')
            {
                memcpy(dst, str, str_len * sizeof(WCHAR));
                dst += str_len;
                qc += 2;
                args++;
                continue;
            }
            qc += (qc[1] == '%');
        }
        *dst++ = *qc++;
    }
    *dst = '\0';
    return dst - base;
}

static BOOL select_item_with_return_key(IAutoCompleteImpl *ac, HWND hwnd)
{
    WCHAR *text;
    HWND hwndListBox = ac->hwndListBox;
    if (!(ac->options & ACO_AUTOSUGGEST))
        return FALSE;

    if (IsWindowVisible(ac->hwndListBoxOwner))
    {
        INT sel = SendMessageW(hwndListBox, LB_GETCURSEL, 0, 0);
        if (sel >= 0)
        {
            text = ac->listbox_strs[sel];
            set_text_and_selection(ac, hwnd, text, 0, lstrlenW(text));
            hide_listbox(ac, hwndListBox, TRUE);
            ac->no_fwd_char = '\r';  /* RETURN char */
            return TRUE;
        }
    }
    hide_listbox(ac, hwndListBox, TRUE);
    return FALSE;
}

static LRESULT change_selection(IAutoCompleteImpl *ac, HWND hwnd, UINT key)
{
    WCHAR *msg;
    UINT len;

    INT count = SendMessageW(ac->hwndListBox, LB_GETCOUNT, 0, 0);
    INT sel = SendMessageW(ac->hwndListBox, LB_GETCURSEL, 0, 0);
    if (key == VK_PRIOR || key == VK_NEXT)
    {
        if (sel < 0)
            sel = (key == VK_PRIOR) ? count - 1 : 0;
        else
        {
            INT base = SendMessageW(ac->hwndListBox, LB_GETTOPINDEX, 0, 0);
            INT pgsz = SendMessageW(ac->hwndListBox, LB_GETLISTBOXINFO, 0, 0);
            pgsz = max(pgsz - 1, 1);
            if (key == VK_PRIOR)
            {
                if (sel == 0)
                    sel = -1;
                else
                {
                    if (sel == base) base -= min(base, pgsz);
                    sel = base;
                }
            }
            else
            {
                if (sel == count - 1)
                    sel = -1;
                else
                {
                    base += pgsz;
                    if (sel >= base) base += pgsz;
                    sel = min(base, count - 1);
                }
            }
        }
    }
    else if (key == VK_UP || (key == VK_TAB && (GetKeyState(VK_SHIFT) & 0x8000)))
        sel = ((sel - 1) < -1) ? count - 1 : sel - 1;
    else
        sel = ((sel + 1) >= count) ? -1 : sel + 1;

    SendMessageW(ac->hwndListBox, LB_SETCURSEL, sel, 0);

    msg = (sel >= 0) ? ac->listbox_strs[sel] : ac->txtbackup;
    len = lstrlenW(msg);
    set_text_and_selection(ac, hwnd, msg, len, len);

    return 0;
}

static BOOL do_aclist_expand(IAutoCompleteImpl *ac, WCHAR *txt, WCHAR *last_delim)
{
    WCHAR c = last_delim[1];

    free_enum_strs(ac);
    IEnumString_Reset(ac->enumstr);  /* call before expand */

    last_delim[1] = '\0';
    IACList_Expand(ac->aclist, txt);
    last_delim[1] = c;
    return TRUE;
}

static BOOL aclist_expand(IAutoCompleteImpl *ac, WCHAR *txt)
{
    /* call IACList::Expand only when needed, if the
       new txt and old_txt require different expansions */

    const WCHAR *old_txt = ac->txtbackup;
    WCHAR c, *p, *last_delim;
    size_t i = 0;

    /* always expand if the enumerator was reset */
    if (!ac->enum_strs) old_txt = L"";

    /* skip the shared prefix */
    while ((c = towlower(txt[i])) == towlower(old_txt[i]))
    {
        if (c == '\0') return FALSE;
        i++;
    }

    /* they differ at this point, check for a delim further in txt */
    for (last_delim = NULL, p = &txt[i]; (p = wcspbrk(p, L"\\/")) != NULL; p++)
        last_delim = p;
    if (last_delim) return do_aclist_expand(ac, txt, last_delim);

    /* txt has no delim after i, check for a delim further in old_txt */
    if (wcspbrk(&old_txt[i], L"\\/"))
    {
        /* scan backwards to find the first delim before txt[i] (if any) */
        while (i--)
            if (wcschr(L"\\/", txt[i]))
                return do_aclist_expand(ac, txt, &txt[i]);

        /* Windows doesn't expand without a delim, but it does reset */
        free_enum_strs(ac);
    }

    return FALSE;
}

static void autoappend_str(IAutoCompleteImpl *ac, WCHAR *text, UINT len, WCHAR *str, HWND hwnd)
{
    DWORD sel_start;
    WCHAR *tmp;
    size_t size;

    /* Don't auto-append unless the caret is at the end */
    SendMessageW(hwnd, EM_GETSEL, (WPARAM)&sel_start, 0);
    if (sel_start != len)
        return;

    /* The character capitalization can be different,
       so merge text and str into a new string */
    size = len + lstrlenW(&str[len]) + 1;

    if ((tmp = malloc(size * sizeof(*tmp))))
    {
        memcpy(tmp, text, len * sizeof(*tmp));
        memcpy(&tmp[len], &str[len], (size - len) * sizeof(*tmp));
    }
    else tmp = str;

    set_text_and_selection(ac, hwnd, tmp, len, size - 1);
    if (tmp != str)
        free(tmp);
}

static BOOL display_matching_strs(IAutoCompleteImpl *ac, WCHAR *text, UINT len,
                                  enum prefix_filtering pfx_filter, HWND hwnd,
                                  enum autoappend_flag flag)
{
    /* Return FALSE if we need to hide the listbox */
    WCHAR **str = ac->enum_strs;
    UINT start, end;
    if (!str) return !(ac->options & ACO_AUTOSUGGEST);

    /* Windows seems to disable autoappend if ACO_NOPREFIXFILTERING is set */
    if (!(ac->options & ACO_NOPREFIXFILTERING) && len)
    {
        start = find_matching_enum_str(ac, 0, text, len, pfx_filter, -1);
        if (start == ~0)
            return !(ac->options & ACO_AUTOSUGGEST);

        if (flag == autoappend_flag_yes)
            autoappend_str(ac, text, len, filter_str_prefix(str[start], pfx_filter), hwnd);
        if (!(ac->options & ACO_AUTOSUGGEST))
            return TRUE;

        /* Find the index beyond the last string that matches */
        end = find_matching_enum_str(ac, start + 1, text, len, pfx_filter, 1);
        end = (end == ~0 ? start : end) + 1;
    }
    else
    {
        if (!(ac->options & ACO_AUTOSUGGEST))
            return TRUE;
        start = 0;
        end = ac->enum_strs_num;
        if (end == 0)
            return FALSE;
    }

    SendMessageW(ac->hwndListBox, WM_SETREDRAW, FALSE, 0);
    SendMessageW(ac->hwndListBox, LB_RESETCONTENT, 0, 0);

    ac->listbox_strs = str + start;
    SendMessageW(ac->hwndListBox, LB_SETCOUNT, end - start, 0);

    show_listbox(ac);
    SendMessageW(ac->hwndListBox, WM_SETREDRAW, TRUE, 0);
    return TRUE;
}

static enum prefix_filtering setup_prefix_filtering(IAutoCompleteImpl *ac, const WCHAR *text)
{
    enum prefix_filtering pfx_filter;
    if (!(ac->options & ACO_FILTERPREFIXES)) return prefix_filtering_none;

    pfx_filter = get_text_prefix_filtering(text);
    if (!ac->enum_strs) return pfx_filter;

    /* If the prefix filtering is different, re-sort the filtered strings */
    if (pfx_filter != get_text_prefix_filtering(ac->txtbackup))
        sort_strs(ac->enum_strs, ac->enum_strs_num, pfx_filter);

    return pfx_filter;
}

static void autocomplete_text(IAutoCompleteImpl *ac, HWND hwnd, enum autoappend_flag flag)
{
    WCHAR *text;
    BOOL expanded = FALSE;
    enum prefix_filtering pfx_filter;
    UINT size, len = SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0);

    if (flag != autoappend_flag_displayempty && len == 0)
    {
        if (ac->options & ACO_AUTOSUGGEST)
            hide_listbox(ac, ac->hwndListBox, FALSE);
        free_enum_strs(ac);
        return;
    }

    size = len + 1;
    if (!(text = malloc(size * sizeof(WCHAR))))
    {
        /* Reset the listbox to prevent potential crash from ResetEnumerator */
        SendMessageW(ac->hwndListBox, LB_RESETCONTENT, 0, 0);
        return;
    }
    len = SendMessageW(hwnd, WM_GETTEXT, size, (LPARAM)text);
    if (len + 1 != size)
        text = realloc(text, (len + 1) * sizeof(WCHAR));

    if (ac->aclist)
    {
        if (text[len - 1] == '\\' || text[len - 1] == '/')
            flag = autoappend_flag_no;
        expanded = aclist_expand(ac, text);
    }
    pfx_filter = setup_prefix_filtering(ac, text);

    if (expanded || !ac->enum_strs)
    {
        if (!expanded) IEnumString_Reset(ac->enumstr);
        enumerate_strings(ac, pfx_filter);
    }

    /* Set txtbackup to point to text itself (which must not be released),
       and it must be done here since aclist_expand uses it to track changes */
    free(ac->txtbackup);
    ac->txtbackup = text;

    if (!display_matching_strs(ac, text, len, pfx_filter, hwnd, flag))
        hide_listbox(ac, ac->hwndListBox, FALSE);
}

static void destroy_autocomplete_object(IAutoCompleteImpl *ac)
{
    ac->hwndEdit = NULL;
    free_enum_strs(ac);
    if (ac->hwndListBoxOwner)
        DestroyWindow(ac->hwndListBoxOwner);
    IAutoComplete2_Release(&ac->IAutoComplete2_iface);
}

/*
   Helper for ACEditSubclassProc
*/
static LRESULT ACEditSubclassProc_KeyDown(IAutoCompleteImpl *ac, HWND hwnd, UINT uMsg,
                                          WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
        case VK_ESCAPE:
            /* When pressing ESC, Windows hides the auto-suggest listbox, if visible */
            if ((ac->options & ACO_AUTOSUGGEST) && IsWindowVisible(ac->hwndListBoxOwner))
            {
                hide_listbox(ac, ac->hwndListBox, FALSE);
                ac->no_fwd_char = 0x1B;  /* ESC char */
                return 0;
            }
            break;
        case VK_RETURN:
            /* If quickComplete is set and control is pressed, replace the string */
            if (ac->quickComplete && (GetKeyState(VK_CONTROL) & 0x8000))
            {
                WCHAR *text, *buf;
                size_t sz;
                UINT len = SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0);
                ac->no_fwd_char = '\n';  /* CTRL+RETURN char */

                if (!(text = malloc((len + 1) * sizeof(WCHAR))))
                    return 0;
                len = SendMessageW(hwnd, WM_GETTEXT, len + 1, (LPARAM)text);
                sz = lstrlenW(ac->quickComplete) + 1 + len;

                if ((buf = malloc(sz * sizeof(WCHAR))))
                {
                    len = format_quick_complete(buf, ac->quickComplete, text, len);
                    set_text_and_selection(ac, hwnd, buf, 0, len);
                    free(buf);
                }

                if (ac->options & ACO_AUTOSUGGEST)
                    hide_listbox(ac, ac->hwndListBox, TRUE);
                free(text);
                return 0;
            }

            if (select_item_with_return_key(ac, hwnd))
                return 0;
            break;
        case VK_TAB:
            if ((ac->options & (ACO_AUTOSUGGEST | ACO_USETAB)) == (ACO_AUTOSUGGEST | ACO_USETAB)
                && IsWindowVisible(ac->hwndListBoxOwner) && !(GetKeyState(VK_CONTROL) & 0x8000))
            {
                ac->no_fwd_char = '\t';
                return change_selection(ac, hwnd, wParam);
            }
            break;
        case VK_UP:
        case VK_DOWN:
        case VK_PRIOR:
        case VK_NEXT:
            /* Two cases here:
               - if the listbox is not visible and ACO_UPDOWNKEYDROPSLIST is
                 set, display it with all the entries, without selecting any
               - if the listbox is visible, change the selection
            */
            if (!(ac->options & ACO_AUTOSUGGEST))
                break;

            if (!IsWindowVisible(ac->hwndListBoxOwner))
            {
                if (ac->options & ACO_UPDOWNKEYDROPSLIST)
                {
                    autocomplete_text(ac, hwnd, autoappend_flag_displayempty);
                    return 0;
                }
            }
            else
                return change_selection(ac, hwnd, wParam);
            break;
        case VK_DELETE:
        {
            LRESULT ret = CallWindowProcW(ac->wpOrigEditProc, hwnd, uMsg, wParam, lParam);
            autocomplete_text(ac, hwnd, autoappend_flag_no);
            return ret;
        }
    }
    ac->no_fwd_char = '\0';
    return CallWindowProcW(ac->wpOrigEditProc, hwnd, uMsg, wParam, lParam);
}

/*
  Window procedure for autocompletion
 */
static LRESULT APIENTRY ACEditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    IAutoCompleteImpl *This = GetPropW(hwnd, autocomplete_propertyW);
    LRESULT ret;

    if (!This->enabled) return CallWindowProcW(This->wpOrigEditProc, hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
        case CB_SHOWDROPDOWN:
            if (This->options & ACO_AUTOSUGGEST)
                hide_listbox(This, This->hwndListBox, TRUE);
            return 0;
        case WM_KILLFOCUS:
            if (This->options & ACO_AUTOSUGGEST)
            {
                if (This->hwndListBoxOwner == (HWND)wParam ||
                    This->hwndListBoxOwner == GetAncestor((HWND)wParam, GA_PARENT))
                    break;
                hide_listbox(This, This->hwndListBox, FALSE);
            }

            /* Reset the enumerator if it's not visible anymore */
            if (!IsWindowVisible(hwnd)) free_enum_strs(This);
            break;
        case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS *pos = (WINDOWPOS*)lParam;

            if ((pos->flags & (SWP_NOMOVE | SWP_NOSIZE)) != (SWP_NOMOVE | SWP_NOSIZE) &&
                This->hwndListBoxOwner && IsWindowVisible(This->hwndListBoxOwner))
                show_listbox(This);
            break;
        }
        case WM_KEYDOWN:
            return ACEditSubclassProc_KeyDown(This, hwnd, uMsg, wParam, lParam);
        case WM_CHAR:
        case WM_UNICHAR:
            if (wParam == This->no_fwd_char) return 0;
            This->no_fwd_char = '\0';

            /* Don't autocomplete at all on most control characters */
            if (wParam < 32 && !(wParam >= '\b' && wParam <= '\r'))
                break;

            ret = CallWindowProcW(This->wpOrigEditProc, hwnd, uMsg, wParam, lParam);
            autocomplete_text(This, hwnd, (This->options & ACO_AUTOAPPEND) && wParam >= ' '
                                          ? autoappend_flag_yes : autoappend_flag_no);
            return ret;
        case WM_SETTEXT:
            if (This->options & ACO_AUTOSUGGEST)
                hide_listbox(This, This->hwndListBox, TRUE);
            break;
        case WM_CUT:
        case WM_CLEAR:
        case WM_UNDO:
            ret = CallWindowProcW(This->wpOrigEditProc, hwnd, uMsg, wParam, lParam);
            autocomplete_text(This, hwnd, autoappend_flag_no);
            return ret;
        case WM_PASTE:
            ret = CallWindowProcW(This->wpOrigEditProc, hwnd, uMsg, wParam, lParam);
            autocomplete_text(This, hwnd, (This->options & ACO_AUTOAPPEND)
                                          ? autoappend_flag_yes : autoappend_flag_no);
            return ret;
        case WM_MOUSEWHEEL:
            if ((This->options & ACO_AUTOSUGGEST) && IsWindowVisible(This->hwndListBoxOwner))
                return SendMessageW(This->hwndListBox, WM_MOUSEWHEEL, wParam, lParam);
            break;
        case WM_SETFONT:
            if (This->hwndListBox)
                set_listbox_font(This, (HFONT)wParam);
            break;
        case WM_DESTROY:
        {
            WNDPROC proc = This->wpOrigEditProc;

            SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)proc);
            RemovePropW(hwnd, autocomplete_propertyW);
            destroy_autocomplete_object(This);
            return CallWindowProcW(proc, hwnd, uMsg, wParam, lParam);
        }
    }
    return CallWindowProcW(This->wpOrigEditProc, hwnd, uMsg, wParam, lParam);
}

static LRESULT APIENTRY ACLBoxSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    IAutoCompleteImpl *This = (IAutoCompleteImpl *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    WCHAR *msg;
    INT sel;

    switch (uMsg) {
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case WM_MOUSEMOVE:
            sel = SendMessageW(hwnd, LB_ITEMFROMPOINT, 0, lParam);
            SendMessageW(hwnd, LB_SETCURSEL, sel, 0);
            return 0;
        case WM_LBUTTONDOWN:
            sel = SendMessageW(hwnd, LB_GETCURSEL, 0, 0);
            if (sel < 0)
                return 0;
            msg = This->listbox_strs[sel];
            set_text_and_selection(This, This->hwndEdit, msg, 0, lstrlenW(msg));
            hide_listbox(This, hwnd, TRUE);
            return 0;
    }
    return CallWindowProcW(This->wpOrigLBoxProc, hwnd, uMsg, wParam, lParam);
}

static LRESULT APIENTRY ACLBoxOwnerSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    IAutoCompleteImpl *This = (IAutoCompleteImpl*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case WM_DRAWITEM:
            if (draw_listbox_item(This, (DRAWITEMSTRUCT*)lParam, wParam))
                return TRUE;
            break;
        case WM_SIZE:
            SetWindowPos(This->hwndListBox, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam),
                         SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_DEFERERASE);
            break;
    }
    return CallWindowProcW(This->wpOrigLBoxOwnerProc, hwnd, uMsg, wParam, lParam);
}

static void create_listbox(IAutoCompleteImpl *This)
{
    This->hwndListBoxOwner = CreateWindowExW(WS_EX_NOACTIVATE, WC_STATICW, NULL,
                                             WS_BORDER | WS_POPUP | WS_CLIPCHILDREN,
                                             0, 0, 0, 0, NULL, NULL, shell32_hInstance, NULL);
    if (!This->hwndListBoxOwner)
    {
        This->options &= ~ACO_AUTOSUGGEST;
        return;
    }

    /* FIXME : The listbox should be resizable with the mouse. WS_THICKFRAME looks ugly */
    This->hwndListBox = CreateWindowExW(WS_EX_NOACTIVATE, WC_LISTBOXW, NULL,
                                        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NODATA | LBS_OWNERDRAWFIXED | LBS_NOINTEGRALHEIGHT,
                                        0, 0, 0, 0, This->hwndListBoxOwner, NULL, shell32_hInstance, NULL);

    if (This->hwndListBox) {
        HFONT edit_font;

        This->wpOrigLBoxProc = (WNDPROC) SetWindowLongPtrW( This->hwndListBox, GWLP_WNDPROC, (LONG_PTR) ACLBoxSubclassProc);
        SetWindowLongPtrW( This->hwndListBox, GWLP_USERDATA, (LONG_PTR)This);

        This->wpOrigLBoxOwnerProc = (WNDPROC)SetWindowLongPtrW(This->hwndListBoxOwner, GWLP_WNDPROC, (LONG_PTR)ACLBoxOwnerSubclassProc);
        SetWindowLongPtrW(This->hwndListBoxOwner, GWLP_USERDATA, (LONG_PTR)This);

        /* Use the same font as the edit control, as it gets destroyed before it anyway */
        edit_font = (HFONT)SendMessageW(This->hwndEdit, WM_GETFONT, 0, 0);
        if (edit_font)
            set_listbox_font(This, edit_font);
        return;
    }

    DestroyWindow(This->hwndListBoxOwner);
    This->hwndListBoxOwner = NULL;
    This->options &= ~ACO_AUTOSUGGEST;
}

/**************************************************************************
 *  AutoComplete_QueryInterface
 */
static HRESULT WINAPI IAutoComplete2_fnQueryInterface(
    IAutoComplete2 * iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    IAutoCompleteImpl *This = impl_from_IAutoComplete2(iface);

    TRACE("(%p)->(IID:%s,%p)\n", This, shdebugstr_guid(riid), ppvObj);
    *ppvObj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAutoComplete) ||
        IsEqualIID(riid, &IID_IAutoComplete2))
    {
        *ppvObj = &This->IAutoComplete2_iface;
    }
    else if (IsEqualIID(riid, &IID_IAutoCompleteDropDown))
    {
        *ppvObj = &This->IAutoCompleteDropDown_iface;
    }

    if (*ppvObj)
    {
	IUnknown_AddRef((IUnknown*)*ppvObj);
	TRACE("-- Interface: (%p)->(%p)\n", ppvObj, *ppvObj);
	return S_OK;
    }
    WARN("unsupported interface: %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

/******************************************************************************
 * IAutoComplete2_fnAddRef
 */
static ULONG WINAPI IAutoComplete2_fnAddRef(
	IAutoComplete2 * iface)
{
    IAutoCompleteImpl *This = impl_from_IAutoComplete2(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(%lu)\n", This, refCount - 1);

    return refCount;
}

/******************************************************************************
 * IAutoComplete2_fnRelease
 */
static ULONG WINAPI IAutoComplete2_fnRelease(
	IAutoComplete2 * iface)
{
    IAutoCompleteImpl *This = impl_from_IAutoComplete2(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%lu)\n", This, refCount + 1);

    if (!refCount) {
        TRACE("destroying IAutoComplete(%p)\n", This);
        free(This->quickComplete);
        free(This->txtbackup);
        if (This->enumstr)
            IEnumString_Release(This->enumstr);
        if (This->aclist)
            IACList_Release(This->aclist);
        free(This);
    }
    return refCount;
}

/******************************************************************************
 * IAutoComplete2_fnEnable
 */
static HRESULT WINAPI IAutoComplete2_fnEnable(
    IAutoComplete2 * iface,
    BOOL fEnable)
{
    IAutoCompleteImpl *This = impl_from_IAutoComplete2(iface);

    TRACE("(%p)->(%s)\n", This, (fEnable)?"true":"false");

    This->enabled = fEnable;

    return S_OK;
}

/******************************************************************************
 * IAutoComplete2_fnInit
 */
static HRESULT WINAPI IAutoComplete2_fnInit(
    IAutoComplete2 * iface,
    HWND hwndEdit,
    IUnknown *punkACL,
    LPCOLESTR pwzsRegKeyPath,
    LPCOLESTR pwszQuickComplete)
{
    IAutoCompleteImpl *prev, *This = impl_from_IAutoComplete2(iface);

    TRACE("(%p)->(%p, %p, %s, %s)\n",
	  This, hwndEdit, punkACL, debugstr_w(pwzsRegKeyPath), debugstr_w(pwszQuickComplete));

    if (This->options & ACO_SEARCH) FIXME(" ACO_SEARCH not supported\n");
    if (This->options & ACO_RTLREADING) FIXME(" ACO_RTLREADING not supported\n");
    if (This->options & ACO_WORD_FILTER) FIXME(" ACO_WORD_FILTER not supported\n");

    if (!hwndEdit || !punkACL)
        return E_INVALIDARG;

    if (This->initialized)
    {
        WARN("Autocompletion object is already initialized\n");
        /* This->hwndEdit is set to NULL when the edit window is destroyed. */
        return This->hwndEdit ? E_FAIL : E_UNEXPECTED;
    }

    if (FAILED (IUnknown_QueryInterface (punkACL, &IID_IEnumString, (LPVOID*)&This->enumstr))) {
        WARN("No IEnumString interface\n");
        return E_NOINTERFACE;
    }

    /* Prevent txtbackup from ever being NULL to simplify aclist_expand */
    if ((This->txtbackup = calloc(1, sizeof(WCHAR))) == NULL)
    {
        IEnumString_Release(This->enumstr);
        This->enumstr = NULL;
        return E_OUTOFMEMORY;
    }

    if (FAILED (IUnknown_QueryInterface (punkACL, &IID_IACList, (LPVOID*)&This->aclist)))
        This->aclist = NULL;

    This->initialized = TRUE;
    This->hwndEdit = hwndEdit;

    /* If another AutoComplete object was previously assigned to this edit control,
       release it but keep the same callback on the control, to avoid an infinite
       recursive loop in ACEditSubclassProc while the property is set to this object */
    prev = GetPropW(hwndEdit, autocomplete_propertyW);
    SetPropW(hwndEdit, autocomplete_propertyW, This);

    if (prev && prev->initialized) {
        This->wpOrigEditProc = prev->wpOrigEditProc;
        destroy_autocomplete_object(prev);
    }
    else
        This->wpOrigEditProc = (WNDPROC) SetWindowLongPtrW(hwndEdit, GWLP_WNDPROC, (LONG_PTR) ACEditSubclassProc);

    /* Keep at least one reference to the object until the edit window is destroyed */
    IAutoComplete2_AddRef(&This->IAutoComplete2_iface);

    if (This->options & ACO_AUTOSUGGEST)
        create_listbox(This);

    if (pwzsRegKeyPath)
    {
        static const HKEY roots[] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
        WCHAR *key, *value;
        DWORD type, sz;
        BYTE *qc;
        HKEY hKey;
        LSTATUS res;
        size_t len;
        UINT i;

        /* pwszRegKeyPath contains the key as well as the value, so split it */
        value = wcsrchr(pwzsRegKeyPath, '\\');
        len = value - pwzsRegKeyPath;

        if (value && (key = malloc((len + 1) * sizeof(*key))) != NULL)
        {
            memcpy(key, pwzsRegKeyPath, len * sizeof(*key));
            key[len] = '\0';
            value++;

            for (i = 0; i < ARRAY_SIZE(roots); i++)
            {
                if (RegOpenKeyExW(roots[i], key, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
                    continue;
                sz = MAX_PATH * sizeof(WCHAR);

                while ((qc = malloc(sz)) != NULL)
                {
                    res = RegQueryValueExW(hKey, value, NULL, &type, qc, &sz);
                    if (res == ERROR_SUCCESS && type == REG_SZ)
                    {
                        This->quickComplete = realloc(qc, sz);
                        i = ARRAY_SIZE(roots);
                        break;
                    }
                    free(qc);
                    if (res != ERROR_MORE_DATA || type != REG_SZ)
                        break;
                }
                RegCloseKey(hKey);
            }
            free(key);
        }
    }

    if (!This->quickComplete && pwszQuickComplete)
    {
        size_t len = lstrlenW(pwszQuickComplete)+1;
        if ((This->quickComplete = malloc(len * sizeof(WCHAR))) != NULL)
            memcpy(This->quickComplete, pwszQuickComplete, len * sizeof(WCHAR));
    }

    return S_OK;
}

/**************************************************************************
 *  IAutoComplete2_fnGetOptions
 */
static HRESULT WINAPI IAutoComplete2_fnGetOptions(
    IAutoComplete2 * iface,
    DWORD *pdwFlag)
{
    IAutoCompleteImpl *This = impl_from_IAutoComplete2(iface);

    TRACE("(%p) -> (%p)\n", This, pdwFlag);

    *pdwFlag = This->options;

    return S_OK;
}

/**************************************************************************
 *  IAutoComplete2_fnSetOptions
 */
static HRESULT WINAPI IAutoComplete2_fnSetOptions(
    IAutoComplete2 * iface,
    DWORD dwFlag)
{
    IAutoCompleteImpl *This = impl_from_IAutoComplete2(iface);
    DWORD changed = This->options ^ dwFlag;
    HRESULT hr = S_OK;

    TRACE("(%p) -> (0x%lx)\n", This, dwFlag);

    This->options = dwFlag;

    if ((This->options & ACO_AUTOSUGGEST) && This->hwndEdit && !This->hwndListBox)
        create_listbox(This);
    else if (!(This->options & ACO_AUTOSUGGEST) && This->hwndListBox)
        hide_listbox(This, This->hwndListBox, TRUE);

    /* If ACO_FILTERPREFIXES changed we might have to reset the enumerator */
    if ((changed & ACO_FILTERPREFIXES) && This->txtbackup)
    {
        if (get_text_prefix_filtering(This->txtbackup) != prefix_filtering_none)
            IAutoCompleteDropDown_ResetEnumerator(&This->IAutoCompleteDropDown_iface);
    }

    return hr;
}

/**************************************************************************
 *  IAutoComplete2 VTable
 */
static const IAutoComplete2Vtbl acvt =
{
    IAutoComplete2_fnQueryInterface,
    IAutoComplete2_fnAddRef,
    IAutoComplete2_fnRelease,
    IAutoComplete2_fnInit,
    IAutoComplete2_fnEnable,
    /* IAutoComplete2 */
    IAutoComplete2_fnSetOptions,
    IAutoComplete2_fnGetOptions,
};


static HRESULT WINAPI IAutoCompleteDropDown_fnQueryInterface(IAutoCompleteDropDown *iface,
            REFIID riid, LPVOID *ppvObj)
{
    IAutoCompleteImpl *This = impl_from_IAutoCompleteDropDown(iface);
    return IAutoComplete2_QueryInterface(&This->IAutoComplete2_iface, riid, ppvObj);
}

static ULONG WINAPI IAutoCompleteDropDown_fnAddRef(IAutoCompleteDropDown *iface)
{
    IAutoCompleteImpl *This = impl_from_IAutoCompleteDropDown(iface);
    return IAutoComplete2_AddRef(&This->IAutoComplete2_iface);
}

static ULONG WINAPI IAutoCompleteDropDown_fnRelease(IAutoCompleteDropDown *iface)
{
    IAutoCompleteImpl *This = impl_from_IAutoCompleteDropDown(iface);
    return IAutoComplete2_Release(&This->IAutoComplete2_iface);
}

/**************************************************************************
 *  IAutoCompleteDropDown_fnGetDropDownStatus
 */
static HRESULT WINAPI IAutoCompleteDropDown_fnGetDropDownStatus(
    IAutoCompleteDropDown *iface,
    DWORD *pdwFlags,
    LPWSTR *ppwszString)
{
    IAutoCompleteImpl *This = impl_from_IAutoCompleteDropDown(iface);
    BOOL dropped;

    TRACE("(%p) -> (%p, %p)\n", This, pdwFlags, ppwszString);

    dropped = IsWindowVisible(This->hwndListBoxOwner);

    if (pdwFlags)
        *pdwFlags = (dropped ? ACDD_VISIBLE : 0);

    if (ppwszString) {
        if (dropped) {
            int sel;

            sel = SendMessageW(This->hwndListBox, LB_GETCURSEL, 0, 0);
            if (sel >= 0)
            {
                WCHAR *str = This->listbox_strs[sel];
                size_t size = (lstrlenW(str) + 1) * sizeof(*str);

                if (!(*ppwszString = CoTaskMemAlloc(size)))
                    return E_OUTOFMEMORY;
                memcpy(*ppwszString, str, size);
            }
            else
                *ppwszString = NULL;
        }
        else
            *ppwszString = NULL;
    }

    return S_OK;
}

/**************************************************************************
 *  IAutoCompleteDropDown_fnResetEnumarator
 */
static HRESULT WINAPI IAutoCompleteDropDown_fnResetEnumerator(
    IAutoCompleteDropDown *iface)
{
    IAutoCompleteImpl *This = impl_from_IAutoCompleteDropDown(iface);

    TRACE("(%p)\n", This);

    if (This->hwndEdit)
    {
        free_enum_strs(This);
        if ((This->options & ACO_AUTOSUGGEST) && IsWindowVisible(This->hwndListBoxOwner))
            autocomplete_text(This, This->hwndEdit, autoappend_flag_displayempty);
    }
    return S_OK;
}

/**************************************************************************
 *  IAutoCompleteDropDown VTable
 */
static const IAutoCompleteDropDownVtbl acdropdownvt =
{
    IAutoCompleteDropDown_fnQueryInterface,
    IAutoCompleteDropDown_fnAddRef,
    IAutoCompleteDropDown_fnRelease,
    IAutoCompleteDropDown_fnGetDropDownStatus,
    IAutoCompleteDropDown_fnResetEnumerator,
};

/**************************************************************************
 *  IAutoComplete_Constructor
 */
HRESULT WINAPI IAutoComplete_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    IAutoCompleteImpl *lpac;
    HRESULT hr;

    if (pUnkOuter && !IsEqualIID (riid, &IID_IUnknown))
        return CLASS_E_NOAGGREGATION;

    lpac = calloc(1, sizeof(*lpac));
    if (!lpac)
        return E_OUTOFMEMORY;

    lpac->ref = 1;
    lpac->IAutoComplete2_iface.lpVtbl = &acvt;
    lpac->IAutoCompleteDropDown_iface.lpVtbl = &acdropdownvt;
    lpac->enabled = TRUE;
    lpac->options = ACO_AUTOAPPEND;

    hr = IAutoComplete2_QueryInterface(&lpac->IAutoComplete2_iface, riid, ppv);
    IAutoComplete2_Release(&lpac->IAutoComplete2_iface);

    TRACE("-- (%p)->\n",lpac);

    return hr;
}
