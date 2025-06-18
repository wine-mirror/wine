/*
 * NamespaceTreeControl implementation.
 *
 * Copyright 2010 David Hedberg
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

#include <stdarg.h>

#define COBJMACROS
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "shellapi.h"
#include "commctrl.h"
#include "commoncontrols.h"

#include "wine/list.h"
#include "wine/debug.h"

#include "explorerframe_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(nstc);

typedef struct nstc_root {
    IShellItem *psi;
    HTREEITEM htreeitem;
    SHCONTF enum_flags;
    NSTCROOTSTYLE root_style;
    IShellItemFilter *pif;
    struct list entry;
} nstc_root;

typedef struct {
    INameSpaceTreeControl2 INameSpaceTreeControl2_iface;
    IOleWindow IOleWindow_iface;
    LONG ref;

    HWND hwnd_main;
    HWND hwnd_tv;

    WNDPROC tv_oldwndproc;

    NSTCSTYLE style;
    NSTCSTYLE2 style2;
    struct list roots;

    INameSpaceTreeControlCustomDraw *customdraw;
    INameSpaceTreeControlDropHandler *dragdrop;
    INameSpaceTreeControlEvents *events;
} NSTC2Impl;

static const DWORD unsupported_styles =
    NSTCS_NOREPLACEOPEN | NSTCS_NOORDERSTREAM | NSTCS_FAVORITESMODE |
    NSTCS_EMPTYTEXT | NSTCS_ALLOWJUNCTIONS | NSTCS_SHOWTABSBUTTON | NSTCS_SHOWDELETEBUTTON |
    NSTCS_SHOWREFRESHBUTTON | NSTCS_SPRINGEXPAND | NSTCS_RICHTOOLTIP | NSTCS_NOINDENTCHECKS;
static const DWORD unsupported_styles2 =
    NSTCS2_INTERRUPTNOTIFICATIONS | NSTCS2_SHOWNULLSPACEMENU | NSTCS2_DISPLAYPADDING |
    NSTCS2_DISPLAYPINNEDONLY | NTSCS2_NOSINGLETONAUTOEXPAND | NTSCS2_NEVERINSERTNONENUMERATED;

static inline NSTC2Impl *impl_from_INameSpaceTreeControl2(INameSpaceTreeControl2 *iface)
{
    return CONTAINING_RECORD(iface, NSTC2Impl, INameSpaceTreeControl2_iface);
}

static inline NSTC2Impl *impl_from_IOleWindow(IOleWindow *iface)
{
    return CONTAINING_RECORD(iface, NSTC2Impl, IOleWindow_iface);
}

/* Forward declarations */
static LRESULT CALLBACK tv_wndproc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

/*************************************************************************
* NamespaceTree event wrappers
*/
static HRESULT events_OnGetDefaultIconIndex(NSTC2Impl *This, IShellItem *psi,
                                            int *piDefaultIcon, int *piOpenIcon)
{
    HRESULT ret;
    LONG refcount;
    if(!This->events) return E_NOTIMPL;

    refcount = IShellItem_AddRef(psi);
    ret = INameSpaceTreeControlEvents_OnGetDefaultIconIndex(This->events, psi, piDefaultIcon, piOpenIcon);
    if(IShellItem_Release(psi) < refcount - 1)
        ERR("ShellItem was released by client - please file a bug.\n");
    return ret;
}

static HRESULT events_OnItemAdded(NSTC2Impl *This, IShellItem *psi, BOOL fIsRoot)
{
    HRESULT ret;
    LONG refcount;
    if(!This->events) return S_OK;

    refcount = IShellItem_AddRef(psi);
    ret = INameSpaceTreeControlEvents_OnItemAdded(This->events, psi, fIsRoot);
    if(IShellItem_Release(psi) < refcount - 1)
        ERR("ShellItem was released by client - please file a bug.\n");
    return ret;
}

static HRESULT events_OnItemDeleted(NSTC2Impl *This, IShellItem *psi, BOOL fIsRoot)
{
    HRESULT ret;
    LONG refcount;
    if(!This->events) return S_OK;

    refcount = IShellItem_AddRef(psi);
    ret = INameSpaceTreeControlEvents_OnItemDeleted(This->events, psi, fIsRoot);
    if(IShellItem_Release(psi) < refcount - 1)
        ERR("ShellItem was released by client - please file a bug.\n");
    return ret;
}

static HRESULT events_OnBeforeExpand(NSTC2Impl *This, IShellItem *psi)
{
    HRESULT ret;
    LONG refcount;
    if(!This->events) return S_OK;

    refcount = IShellItem_AddRef(psi);
    ret = INameSpaceTreeControlEvents_OnBeforeExpand(This->events, psi);
    if(IShellItem_Release(psi) < refcount - 1)
        ERR("ShellItem was released by client - please file a bug.\n");
    return ret;
}

static HRESULT events_OnAfterExpand(NSTC2Impl *This, IShellItem *psi)
{
    HRESULT ret;
    LONG refcount;
    if(!This->events) return S_OK;

    refcount = IShellItem_AddRef(psi);
    ret = INameSpaceTreeControlEvents_OnAfterExpand(This->events, psi);
    if(IShellItem_Release(psi) < refcount - 1)
        ERR("ShellItem was released by client - please file a bug.\n");
    return ret;
}

static HRESULT events_OnItemClick(NSTC2Impl *This, IShellItem *psi,
                                  NSTCEHITTEST nstceHitTest, NSTCECLICKTYPE nstceClickType)
{
    HRESULT ret;
    LONG refcount;
    if(!This->events) return S_OK;

    refcount = IShellItem_AddRef(psi);
    ret = INameSpaceTreeControlEvents_OnItemClick(This->events, psi, nstceHitTest, nstceClickType);
    if(IShellItem_Release(psi) < refcount - 1)
        ERR("ShellItem was released by client - please file a bug.\n");
    return ret;
}

static HRESULT events_OnSelectionChanged(NSTC2Impl *This, IShellItemArray *psia)
{
    if(!This->events) return S_OK;

    return INameSpaceTreeControlEvents_OnSelectionChanged(This->events, psia);
}

static HRESULT events_OnKeyboardInput(NSTC2Impl *This, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if(!This->events) return S_OK;

    return INameSpaceTreeControlEvents_OnKeyboardInput(This->events, uMsg, wParam, lParam);
}

/*************************************************************************
 * NamespaceTree helper functions
 */
static DWORD treeview_style_from_nstcs(NSTC2Impl *This, NSTCSTYLE nstcs,
                                       NSTCSTYLE nstcs_mask, DWORD *new_style)
{
    DWORD old_style, tv_mask = 0;
    TRACE("%p, %lx, %lx, %p\n", This, nstcs, nstcs_mask, new_style);

    if(This->hwnd_tv)
        old_style = GetWindowLongPtrW(This->hwnd_tv, GWL_STYLE);
    else
        old_style = /* The default */
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
            WS_TABSTOP | TVS_NOHSCROLL | TVS_NONEVENHEIGHT | TVS_INFOTIP |
            TVS_EDITLABELS | TVS_TRACKSELECT;

    if(nstcs_mask & NSTCS_HASEXPANDOS)         tv_mask |= TVS_HASBUTTONS;
    if(nstcs_mask & NSTCS_HASLINES)            tv_mask |= TVS_HASLINES;
    if(nstcs_mask & NSTCS_FULLROWSELECT)       tv_mask |= TVS_FULLROWSELECT;
    if(nstcs_mask & NSTCS_HORIZONTALSCROLL)    tv_mask |= TVS_NOHSCROLL;
    if(nstcs_mask & NSTCS_ROOTHASEXPANDO)      tv_mask |= TVS_LINESATROOT;
    if(nstcs_mask & NSTCS_SHOWSELECTIONALWAYS) tv_mask |= TVS_SHOWSELALWAYS;
    if(nstcs_mask & NSTCS_NOINFOTIP)           tv_mask |= TVS_INFOTIP;
    if(nstcs_mask & NSTCS_EVENHEIGHT)          tv_mask |= TVS_NONEVENHEIGHT;
    if(nstcs_mask & NSTCS_DISABLEDRAGDROP)     tv_mask |= TVS_DISABLEDRAGDROP;
    if(nstcs_mask & NSTCS_NOEDITLABELS)        tv_mask |= TVS_EDITLABELS;
    if(nstcs_mask & NSTCS_CHECKBOXES)          tv_mask |= TVS_CHECKBOXES;
    if(nstcs_mask & NSTCS_SINGLECLICKEXPAND)   tv_mask |= TVS_SINGLEEXPAND;

    *new_style = 0;

    if(nstcs & NSTCS_HASEXPANDOS)         *new_style |= TVS_HASBUTTONS;
    if(nstcs & NSTCS_HASLINES)            *new_style |= TVS_HASLINES;
    if(nstcs & NSTCS_FULLROWSELECT)       *new_style |= TVS_FULLROWSELECT;
    if(!(nstcs & NSTCS_HORIZONTALSCROLL)) *new_style |= TVS_NOHSCROLL;
    if(nstcs & NSTCS_ROOTHASEXPANDO)      *new_style |= TVS_LINESATROOT;
    if(nstcs & NSTCS_SHOWSELECTIONALWAYS) *new_style |= TVS_SHOWSELALWAYS;
    if(!(nstcs & NSTCS_NOINFOTIP))        *new_style |= TVS_INFOTIP;
    if(!(nstcs & NSTCS_EVENHEIGHT))       *new_style |= TVS_NONEVENHEIGHT;
    if(nstcs & NSTCS_DISABLEDRAGDROP)     *new_style |= TVS_DISABLEDRAGDROP;
    if(!(nstcs & NSTCS_NOEDITLABELS))     *new_style |= TVS_EDITLABELS;
    if(nstcs & NSTCS_CHECKBOXES)          *new_style |= TVS_CHECKBOXES;
    if(nstcs & NSTCS_SINGLECLICKEXPAND)   *new_style |= TVS_SINGLEEXPAND;

    *new_style = (old_style & ~tv_mask) | (*new_style & tv_mask);

    TRACE("old: %08lx, new: %08lx\n", old_style, *new_style);

    return old_style^*new_style;
}

static IShellItem *shellitem_from_treeitem(NSTC2Impl *This, HTREEITEM hitem)
{
    TVITEMEXW tvi;

    tvi.mask = TVIF_PARAM;
    tvi.lParam = 0;
    tvi.hItem = hitem;

    SendMessageW(This->hwnd_tv, TVM_GETITEMW, 0, (LPARAM)&tvi);

    TRACE("ShellItem: %p\n", (void*)tvi.lParam);
    return (IShellItem*)tvi.lParam;
}

/* Returns the root that the given treeitem belongs to. */
static nstc_root *root_for_treeitem(NSTC2Impl *This, HTREEITEM hitem)
{
    HTREEITEM tmp, hroot = hitem;
    nstc_root *root;

    /* Work our way up the hierarchy */
    for(tmp = hitem; tmp != NULL; hroot = tmp?tmp:hroot)
        tmp = (HTREEITEM)SendMessageW(This->hwnd_tv, TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM)hroot);

    /* Search through the list of roots for a match */
    LIST_FOR_EACH_ENTRY(root, &This->roots, nstc_root, entry)
        if(root->htreeitem == hroot)
            break;

    TRACE("root is %p\n", root);
    return root;
}

/* Find a shellitem in the tree, starting from the given node. */
static HTREEITEM search_for_shellitem(NSTC2Impl *This, HTREEITEM node,
                                      IShellItem *psi)
{
    IShellItem *psi_node;
    HTREEITEM next, result = NULL;
    HRESULT hr;
    int cmpo;
    TRACE("%p, %p, %p\n", This, node, psi);

    /* Check this node */
    psi_node = shellitem_from_treeitem(This, node);
    hr = IShellItem_Compare(psi, psi_node, SICHINT_DISPLAY, &cmpo);
    if(hr == S_OK)
        return node;

    /* Any children? */
    next = (HTREEITEM)SendMessageW(This->hwnd_tv, TVM_GETNEXTITEM,
                                   TVGN_CHILD, (LPARAM)node);
    if(next)
    {
        result = search_for_shellitem(This, next, psi);
        if(result) return result;
    }

    /* Try our next sibling. */
    next = (HTREEITEM)SendMessageW(This->hwnd_tv, TVM_GETNEXTITEM,
                                   TVGN_NEXT, (LPARAM)node);
    if(next)
        result = search_for_shellitem(This, next, psi);

    return result;
}

static HTREEITEM treeitem_from_shellitem(NSTC2Impl *This, IShellItem *psi)
{
    HTREEITEM root;
    TRACE("%p, %p\n", This, psi);

    root = (HTREEITEM)SendMessageW(This->hwnd_tv, TVM_GETNEXTITEM,
                                   TVGN_ROOT, 0);
    if(!root)
        return NULL;

    return search_for_shellitem(This, root, psi);
}

static int get_icon(LPCITEMIDLIST lpi, UINT extra_flags)
{
    SHFILEINFOW sfi;
    UINT flags = SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON;
    IImageList *list;

    list = (IImageList *)SHGetFileInfoW((LPCWSTR)lpi, 0 ,&sfi, sizeof(SHFILEINFOW), flags | extra_flags);
    if (list) IImageList_Release(list);
    return sfi.iIcon;
}

/* Insert a shellitem into the given place in the tree and return the
   resulting treeitem. */
static HTREEITEM insert_shellitem(NSTC2Impl *This, IShellItem *psi,
                                  HTREEITEM hParent, HTREEITEM hInsertAfter)
{
    TVINSERTSTRUCTW tvins;
    TVITEMEXW *tvi = &tvins.itemex;
    HTREEITEM hinserted;
    TRACE("%p (%p, %p)\n", psi, hParent, hInsertAfter);

    tvi->mask = TVIF_PARAM | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT;
    tvi->cChildren = I_CHILDRENCALLBACK;
    tvi->iImage = tvi->iSelectedImage = I_IMAGECALLBACK;
    tvi->pszText = LPSTR_TEXTCALLBACKW;

    /* Every treeitem contains a pointer to the corresponding ShellItem. */
    tvi->lParam = (LPARAM)psi;
    tvins.hParent = hParent;
    tvins.hInsertAfter = hInsertAfter;

    hinserted = (HTREEITEM)SendMessageW(This->hwnd_tv, TVM_INSERTITEMW, 0,
                                        (LPARAM)(LPTVINSERTSTRUCTW)&tvins);
    if(hinserted)
        IShellItem_AddRef(psi);

    return hinserted;
}

/* Enumerates the children of the folder represented by hitem
 * according to the settings for the root, and adds them to the
 * treeview. Returns the number of children added. */
static UINT fill_sublevel(NSTC2Impl *This, HTREEITEM hitem)
{
    IShellItem *psiParent = shellitem_from_treeitem(This, hitem);
    nstc_root *root = root_for_treeitem(This, hitem);
    LPITEMIDLIST pidl_parent;
    IShellFolder *psf;
    IEnumIDList *peidl;
    UINT added = 0;
    HRESULT hr;

    hr = SHGetIDListFromObject((IUnknown*)psiParent, &pidl_parent);
    if(SUCCEEDED(hr))
    {
        hr = IShellItem_BindToHandler(psiParent, NULL, &BHID_SFObject, &IID_IShellFolder, (void**)&psf);
        if(SUCCEEDED(hr))
        {
            hr = IShellFolder_EnumObjects(psf, NULL, root->enum_flags, &peidl);
            if(SUCCEEDED(hr))
            {
                LPITEMIDLIST pidl;
                IShellItem *psi;
                ULONG fetched;

                while( S_OK == IEnumIDList_Next(peidl, 1, &pidl, &fetched) )
                {
                    hr = SHCreateShellItem(NULL, psf , pidl, &psi);
                    ILFree(pidl);
                    if(SUCCEEDED(hr))
                    {
                        if(insert_shellitem(This, psi, hitem, NULL))
                        {
                            events_OnItemAdded(This, psi, FALSE);
                            added++;
                        }

                        IShellItem_Release(psi);
                    }
                    else
                        ERR("SHCreateShellItem failed with 0x%08lx\n", hr);
                }
                IEnumIDList_Release(peidl);
            }
            else
                ERR("EnumObjects failed with 0x%08lx\n", hr);

            IShellFolder_Release(psf);
        }
        else
            ERR("BindToHandler failed with 0x%08lx\n", hr);

        ILFree(pidl_parent);
    }
    else
        ERR("SHGetIDListFromObject failed with 0x%08lx\n", hr);

    return added;
}

static HTREEITEM get_selected_treeitem(NSTC2Impl *This)
{
    return (HTREEITEM)SendMessageW(This->hwnd_tv, TVM_GETNEXTITEM, TVGN_CARET, 0);
}

static IShellItem *get_selected_shellitem(NSTC2Impl *This)
{
    return shellitem_from_treeitem(This, get_selected_treeitem(This));
}

static void collapse_all(NSTC2Impl *This, HTREEITEM node)
{
    HTREEITEM next;

    /* Collapse this node first, and then first child/next sibling. */
    SendMessageW(This->hwnd_tv, TVM_EXPAND, TVE_COLLAPSE, (LPARAM)node);

    next = (HTREEITEM)SendMessageW(This->hwnd_tv, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)node);
    if(next) collapse_all(This, next);

    next = (HTREEITEM)SendMessageW(This->hwnd_tv, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)node);
    if(next) collapse_all(This, next);
}

static HTREEITEM treeitem_from_point(NSTC2Impl *This, const POINT *pt, UINT *hitflag)
{
    TVHITTESTINFO tviht;
    tviht.pt.x = pt->x;
    tviht.pt.y = pt->y;
    tviht.hItem = NULL;

    SendMessageW(This->hwnd_tv, TVM_HITTEST, 0, (LPARAM)&tviht);
    if(hitflag) *hitflag = tviht.flags;
    return tviht.hItem;
}

/*************************************************************************
 * NamespaceTree window functions
 */
static LRESULT create_namespacetree(HWND hWnd, CREATESTRUCTW *crs)
{
    NSTC2Impl *This = crs->lpCreateParams;
    HIMAGELIST ShellSmallIconList;
    DWORD treeview_style, treeview_ex_style;

    TRACE("%p (%p)\n", This, crs);
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LPARAM)This);
    This->hwnd_main = hWnd;

    treeview_style_from_nstcs(This, This->style, 0xFFFFFFFF, &treeview_style);

    This->hwnd_tv = CreateWindowExW(0, WC_TREEVIEWW, NULL, treeview_style,
                                    0, 0, crs->cx, crs->cy,
                                    hWnd, NULL, explorerframe_hinstance, NULL);

    if(!This->hwnd_tv)
    {
        ERR("Failed to create treeview!\n");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    treeview_ex_style = TVS_EX_DRAWIMAGEASYNC | TVS_EX_RICHTOOLTIP |
        TVS_EX_DOUBLEBUFFER | TVS_EX_NOSINGLECOLLAPSE;

    if(This->style & NSTCS_AUTOHSCROLL)
        treeview_ex_style |= TVS_EX_AUTOHSCROLL;
    if(This->style & NSTCS_FADEINOUTEXPANDOS)
        treeview_ex_style |= TVS_EX_FADEINOUTEXPANDOS;
    if(This->style & NSTCS_PARTIALCHECKBOXES)
        treeview_ex_style |= TVS_EX_PARTIALCHECKBOXES;
    if(This->style & NSTCS_EXCLUSIONCHECKBOXES)
        treeview_ex_style |= TVS_EX_EXCLUSIONCHECKBOXES;
    if(This->style & NSTCS_DIMMEDCHECKBOXES)
        treeview_ex_style |= TVS_EX_DIMMEDCHECKBOXES;

    SendMessageW(This->hwnd_tv, TVM_SETEXTENDEDSTYLE, treeview_ex_style, 0xffff);

    if(Shell_GetImageLists(NULL, &ShellSmallIconList))
    {
        SendMessageW(This->hwnd_tv, TVM_SETIMAGELIST,
                     (WPARAM)TVSIL_NORMAL, (LPARAM)ShellSmallIconList);
    }
    else
    {
        ERR("Failed to get the System Image List.\n");
    }

    INameSpaceTreeControl2_AddRef(&This->INameSpaceTreeControl2_iface);

    /* Subclass the treeview to get the keyboard events. */
    This->tv_oldwndproc = (WNDPROC)SetWindowLongPtrW(This->hwnd_tv, GWLP_WNDPROC,
                                                     (ULONG_PTR)tv_wndproc);
    if(This->tv_oldwndproc)
        SetPropW(This->hwnd_tv, L"PROP_THIS", This);

    return TRUE;
}

static LRESULT resize_namespacetree(NSTC2Impl *This)
{
    RECT rc;
    TRACE("%p\n", This);

    GetClientRect(This->hwnd_main, &rc);
    MoveWindow(This->hwnd_tv, 0, 0, rc.right-rc.left, rc.bottom-rc.top, TRUE);

    return TRUE;
}

static LRESULT destroy_namespacetree(NSTC2Impl *This)
{
    TRACE("%p\n", This);

    /* Undo the subclassing */
    if(This->tv_oldwndproc)
    {
        SetWindowLongPtrW(This->hwnd_tv, GWLP_WNDPROC, (ULONG_PTR)This->tv_oldwndproc);
        RemovePropW(This->hwnd_tv, L"PROP_THIS");
    }

    INameSpaceTreeControl2_RemoveAllRoots(&This->INameSpaceTreeControl2_iface);

    /* This reference was added in create_namespacetree */
    INameSpaceTreeControl2_Release(&This->INameSpaceTreeControl2_iface);
    return TRUE;
}

static LRESULT on_tvn_deleteitemw(NSTC2Impl *This, LPARAM lParam)
{
    NMTREEVIEWW *nmtv = (NMTREEVIEWW*)lParam;
    TRACE("%p\n", This);

    IShellItem_Release((IShellItem*)nmtv->itemOld.lParam);
    return TRUE;
}

static LRESULT on_tvn_getdispinfow(NSTC2Impl *This, LPARAM lParam)
{
    NMTVDISPINFOW *dispinfo = (NMTVDISPINFOW*)lParam;
    TVITEMEXW *item = (TVITEMEXW*)&dispinfo->item;
    IShellItem *psi = shellitem_from_treeitem(This, item->hItem);
    HRESULT hr;

    TRACE("%p, %p (mask: %x)\n", This, dispinfo, item->mask);

    if(item->mask & TVIF_CHILDREN)
    {
        SFGAOF sfgao;

        hr = IShellItem_GetAttributes(psi, SFGAO_HASSUBFOLDER, &sfgao);
        if(SUCCEEDED(hr))
            item->cChildren = (sfgao & SFGAO_HASSUBFOLDER)?1:0;
        else
            item->cChildren = 1;

        item->mask |= TVIF_DI_SETITEM;
    }

    if(item->mask & (TVIF_IMAGE|TVIF_SELECTEDIMAGE))
    {
        LPITEMIDLIST pidl;

        hr = events_OnGetDefaultIconIndex(This, psi, &item->iImage, &item->iSelectedImage);
        if(FAILED(hr))
        {
            hr = SHGetIDListFromObject((IUnknown*)psi, &pidl);
            if(SUCCEEDED(hr))
            {
                item->iImage = item->iSelectedImage = get_icon(pidl, 0);
                item->mask |= TVIF_DI_SETITEM;
                ILFree(pidl);
            }
            else
                ERR("Failed to get IDList (%08lx).\n", hr);
        }
    }

    if(item->mask & TVIF_TEXT)
    {
        LPWSTR display_name;

        hr = IShellItem_GetDisplayName(psi, SIGDN_NORMALDISPLAY, &display_name);
        if(SUCCEEDED(hr))
        {
            lstrcpynW(item->pszText, display_name, MAX_PATH);
            item->mask |= TVIF_DI_SETITEM;
            CoTaskMemFree(display_name);
        }
        else
            ERR("Failed to get display name (%08lx).\n", hr);
    }

    return TRUE;
}

static BOOL treenode_has_subfolders(NSTC2Impl *This, HTREEITEM node)
{
    return SendMessageW(This->hwnd_tv, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)node);
}

static LRESULT on_tvn_itemexpandingw(NSTC2Impl *This, LPARAM lParam)
{
    NMTREEVIEWW *nmtv = (NMTREEVIEWW*)lParam;
    IShellItem *psi;
    TRACE("%p\n", This);

    psi = shellitem_from_treeitem(This, nmtv->itemNew.hItem);
    events_OnBeforeExpand(This, psi);

    if(!treenode_has_subfolders(This, nmtv->itemNew.hItem))
    {
        /* The node has no children, try to find some */
        if(!fill_sublevel(This, nmtv->itemNew.hItem))
        {
            TVITEMEXW tvi;
            /* Failed to enumerate any children, remove the expando
             * (if any). */
            tvi.hItem = nmtv->itemNew.hItem;
            tvi.mask = TVIF_CHILDREN;
            tvi.cChildren = 0;
            SendMessageW(This->hwnd_tv, TVM_SETITEMW, 0, (LPARAM)&tvi);

            return TRUE;
        }
    }
    return FALSE;
}

static LRESULT on_tvn_itemexpandedw(NSTC2Impl *This, LPARAM lParam)
{
    NMTREEVIEWW *nmtv = (NMTREEVIEWW*)lParam;
    IShellItem *psi;
    TRACE("%p\n", This);

    psi = shellitem_from_treeitem(This, nmtv->itemNew.hItem);
    events_OnAfterExpand(This, psi);
    return TRUE;
}

static LRESULT on_tvn_selchangedw(NSTC2Impl *This, LPARAM lParam)
{
    NMTREEVIEWW *nmtv = (NMTREEVIEWW*)lParam;
    IShellItemArray *psia;
    IShellItem *psi;
    HRESULT hr;
    TRACE("%p\n", This);

    /* Note: Only supports one selected item. */
    psi = shellitem_from_treeitem(This, nmtv->itemNew.hItem);
    hr = SHCreateShellItemArrayFromShellItem(psi, &IID_IShellItemArray, (void**)&psia);
    if(SUCCEEDED(hr))
    {
        events_OnSelectionChanged(This, psia);
        IShellItemArray_Release(psia);
    }

    return TRUE;
}

static LRESULT on_nm_click(NSTC2Impl *This, NMHDR *nmhdr)
{
    TVHITTESTINFO tvhit;
    IShellItem *psi;

    TRACE("%p (%p)\n", This, nmhdr);

    GetCursorPos(&tvhit.pt);
    ScreenToClient(This->hwnd_tv, &tvhit.pt);
    SendMessageW(This->hwnd_tv, TVM_HITTEST, 0, (LPARAM)&tvhit);

    if(tvhit.flags & (TVHT_NOWHERE|TVHT_ABOVE|TVHT_BELOW))
        return TRUE;

    /* TVHT_ maps onto the corresponding NSTCEHT_ */
    psi = shellitem_from_treeitem(This, tvhit.hItem);
    return FAILED(events_OnItemClick(This, psi, tvhit.flags, NSTCECT_LBUTTON));
}

static LRESULT on_wm_mbuttonup(NSTC2Impl *This, WPARAM wParam, LPARAM lParam)
{
    TVHITTESTINFO tvhit;
    IShellItem *psi;
    HRESULT hr;
    TRACE("%p (%Ix, %Ix)\n", This, wParam, lParam);

    tvhit.pt.x = (int)(short)LOWORD(lParam);
    tvhit.pt.y = (int)(short)HIWORD(lParam);

    SendMessageW(This->hwnd_tv, TVM_HITTEST, 0, (LPARAM)&tvhit);

    /* Seems to generate only ONITEM and ONITEMICON */
    if( !(tvhit.flags & (TVHT_ONITEM|TVHT_ONITEMICON)) )
        return FALSE;

    psi = shellitem_from_treeitem(This, tvhit.hItem);
    hr = events_OnItemClick(This, psi, tvhit.flags, NSTCECT_MBUTTON);

    if(SUCCEEDED(hr))
        return FALSE;
    else
        return TRUE;
}

static LRESULT on_kbd_event(NSTC2Impl *This, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    IShellItem *psi;
    HTREEITEM hitem;
    TRACE("%p : %d, %Ix, %Ix\n", This, uMsg, wParam, lParam);

    /* Handled by the client? */
    if(FAILED(events_OnKeyboardInput(This, uMsg, wParam, lParam)))
        return TRUE;

    if(uMsg == WM_KEYDOWN)
    {
        switch(wParam)
        {
        case VK_DELETE:
            psi = get_selected_shellitem(This);
            FIXME("Deletion of file requested (shellitem: %p).\n", psi);
            return TRUE;

        case VK_F2:
            hitem = get_selected_treeitem(This);
            SendMessageW(This->hwnd_tv, TVM_EDITLABELW, 0, (LPARAM)hitem);
            return TRUE;
        }
    }

    /* Let the TreeView handle the key */
    return FALSE;
}

static LRESULT CALLBACK tv_wndproc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    NSTC2Impl *This = (NSTC2Impl*)GetPropW(hWnd, L"PROP_THIS");

    switch(uMessage) {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_SYSCHAR:
        if(on_kbd_event(This, uMessage, wParam, lParam))
            return TRUE;
        break;

    case WM_MBUTTONUP:        return on_wm_mbuttonup(This, wParam, lParam);
    }

    /* Pass the message on to the treeview */
    return CallWindowProcW(This->tv_oldwndproc, hWnd, uMessage, wParam, lParam);
}

static LRESULT CALLBACK NSTC2_WndProc(HWND hWnd, UINT uMessage,
                                      WPARAM wParam, LPARAM lParam)
{
    NSTC2Impl *This = (NSTC2Impl*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
    NMHDR *nmhdr;

    switch(uMessage)
    {
    case WM_NCCREATE:         return create_namespacetree(hWnd, (CREATESTRUCTW*)lParam);
    case WM_SIZE:             return resize_namespacetree(This);
    case WM_DESTROY:          return destroy_namespacetree(This);
    case WM_NOTIFY:
        nmhdr = (NMHDR*)lParam;
        switch(nmhdr->code)
        {
        case NM_CLICK:            return on_nm_click(This, nmhdr);
        case TVN_DELETEITEMW:     return on_tvn_deleteitemw(This, lParam);
        case TVN_GETDISPINFOW:    return on_tvn_getdispinfow(This, lParam);
        case TVN_ITEMEXPANDINGW:  return on_tvn_itemexpandingw(This, lParam);
        case TVN_ITEMEXPANDEDW:   return on_tvn_itemexpandedw(This, lParam);
        case TVN_SELCHANGEDW:     return on_tvn_selchangedw(This, lParam);
        default:                  break;
        }
        break;
    default:                  return DefWindowProcW(hWnd, uMessage, wParam, lParam);
    }
    return 0;
}

/**************************************************************************
 * INameSpaceTreeControl2 Implementation
 */
static HRESULT WINAPI NSTC2_fnQueryInterface(INameSpaceTreeControl2* iface,
                                             REFIID riid,
                                             void **ppvObject)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    TRACE("%p (%s, %p)\n", This, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;
    if(IsEqualIID(riid, &IID_INameSpaceTreeControl2) ||
       IsEqualIID(riid, &IID_INameSpaceTreeControl) ||
       IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObject = &This->INameSpaceTreeControl2_iface;
    }
    else if(IsEqualIID(riid, &IID_IOleWindow))
    {
        *ppvObject = &This->IOleWindow_iface;
    }

    if(*ppvObject)
    {
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI NSTC2_fnAddRef(INameSpaceTreeControl2* iface)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p - ref %ld\n", This, ref);

    return ref;
}

static ULONG WINAPI NSTC2_fnRelease(INameSpaceTreeControl2* iface)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p - ref: %ld\n", This, ref);

    if(!ref)
    {
        TRACE("Freeing.\n");
        free(This);
        EFRAME_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI NSTC2_fnInitialize(INameSpaceTreeControl2* iface,
                                         HWND hwndParent,
                                         RECT *prc,
                                         NSTCSTYLE nstcsFlags)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    WNDCLASSW wc;
    DWORD window_style, window_ex_style;
    INITCOMMONCONTROLSEX icex;
    RECT rc;
    static const WCHAR NSTC2_CLASS_NAME[] = L"NamespaceTreeControl";

    TRACE("%p (%p, %p, %lx)\n", This, hwndParent, prc, nstcsFlags);

    if(nstcsFlags & unsupported_styles)
        FIXME("0x%08lx contains the unsupported style(s) 0x%08lx\n",
              nstcsFlags, nstcsFlags & unsupported_styles);

    This->style = nstcsFlags;

    icex.dwSize = sizeof( icex );
    icex.dwICC = ICC_TREEVIEW_CLASSES;
    InitCommonControlsEx( &icex );

    if(!GetClassInfoW(explorerframe_hinstance, NSTC2_CLASS_NAME, &wc))
    {
        wc.style            = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc      = NSTC2_WndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = explorerframe_hinstance;
        wc.hIcon            = 0;
        wc.hCursor          = LoadCursorW(0, (LPWSTR)IDC_ARROW);
        wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = NSTC2_CLASS_NAME;

        if (!RegisterClassW(&wc)) return E_FAIL;
    }

    /* NSTCS_TABSTOP and NSTCS_BORDER affects the host window */
    window_style = WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
        (nstcsFlags & NSTCS_BORDER ? WS_BORDER : 0);
    window_ex_style = nstcsFlags & NSTCS_TABSTOP ? WS_EX_CONTROLPARENT : 0;

    if(prc)
        rc = *prc;
    else
        SetRectEmpty(&rc);

    This->hwnd_main = CreateWindowExW(window_ex_style, NSTC2_CLASS_NAME, NULL, window_style,
                                      rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                                      hwndParent, 0, explorerframe_hinstance, This);

    if(!This->hwnd_main)
    {
        ERR("Failed to create the window.\n");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

static HRESULT WINAPI NSTC2_fnTreeAdvise(INameSpaceTreeControl2* iface, IUnknown *handler, DWORD *cookie)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);

    TRACE("%p (%p, %p)\n", This, handler, cookie);

    *cookie = 0;

    /* Only one client supported */
    if (This->events || This->customdraw || This->dragdrop)
        return E_FAIL;

    /* FIXME: request INameSpaceTreeAccessible too */
    IUnknown_QueryInterface(handler, &IID_INameSpaceTreeControlEvents, (void**)&This->events);
    IUnknown_QueryInterface(handler, &IID_INameSpaceTreeControlCustomDraw, (void**)&This->customdraw);
    IUnknown_QueryInterface(handler, &IID_INameSpaceTreeControlDropHandler, (void**)&This->dragdrop);

    if (This->events || This->customdraw || This->dragdrop)
        *cookie = 1;

    return *cookie ? S_OK : E_FAIL;
}

static HRESULT WINAPI NSTC2_fnTreeUnadvise(INameSpaceTreeControl2* iface, DWORD cookie)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);

    TRACE("%p (%lx)\n", This, cookie);

    /* The cookie is ignored. */

    if (This->events)
    {
        INameSpaceTreeControlEvents_Release(This->events);
        This->events = NULL;
    }

    if (This->customdraw)
    {
        INameSpaceTreeControlCustomDraw_Release(This->customdraw);
        This->customdraw = NULL;
    }

    if (This->dragdrop)
    {
        INameSpaceTreeControlDropHandler_Release(This->dragdrop);
        This->dragdrop = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI NSTC2_fnInsertRoot(INameSpaceTreeControl2* iface,
                                         int iIndex,
                                         IShellItem *psiRoot,
                                         SHCONTF grfEnumFlags,
                                         NSTCROOTSTYLE grfRootStyle,
                                         IShellItemFilter *pif)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    nstc_root *new_root;
    struct list *add_after_entry;
    HTREEITEM add_after_hitem;
    int i;

    TRACE("%p, %d, %p, %lx, %lx, %p\n", This, iIndex, psiRoot, grfEnumFlags, grfRootStyle, pif);

    new_root = malloc(sizeof(*new_root));
    if(!new_root)
        return E_OUTOFMEMORY;

    new_root->psi = psiRoot;
    new_root->enum_flags = grfEnumFlags;
    new_root->root_style = grfRootStyle;
    new_root->pif = pif;

    /* We want to keep the roots in the internal list and in the
     * treeview in the same order. */
    add_after_entry = &This->roots;
    for(i = 0; i < max(0, iIndex) && list_next(&This->roots, add_after_entry); i++)
        add_after_entry = list_next(&This->roots, add_after_entry);

    if(add_after_entry == &This->roots)
        add_after_hitem = TVI_FIRST;
    else
        add_after_hitem = LIST_ENTRY(add_after_entry, nstc_root, entry)->htreeitem;

    new_root->htreeitem = insert_shellitem(This, psiRoot, TVI_ROOT, add_after_hitem);
    if(!new_root->htreeitem)
    {
        WARN("Failed to add the root.\n");
        free(new_root);
        return E_FAIL;
    }

    list_add_after(add_after_entry, &new_root->entry);
    events_OnItemAdded(This, psiRoot, TRUE);

    if(grfRootStyle & NSTCRS_HIDDEN)
    {
        TVITEMEXW tvi;
        tvi.mask = TVIF_STATEEX;
        tvi.uStateEx = TVIS_EX_FLAT;
        tvi.hItem = new_root->htreeitem;

        SendMessageW(This->hwnd_tv, TVM_SETITEMW, 0, (LPARAM)&tvi);
    }

    if(grfRootStyle & NSTCRS_EXPANDED)
        SendMessageW(This->hwnd_tv, TVM_EXPAND, TVE_EXPAND,
                     (LPARAM)new_root->htreeitem);

    return S_OK;
}

static HRESULT WINAPI NSTC2_fnAppendRoot(INameSpaceTreeControl2* iface,
                                         IShellItem *psiRoot,
                                         SHCONTF grfEnumFlags,
                                         NSTCROOTSTYLE grfRootStyle,
                                         IShellItemFilter *pif)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    UINT root_count;
    TRACE("%p, %p, %lx, %lx, %p\n",
          This, psiRoot, grfEnumFlags, grfRootStyle, pif);

    root_count = list_count(&This->roots);

    return INameSpaceTreeControl2_InsertRoot(iface, root_count, psiRoot, grfEnumFlags, grfRootStyle, pif);
}

static HRESULT WINAPI NSTC2_fnRemoveRoot(INameSpaceTreeControl2* iface,
                                         IShellItem *psiRoot)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    nstc_root *cursor, *root = NULL;
    TRACE("%p (%p)\n", This, psiRoot);

    if(!psiRoot)
        return E_NOINTERFACE;

    LIST_FOR_EACH_ENTRY(cursor, &This->roots, nstc_root, entry)
    {
        HRESULT hr;
        int order;
        hr = IShellItem_Compare(psiRoot, cursor->psi, SICHINT_DISPLAY, &order);
        if(hr == S_OK)
        {
            root = cursor;
            break;
        }
    }

    TRACE("root %p\n", root);
    if(root)
    {
        events_OnItemDeleted(This, root->psi, TRUE);
        SendMessageW(This->hwnd_tv, TVM_DELETEITEM, 0, (LPARAM)root->htreeitem);
        list_remove(&root->entry);
        free(root);
        return S_OK;
    }
    else
    {
        WARN("No matching root found.\n");
        return E_FAIL;
    }
}

static HRESULT WINAPI NSTC2_fnRemoveAllRoots(INameSpaceTreeControl2* iface)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    nstc_root *cur1, *cur2;

    TRACE("%p\n", This);

    if (list_empty(&This->roots))
        return E_INVALIDARG;

    LIST_FOR_EACH_ENTRY_SAFE(cur1, cur2, &This->roots, nstc_root, entry)
        INameSpaceTreeControl2_RemoveRoot(iface, cur1->psi);

    return S_OK;
}

static HRESULT WINAPI NSTC2_fnGetRootItems(INameSpaceTreeControl2* iface,
                                           IShellItemArray **ppsiaRootItems)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    IShellFolder *psf;
    LPITEMIDLIST *array;
    nstc_root *root;
    UINT count, i;
    HRESULT hr;
    TRACE("%p (%p)\n", This, ppsiaRootItems);

    count = list_count(&This->roots);

    if(!count)
        return E_INVALIDARG;

    array = malloc(sizeof(LPITEMIDLIST)*count);

    i = 0;
    LIST_FOR_EACH_ENTRY(root, &This->roots, nstc_root, entry)
        SHGetIDListFromObject((IUnknown*)root->psi, &array[i++]);

    SHGetDesktopFolder(&psf);
    hr = SHCreateShellItemArray(NULL, psf, count, (PCUITEMID_CHILD_ARRAY)array,
                                ppsiaRootItems);
    IShellFolder_Release(psf);

    for(i = 0; i < count; i++)
        ILFree(array[i]);

    free(array);

    return hr;
}

static HRESULT WINAPI NSTC2_fnSetItemState(INameSpaceTreeControl2* iface,
                                           IShellItem *psi,
                                           NSTCITEMSTATE nstcisMask,
                                           NSTCITEMSTATE nstcisFlags)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    TVITEMEXW tvi;
    HTREEITEM hitem;

    TRACE("%p (%p, %lx, %lx)\n", This, psi, nstcisMask, nstcisFlags);

    hitem = treeitem_from_shellitem(This, psi);
    if(!hitem) return E_INVALIDARG;

    /* Passing both NSTCIS_SELECTED and NSTCIS_SELECTEDNOEXPAND results
       in two TVM_SETITEMW's */
    if((nstcisMask&nstcisFlags) & NSTCIS_SELECTED)
    {
        SendMessageW(This->hwnd_tv, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hitem);
        SendMessageW(This->hwnd_tv, TVM_ENSUREVISIBLE, 0, (LPARAM)hitem);
    }
    if((nstcisMask&nstcisFlags) & NSTCIS_SELECTEDNOEXPAND)
    {
        SendMessageW(This->hwnd_tv, TVM_SELECTITEM, TVGN_CARET|TVSI_NOSINGLEEXPAND, (LPARAM)hitem);
    }

    /* If NSTCIS_EXPANDED is among the flags, the mask is ignored. */
    if((nstcisMask|nstcisFlags) & NSTCIS_EXPANDED)
    {
        WPARAM arg = nstcisFlags&NSTCIS_EXPANDED ? TVE_EXPAND:TVE_COLLAPSE;
        SendMessageW(This->hwnd_tv, TVM_EXPAND, arg, (LPARAM)hitem);
    }

    if(nstcisMask & NSTCIS_DISABLED)
        tvi.mask = TVIF_STATE | TVIF_STATEEX;
    else if( ((nstcisMask^nstcisFlags) & (NSTCIS_SELECTED|NSTCIS_EXPANDED|NSTCIS_SELECTEDNOEXPAND)) ||
             ((nstcisMask|nstcisFlags) & NSTCIS_BOLD) ||
             (nstcisFlags & NSTCIS_DISABLED) )
        tvi.mask = TVIF_STATE;
    else
        tvi.mask = 0;

    if(tvi.mask)
    {
        tvi.stateMask = tvi.state = 0;
        tvi.stateMask |= ((nstcisFlags^nstcisMask)&NSTCIS_SELECTED) ? TVIS_SELECTED : 0;
        tvi.stateMask |= (nstcisMask|nstcisFlags)&NSTCIS_BOLD ? TVIS_BOLD:0;
        tvi.state     |= (nstcisMask&nstcisFlags)&NSTCIS_BOLD ? TVIS_BOLD:0;

        if((nstcisMask&NSTCIS_EXPANDED)^(nstcisFlags&NSTCIS_EXPANDED))
        {
            tvi.stateMask = 0;
        }

        tvi.uStateEx = (nstcisFlags&nstcisMask)&NSTCIS_DISABLED?TVIS_EX_DISABLED:0;
        tvi.hItem = hitem;

        SendMessageW(This->hwnd_tv, TVM_SETITEMW, 0, (LPARAM)&tvi);
    }

    return S_OK;
}

static HRESULT WINAPI NSTC2_fnGetItemState(INameSpaceTreeControl2* iface,
                                           IShellItem *psi,
                                           NSTCITEMSTATE nstcisMask,
                                           NSTCITEMSTATE *pnstcisFlags)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    HTREEITEM hitem;
    TVITEMEXW tvi;
    TRACE("%p (%p, %lx, %p)\n", This, psi, nstcisMask, pnstcisFlags);

    hitem = treeitem_from_shellitem(This, psi);
    if(!hitem)
        return E_INVALIDARG;

    *pnstcisFlags = 0;

    tvi.hItem = hitem;
    tvi.mask = TVIF_STATE;
    tvi.stateMask = TVIS_SELECTED|TVIS_EXPANDED|TVIS_BOLD;

    if(nstcisMask & NSTCIS_DISABLED)
        tvi.mask |= TVIF_STATEEX;

    SendMessageW(This->hwnd_tv, TVM_GETITEMW, 0, (LPARAM)&tvi);
    *pnstcisFlags |= (tvi.state & TVIS_SELECTED)?NSTCIS_SELECTED:0;
    *pnstcisFlags |= (tvi.state & TVIS_EXPANDED)?NSTCIS_EXPANDED:0;
    *pnstcisFlags |= (tvi.state & TVIS_BOLD)?NSTCIS_BOLD:0;
    *pnstcisFlags |= (tvi.uStateEx & TVIS_EX_DISABLED)?NSTCIS_DISABLED:0;

    *pnstcisFlags &= nstcisMask;

    return S_OK;
}

static HRESULT WINAPI NSTC2_fnGetSelectedItems(INameSpaceTreeControl2* iface,
                                               IShellItemArray **psiaItems)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    IShellItem *psiselected;

    TRACE("%p (%p)\n", This, psiaItems);

    psiselected = get_selected_shellitem(This);
    if(!psiselected)
    {
        *psiaItems = NULL;
        return E_FAIL;
    }

    return SHCreateShellItemArrayFromShellItem(psiselected, &IID_IShellItemArray,
                                             (void**)psiaItems);
}

static HRESULT WINAPI NSTC2_fnGetItemCustomState(INameSpaceTreeControl2* iface,
                                                 IShellItem *psi,
                                                 int *piStateNumber)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    FIXME("stub, %p (%p, %p)\n", This, psi, piStateNumber);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnSetItemCustomState(INameSpaceTreeControl2* iface,
                                                 IShellItem *psi,
                                                 int iStateNumber)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    FIXME("stub, %p (%p, %d)\n", This, psi, iStateNumber);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnEnsureItemVisible(INameSpaceTreeControl2* iface,
                                                IShellItem *psi)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    HTREEITEM hitem;

    TRACE("%p (%p)\n", This, psi);

    hitem = treeitem_from_shellitem(This, psi);
    if(hitem)
    {
        SendMessageW(This->hwnd_tv, TVM_ENSUREVISIBLE, 0, (WPARAM)hitem);
        return S_OK;
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI NSTC2_fnSetTheme(INameSpaceTreeControl2* iface,
                                       LPCWSTR pszTheme)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    FIXME("stub, %p (%p)\n", This, pszTheme);
    return E_NOTIMPL;
}

static HRESULT WINAPI NSTC2_fnGetNextItem(INameSpaceTreeControl2* iface,
                                          IShellItem *psi,
                                          NSTCGNI nstcgi,
                                          IShellItem **ppsiNext)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    HTREEITEM hitem, hnext;
    UINT tvgn;
    TRACE("%p (%p, %x, %p)\n", This, psi, nstcgi, ppsiNext);

    if(!ppsiNext) return E_POINTER;
    if(!psi)      return E_FAIL;

    *ppsiNext = NULL;

    hitem = treeitem_from_shellitem(This, psi);
    if(!hitem)
        return E_INVALIDARG;

    switch(nstcgi)
    {
    case NSTCGNI_NEXT:         tvgn = TVGN_NEXT; break;
    case NSTCGNI_NEXTVISIBLE:  tvgn = TVGN_NEXTVISIBLE; break;
    case NSTCGNI_PREV:         tvgn = TVGN_PREVIOUS; break;
    case NSTCGNI_PREVVISIBLE:  tvgn = TVGN_PREVIOUSVISIBLE; break;
    case NSTCGNI_PARENT:       tvgn = TVGN_PARENT; break;
    case NSTCGNI_CHILD:        tvgn = TVGN_CHILD; break;
    case NSTCGNI_FIRSTVISIBLE: tvgn = TVGN_FIRSTVISIBLE; break;
    case NSTCGNI_LASTVISIBLE:  tvgn = TVGN_LASTVISIBLE; break;
    default:
        FIXME("Unknown nstcgi value %d\n", nstcgi);
        return E_FAIL;
    }

    hnext = (HTREEITEM)SendMessageW(This->hwnd_tv, TVM_GETNEXTITEM, tvgn, (WPARAM)hitem);
    if(hnext)
    {
        *ppsiNext = shellitem_from_treeitem(This, hnext);
        IShellItem_AddRef(*ppsiNext);
        return S_OK;
    }

    return E_FAIL;
}

static HRESULT WINAPI NSTC2_fnHitTest(INameSpaceTreeControl2* iface,
                                      POINT *ppt,
                                      IShellItem **ppsiOut)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    HTREEITEM hitem;
    TRACE("%p (%p, %p)\n", This, ppsiOut, ppt);

    if(!ppt || !ppsiOut)
        return E_POINTER;

    *ppsiOut = NULL;

    hitem = treeitem_from_point(This, ppt, NULL);
    if(hitem)
        *ppsiOut = shellitem_from_treeitem(This, hitem);

    if(*ppsiOut)
    {
        IShellItem_AddRef(*ppsiOut);
        return S_OK;
    }

    return S_FALSE;
}

static HRESULT WINAPI NSTC2_fnGetItemRect(INameSpaceTreeControl2* iface,
                                          IShellItem *psi,
                                          RECT *prect)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    HTREEITEM hitem;
    TRACE("%p (%p, %p)\n", This, psi, prect);

    if(!psi || !prect)
        return E_POINTER;

    hitem = treeitem_from_shellitem(This, psi);
    if(hitem)
    {
        *(HTREEITEM*)prect = hitem;
        if(SendMessageW(This->hwnd_tv, TVM_GETITEMRECT, FALSE, (LPARAM)prect))
        {
            MapWindowPoints(This->hwnd_tv, HWND_DESKTOP, (POINT*)prect, 2);
            return S_OK;
        }
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI NSTC2_fnCollapseAll(INameSpaceTreeControl2* iface)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    nstc_root *root;
    TRACE("%p\n", This);

    LIST_FOR_EACH_ENTRY(root, &This->roots, nstc_root, entry)
        collapse_all(This, root->htreeitem);

    return S_OK;
}

static HRESULT WINAPI NSTC2_fnSetControlStyle(INameSpaceTreeControl2* iface,
                                              NSTCSTYLE nstcsMask,
                                              NSTCSTYLE nstcsStyle)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    static const DWORD tv_style_flags =
        NSTCS_HASEXPANDOS | NSTCS_HASLINES | NSTCS_FULLROWSELECT |
        NSTCS_HORIZONTALSCROLL | NSTCS_ROOTHASEXPANDO |
        NSTCS_SHOWSELECTIONALWAYS | NSTCS_NOINFOTIP | NSTCS_EVENHEIGHT |
        NSTCS_DISABLEDRAGDROP | NSTCS_NOEDITLABELS | NSTCS_CHECKBOXES;
    static const DWORD host_style_flags = NSTCS_TABSTOP | NSTCS_BORDER;
    static const DWORD nstc_flags =
        NSTCS_SINGLECLICKEXPAND | NSTCS_NOREPLACEOPEN | NSTCS_NOORDERSTREAM |
        NSTCS_FAVORITESMODE | NSTCS_EMPTYTEXT | NSTCS_ALLOWJUNCTIONS |
        NSTCS_SHOWTABSBUTTON | NSTCS_SHOWDELETEBUTTON | NSTCS_SHOWREFRESHBUTTON;
    TRACE("%p (%lx, %lx)\n", This, nstcsMask, nstcsStyle);

    /* Fail if there is an attempt to set an unknown style. */
    if(nstcsMask & ~(tv_style_flags | host_style_flags | nstc_flags))
        return E_FAIL;

    if(nstcsMask & tv_style_flags)
    {
        DWORD new_style;
        treeview_style_from_nstcs(This, nstcsStyle, nstcsMask, &new_style);
        SetWindowLongPtrW(This->hwnd_tv, GWL_STYLE, new_style);
    }

    /* Flags affecting the host window */
    if(nstcsMask & NSTCS_BORDER)
    {
        DWORD new_style = GetWindowLongPtrW(This->hwnd_main, GWL_STYLE);
        new_style &= ~WS_BORDER;
        new_style |= nstcsStyle & NSTCS_BORDER ? WS_BORDER : 0;
        SetWindowLongPtrW(This->hwnd_main, GWL_STYLE, new_style);
    }
    if(nstcsMask & NSTCS_TABSTOP)
    {
        DWORD new_style = GetWindowLongPtrW(This->hwnd_main, GWL_EXSTYLE);
        new_style &= ~WS_EX_CONTROLPARENT;
        new_style |= nstcsStyle & NSTCS_TABSTOP ? WS_EX_CONTROLPARENT : 0;
        SetWindowLongPtrW(This->hwnd_main, GWL_EXSTYLE, new_style);
    }

    if((nstcsStyle & nstcsMask) & unsupported_styles)
        FIXME("mask & style (0x%08lx) contains unsupported style(s): 0x%08lx\n",
              (nstcsStyle & nstcsMask),
              (nstcsStyle & nstcsMask) & unsupported_styles);

    This->style &= ~nstcsMask;
    This->style |= (nstcsStyle & nstcsMask);

    return S_OK;
}

static HRESULT WINAPI NSTC2_fnGetControlStyle(INameSpaceTreeControl2* iface,
                                              NSTCSTYLE nstcsMask,
                                              NSTCSTYLE *pnstcsStyle)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    TRACE("%p (%lx, %p)\n", This, nstcsMask, pnstcsStyle);

    *pnstcsStyle = (This->style & nstcsMask);

    return S_OK;
}

static HRESULT WINAPI NSTC2_fnSetControlStyle2(INameSpaceTreeControl2* iface,
                                               NSTCSTYLE2 nstcsMask,
                                               NSTCSTYLE2 nstcsStyle)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    TRACE("%p (%x, %x)\n", This, nstcsMask, nstcsStyle);

    if((nstcsStyle & nstcsMask) & unsupported_styles2)
        FIXME("mask & style (0x%08x) contains unsupported style(s): 0x%08lx\n",
              (nstcsStyle & nstcsMask),
              (nstcsStyle & nstcsMask) & unsupported_styles2);

    This->style2 &= ~nstcsMask;
    This->style2 |= (nstcsStyle & nstcsMask);

    return S_OK;
}

static HRESULT WINAPI NSTC2_fnGetControlStyle2(INameSpaceTreeControl2* iface,
                                               NSTCSTYLE2 nstcsMask,
                                               NSTCSTYLE2 *pnstcsStyle)
{
    NSTC2Impl *This = impl_from_INameSpaceTreeControl2(iface);
    TRACE("%p (%x, %p)\n", This, nstcsMask, pnstcsStyle);

    *pnstcsStyle = (This->style2 & nstcsMask);

    return S_OK;
}

static const INameSpaceTreeControl2Vtbl vt_INameSpaceTreeControl2 = {
    NSTC2_fnQueryInterface,
    NSTC2_fnAddRef,
    NSTC2_fnRelease,
    NSTC2_fnInitialize,
    NSTC2_fnTreeAdvise,
    NSTC2_fnTreeUnadvise,
    NSTC2_fnAppendRoot,
    NSTC2_fnInsertRoot,
    NSTC2_fnRemoveRoot,
    NSTC2_fnRemoveAllRoots,
    NSTC2_fnGetRootItems,
    NSTC2_fnSetItemState,
    NSTC2_fnGetItemState,
    NSTC2_fnGetSelectedItems,
    NSTC2_fnGetItemCustomState,
    NSTC2_fnSetItemCustomState,
    NSTC2_fnEnsureItemVisible,
    NSTC2_fnSetTheme,
    NSTC2_fnGetNextItem,
    NSTC2_fnHitTest,
    NSTC2_fnGetItemRect,
    NSTC2_fnCollapseAll,
    NSTC2_fnSetControlStyle,
    NSTC2_fnGetControlStyle,
    NSTC2_fnSetControlStyle2,
    NSTC2_fnGetControlStyle2
};

/**************************************************************************
 * IOleWindow Implementation
 */

static HRESULT WINAPI IOW_fnQueryInterface(IOleWindow *iface, REFIID riid, void **ppvObject)
{
    NSTC2Impl *This = impl_from_IOleWindow(iface);
    return INameSpaceTreeControl2_QueryInterface(&This->INameSpaceTreeControl2_iface, riid, ppvObject);
}

static ULONG WINAPI IOW_fnAddRef(IOleWindow *iface)
{
    NSTC2Impl *This = impl_from_IOleWindow(iface);
    return INameSpaceTreeControl2_AddRef(&This->INameSpaceTreeControl2_iface);
}

static ULONG WINAPI IOW_fnRelease(IOleWindow *iface)
{
    NSTC2Impl *This = impl_from_IOleWindow(iface);
    return INameSpaceTreeControl2_Release(&This->INameSpaceTreeControl2_iface);
}

static HRESULT WINAPI IOW_fnGetWindow(IOleWindow *iface, HWND *phwnd)
{
    NSTC2Impl *This = impl_from_IOleWindow(iface);
    TRACE("%p (%p)\n", This, phwnd);

    *phwnd = This->hwnd_main;
    return S_OK;
}

static HRESULT WINAPI IOW_fnContextSensitiveHelp(IOleWindow *iface, BOOL fEnterMode)
{
    NSTC2Impl *This = impl_from_IOleWindow(iface);
    TRACE("%p (%d)\n", This, fEnterMode);

    /* Not implemented */
    return E_NOTIMPL;
}

static const IOleWindowVtbl vt_IOleWindow = {
    IOW_fnQueryInterface,
    IOW_fnAddRef,
    IOW_fnRelease,
    IOW_fnGetWindow,
    IOW_fnContextSensitiveHelp
};

HRESULT NamespaceTreeControl_Constructor(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    NSTC2Impl *nstc;
    HRESULT ret;

    TRACE ("%p %s %p\n", pUnkOuter, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    if(pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    EFRAME_LockModule();

    nstc = calloc(1, sizeof(*nstc));
    if (!nstc)
        return E_OUTOFMEMORY;

    nstc->ref = 1;
    nstc->INameSpaceTreeControl2_iface.lpVtbl = &vt_INameSpaceTreeControl2;
    nstc->IOleWindow_iface.lpVtbl = &vt_IOleWindow;

    list_init(&nstc->roots);

    ret = INameSpaceTreeControl2_QueryInterface(&nstc->INameSpaceTreeControl2_iface, riid, ppv);
    INameSpaceTreeControl2_Release(&nstc->INameSpaceTreeControl2_iface);

    TRACE("--(%p)\n", ppv);
    return ret;
}
