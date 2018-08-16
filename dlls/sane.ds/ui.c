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

#include "config.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#define NONAMELESSUNION

#include "sane_i.h"
#include "winuser.h"
#include "winnls.h"
#include "wingdi.h"
#include "prsht.h"
#include "wine/debug.h"
#include "resource.h"
#include "wine/unicode.h"

#ifdef SONAME_LIBSANE

WINE_DEFAULT_DEBUG_CHANNEL(twain);

#define ID_BASE 0x100
#define ID_EDIT_BASE 0x1000
#define ID_STATIC_BASE 0x2000

static INT_PTR CALLBACK DialogProc (HWND , UINT , WPARAM , LPARAM );
static INT CALLBACK PropSheetProc(HWND, UINT,LPARAM);

static int create_leading_static(HDC hdc, LPCSTR text,
        LPDLGITEMTEMPLATEW* template_out, int y, int id)
{
    LPDLGITEMTEMPLATEW tpl =  NULL;
    INT len;
    SIZE size;
    LPBYTE ptr;
    LONG base;

    *template_out = NULL;

    if (!text)
        return 0;

    base = GetDialogBaseUnits();

    len = MultiByteToWideChar(CP_ACP,0,text,-1,NULL,0);
    len *= sizeof(WCHAR);
    len += sizeof(DLGITEMTEMPLATE);
    len += 3*sizeof(WORD);

    tpl = HeapAlloc(GetProcessHeap(),0,len);
    tpl->style=WS_VISIBLE;
    tpl->dwExtendedStyle = 0;
    tpl->x = 4;
    tpl->y = y;
    tpl->id = ID_BASE;

    GetTextExtentPoint32A(hdc,text,lstrlenA(text),&size);

    tpl->cx =  MulDiv(size.cx,4,LOWORD(base));
    tpl->cy =  MulDiv(size.cy,8,HIWORD(base)) * 2;
    ptr = (LPBYTE)tpl + sizeof(DLGITEMTEMPLATE);
    *(LPWORD)ptr = 0xffff;
    ptr += sizeof(WORD);
    *(LPWORD)ptr = 0x0082;
    ptr += sizeof(WORD);
    ptr += MultiByteToWideChar(CP_ACP,0,text,-1,(LPWSTR)ptr,len) * sizeof(WCHAR);
    *(LPWORD)ptr = 0x0000;

    *template_out = tpl;
    return len;
}

static int create_trailing_edit(HDC hdc, LPDLGITEMTEMPLATEW* template_out, int id,
        int y, LPCSTR text,BOOL is_int)
{
    LPDLGITEMTEMPLATEW tpl =  NULL;
    INT len;
    LPBYTE ptr;
    SIZE size;
    LONG base;
    static const char int_base[] = "0000 xxx";
    static const char float_base[] = "0000.0000 xxx";

    base = GetDialogBaseUnits();

    len = MultiByteToWideChar(CP_ACP,0,text,-1,NULL,0);
    len *= sizeof(WCHAR);
    len += sizeof(DLGITEMTEMPLATE);
    len += 3*sizeof(WORD);

    tpl = HeapAlloc(GetProcessHeap(),0,len);
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

    ptr = (LPBYTE)tpl + sizeof(DLGITEMTEMPLATE);
    *(LPWORD)ptr = 0xffff;
    ptr += sizeof(WORD);
    *(LPWORD)ptr = 0x0081;
    ptr += sizeof(WORD);
    ptr += MultiByteToWideChar(CP_ACP,0,text,-1,(LPWSTR)ptr,len) * sizeof(WCHAR);
    *(LPWORD)ptr = 0x0000;

    *template_out = tpl;
    return len;
}


static int create_item(HDC hdc, const SANE_Option_Descriptor *opt,
        INT id, LPDLGITEMTEMPLATEW *template_out, int y, int *cx, int* count)
{
    LPDLGITEMTEMPLATEW tpl = NULL,rc = NULL;
    WORD class = 0xffff;
    DWORD styles = WS_VISIBLE;
    LPBYTE ptr = NULL;
    LPDLGITEMTEMPLATEW lead_static = NULL;
    LPDLGITEMTEMPLATEW trail_edit = NULL;
    DWORD leading_len = 0;
    DWORD trail_len = 0;
    DWORD local_len = 0;
    LPCSTR title = NULL;
    CHAR buffer[255];
    int padding = 0;
    int padding2 = 0;
    int base_x = 0;
    LONG base;
    int ctl_cx = 0;
    SIZE size;

    GetTextExtentPoint32A(hdc,"X",1,&size);
    base = GetDialogBaseUnits();
    base_x = MulDiv(size.cx,4,LOWORD(base));

    if (opt->type == SANE_TYPE_BOOL)
    {
        class = 0x0080; /* Button */
        styles |= BS_AUTOCHECKBOX;
        local_len += MultiByteToWideChar(CP_ACP,0,opt->title,-1,NULL,0);
        local_len *= sizeof(WCHAR);
        title = opt->title;
    }
    else if (opt->type == SANE_TYPE_INT)
    {
        SANE_Int i;

        psane_control_option(activeDS.deviceHandle, id-ID_BASE,
                SANE_ACTION_GET_VALUE, &i,NULL);

        sprintf(buffer,"%i",i);

        if (opt->constraint_type == SANE_CONSTRAINT_NONE)
        {
            class = 0x0081; /* Edit*/
            styles |= ES_NUMBER;
            title = buffer;
            local_len += MultiByteToWideChar(CP_ACP,0,title,-1,NULL,0);
            local_len *= sizeof(WCHAR);
        }
        else if (opt->constraint_type == SANE_CONSTRAINT_RANGE)
        {
            class = 0x0084; /* scroll */
            ctl_cx = 10 * base_x;
            trail_len += create_trailing_edit(hdc, &trail_edit, id +
                    ID_EDIT_BASE, y,buffer,TRUE);
        }
        else
        {
            class= 0x0085; /* Combo */
            ctl_cx = 10 * base_x;
            styles |= CBS_DROPDOWNLIST;
        }
        leading_len += create_leading_static(hdc, opt->title, &lead_static, y, 
                id+ID_STATIC_BASE);
    }
    else if (opt->type == SANE_TYPE_FIXED)
    {
        SANE_Fixed *i;
        double dd;

        i = HeapAlloc(GetProcessHeap(),0,opt->size*sizeof(SANE_Word));

        psane_control_option(activeDS.deviceHandle, id-ID_BASE,
                SANE_ACTION_GET_VALUE, i, NULL);

        dd = SANE_UNFIX(*i);
        sprintf(buffer,"%f",dd);
        HeapFree(GetProcessHeap(),0,i);

        if (opt->constraint_type == SANE_CONSTRAINT_NONE)
        {
            class = 0x0081; /* Edit */
            title = buffer;
            local_len += MultiByteToWideChar(CP_ACP,0,title,-1,NULL,0);
            local_len *= sizeof(WCHAR);
        }
        else if (opt->constraint_type == SANE_CONSTRAINT_RANGE)
        {
            class= 0x0084; /* scroll */
            ctl_cx = 10 * base_x;
            trail_len += create_trailing_edit(hdc, &trail_edit, id +
                    ID_EDIT_BASE, y,buffer,FALSE);
        }
        else
        {
            class= 0x0085; /* Combo */
            ctl_cx = 10 * base_x;
            styles |= CBS_DROPDOWNLIST;
        }
        leading_len += create_leading_static(hdc, opt->title, &lead_static, y,
                id+ID_STATIC_BASE);
    }
    else if (opt->type == SANE_TYPE_STRING)
    {
        if (opt->constraint_type == SANE_CONSTRAINT_NONE)
        {
            class = 0x0081; /* Edit*/
        }
        else
        {
            class= 0x0085; /* Combo */
            ctl_cx = opt->size * base_x;
            styles |= CBS_DROPDOWNLIST;
        }
        leading_len += create_leading_static(hdc, opt->title, &lead_static, y,
                id+ID_STATIC_BASE);
        psane_control_option(activeDS.deviceHandle, id-ID_BASE,
                SANE_ACTION_GET_VALUE, buffer,NULL);

        title = buffer;
        local_len += MultiByteToWideChar(CP_ACP,0,title,-1,NULL,0);
        local_len *= sizeof(WCHAR);
    }
    else if (opt->type == SANE_TYPE_BUTTON)
    {
        class = 0x0080; /* Button */
        local_len += MultiByteToWideChar(CP_ACP,0,opt->title,-1,NULL,0);
        local_len *= sizeof(WCHAR);
        title = opt->title;
    }
    else if (opt->type == SANE_TYPE_GROUP)
    {
        class = 0x0080; /* Button */
        styles |= BS_GROUPBOX;
        local_len += MultiByteToWideChar(CP_ACP,0,opt->title,-1,NULL,0);
        local_len *= sizeof(WCHAR);
        title = opt->title;
    }

    local_len += sizeof(DLGITEMTEMPLATE);
    if (title)
        local_len += 3*sizeof(WORD);
    else
        local_len += 4*sizeof(WORD);

    if (lead_static)
    {
        padding = leading_len % sizeof(DWORD);
        rc = HeapReAlloc(GetProcessHeap(),0,lead_static,leading_len+local_len + padding);
        tpl = (LPDLGITEMTEMPLATEW)((LPBYTE)rc + leading_len + padding);
    }   
    else
        rc = tpl = HeapAlloc(GetProcessHeap(),0,local_len);

    tpl->style=styles;
    tpl->dwExtendedStyle = 0;
    if (lead_static)
        tpl->x = lead_static->x + lead_static->cx + 1;
    else if (opt->type == SANE_TYPE_GROUP)
        tpl->x = 2;
    else
        tpl->x = 4;
    tpl->y = y;
    tpl->id = id;

    if (title)
    {
        GetTextExtentPoint32A(hdc,title,lstrlenA(title),&size);
        tpl->cx = size.cx;
        tpl->cy = size.cy;
    }
    else
    {
        if (lead_static)
            tpl->cy = lead_static->cy;
        else
            tpl->cy = 15;

        if (!ctl_cx)
            ctl_cx = 15;

        tpl->cx = ctl_cx;
    }
    ptr = (LPBYTE)tpl + sizeof(DLGITEMTEMPLATE);
    *(LPWORD)ptr = 0xffff;
    ptr += sizeof(WORD);
    *(LPWORD)ptr = class;
    ptr += sizeof(WORD);
    if (title)
    {
        ptr += MultiByteToWideChar(CP_ACP,0,title,-1,(LPWSTR)ptr,local_len) * sizeof(WCHAR);
    }
    else
    {
        *(LPWORD)ptr = 0x0000;
        ptr += sizeof(WORD);
    }

    *((LPWORD)ptr) = 0x0000;
    ptr += sizeof(WORD);

    if (trail_edit)
    {
        trail_edit->x = tpl->cx + tpl->x + 2;
        *cx = trail_edit->x + trail_edit->cx;

        padding2 = (leading_len + local_len + padding)% sizeof(DWORD);

        rc = HeapReAlloc(GetProcessHeap(),0,rc,leading_len+local_len + padding
                +padding2+trail_len);

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

    return leading_len + local_len + padding + padding2 + trail_len;
}


static LPDLGTEMPLATEW create_options_page(HDC hdc, int *from_index,
                                          SANE_Int optcount, BOOL split_tabs)
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
        const SANE_Option_Descriptor *opt;
        int len;
        int padding = 0;
        int x;
        int count;
        int hold_for_group = 0;

        opt = psane_get_option_descriptor(activeDS.deviceHandle, i);
        if (!opt)
            continue;
        if (opt->type == SANE_TYPE_GROUP && split_tabs)
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
        if (!SANE_OPTION_IS_ACTIVE (opt->cap))
            continue;

        len = create_item(hdc, opt, ID_BASE + i, &item_tpl, y, &x, &count);

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
            LPBYTE newone;
            newone = HeapReAlloc(GetProcessHeap(),0,all_controls,
                    control_len + len + padding);
            all_controls = newone;
            memcpy(all_controls+control_len,item_tpl,len);
            memset(all_controls+control_len+len,0xca,padding);
            HeapFree(GetProcessHeap(),0,item_tpl);
        }
        else
        {
            if (!padding)
            {
                all_controls = (LPBYTE)item_tpl;
            }
            else
            {
                all_controls = HeapAlloc(GetProcessHeap(),0,len + padding);
                memcpy(all_controls,item_tpl,len);
                memset(all_controls+len,0xcb,padding);
                HeapFree(GetProcessHeap(),0,item_tpl);
            }
        }

        if (opt->type == SANE_TYPE_GROUP)
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

    tpl = HeapAlloc(GetProcessHeap(),0,sizeof(DLGTEMPLATE) + 3*sizeof(WORD) + 
            control_len);

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

    HeapFree(GetProcessHeap(),0,all_controls);

    return tpl;
}

BOOL DoScannerUI(void)
{
    HDC hdc;
    PROPSHEETPAGEW psp[10];
    int page_count= 0;
    PROPSHEETHEADERW psh;
    int index = 1;
    SANE_Status rc;
    SANE_Int optcount;
    UINT psrc;
    LPWSTR szCaption;
    DWORD len;

    hdc = GetDC(0);

    memset(psp,0,sizeof(psp));
    rc = psane_control_option(activeDS.deviceHandle, 0, SANE_ACTION_GET_VALUE,
            &optcount, NULL);
    if (rc != SANE_STATUS_GOOD)
    {
        ERR("Unable to read number of options\n");
        return FALSE;
    }

    while (index < optcount)
    {
        const SANE_Option_Descriptor *opt;
        psp[page_count].u.pResource = create_options_page(hdc, &index,
                optcount, TRUE);
        opt = psane_get_option_descriptor(activeDS.deviceHandle, index);

        if (opt->type == SANE_TYPE_GROUP)
        {
            LPWSTR title = NULL;
            INT len;

            len = MultiByteToWideChar(CP_ACP,0,opt->title,-1,NULL,0);
            title = HeapAlloc(GetProcessHeap(),0,len * sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP,0,opt->title,-1,title,len);

            psp[page_count].pszTitle = title;
        }

        if (psp[page_count].u.pResource)
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
    szCaption = HeapAlloc(GetProcessHeap(),0,len *sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP,0,activeDS.identity.Manufacturer,-1,
            szCaption,len);
    szCaption[lstrlenA(activeDS.identity.Manufacturer)] = ' ';
    MultiByteToWideChar(CP_ACP,0,activeDS.identity.ProductName,-1,
            &szCaption[lstrlenA(activeDS.identity.Manufacturer)+1],len);
    psh.dwSize = sizeof(PROPSHEETHEADERW);
    psh.dwFlags = PSH_PROPSHEETPAGE|PSH_PROPTITLE|PSH_USECALLBACK;
    psh.hwndParent = activeDS.hwndOwner;
    psh.hInstance = SANE_instance;
    psh.u.pszIcon = 0;
    psh.pszCaption = szCaption;
    psh.nPages = page_count;
    psh.u2.nStartPage = 0;
    psh.u3.ppsp = (LPCPROPSHEETPAGEW)psp;
    psh.pfnCallback = PropSheetProc;

    psrc = PropertySheetW(&psh);

    for(index = 0; index < page_count; index ++)
    {
        HeapFree(GetProcessHeap(),0,(LPBYTE)psp[index].u.pResource);
        HeapFree(GetProcessHeap(),0,(LPBYTE)psp[index].pszTitle);
    }
    HeapFree(GetProcessHeap(),0,szCaption);
    
    if (psrc == IDOK)
        return TRUE;
    else
        return FALSE;
}

static void UpdateRelevantEdit(HWND hwnd, const SANE_Option_Descriptor *opt, 
        int index, int position)
{
    WCHAR buffer[244];
    HWND edit_w;
    int len;

    if (opt->type == SANE_TYPE_INT)
    {
        static const WCHAR formatW[] = {'%','i',0};
        INT si;

        if (opt->constraint.range->quant)
            si = position * opt->constraint.range->quant;
        else
            si = position;

        len = sprintfW( buffer, formatW, si );
    }
    else if  (opt->type == SANE_TYPE_FIXED)
    {
        static const WCHAR formatW[] = {'%','f',0};
        double s_quant, dd;

        s_quant = SANE_UNFIX(opt->constraint.range->quant);

        if (s_quant)
            dd = position * s_quant;
        else
            dd = position * 0.01;

        len = sprintfW( buffer, formatW, dd );
    }
    else return;

    buffer[len++] = ' ';
    LoadStringW( SANE_instance, opt->unit, buffer + len, ARRAY_SIZE( buffer ) - len );

    edit_w = GetDlgItem(hwnd,index+ID_BASE+ID_EDIT_BASE);
    if (edit_w) SetWindowTextW(edit_w,buffer);

}

static BOOL UpdateSaneScrollOption(
        const SANE_Option_Descriptor *opt, int index, DWORD position)
{
    SANE_Status rc = SANE_STATUS_GOOD;  
    SANE_Int result = 0;

    if (opt->type == SANE_TYPE_INT)
    {
        SANE_Int si;

        if (opt->constraint.range->quant)
            si = position * opt->constraint.range->quant;
        else
            si = position;

        rc = psane_control_option (activeDS.deviceHandle,index,
            SANE_ACTION_SET_VALUE, &si, &result);

    }
    else if  (opt->type == SANE_TYPE_FIXED)
    {
        double s_quant, dd;
        SANE_Fixed *sf;

        s_quant = SANE_UNFIX(opt->constraint.range->quant);

        if (s_quant)
            dd = position * s_quant;
        else
            dd = position * 0.01;

        sf = HeapAlloc(GetProcessHeap(),0,opt->size*sizeof(SANE_Word));

        *sf = SANE_FIX(dd);

        rc = psane_control_option (activeDS.deviceHandle,index,
            SANE_ACTION_SET_VALUE, sf, &result);

        HeapFree(GetProcessHeap(),0,sf);
    }

    if(rc == SANE_STATUS_GOOD)
    {
        if (result & SANE_INFO_RELOAD_OPTIONS || 
                result & SANE_INFO_RELOAD_PARAMS || result & SANE_INFO_INEXACT) 
            return TRUE;
    }
    return FALSE;
}

static BOOL UpdateSaneBoolOption(int index, BOOL position)
{
    SANE_Status rc = SANE_STATUS_GOOD;  
    SANE_Int result = 0;
    SANE_Bool si;

    si = position;

    rc = psane_control_option (activeDS.deviceHandle,index,
            SANE_ACTION_SET_VALUE, &si, &result);

    if(rc == SANE_STATUS_GOOD)
    {
        if (result & SANE_INFO_RELOAD_OPTIONS || 
                result & SANE_INFO_RELOAD_PARAMS || result & SANE_INFO_INEXACT) 
            return TRUE;
    }
    return FALSE;
}

static BOOL UpdateSaneIntOption(int index, SANE_Int value)
{
    SANE_Status rc = SANE_STATUS_GOOD;
    SANE_Int result = 0;

    rc = psane_control_option (activeDS.deviceHandle,index,
            SANE_ACTION_SET_VALUE, &value, &result);

    if(rc == SANE_STATUS_GOOD)
    {
        if (result & SANE_INFO_RELOAD_OPTIONS ||
                result & SANE_INFO_RELOAD_PARAMS || result & SANE_INFO_INEXACT)
            return TRUE;
    }
    return FALSE;
}

static BOOL UpdateSaneStringOption(int index, SANE_String value)
{
    SANE_Status rc = SANE_STATUS_GOOD;  
    SANE_Int result = 0;

    rc = psane_control_option (activeDS.deviceHandle,index,
            SANE_ACTION_SET_VALUE, value, &result);

    if(rc == SANE_STATUS_GOOD)
    {
        if (result & SANE_INFO_RELOAD_OPTIONS || 
                result & SANE_INFO_RELOAD_PARAMS || result & SANE_INFO_INEXACT) 
            return TRUE;
    }
    return FALSE;
}

static INT_PTR InitializeDialog(HWND hwnd)
{
    SANE_Status rc;
    SANE_Int optcount;
    HWND control;
    int i;

    rc = psane_control_option(activeDS.deviceHandle, 0, SANE_ACTION_GET_VALUE,
            &optcount, NULL);
    if (rc != SANE_STATUS_GOOD)
    {
        ERR("Unable to read number of options\n");
        return FALSE;
    }

    for ( i = 1; i < optcount; i++)
    {
        const SANE_Option_Descriptor *opt;

        control = GetDlgItem(hwnd,i+ID_BASE);

        if (!control)
            continue;

        opt = psane_get_option_descriptor(activeDS.deviceHandle, i);

        TRACE("%i %s %i %i\n",i,opt->title,opt->type,opt->constraint_type);
        
        if (!SANE_OPTION_IS_ACTIVE(opt->cap))
            EnableWindow(control,FALSE);
        else
            EnableWindow(control,TRUE);

        SendMessageA(control,CB_RESETCONTENT,0,0);
        /* initialize values */
        if (opt->type == SANE_TYPE_STRING && opt->constraint_type !=
                SANE_CONSTRAINT_NONE)
        {
            int j = 0;
            CHAR buffer[255];
            while (opt->constraint.string_list[j]!=NULL)
            {
                SendMessageA(control,CB_ADDSTRING,0,
                        (LPARAM)opt->constraint.string_list[j]);
                j++;
            }
            psane_control_option(activeDS.deviceHandle, i, SANE_ACTION_GET_VALUE, buffer,NULL);
            SendMessageA(control,CB_SELECTSTRING,0,(LPARAM)buffer);
        }
        else if (opt->type == SANE_TYPE_BOOL)
        {
            SANE_Bool b;
            psane_control_option(activeDS.deviceHandle, i,
                    SANE_ACTION_GET_VALUE, &b, NULL);
            if (b)
                SendMessageA(control,BM_SETCHECK,BST_CHECKED,0);

        }
        else if (opt->type == SANE_TYPE_INT &&
                 opt->constraint_type == SANE_CONSTRAINT_WORD_LIST)
        {
            int j, count = opt->constraint.word_list[0];
            CHAR buffer[16];
            SANE_Int val;
            for (j=1; j<=count; j++)
            {
                sprintf(buffer, "%d", opt->constraint.word_list[j]);
                SendMessageA(control, CB_ADDSTRING, 0, (LPARAM)buffer);
            }
            psane_control_option(activeDS.deviceHandle, i, SANE_ACTION_GET_VALUE, &val, NULL);
            sprintf(buffer, "%d", val);
            SendMessageA(control,CB_SELECTSTRING,0,(LPARAM)buffer);
        }
        else if (opt->constraint_type == SANE_CONSTRAINT_RANGE)
        {
            if (opt->type == SANE_TYPE_INT)
            {
                SANE_Int si;
                int min,max;

                min = (SANE_Int)opt->constraint.range->min /
                    (((SANE_Int)opt->constraint.range->quant)?
                    (SANE_Int)opt->constraint.range->quant:1);

                max = (SANE_Int)opt->constraint.range->max /
                    (((SANE_Int)opt->constraint.range->quant)
                    ?(SANE_Int)opt->constraint.range->quant:1);

                SendMessageA(control,SBM_SETRANGE,min,max);

                psane_control_option(activeDS.deviceHandle, i,
                        SANE_ACTION_GET_VALUE, &si,NULL);
                if (opt->constraint.range->quant)
                    si = si / opt->constraint.range->quant;

                SendMessageW(control,SBM_SETPOS, si, TRUE);
                UpdateRelevantEdit(hwnd, opt, i, si);
            }
            else if (opt->type == SANE_TYPE_FIXED)
            {
                SANE_Fixed *sf;

                double dd;
                double s_min,s_max,s_quant;
                INT pos;
                int min,max;

                s_min = SANE_UNFIX(opt->constraint.range->min);
                s_max = SANE_UNFIX(opt->constraint.range->max);
                s_quant = SANE_UNFIX(opt->constraint.range->quant);

                if (s_quant)
                {
                    min = (s_min / s_quant);
                    max = (s_max / s_quant);
                }
                else
                {
                    min = s_min / 0.01;
                    max = s_max / 0.01;
                }

                SendMessageA(control,SBM_SETRANGE,min,max);


                sf = HeapAlloc(GetProcessHeap(),0,opt->size*sizeof(SANE_Word));
                psane_control_option(activeDS.deviceHandle, i,
                        SANE_ACTION_GET_VALUE, sf,NULL);

                dd = SANE_UNFIX(*sf);
                HeapFree(GetProcessHeap(),0,sf);

                /* Note that conversion of float -> SANE_Fixed is lossy;
                 *   and when you truncate it into an integer, you can get
                 *   unfortunate results.  This calculation attempts
                 *   to mitigate that harm */
                if (s_quant)
                    pos = ((dd + (s_quant/2.0)) / s_quant);
                else
                    pos = (dd + 0.005) / 0.01;

                SendMessageW(control, SBM_SETPOS, pos, TRUE);
                
                UpdateRelevantEdit(hwnd, opt, i, pos);
            }
        }
    }

    return TRUE;
}

static INT_PTR ProcessScroll(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    int index;
    const SANE_Option_Descriptor *opt;
    WORD scroll;
    DWORD position;

    index = GetDlgCtrlID((HWND)lParam)- ID_BASE;
    if (index < 0)
        return FALSE;

    opt = psane_get_option_descriptor(activeDS.deviceHandle, index);

    if (!opt)
        return FALSE;

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

    UpdateRelevantEdit(hwnd, opt, index, position);
    if (UpdateSaneScrollOption(opt, index, position))
        InitializeDialog(hwnd);

    return TRUE;
}


static void ButtonClicked(HWND hwnd, INT id, HWND control)
{
    int index;
    const SANE_Option_Descriptor *opt;

    index = id - ID_BASE;
    if (index < 0)
        return;

    opt = psane_get_option_descriptor(activeDS.deviceHandle, index);

    if (!opt)
        return;

    if (opt->type == SANE_TYPE_BOOL)
    {
        BOOL r = SendMessageW(control,BM_GETCHECK,0,0)==BST_CHECKED;
        if (UpdateSaneBoolOption(index, r))
                InitializeDialog(hwnd);
    }
}

static void ComboChanged(HWND hwnd, INT id, HWND control)
{
    int index;
    int selection;
    int len;
    const SANE_Option_Descriptor *opt;
    SANE_String value;

    index = id - ID_BASE;
    if (index < 0)
        return;

    opt = psane_get_option_descriptor(activeDS.deviceHandle, index);

    if (!opt)
        return;

    selection = SendMessageW(control,CB_GETCURSEL,0,0);
    len = SendMessageW(control,CB_GETLBTEXTLEN,selection,0);

    len++;
    value = HeapAlloc(GetProcessHeap(),0,len);
    SendMessageA(control,CB_GETLBTEXT,selection,(LPARAM)value);

    if (opt->type == SANE_TYPE_STRING)
    {
        if (UpdateSaneStringOption(index, value))
                InitializeDialog(hwnd);
    }
    else if (opt->type == SANE_TYPE_INT)
    {
        if (UpdateSaneIntOption(index, atoi(value)))
            InitializeDialog(hwnd);
    }
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

#else  /* SONAME_LIBSANE */

BOOL DoScannerUI(void)
{
    return FALSE;
}

#endif  /* SONAME_LIBSANE */
