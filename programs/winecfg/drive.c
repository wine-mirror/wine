/*
 * Drive management UI code
 *
 * Copyright 2003 Mark Westcott
 * Copyright 2003 Mike Hearn
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wine/debug.h>
#include <shellapi.h>
#include <objbase.h>
#include <shlguid.h>
#include <shlwapi.h>
#include <shlobj.h>

#include "winecfg.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

/*******************************************************************/
/*  the configuration properties affected by this code             */
/*******************************************************************/
static BOOL initialized = FALSE;

void initDriveDlg (HWND hDlg)
{
    int i;

    /* Clear the listbox */
    SendMessageA(GetDlgItem(hDlg, IDC_LIST_DRIVES), LB_RESETCONTENT, 0, 0);
    for (i = 0; i < config.driveCount; i++) {
	int itemIndex;
	DRIVE_DESC *drive = DPA_GetPtr(config.pDrives, i);
	char *title = malloc(MAX_NAME_LENGTH);
	
	WINE_TRACE("Iterating, item %d of %d, drive=%p\n", i, config.driveCount, drive);
	assert(drive);

	WINE_TRACE("Adding %s (%s) to the listbox\n", drive->szName, drive->szLabel);

	/* the first SendMessage call adds the string and returns the index, the second associates that index with it */
	snprintf(title, MAX_NAME_LENGTH, "Drive %s (%s)", drive->szName, drive->szLabel);
	itemIndex = SendMessageA(GetDlgItem(hDlg, IDC_LIST_DRIVES), LB_ADDSTRING ,(WPARAM) -1, (LPARAM) title);
	SendMessageA(GetDlgItem(hDlg, IDC_LIST_DRIVES), LB_SETITEMDATA, itemIndex, (LPARAM) itemIndex);
	free(title);
    }
    
    initialized = TRUE;
}


void saveDriveDlgSettings (HWND hDlg)
{

}



/******************************************************************************/
/*  The Drive Editing Dialog                                                  */
/******************************************************************************/
#define DRIVE_MASK_BIT(B) 1<<(toupper(B)-'A')

typedef struct{
  char *sCode;
  char *sDesc;
} code_desc_pair;

static code_desc_pair type_pairs[] = {
  {"hd", "Local hard disk"},
  {"network", "Network share" },
  {"floppy", "Floppy disk"},
  {"cdrom", "CD-ROM"}
};
#define DRIVE_TYPE_DEFAULT 1

static code_desc_pair fs_pairs[] = {
  {"win95", "Long file names"},
  {"msdos", "MS-DOS 8 character file names"},
  {"unix", "UNIX file names"}
};
#define DRIVE_FS_DEFAULT 0

long drive_available_mask(DRIVE_DESC *pCurrentDrive)
{
  int i;
  DRIVE_DESC *pDrive;
  long result = 0;
  
  for( i=0; i < config.pDrives->nItemCount; ++i) {
    pDrive = (DRIVE_DESC *) DPA_GetPtr( config.pDrives, i );
    if(pDrive) {
      result |= DRIVE_MASK_BIT(pDrive->szName[0]);
    }
  }
  
  result = ~result; /*flag unused, not used*/
  result |= DRIVE_MASK_BIT(pCurrentDrive->szName[0]);
  WINE_TRACE( "finished drive letter loop with %ld\n", result );

  return result;
}

void fill_drive_droplist(long mask, DRIVE_DESC *pCurrentDrive, HWND hDlg)
{
  int i;
  int selection;
  int count;
  int next_letter;
  char sName[4] = "A:";

  for( i=0, count=0, selection=-1, next_letter=-1; i <= 'Z'-'A'; ++i ) {
    if( mask & DRIVE_MASK_BIT('A'+i) ) {
      sName[0] = 'A' + i;
      SendDlgItemMessage( hDlg, IDC_COMBO_LETTER, CB_ADDSTRING, 0, (LPARAM) sName );
      
      if( toupper(pCurrentDrive->szName[0]) == 'A' + i ) {
	selection = count;
      }
      
      if( i >= 2 && next_letter == -1){ /*default drive is first one of C-Z */
	next_letter = count;
      }
      
      count++;
    }
  }
  
  if( selection == -1 ) {
    selection = next_letter;
  }
  
  SendDlgItemMessage( hDlg, IDC_COMBO_LETTER, CB_SETCURSEL, selection, 0 );
}


/* if bEnable is 1 then we are editing a CDROM, so can enable all controls, otherwise we want to disable
 * "detect from device" and "serial number", but still allow the user to manually set the path. The UI
 * for this could be somewhat better -mike
 */
void enable_labelserial_box(HWND hDlg, int bEnable)
{
  EnableWindow( GetDlgItem( hDlg, IDC_RADIO_AUTODETECT ), bEnable );
  EnableWindow( GetDlgItem( hDlg, IDC_EDIT_DEVICE ), bEnable );
  EnableWindow( GetDlgItem( hDlg, IDC_BUTTON_BROWSE_DEVICE ), bEnable );
  EnableWindow( GetDlgItem( hDlg, IDC_EDIT_SERIAL ), bEnable );
  EnableWindow( GetDlgItem( hDlg, IDC_STATIC_SERIAL ), bEnable );
  EnableWindow( GetDlgItem( hDlg, IDC_STATIC_LABEL ), bEnable );
}


INT_PTR CALLBACK DriveEditDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int i;
  int selection;

  switch (uMsg)  {
    case WM_INITDIALOG: {
	DRIVE_DESC* pDrive = (DRIVE_DESC *)lParam;
	
	SetProp(hDlg, "PDRIVE", pDrive);

	/* Fill in the list boxes */
	/* Drive letters */
	fill_drive_droplist( drive_available_mask( pDrive ), pDrive, hDlg );

	/* drive type*/
	for( i=0, selection=-1; i < sizeof(type_pairs)/sizeof(code_desc_pair); i++) {
	  SendDlgItemMessage(hDlg, IDC_COMBO_TYPE, CB_ADDSTRING, 0,
			     (LPARAM) type_pairs[i].sDesc);
	  if(strcasecmp(type_pairs[i].sCode, pDrive->szType) == 0){
	    selection = i;
	  }
	}
	
	if( selection == -1 ) {
	  selection = DRIVE_TYPE_DEFAULT;
	}
	SendDlgItemMessage(hDlg, IDC_COMBO_TYPE, CB_SETCURSEL, selection, 0);

	/* FileSystem name handling */
	for( i=0, selection=-1; i < sizeof(fs_pairs)/sizeof(code_desc_pair); i++) {
	  SendDlgItemMessage(hDlg, IDC_COMBO_NAMES, CB_ADDSTRING, 0,
			     (LPARAM) fs_pairs[i].sDesc);
	  if(strcasecmp(fs_pairs[i].sCode, pDrive->szFS) == 0){
	    selection = i;
	  }
	}
	
	if( selection == -1 ) {
	  selection = DRIVE_FS_DEFAULT;
	}
	SendDlgItemMessage(hDlg, IDC_COMBO_NAMES, CB_SETCURSEL, selection, 0);

       
	/* removeable media properties */
	SendDlgItemMessage(hDlg, IDC_EDIT_SERIAL, WM_SETTEXT, 0,(LPARAM)pDrive->szSerial);
	SendDlgItemMessage(hDlg, IDC_EDIT_LABEL, WM_SETTEXT, 0,(LPARAM)pDrive->szLabel);
	SendDlgItemMessage(hDlg, IDC_EDIT_DEVICE, WM_SETTEXT, 0,(LPARAM)pDrive->szDevice);
	if( strcmp("cdrom", pDrive->szType) == 0 ||
	    strcmp("floppy", pDrive->szType) == 0) {
	  if( (strlen( pDrive->szDevice ) == 0) &&
	      ((strlen( pDrive->szSerial ) > 0) || (strlen( pDrive->szLabel ) > 0)) ) {
	    selection = IDC_RADIO_ASSIGN;
	  }
	  else {
	    selection = IDC_RADIO_AUTODETECT;
	  }
	
	  enable_labelserial_box( hDlg, 1 );
	}
	else {
	  enable_labelserial_box( hDlg, 0 );
	  selection = IDC_RADIO_ASSIGN;
	}
	
	CheckRadioButton( hDlg, IDC_RADIO_AUTODETECT, IDC_RADIO_ASSIGN, selection );
	SendDlgItemMessage(hDlg, IDC_EDIT_PATH, WM_SETTEXT, 0,(LPARAM)pDrive->szPath);
	break;
      }

    case WM_COMMAND:
      switch (LOWORD(wParam))
	{
	case IDC_COMBO_TYPE:
	  switch( HIWORD(wParam)) 
	    {
	    case CBN_SELCHANGE:
	      selection = SendDlgItemMessage( hDlg, IDC_COMBO_TYPE, CB_GETCURSEL, 0, 0);
	      if( selection == 2 || selection == 3 ) { /* cdrom or floppy */
		enable_labelserial_box( hDlg, 1 );
	      }
	      else {
		enable_labelserial_box( hDlg, 0 );
	      }
	      break;
	    }
	  break;

	case ID_BUTTON_OK:
	  {
	    DRIVE_DESC *pDrive = GetProp(hDlg, "PDRIVE");
	    char buffer[MAX_NAME_LENGTH];
	    
	    /* fixme: do it in a cleanup */
	    RemoveProp(hDlg, "PDRIVE");

	    ZeroMemory(&buffer[0], MAX_NAME_LENGTH);
	    GetWindowText(GetDlgItem(hDlg, IDC_EDIT_PATH), buffer, MAX_NAME_LENGTH);
	    if (strlen(buffer) > 0) {
	      strncpy(pDrive->szPath, buffer, MAX_NAME_LENGTH);
	      
	      /* only fill in the other values if we have a path */
	      selection = SendDlgItemMessage( hDlg, IDC_COMBO_TYPE, CB_GETCURSEL, 0, 0);
	      strncpy(pDrive->szType, type_pairs[selection].sCode, MAX_NAME_LENGTH);
	      
	      selection = SendDlgItemMessage( hDlg, IDC_COMBO_NAMES, CB_GETCURSEL, 0, 0);
	      strncpy(pDrive->szFS, fs_pairs[selection].sCode, MAX_NAME_LENGTH);
	      
	      selection = SendDlgItemMessage( hDlg, IDC_COMBO_LETTER, CB_GETCURSEL, 0, 0);
	      SendDlgItemMessage( hDlg, IDC_COMBO_LETTER, CB_GETLBTEXT, selection, (WPARAM)buffer);
	      strncpy(pDrive->szName, buffer, MAX_NAME_LENGTH);
	      pDrive->szName[1] = 0; /* truncate to only the letter */

	      if( strncmp( pDrive->szType, "cdrom", MAX_NAME_LENGTH ) == 0) {
		if( IsDlgButtonChecked( hDlg, IDC_RADIO_ASSIGN ) == BST_CHECKED ) {
		  GetWindowText(GetDlgItem(hDlg, IDC_EDIT_LABEL), buffer, 
				MAX_NAME_LENGTH);
		  strncpy(pDrive->szLabel, buffer, MAX_NAME_LENGTH);
		  GetWindowText(GetDlgItem(hDlg, IDC_EDIT_SERIAL), buffer, 
				MAX_NAME_LENGTH);
		  strncpy(pDrive->szSerial, buffer, MAX_NAME_LENGTH);
		  pDrive->szDevice[0] = 0;
		}
		else {
		  GetWindowText(GetDlgItem(hDlg, IDC_EDIT_DEVICE), buffer, 
				MAX_NAME_LENGTH);
		  strncpy(pDrive->szDevice, buffer, MAX_NAME_LENGTH);
		  pDrive->szLabel[0] = 0;
		  pDrive->szSerial[0] = 0;
		}
	      }
	      else {
		GetWindowText(GetDlgItem(hDlg, IDC_EDIT_LABEL), pDrive->szLabel, MAX_NAME_LENGTH);
		GetWindowText(GetDlgItem(hDlg, IDC_EDIT_SERIAL), pDrive->szSerial, MAX_NAME_LENGTH);
		GetWindowText(GetDlgItem(hDlg, IDC_EDIT_DEVICE), pDrive->szDevice, MAX_NAME_LENGTH);
	      }
	      EndDialog(hDlg, wParam);
	    }
	    else { /* missing a path */
	      MessageBox( hDlg, "Please enter a valid path", "ERROR", MB_OK );
	    }
	    return TRUE;
	    
	  }

	  
	  /* Fall through. */
	  
	case ID_BUTTON_CANCEL:
	  /* fixme: do it in a cleanup */
	  RemoveProp(hDlg, "PDRIVE");
	  EndDialog(hDlg, wParam);
	  return TRUE;
	}
    }
  return FALSE;
}


/***************************************************************************/
/*  The Actual Property Sheet Page                                         */
/***************************************************************************/

INT_PTR CALLBACK
DriveDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int selection = -1;

  switch (uMsg)
    {
    case WM_INITDIALOG:
      break;
      
    case WM_COMMAND:
      switch (LOWORD(wParam))
	{
	case IDC_LIST_DRIVES:
	  switch( HIWORD( wParam ) )
	    {
	    case LBN_DBLCLK:
	      selection = -1;
	      break;
	    }
	case IDC_BUTTON_ADD:
	  if (HIWORD(wParam)==BN_CLICKED) {
	    DRIVE_DESC* pDrive = malloc(sizeof(DRIVE_DESC));
	    
	    /* adding a new drive */
	    ZeroMemory(pDrive, sizeof(*pDrive));
	    DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_DRIVE_EDIT2),
			   NULL, (DLGPROC) DriveEditDlgProc, (LPARAM) pDrive );
	    /* if pDrive is blank, free the memory, otherwise, add the pointer to HDPA and add to listbox */
	    if (!(pDrive->szName[0]))
	      free(pDrive);
	    else {
	      char DRIVE_NAME[7];
	      int pos;
	      
	      ZeroMemory(DRIVE_NAME,sizeof(DRIVE_NAME));
	      strcat(DRIVE_NAME, "Drive \0");
	      strncat(DRIVE_NAME, pDrive->szName, 1);
	      pos = SendMessageA(GetDlgItem(hDlg, IDC_LIST_DRIVES), LB_ADDSTRING ,(WPARAM) -1, (LPARAM) DRIVE_NAME);
	      SendMessageA(GetDlgItem(hDlg, IDC_LIST_DRIVES), LB_SETITEMDATA, pos, (LPARAM) pos);
	      DPA_InsertPtr(config.pDrives, pos, pDrive);
	    }
	  }
	  break;
	case IDC_BUTTON_EDIT:
	  if (HIWORD(wParam)==BN_CLICKED) {
	    int nItem = SendMessage(GetDlgItem(hDlg, IDC_LIST_DRIVES),  LB_GETCURSEL, 0, 0);
	    int i = SendMessage(GetDlgItem(hDlg, IDC_LIST_DRIVES), LB_GETITEMDATA, nItem, 0);
	    DRIVE_DESC* pDrive = DPA_GetPtr(config.pDrives, i);
	    
	    if (pDrive != NULL) {
	      DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_DRIVE_EDIT2),
			     NULL, (DLGPROC) DriveEditDlgProc, (LPARAM) pDrive );
	      SetProp(hDlg, "PDRIVE", pDrive);
	      WINE_TRACE("refreshing main dialog\n");
	      initDriveDlg(hDlg);
	    }
	  }
	  break;
	case IDC_BUTTON_REMOVE:
	  if (HIWORD(wParam)==BN_CLICKED) {
	    int nItem = SendMessage(GetDlgItem(hDlg, IDC_LIST_DRIVES), LB_GETCURSEL, 0, 0);
	    SendMessage(GetDlgItem(hDlg, IDC_LIST_DRIVES), LB_DELETESTRING, (WPARAM) nItem, (LPARAM) 0 );
	    free(DPA_GetPtr(config.pDrives, nItem));
	  }

	default:
	  /** SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0); */
	  break;
	}
      break;

    case WM_NOTIFY:
      switch(((LPNMHDR)lParam)->code)
	{
	case PSN_KILLACTIVE: {
	  /* validate user info.  Lets just assume everything is okay for now */
	  SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
	}
	case PSN_APPLY: {
	  /* should probably check everything is really all rosy :) */
	  saveDriveDlgSettings (hDlg);
	  SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
	}
	case PSN_SETACTIVE: {
	  if (!initialized)
	    initDriveDlg (hDlg);

	}
	}

    default:
      break;
    }
  return FALSE;
}
