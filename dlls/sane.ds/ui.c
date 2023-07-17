/*
* TWAIN32 Options UI
*
* Copyright 2006 CodeWeavers, Aric Stewart
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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "sane_i.h"
#include "winuser.h"
#include "winnls.h"
#include "wingdi.h"
#include "prsht.h"
#include "wine/debug.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

#define ID_BASE 0x100
#define ID_EDIT_BASE 0x1000
#define ID_STATIC_BASE 0x2000

static INT_PTR CALLBACK DialogProc (HWND , UINT , WPARAM , LPARAM );
static INT CALLBACK PropSheetProc(HWND, UINT,LPARAM);

static int create_leading_static(HDC hdc, const WCHAR *text,
        LPDLGITEMTEMPLATEW* template_out, int y, int id)
{
    LPDLGITEMTEMPLATEW tpl =  NULL;
    INT len;
    SIZE size;
    WORD *ptr;
    LONG base;

    *template_out = NULL;

    if (!text)
        return 0;

    base = GetDialogBaseUnits();

    len = lstrlenW(text) * sizeof(WCHAR);
    len += sizeof(DLGITEMTEMPLATE);
    len += 4*sizeof(WORD);

    tpl = malloc(len);
    tpl->style=WS_VISIBLE;
    tpl->dwExtendedStyle = 0;
    tpl->x = 4;
    tpl->y = y;
    tpl->id = ID_BASE;

    GetTextExtentPoint32W(hdc,text,lstrlenW(text),&size);

    tpl->cx =  MulDiv(size.cx,4,LOWORD(base));
    tpl->cy =  MulDiv(size.cy,8,HIWORD(base)) * 2;
    ptr = (WORD *)(tpl + 1);
    *ptr++ = 0xffff;
    *ptr++ = 0x0082;
    lstrcpyW( ptr, text );
    ptr += lstrlenW(ptr) + 1;
    *ptr = 0;

    *template_out = tpl;
    return len;
}

static int create_trailing_edit(HDC hdc, LPDLGITEMTEMPLATEW* template_out, int id,
        int y, const WCHAR *text,BOOL is_int)
{
    LPDLGITEMTEMPLATEW tpl =  NULL;
    INT len;
    WORD *ptr;
    SIZE size;
    LONG base;
    static const char int_base[] = "0000 xxx";
    static const char float_base[] = "0000.0000 xxx";

    base = GetDialogBaseUnits();

    len = lstrlenW(text) * sizeof(WCHAR);
    len += sizeof(DLGITEMTEMPLATE);
    len += 4*sizeof(WORD);

    tpl = malloc(len);
    tpl->style=WS_VISIBLE|ES_READONLY|WS_BORDER;
    tpl->dwExtendedStyle = 0;
    tpl->x = 1;
    tpl->y = y;
    tpl->id = id;

    if (is_int)
        GetTextExtentPoint32A(hdc,int_base,lstrlenA(int_base),&size);
    else
        GetTextExtentPoint32A(hdc,float_base,lstrlenA(float_base),&size);

    tpl->cx =  MulDiv(size.cx*2,4,LOWORD(base));
    tpl->cy =  MulDiv(size.cy,8,HIWORD(base)) * 2;

    ptr = (WORD *)(tpl + 1);
    *ptr++ = 0xffff;
    *ptr++ = 0x0081;
    lstrcpyW( ptr, text );
    ptr += lstrlenW(ptr) + 1;
    *ptr = 0;

    *template_out = tpl;
    return len;
}


static int create_item(HDC hdc, const struct option_descriptor *opt,
        INT id, LPDLGITEMTEMPLATEW *template_out, int y, int *cx, int* count)
{
    LPDLGITEMTEMPLATEW tpl = NULL,rc = NULL;
    WORD class = 0xffff;
    DWORD styles = WS_VISIBLE;
    WORD *ptr = NULL;
    LPDLGITEMTEMPLATEW lead_static = NULL;
    LPDLGITEMTEMPLATEW trail_edit = NULL;
    DWORD leading_len = 0;
    DWORD trail_len = 0;
    DWORD local_len = 0;
    const WCHAR *title = NULL;
    WCHAR buffer[255];
    int padding = 0;
    int padding2 = 0;
    int base_x = 0;
    LONG base;
    int ctl_cx = 0;
    SIZE size;

    GetTextExtentPoint32A(hdc,"X",1,&size);
    base = GetDialogBaseUnits();
    base_x = MulDiv(size.cx,4,LOWORD(base));

    switch (opt->type)
    {
    case TYPE_BOOL:
        class = 0x0080; /* Button */
        styles |= BS_AUTOCHECKBOX;
        title = opt->title;
        break;
    case TYPE_INT:
    {
        int i;
        sane_option_get_value( id - ID_BASE, &i );
        swprintf(buffer, ARRAY_SIZE(buffer), L"%i", i);

        switch (opt->constraint_type)
        {
        case CONSTRAINT_NONE:
            class = 0x0081; /* Edit*/
            styles |= ES_NUMBER;
            title = buffer;
            break;
        case CONSTRAINT_RANGE:
            class = 0x0084; /* scroll */
            ctl_cx = 10 * base_x;
            trail_len += create_trailing_edit(hdc, &trail_edit, id +
                    ID_EDIT_BASE, y,buffer,TRUE);
            break;
        default:
            class= 0x0085; /* Combo */
            ctl_cx = 10 * base_x;
            styles |= CBS_DROPDOWNLIST;
            break;
        }
        leading_len += create_leading_static(hdc, opt->title, &lead_static, y, id+ID_STATIC_BASE);
        break;
    }
    case TYPE_FIXED:
    {
        int *i = calloc( opt->size, sizeof(int) );

        sane_option_get_value( id - ID_BASE, i );

        swprintf(buffer, ARRAY_SIZE(buffer), L"%f", *i / 65536.0);
        free( i );

        switch (opt->constraint_type)
        {
        case CONSTRAINT_NONE:
            class = 0x0081; /* Edit */
            title = buffer;
            break;
        case CONSTRAINT_RANGE:
            class= 0x0084; /* scroll */
            ctl_cx = 10 * base_x;
            trail_len += create_trailing_edit(hdc, &trail_edit, id + ID_EDIT_BASE, y,buffer,FALSE);
            break;
        default:
            class= 0x0085; /* Combo */
            ctl_cx = 10 * base_x;
            styles |= CBS_DROPDOWNLIST;
            break;
        }
        leading_len += create_leading_static(hdc, opt->title, &lead_static, y, id+ID_STATIC_BASE);
        break;
    }
    case TYPE_STRING:
    {
        char str[256];
        switch (opt->constraint_type)
        {
        case CONSTRAINT_NONE:
            class = 0x0081; /* Edit*/
            break;
        default:
            class= 0x0085; /* Combo */
            ctl_cx = opt->size * base_x;
            styles |= CBS_DROPDOWNLIST;
            break;
        }
        leading_len += create_leading_static(hdc, opt->title, &lead_static, y, id+ID_STATIC_BASE);
        sane_option_get_value( id - ID_BASE, str );
        MultiByteToWideChar( CP_UNIXCP, 0, str, -1, buffer, ARRAY_SIZE(buffer) );
        title = buffer;
        break;
    }
    case TYPE_BUTTON:
        class = 0x0080; /* Button */
        title = opt->title;
        break;
    case TYPE_GROUP:
        class = 0x0080; /* Button */
        styles |= BS_GROUPBOX;
        title = opt->title;
        break;
    }

    local_len += sizeof(DLGITEMTEMPLATE);
    if (title) local_len += lstrlenW(title) * sizeof(WCHAR);
    local_len += 4*sizeof(WORD);

    padding = leading_len % sizeof(DWORD);
    rc = realloc(lead_static, leading_len + local_len + padding);
    tpl = (LPDLGITEMTEMPLATEW)((LPBYTE)rc + leading_len + padding);
    tpl->style=styles;
    tpl->dwExtendedStyle = 0;
    if (lead_static)
        tpl->x = rc->x + rc->cx + 1;
    else if (opt->type == TYPE_GROUP)
        tpl->x = 2;
    else
        tpl->x = 4;
    tpl->y = y;
    tpl->id = id;

    if (title)
    {
        GetTextExtentPoint32W(hdc,title,lstrlenW(title),&size);
        tpl->cx = size.cx;
        tpl->cy = size.cy;
    }
    else
    {
        if (lead_static)
            tpl->cy = rc->cy;
        else
            tpl->cy = 15;

        if (!ctl_cx)
            ctl_cx = 15;

        tpl->cx = ctl_cx;
    }
    ptr = (WORD *)(tpl + 1);
    *ptr++ = 0xffff;
    *ptr++ = class;
    if (title)
    {
        lstrcpyW( ptr, title );
        ptr += lstrlenW(ptr);
    }
    *ptr++ = 0;
    *ptr = 0;

    if (trail_edit)
    {
        trail_edit->x = tpl->cx + tpl->x + 2;
        *cx = trail_edit->x + trail_edit->cx;

        padding2 = (leading_len + local_len + padding)% sizeof(DWORD);

        rc = realloc( rc, leading_len + local_len + padding +padding2 + trail_len);

        memcpy(((LPBYTE)rc) + leading_len + local_len + padding + padding2,
                trail_edit,trail_len);
    }   
    else
        *cx = tpl->cx + tpl->x;
    
    *template_out = rc;
    if (leading_len)
        *count = 2;
    else
        *count = 1;

    if (trail_edit)
        *count+=1;

    free(trail_edit);
    return leading_len + local_len + padding + padding2 + trail_len;
}


static LPDLGTEMPLATEW create_options_page(HDC hdc, int *from_index,
                                          int optcount, BOOL split_tabs)
{
    int i;
    INT y = 2;
    LPDLGTEMPLATEW tpl = NULL;
    LPBYTE all_controls = NULL;
    DWORD control_len = 0;
    int max_cx = 0;
    int group_max_cx = 0;
    LPBYTE ptr;
    int group_offset = -1;
    INT control_count = 0;

    for (i = *from_index; i < optcount; i++)
    {
        LPDLGITEMTEMPLATEW item_tpl = NULL;
        struct option_descriptor opt;
        int len;
        int padding = 0;
        int x;
        int count;
        int hold_for_group = 0;

        opt.optno = i;
        if (SANE_CALL( option_get_descriptor, &opt )) continue;
        if (opt.type == TYPE_GROUP && split_tabs)
        {
            if (control_len > 0)
            {
                *from_index = i - 1;
                goto exit;
            }
            else
            {
                *from_index = i;
                return NULL;
            }
        }
        if (!opt.is_active)
            continue;

        len = create_item(hdc, &opt, ID_BASE + i, &item_tpl, y, &x, &count);

        control_count += count;

        if (!len)
        {
            continue;
        }

        hold_for_group = y;
        y+= item_tpl->cy + 1;
        max_cx = max(max_cx, x + 2);
        group_max_cx = max(group_max_cx, x );

        padding = len % sizeof(DWORD);

        if (all_controls)
        {
            all_controls = realloc(all_controls, control_len + len + padding);
            memcpy(all_controls+control_len,item_tpl,len);
            memset(all_controls+control_len+len,0xca,padding);
            free(item_tpl);
        }
        else
        {
            if (!padding)
            {
                all_controls = (LPBYTE)item_tpl;
            }
            else
            {
                all_controls = malloc(len + padding);
                memcpy(all_controls,item_tpl,len);
                memset(all_controls+len,0xcb,padding);
                free(item_tpl);
            }
        }

        if (opt.type == TYPE_GROUP)
        {
            if (group_offset == -1)
            {
                group_offset  = control_len;
                group_max_cx = 0;
            }
            else
            {
                LPDLGITEMTEMPLATEW group =
                    (LPDLGITEMTEMPLATEW)(all_controls+group_offset);

                group->cy = hold_for_group - group->y;
                group->cx = group_max_cx;

                group = (LPDLGITEMTEMPLATEW)(all_controls+control_len);
                group->y += 2;
                y+=2;
                group_max_cx = 0;
                group_offset  = control_len;
            }
        }

        control_len += len + padding;
    }

    if ( group_offset && !split_tabs )
    {
        LPDLGITEMTEMPLATEW group =
            (LPDLGITEMTEMPLATEW)(all_controls+group_offset);
        group->cy = y - group->y;
        group->cx = group_max_cx;
        y+=2;
    }

    *from_index = i-1;
exit:

    tpl = malloc(sizeof(DLGTEMPLATE) + 3*sizeof(WORD) + control_len);

    tpl->style = WS_VISIBLE | WS_OVERLAPPEDWINDOW;
    tpl->dwExtendedStyle = 0;
    tpl->cdit = control_count;
    tpl->x = 0;
    tpl->y = 0;
    tpl->cx = max_cx + 10;
    tpl->cy = y + 10;
    ptr = (LPBYTE)tpl + sizeof(DLGTEMPLATE);
    *(LPWORD)ptr = 0x0000;
    ptr+=sizeof(WORD);
    *(LPWORD)ptr = 0x0000;
    ptr+=sizeof(WORD);
    *(LPWORD)ptr = 0x0000;
    ptr+=sizeof(WORD);
    memcpy(ptr,all_controls,control_len);

    free(all_controls);

    return tpl;
}

BOOL DoScannerUI(void)
{
    HDC hdc;
    PROPSHEETPAGEW psp[10];
    int page_count= 0;
    PROPSHEETHEADERW psh;
    int index = 1;
    TW_UINT16 rc;
    int optcount;
    UINT psrc;
    LPWSTR szCaption;
    DWORD len;

    hdc = GetDC(0);

    memset(psp,0,sizeof(psp));
    rc = sane_option_get_value( 0, &optcount );
    if (rc != TWCC_SUCCESS)
    {
        ERR("Unable to read number of options\n");
        return FALSE;
    }

    while (index < optcount)
    {
        struct option_descriptor opt;

        psp[page_count].pResource = create_options_page(hdc, &index,
                optcount, TRUE);
        opt.optno = index;
        SANE_CALL( option_get_descriptor, &opt );

        if (opt.type == TYPE_GROUP)
        {
            psp[page_count].pszTitle = wcsdup( opt.title );
        }

        if (psp[page_count].pResource)
        {
            psp[page_count].dwSize = sizeof(PROPSHEETPAGEW);
            psp[page_count].dwFlags =  PSP_DLGINDIRECT | PSP_USETITLE;
            psp[page_count].hInstance = SANE_instance;
            psp[page_count].pfnDlgProc = DialogProc;
            psp[page_count].lParam = (LPARAM)&activeDS;
            page_count ++;
        }
       
        index ++;
    }
 
    len = lstrlenA(activeDS.identity.Manufacturer)
         + lstrlenA(activeDS.identity.ProductName) + 2;
    szCaption = malloc(len *sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP,0,activeDS.identity.Manufacturer,-1,
            szCaption,len);
    szCaption[lstrlenA(activeDS.identity.Manufacturer)] = ' ';
    MultiByteToWideChar(CP_ACP,0,activeDS.identity.ProductName,-1,
            &szCaption[lstrlenA(activeDS.identity.Manufacturer)+1],len);
    psh.dwSize = sizeof(PROPSHEETHEADERW);
    psh.dwFlags = PSH_PROPSHEETPAGE|PSH_PROPTITLE|PSH_USECALLBACK;
    psh.hwndParent = activeDS.hwndOwner;
    psh.hInstance = SANE_instance;
    psh.pszIcon = 0;
    psh.pszCaption = szCaption;
    psh.nPages = page_count;
    psh.nStartPage = 0;
    psh.ppsp = (LPCPROPSHEETPAGEW)psp;
    psh.pfnCallback = PropSheetProc;

    psrc = PropertySheetW(&psh);

    for(index = 0; index < page_count; index ++)
    {
        free((LPBYTE)psp[index].pResource);
        free((LPBYTE)psp[index].pszTitle);
    }
    free(szCaption);
    
    if (psrc == IDOK)
        return TRUE;
    else
        return FALSE;
}

static void UpdateRelevantEdit(HWND hwnd, const struct option_descriptor *opt, int position)
{
    WCHAR buffer[244];
    HWND edit_w;
    int len;

    switch (opt->type)
    {
    case TYPE_INT:
    {
        INT si;

        if (opt->constraint.range.quant)
            si = position * opt->constraint.range.quant;
        else
            si = position;

        len = swprintf( buffer, ARRAY_SIZE(buffer), L"%i", si );
        break;
    }
    case TYPE_FIXED:
    {
        double dd;

        if (opt->constraint.range.quant)
            dd = position * (opt->constraint.range.quant / 65536.0);
        else
            dd = position * 0.01;

        len = swprintf( buffer, ARRAY_SIZE(buffer), L"%f", dd );
        break;
    }
    default:
        return;
    }

    buffer[len++] = ' ';
    LoadStringW( SANE_instance, opt->unit, buffer + len, ARRAY_SIZE( buffer ) - len );

    edit_w = GetDlgItem(hwnd,opt->optno + ID_BASE + ID_EDIT_BASE);
    if (edit_w) SetWindowTextW(edit_w,buffer);

}

static BOOL UpdateSaneScrollOption(const struct option_descriptor *opt, DWORD position)
{
    BOOL result = FALSE;
    int si;

    switch (opt->type)
    {
    case TYPE_INT:
    {
        if (opt->constraint.range.quant)
            si = position * opt->constraint.range.quant;
        else
            si = position;

        sane_option_set_value( opt->optno, &si, &result );
        break;
    }
    case TYPE_FIXED:
        if (opt->constraint.range.quant)
            si = position * opt->constraint.range.quant;
        else
            si = MulDiv( position, 65536, 100 );

        sane_option_set_value( opt->optno, &si, &result );
        break;
    default:
        break;
    }

    return result;
}

static INT_PTR InitializeDialog(HWND hwnd)
{
    TW_UINT16 rc;
    int optcount;
    HWND control;
    int i;

    rc = sane_option_get_value( 0, &optcount );
    if (rc != TWCC_SUCCESS)
    {
        ERR("Unable to read number of options\n");
        return FALSE;
    }

    for ( i = 1; i < optcount; i++)
    {
        struct option_descriptor opt;

        control = GetDlgItem(hwnd,i+ID_BASE);

        if (!control)
            continue;

        opt.optno = i;
        SANE_CALL( option_get_descriptor, &opt );

        TRACE("%i %s %i %i\n",i,debugstr_w(opt.title),opt.type,opt.constraint_type);
        EnableWindow(control,opt.is_active);

        SendMessageA(control,CB_RESETCONTENT,0,0);
        /* initialize values */
        if (opt.type == TYPE_STRING && opt.constraint_type != CONSTRAINT_NONE)
        {
            CHAR buffer[255];
            WCHAR *p;

            for (p = opt.constraint.strings; *p; p += lstrlenW(p) + 1)
                SendMessageW( control,CB_ADDSTRING,0, (LPARAM)p );
            sane_option_get_value( i, buffer );
            SendMessageA(control,CB_SELECTSTRING,0,(LPARAM)buffer);
        }
        else if (opt.type == TYPE_BOOL)
        {
            BOOL b;
            sane_option_get_value( i, &b );
            if (b)
                SendMessageA(control,BM_SETCHECK,BST_CHECKED,0);

        }
        else if (opt.type == TYPE_INT && opt.constraint_type == CONSTRAINT_WORD_LIST)
        {
            int j, count = opt.constraint.word_list[0];
            CHAR buffer[16];
            int val;
            for (j=1; j<=count; j++)
            {
                sprintf(buffer, "%d", opt.constraint.word_list[j]);
                SendMessageA(control, CB_ADDSTRING, 0, (LPARAM)buffer);
            }
            sane_option_get_value( i, &val );
            sprintf(buffer, "%d", val);
            SendMessageA(control,CB_SELECTSTRING,0,(LPARAM)buffer);
        }
        else if (opt.constraint_type == CONSTRAINT_RANGE)
        {
            if (opt.type == TYPE_INT)
            {
                int si;
                int min,max;

                min = opt.constraint.range.min /
                    (opt.constraint.range.quant ? opt.constraint.range.quant : 1);

                max = opt.constraint.range.max /
                    (opt.constraint.range.quant ? opt.constraint.range.quant : 1);

                SendMessageA(control,SBM_SETRANGE,min,max);

                sane_option_get_value( i, &si );
                if (opt.constraint.range.quant)
                    si = si / opt.constraint.range.quant;

                SendMessageW(control,SBM_SETPOS, si, TRUE);
                UpdateRelevantEdit(hwnd, &opt, si);
            }
            else if (opt.type == TYPE_FIXED)
            {
                int pos, min, max, *sf;

                if (opt.constraint.range.quant)
                {
                    min = opt.constraint.range.min / opt.constraint.range.quant;
                    max = opt.constraint.range.max / opt.constraint.range.quant;
                }
                else
                {
                    min = MulDiv( opt.constraint.range.min, 100, 65536 );
                    max = MulDiv( opt.constraint.range.max, 100, 65536 );
                }

                SendMessageA(control,SBM_SETRANGE,min,max);


                sf = calloc( opt.size, sizeof(int) );
                sane_option_get_value( i, sf );

                /* Note that conversion of float -> SANE_Fixed is lossy;
                 *   and when you truncate it into an integer, you can get
                 *   unfortunate results.  This calculation attempts
                 *   to mitigate that harm */
                if (opt.constraint.range.quant)
                    pos = *sf / opt.constraint.range.quant;
                else
                    pos = MulDiv( *sf, 100, 65536 );

                free(sf);
                SendMessageW(control, SBM_SETPOS, pos, TRUE);
                UpdateRelevantEdit(hwnd, &opt, pos);
            }
        }
    }

    return TRUE;
}

static INT_PTR ProcessScroll(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    struct option_descriptor opt;
    WORD scroll;
    DWORD position;

    opt.optno = GetDlgCtrlID((HWND)lParam)- ID_BASE;
    if (opt.optno < 0)
        return FALSE;

    if (SANE_CALL( option_get_descriptor, &opt )) return FALSE;

    scroll = LOWORD(wParam);

    switch (scroll)
    {
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
        {
            SCROLLINFO si;
            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_TRACKPOS;
            GetScrollInfo((HWND)lParam,SB_CTL, &si);
            position = si.nTrackPos;
        }
            break;
        case SB_LEFT:
        case SB_LINELEFT:
        case SB_PAGELEFT:
            position = SendMessageW((HWND)lParam,SBM_GETPOS,0,0);
            position--;
            break;
        case SB_RIGHT:
        case SB_LINERIGHT:
        case SB_PAGERIGHT:
            position = SendMessageW((HWND)lParam,SBM_GETPOS,0,0);
            position++;
            break;
        default:
            position = SendMessageW((HWND)lParam,SBM_GETPOS,0,0);
    }

    SendMessageW((HWND)lParam,SBM_SETPOS, position, TRUE);
    position = SendMessageW((HWND)lParam,SBM_GETPOS,0,0);

    UpdateRelevantEdit(hwnd, &opt, position);
    if (UpdateSaneScrollOption(&opt, position))
        InitializeDialog(hwnd);

    return TRUE;
}


static void ButtonClicked(HWND hwnd, INT id, HWND control)
{
    struct option_descriptor opt;
    BOOL changed = FALSE;

    opt.optno = id - ID_BASE;
    if (opt.optno < 0)
        return;

    if (SANE_CALL( option_get_descriptor, &opt )) return;

    if (opt.type == TYPE_BOOL)
    {
        BOOL r = SendMessageW(control,BM_GETCHECK,0,0)==BST_CHECKED;
        sane_option_set_value( opt.optno, &r, &changed );
        if (changed) InitializeDialog(hwnd);
    }
}

static void ComboChanged(HWND hwnd, INT id, HWND control)
{
    int selection;
    int len;
    struct option_descriptor opt;
    char *value;
    BOOL changed = FALSE;

    opt.optno = id - ID_BASE;
    if (opt.optno < 0)
        return;

    if (SANE_CALL( option_get_descriptor, &opt )) return;

    selection = SendMessageW(control,CB_GETCURSEL,0,0);
    len = SendMessageW(control,CB_GETLBTEXTLEN,selection,0);

    len++;
    value = malloc(len);
    SendMessageA(control,CB_GETLBTEXT,selection,(LPARAM)value);

    if (opt.type == TYPE_STRING)
    {
        sane_option_set_value( opt.optno, value, &changed );
    }
    else if (opt.type == TYPE_INT)
    {
        int val = atoi( value );
        sane_option_set_value( opt.optno, &val, &changed );
    }
    if (changed) InitializeDialog(hwnd);
    free( value );
}


static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
            return InitializeDialog(hwndDlg);
        case WM_HSCROLL:
            return ProcessScroll(hwndDlg, wParam, lParam);
        case WM_NOTIFY:
            {
                LPPSHNOTIFY psn = (LPPSHNOTIFY)lParam;
                switch (((NMHDR*)lParam)->code)
                {
                    case PSN_APPLY:
                        if (psn->lParam)
                        {
                            activeDS.currentState = 6;
                            SANE_Notify(MSG_XFERREADY);
                        }
                        break;
                    case PSN_QUERYCANCEL:
                        SANE_Notify(MSG_CLOSEDSREQ);
                        break;
                    case PSN_SETACTIVE:
                        InitializeDialog(hwndDlg);
                        break;
                }
                break;
            }
        case WM_COMMAND:
            switch (HIWORD(wParam))
            {
                case BN_CLICKED:
                    ButtonClicked(hwndDlg,LOWORD(wParam), (HWND)lParam);
                    break;
                case CBN_SELCHANGE:
                    ComboChanged(hwndDlg,LOWORD(wParam), (HWND)lParam);
            }
    }

    return FALSE;
}

static int CALLBACK PropSheetProc(HWND hwnd, UINT msg, LPARAM lParam)
{
    if (msg == PSCB_INITIALIZED)
    {
        /* rename OK button to Scan */
        HWND scan = GetDlgItem(hwnd,IDOK);
        SetWindowTextA(scan,"Scan");
    }
    return TRUE;
}


static INT_PTR CALLBACK ScanningProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}

HWND ScanningDialogBox(HWND dialog, LONG progress)
{
    if (!dialog)
        dialog = CreateDialogW(SANE_instance,
                (LPWSTR)MAKEINTRESOURCE(IDD_DIALOG1), NULL, ScanningProc);

    if (progress == -1)
    {
        EndDialog(dialog,0);
        return NULL;
    }

    RedrawWindow(dialog,NULL,NULL,
            RDW_INTERNALPAINT|RDW_UPDATENOW|RDW_ALLCHILDREN);

    return dialog;
}
