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

#include "gphoto2_i.h"
#include "winuser.h"
#include "winnls.h"
#include "wingdi.h"
#include "winreg.h"
#include "commctrl.h"
#include "prsht.h"
#include "wine/debug.h"
#include "resource.h"

static const char settings_key[] = "Software\\Wine\\Gphoto2";
static const char settings_value[] = "SkipUI";
static BOOL disable_dialog;
static HBITMAP static_bitmap;

static INT_PTR CALLBACK ConnectingProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}

static void on_disable_dialog_clicked(HWND dialog)
{
    if (IsDlgButtonChecked(dialog, IDC_SKIP) == BST_CHECKED)
        disable_dialog = TRUE;
    else
        disable_dialog = FALSE;
}

static void UI_EndDialog(HWND hwnd, INT_PTR rc)
{
    if (disable_dialog)
    {
        HKEY key;
        const DWORD data = 1;
        if (RegCreateKeyExA(HKEY_CURRENT_USER, settings_key, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL) == ERROR_SUCCESS)
        {
            RegSetValueExA(key, settings_value, 0, REG_DWORD, (const BYTE *)&data, sizeof(DWORD));
            RegCloseKey(key);
        }
    }
    EndDialog(hwnd, rc);
}

static BOOL GetAllImages(void)
{
    struct gphoto2_file *file;
    BOOL has_images = FALSE;

    LIST_FOR_EACH_ENTRY( file, &activeDS.files, struct gphoto2_file, entry)
    {
        if (strstr(file->filename,".JPG") || strstr(file->filename,".jpg"))
        {
            file->download = TRUE;
            has_images = TRUE;
        }
    }
    return has_images;
}

static void PopulateListView(HWND List)
{
	struct gphoto2_file *file;
	LVITEMA item;
	int index = 0;

	LIST_FOR_EACH_ENTRY( file, &activeDS.files, struct gphoto2_file, entry)
	{
		if (strstr(file->filename,".JPG") || strstr(file->filename,".jpg")) 
		{
			item.mask = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE ;
			item.iItem = index;
			item.iSubItem = 0;
			item.pszText = file->filename;
			item.iImage = index;
			item.lParam= (LPARAM)file;

			SendMessageA(List, LVM_INSERTITEMA,0,(LPARAM)&item);
			index ++;
		}
	}
}

static void PopulateImageList(HIMAGELIST *iList, HWND list)
{
	struct gphoto2_file *file;
	HWND 	progress_dialog;

	progress_dialog =
		CreateDialogW(GPHOTO2_instance,(LPWSTR)MAKEINTRESOURCE(IDD_CONNECTING),
				NULL, ConnectingProc);
	
	LIST_FOR_EACH_ENTRY( file, &activeDS.files, struct gphoto2_file, entry)
	{
		if (strstr(file->filename,".JPG") || strstr(file->filename,".jpg")) 
		{
			HBITMAP 	bitmap;
			BITMAP		bmpInfo;

#ifdef HAVE_GPHOTO2
			_get_gphoto2_file_as_DIB(file->folder, file->filename,
					GP_FILE_TYPE_PREVIEW, 0, &bitmap); 
#else
			bitmap = 0;
#endif
			GetObjectA(bitmap,sizeof(BITMAP),&bmpInfo);

			if (*iList == 0)
			{
				*iList = ImageList_Create(bmpInfo.bmWidth,
						bmpInfo.bmHeight,ILC_COLOR24, 10,10);

				SendMessageW(list, LVM_SETICONSPACING, 0,
						MAKELONG(bmpInfo.bmWidth+6, bmpInfo.bmHeight+15) ); }

			ImageList_Add(*iList, bitmap, 0);

			DeleteObject(static_bitmap);
			static_bitmap = bitmap;
			SendMessageW(GetDlgItem(progress_dialog,IDC_BITMAP),STM_SETIMAGE,
					IMAGE_BITMAP, (LPARAM)static_bitmap);
			RedrawWindow(progress_dialog,NULL,NULL,RDW_INTERNALPAINT|RDW_UPDATENOW|RDW_ALLCHILDREN);
		}
	}
	EndDialog(progress_dialog,0);
	
}

static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_INITDIALOG:
			{
				disable_dialog = FALSE;
				EnableWindow(GetDlgItem(hwnd,IDC_IMPORT),FALSE);
			}
			break;
		case WM_NOTIFY:
			if (((LPNMHDR)lParam)->code == LVN_ITEMCHANGED)
			{
				HWND list = GetDlgItem(hwnd,IDC_LIST1);
				int count = SendMessageA(list,LVM_GETSELECTEDCOUNT,0,0);
				if (count > 0)
					EnableWindow(GetDlgItem(hwnd,IDC_IMPORT),TRUE);
				else
					EnableWindow(GetDlgItem(hwnd,IDC_IMPORT),FALSE);
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_SKIP:
					on_disable_dialog_clicked(hwnd);
					break;
				case IDC_EXIT:
					UI_EndDialog(hwnd,0);
					break;
				case IDC_IMPORT:
					{
						HWND list = GetDlgItem(hwnd,IDC_LIST1);
						int count = SendMessageA(list,LVM_GETSELECTEDCOUNT,0,0);
						int i;

						if (count ==0)
						{
							UI_EndDialog(hwnd,0);
							return FALSE;
						}

						count = SendMessageA(list,LVM_GETITEMCOUNT,0,0);
						for ( i = 0; i < count; i++)
						{
							INT state = 0x00000000;

							state = SendMessageA(list,LVM_GETITEMSTATE,i,
									LVIS_SELECTED);

							if (state)
							{
								LVITEMA item;
								struct gphoto2_file *file;


								item.mask = LVIF_PARAM;
								item.iItem = i;

								item.iSubItem = 0;
								SendMessageA(list,LVM_GETITEMA,0,(LPARAM)&item);

								file = (struct gphoto2_file*)item.lParam;
								file->download = TRUE;
							}
						}

						UI_EndDialog(hwnd,1);
					}
					break;
				case IDC_IMPORTALL:
					{
						if (!GetAllImages())
						{
							UI_EndDialog(hwnd,0);
							return FALSE;
						}
						UI_EndDialog(hwnd,1);
					}
					break;
				case IDC_FETCH:
					{
						HIMAGELIST ilist = 0;
						HWND list = GetDlgItem(hwnd,IDC_LIST1);
						PopulateImageList(&ilist,list);

						SendMessageA(list, LVM_SETIMAGELIST,LVSIL_NORMAL,(LPARAM)ilist);
						PopulateListView(list);
						EnableWindow(GetDlgItem(hwnd,IDC_FETCH),FALSE);
					}
					break;
			}
			break;
	}
	return FALSE;
}

BOOL DoCameraUI(void)
{
	HKEY key;
	DWORD data = 0;
	DWORD size = sizeof(data);
	if (RegOpenKeyExA(HKEY_CURRENT_USER, settings_key, 0, KEY_READ, &key) == ERROR_SUCCESS) {
		RegQueryValueExA(key, settings_value, NULL, NULL, (LPBYTE) &data, &size);
		RegCloseKey(key);
		if (data)
			return GetAllImages();
	}
	return DialogBoxW(GPHOTO2_instance,
			(LPWSTR)MAKEINTRESOURCE(IDD_CAMERAUI),NULL, DialogProc);
}

static INT_PTR CALLBACK ProgressProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM
		lParam)
{
	    return FALSE;
}   

HWND TransferringDialogBox(HWND dialog, LONG progress)
{
	if (!dialog)
		dialog = CreateDialogW(GPHOTO2_instance,
				(LPWSTR)MAKEINTRESOURCE(IDD_DIALOG1), NULL, ProgressProc);

	if (progress == -1)
	{
		EndDialog(dialog,0);
		return NULL;
	}

	RedrawWindow(dialog,NULL,NULL,
			RDW_INTERNALPAINT|RDW_UPDATENOW|RDW_ALLCHILDREN);

	return dialog;
}
