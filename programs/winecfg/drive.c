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

static BOOL updatingUI = FALSE;
static char editWindowLetter; /* drive letter of the drive we are currently editing */
static int lastSel = 0; /* the last drive selected in the property sheet */


/* returns NULL on failure. caller is responsible for freeing result */
char *getDriveValue(char letter, const char *valueName) {
  HKEY hkDrive = 0;
  char *subKeyName;
  char *result = NULL;
  HRESULT hr;
  DWORD bufferSize;
  
  WINE_TRACE("letter=%c, valueName=%s\n", letter, valueName);

  subKeyName = malloc(strlen("Drive X")+1);
  sprintf(subKeyName, "Drive %c", letter);

  hr = RegOpenKeyEx(configKey, subKeyName, 0, KEY_READ, &hkDrive);
  if (hr != ERROR_SUCCESS) goto end;

  hr = RegQueryValueEx(hkDrive, valueName, NULL, NULL, NULL, &bufferSize);
  if (hr != ERROR_SUCCESS) goto end;

  result = malloc(bufferSize);
  hr = RegQueryValueEx(hkDrive, valueName, NULL, NULL, result, &bufferSize);
  if (hr != ERROR_SUCCESS) goto end;
  
end:
  if (hkDrive) RegCloseKey(hkDrive);
  free(subKeyName);
  return result;
}

/* call with newValue == NULL to remove a value */
void setDriveValue(char letter, const char *valueName, const char *newValue) {
  char *driveSection = malloc(strlen("Drive X")+1);
  sprintf(driveSection, "Drive %c", letter);
  if (newValue)
    addTransaction(driveSection, valueName, ACTION_SET, newValue);
  else
    addTransaction(driveSection, valueName, ACTION_REMOVE, NULL);
  free(driveSection);
}

/* copies a drive configuration branch */
void copyDrive(char srcLetter, char destLetter) {
  char driveSection[sizeof("Drive X")];
  char *path, *label, *type, *serial, *fs;
  
  WINE_TRACE("srcLetter=%c, destLetter=%c\n", srcLetter, destLetter);

  sprintf(driveSection, "Drive %c", srcLetter);
  path = getDriveValue(srcLetter, "Path");
  label = getDriveValue(srcLetter, "Label");
  type = getDriveValue(srcLetter, "Type");
  serial = getDriveValue(srcLetter, "Serial");
  fs = getDriveValue(srcLetter, "FileSystem");
  
  sprintf(driveSection, "Drive %c", destLetter);
  if (path)   addTransaction(driveSection, "Path", ACTION_SET, path);
  if (label)  addTransaction(driveSection, "Label", ACTION_SET, label);
  if (type)   addTransaction(driveSection, "Type", ACTION_SET, type);
  if (serial) addTransaction(driveSection, "Serial", ACTION_SET, serial);
  if (fs)     addTransaction(driveSection, "FileSystem", ACTION_SET, fs);

  if (path)   free(path);
  if (label)  free(label);
  if (type)   free(type);
  if (serial) free(serial);
  if (fs)     free(fs);
}

void removeDrive(char letter) {
  char driveSection[sizeof("Drive X")];
  sprintf(driveSection, "Drive %c", letter);
  addTransaction(driveSection, NULL, ACTION_REMOVE, NULL);
}

int refreshDriveDlg (HWND dialog)
{
  int i;
  char *subKeyName = malloc(MAX_NAME_LENGTH);
  int driveCount = 0;
  DWORD sizeOfSubKeyName = MAX_NAME_LENGTH;
  int doesDriveCExist = FALSE;

  WINE_TRACE("\n");

  updatingUI = TRUE;

  /* Clear the listbox */
  SendMessageA(GetDlgItem(dialog, IDC_LIST_DRIVES), LB_RESETCONTENT, 0, 0);
  for (i = 0;
       RegEnumKeyExA(configKey, i, subKeyName, &sizeOfSubKeyName, NULL, NULL, NULL, NULL ) != ERROR_NO_MORE_ITEMS;
       ++i, sizeOfSubKeyName = MAX_NAME_LENGTH) {
    
    HKEY hkDrive;
    char returnBuffer[MAX_NAME_LENGTH];
    DWORD sizeOfReturnBuffer = sizeof(returnBuffer);
    LONG r;
    WINE_TRACE("%s\n", subKeyName);
    
    if (!strncmp("Drive ", subKeyName, 5)) {
      char driveLetter = '\0';
      char *label;
      char *title;
      char *device;
      int titleLen;
      const char *itemLabel = "Drive %s (%s)";
      int itemIndex;
      
      if (RegOpenKeyExA (configKey, subKeyName, 0, KEY_READ, &hkDrive) != ERROR_SUCCESS)        {
        WINE_ERR("unable to open drive registry key");
        RegCloseKey(configKey);
        return -1;
      }
      
      /* extract the drive letter, force to upper case */
      driveLetter = subKeyName[strlen(subKeyName)-1];
      if (driveLetter) driveLetter = toupper(driveLetter);
      if (driveLetter == 'C') doesDriveCExist = TRUE;
      
      ZeroMemory(returnBuffer, sizeof(*returnBuffer));
      sizeOfReturnBuffer = sizeof(returnBuffer);
      r = RegQueryValueExA(hkDrive, "Label", NULL, NULL, returnBuffer, &sizeOfReturnBuffer);
      if (r == ERROR_SUCCESS) {
        label = malloc(sizeOfReturnBuffer);
        strncpy(label, returnBuffer, sizeOfReturnBuffer);
      } else {
        WINE_WARN("label not loaded: %ld\n", r);
        label = NULL;
      }

      device = getDriveValue(driveLetter, "Device");
      
      /* We now know the label and drive letter, so we can add to the list. The list items will have the letter associated
       * with them, which acts as the key. We can then use that letter to get/set the properties of the drive. */
      WINE_TRACE("Adding %c: label=%s to the listbox, device=%s\n", driveLetter, label, device);

      /* fixup label */
      if (!label && device) {
	label = malloc(strlen("[label read from device ]")+1+strlen(device));
	sprintf(label, "[label read from device %s]", device);
      }
      if (!label) label = strdup("(no label)");
      
      titleLen = strlen(itemLabel) - 1 + strlen(label) - 2 + 1;
      title = malloc(titleLen);
      /* the %s in the item label will be replaced by the drive letter, so -1, then
	 -2 for the second %s which will be expanded to the label, finally + 1 for terminating #0 */
      snprintf(title, titleLen, "Drive %c: %s", driveLetter, label);
      
      /* the first SendMessage call adds the string and returns the index, the second associates that index with it */
      itemIndex = SendMessageA(GetDlgItem(dialog, IDC_LIST_DRIVES), LB_ADDSTRING ,(WPARAM) -1, (LPARAM) title);
      SendMessageA(GetDlgItem(dialog, IDC_LIST_DRIVES), LB_SETITEMDATA, itemIndex, (LPARAM) driveLetter);
      
      free(title);
      free(label);

      driveCount++;
	
    }
  }
  
  WINE_TRACE("loaded %d drives\n", driveCount);
  SendDlgItemMessage(dialog, IDC_LIST_DRIVES, LB_SETSEL, TRUE, lastSel);

  /* show the warning if there is no Drive C */
  if (!doesDriveCExist)
    ShowWindow(GetDlgItem(dialog, IDS_DRIVE_NO_C), SW_NORMAL);
  else
    ShowWindow(GetDlgItem(dialog, IDS_DRIVE_NO_C), SW_HIDE);
  
  free(subKeyName);

  /* disable or enable controls depending on whether we are editing global vs app specific config */
  if (appSettings == EDITING_GLOBAL) {
    WINE_TRACE("enabling controls\n");
    enable(IDC_LIST_DRIVES);
    enable(IDC_BUTTON_ADD);
    enable(IDC_BUTTON_REMOVE);
    enable(IDC_BUTTON_EDIT);
    enable(IDC_BUTTON_AUTODETECT);
    
  } else {
    WINE_TRACE("disabling controls\n");
    disable(IDC_LIST_DRIVES);
    disable(IDC_BUTTON_ADD);
    disable(IDC_BUTTON_REMOVE);
    disable(IDC_BUTTON_EDIT);
    disable(IDC_BUTTON_AUTODETECT);    
  }

  
  updatingUI = FALSE;
  return driveCount;
}

/******************************************************************************/
/*  The Drive Editing Dialog                                                  */
/******************************************************************************/
#define DRIVE_MASK_BIT(B) 1<<(toupper(B)-'A')

typedef struct {
  const char *sCode;
  const char *sDesc;
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


void fill_drive_droplist(long mask, char currentLetter, HWND hDlg)
{
  int i;
  int selection;
  int count;
  int next_letter;
  char sName[4] = "A:";

  for( i=0, count=0, selection=-1, next_letter=-1; i <= 'Z'-'A'; ++i ) {
    if( mask & DRIVE_MASK_BIT('A'+i) ) {
      int index;
      
      sName[0] = 'A' + i;
      index = SendDlgItemMessage( hDlg, IDC_COMBO_LETTER, CB_ADDSTRING, 0, (LPARAM) sName );
			  
      if( toupper(currentLetter) == 'A' + i ) {
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

#define BOX_MODE_CD_ASSIGN 1
#define BOX_MODE_CD_AUTODETECT 2
#define BOX_MODE_NONE 3
#define BOX_MODE_NORMAL 4
void enable_labelserial_box(HWND dialog, int mode)
{
  WINE_TRACE("mode=%d\n", mode);
  switch (mode) {
      case BOX_MODE_CD_ASSIGN:
	enable(IDC_RADIO_AUTODETECT);
	enable(IDC_RADIO_ASSIGN);
	disable(IDC_EDIT_DEVICE);
	disable(IDC_BUTTON_BROWSE_DEVICE);
	enable(IDC_EDIT_SERIAL);
	enable(IDC_EDIT_LABEL);
	enable(IDC_STATIC_SERIAL);
	enable(IDC_STATIC_LABEL);
	break;
	
      case BOX_MODE_CD_AUTODETECT:
	enable(IDC_RADIO_AUTODETECT);
	enable(IDC_RADIO_ASSIGN);
	enable(IDC_EDIT_DEVICE);
	enable(IDC_BUTTON_BROWSE_DEVICE);
	disable(IDC_EDIT_SERIAL);
	disable(IDC_EDIT_LABEL);
	disable(IDC_STATIC_SERIAL);
	disable(IDC_STATIC_LABEL);
	break;

      case BOX_MODE_NONE:
	disable(IDC_RADIO_AUTODETECT);
	disable(IDC_RADIO_ASSIGN);
	disable(IDC_EDIT_DEVICE);
	disable(IDC_BUTTON_BROWSE_DEVICE);
	disable(IDC_EDIT_SERIAL);
	disable(IDC_EDIT_LABEL);
	disable(IDC_STATIC_SERIAL);
	disable(IDC_STATIC_LABEL);
	break;

      case BOX_MODE_NORMAL:
	disable(IDC_RADIO_AUTODETECT);
	enable(IDC_RADIO_ASSIGN);
	disable(IDC_EDIT_DEVICE);
	disable(IDC_BUTTON_BROWSE_DEVICE);
	enable(IDC_EDIT_SERIAL);
	enable(IDC_EDIT_LABEL);
	enable(IDC_STATIC_SERIAL);
	enable(IDC_STATIC_LABEL);
	break;	
  }
}

/* This function produces a mask for each drive letter that isn't currently used. Each bit of the long result
 * represents a letter, with A being the least significant bit, and Z being the most significant.
 *
 * To calculate this, we loop over each letter, and see if we can get a drive entry for it. If so, we
 * set the appropriate bit. At the end, we flip each bit, to give the desired result.
 *
 * The letter parameter is always marked as being available. This is so the edit dialog can display the
 * currently used drive letter alongside the available ones.
 */
long drive_available_mask(char letter)
{
  long result = 0;
  char curLetter;
  char *slop;
  
  WINE_TRACE("\n");
 
  for (curLetter = 'A'; curLetter < 'Z'; curLetter++) {
    slop = getDriveValue(curLetter, "Path");
    if (slop != NULL) {
      result |= DRIVE_MASK_BIT(curLetter);
      free(slop);
    }
  }
  
  result = ~result;
  if (letter) result |= DRIVE_MASK_BIT(letter);
  
  WINE_TRACE( "finished drive letter loop with %lx\n", result );
  return result;
}


void refreshDriveEditDialog(HWND dialog) {
  char *path;
  char *type;
  char *fs;
  char *serial;
  char *label;
  char *device;
  int i, selection;

  updatingUI = TRUE;
  
  /* Drive letters */
  fill_drive_droplist( drive_available_mask( editWindowLetter ), editWindowLetter, dialog );

  /* path */
  path = getDriveValue(editWindowLetter, "Path");
  if (path) {
    SetWindowText(GetDlgItem(dialog, IDC_EDIT_PATH), path);
  } else WINE_WARN("no Path field?\n");
  
  /* drive type */
  type = getDriveValue(editWindowLetter, "Type");
  if (type) {
    for(i = 0, selection = -1; i < sizeof(type_pairs)/sizeof(code_desc_pair); i++) {
      SendDlgItemMessage(dialog, IDC_COMBO_TYPE, CB_ADDSTRING, 0,
			 (LPARAM) type_pairs[i].sDesc);
      if(strcasecmp(type_pairs[i].sCode, type) == 0){
	selection = i;
      }
    }
  
    if( selection == -1 ) selection = DRIVE_TYPE_DEFAULT;
    SendDlgItemMessage(dialog, IDC_COMBO_TYPE, CB_SETCURSEL, selection, 0);
  } else WINE_WARN("no Type field?\n");

  
  /* FileSystem name handling */
  fs = getDriveValue(editWindowLetter, "FileSystem");
  if (fs) {
    for( i=0, selection=-1; i < sizeof(fs_pairs)/sizeof(code_desc_pair); i++) {
      SendDlgItemMessage(dialog, IDC_COMBO_NAMES, CB_ADDSTRING, 0,
			 (LPARAM) fs_pairs[i].sDesc);
      if(strcasecmp(fs_pairs[i].sCode, fs) == 0){
	selection = i;
      }
    }
  
    if( selection == -1 ) selection = DRIVE_FS_DEFAULT;
    SendDlgItemMessage(dialog, IDC_COMBO_NAMES, CB_SETCURSEL, selection, 0);
  } else WINE_WARN("no FileSystem field?\n");


  /* removeable media properties */
  serial = getDriveValue(editWindowLetter, "Serial");
  if (serial) {
    SendDlgItemMessage(dialog, IDC_EDIT_SERIAL, WM_SETTEXT, 0,(LPARAM)serial);
  } else WINE_WARN("no Serial field?\n");

  label = getDriveValue(editWindowLetter, "Label");
  if (label) {
    SendDlgItemMessage(dialog, IDC_EDIT_LABEL, WM_SETTEXT, 0,(LPARAM)label);
  } else WINE_WARN("no Label field?\n");

  device = getDriveValue(editWindowLetter, "Device");
  if (device) {
    SendDlgItemMessage(dialog, IDC_EDIT_DEVICE, WM_SETTEXT, 0,(LPARAM)device);
  } else WINE_WARN("no Device field?\n");

  selection = IDC_RADIO_ASSIGN;
  if ((type && strcmp("cdrom", type) == 0) || (type && strcmp("floppy", type) == 0)) {
    if (device) {
      selection = IDC_RADIO_AUTODETECT;
      enable_labelserial_box(dialog, BOX_MODE_CD_AUTODETECT);
    } else {
      selection = IDC_RADIO_ASSIGN;
      enable_labelserial_box(dialog, BOX_MODE_CD_ASSIGN);
    }
  } else {
    enable_labelserial_box(dialog, BOX_MODE_NORMAL);
    selection = IDC_RADIO_ASSIGN;
  }

  CheckRadioButton( dialog, IDC_RADIO_AUTODETECT, IDC_RADIO_ASSIGN, selection );
  if (path) SendDlgItemMessage(dialog, IDC_EDIT_PATH, WM_SETTEXT, 0,(LPARAM)path);
  
  if (path) free(path);
  if (type) free(type);
  if (fs) free(fs);
  if (serial) free(serial);
  if (label) free(label);
  if (device) free(device);

  
  updatingUI = FALSE;
  
  return;
}

/* storing the drive propsheet HWND here is a bit ugly, but the simplest solution for now */
static HWND driveDlgHandle;

void onEditChanged(HWND hDlg, WORD controlID) {
  WINE_TRACE("controlID=%d\n", controlID);
  switch (controlID) {
      case IDC_EDIT_LABEL: {
        char *label = getDialogItemText(hDlg, controlID);
        setDriveValue(editWindowLetter, "Label", label);
        refreshDriveDlg(driveDlgHandle);
        if (label) free(label);
        break;
      }
      case IDC_EDIT_PATH: {
	char *path = getDialogItemText(hDlg, controlID);
	if (!path) path = strdup("fake_windows"); /* default to assuming fake_windows in the .wine directory */
	setDriveValue(editWindowLetter, "Path", path);
	free(path);
	break;
      }
      case IDC_EDIT_SERIAL: {
	char *serial = getDialogItemText(hDlg, controlID);
	setDriveValue(editWindowLetter, "Serial", serial);
	if (serial) free (serial);
	break;
      }
      case IDC_EDIT_DEVICE: {
	char *device = getDialogItemText(hDlg,controlID);
	setDriveValue(editWindowLetter, "Device", device);
	if (device) free(device);
	refreshDriveDlg(driveDlgHandle);
	break;
      }
  }
}

INT_PTR CALLBACK DriveEditDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int selection;

  switch (uMsg)  {
      case WM_INITDIALOG: {
	editWindowLetter = (char) lParam;
	refreshDriveEditDialog(hDlg);
      }

    case WM_COMMAND:
      switch (LOWORD(wParam)) {
	  case IDC_COMBO_TYPE:
	    if (HIWORD(wParam) != CBN_SELCHANGE) break;
	    selection = SendDlgItemMessage( hDlg, IDC_COMBO_TYPE, CB_GETCURSEL, 0, 0);
	    if( selection == 2 || selection == 3 ) { /* cdrom or floppy */
	      if (IsDlgButtonChecked(hDlg, IDC_RADIO_AUTODETECT))
		enable_labelserial_box(hDlg, BOX_MODE_CD_AUTODETECT);
	      else
		enable_labelserial_box(hDlg, BOX_MODE_CD_ASSIGN);
	    }
	    else {
	      enable_labelserial_box( hDlg, BOX_MODE_NORMAL );
	    }
	    setDriveValue(editWindowLetter, "Type", type_pairs[selection].sCode);
	    break;

	  case IDC_COMBO_LETTER: {
	    int item = SendDlgItemMessage(hDlg, IDC_COMBO_LETTER, CB_GETCURSEL, 0, 0);
	    char newLetter;
	    SendDlgItemMessage(hDlg, IDC_COMBO_LETTER, CB_GETLBTEXT, item, (LPARAM) &newLetter);
	    
	    if (HIWORD(wParam) != CBN_SELCHANGE) break;
	    if (newLetter == editWindowLetter) break;
	    	    
	    WINE_TRACE("changing drive letter to %c\n", newLetter);
	    copyDrive(editWindowLetter, newLetter);
	    removeDrive(editWindowLetter);
	    editWindowLetter = newLetter;
	    refreshDriveDlg(driveDlgHandle);
	    break;
	  }

	  case IDC_BUTTON_BROWSE_PATH:
	    WRITEME(hDlg);
	    break;

	  case IDC_RADIO_AUTODETECT: {
	    setDriveValue(editWindowLetter, "Label", NULL);
	    setDriveValue(editWindowLetter, "Serial", NULL);
	    setDriveValue(editWindowLetter, "Device", getDialogItemText(hDlg, IDC_EDIT_DEVICE));
	    enable_labelserial_box(hDlg, BOX_MODE_CD_AUTODETECT);
	    refreshDriveDlg(driveDlgHandle);
	    break;
	  }
	    
	  case IDC_RADIO_ASSIGN:
	    setDriveValue(editWindowLetter, "Device", NULL);
	    setDriveValue(editWindowLetter, "Label", getDialogItemText(hDlg, IDC_EDIT_LABEL));
	    setDriveValue(editWindowLetter, "Serial", getDialogItemText(hDlg, IDC_EDIT_SERIAL));
	    enable_labelserial_box(hDlg, BOX_MODE_CD_ASSIGN);
	    refreshDriveDlg(driveDlgHandle);
	    break;
	    
	  case ID_BUTTON_OK:	  
	    EndDialog(hDlg, wParam);
	    return TRUE;
      }
      if (HIWORD(wParam) == EN_CHANGE) onEditChanged(hDlg, LOWORD(wParam));
      break;
  }
  return FALSE;
}

void onAddDriveClicked(HWND hDlg) {
  /* we should allocate a drive letter automatically. We also need some way to let the user choose the mapping point,
     for now we will just force them to enter a path automatically, with / being the default. In future we should
     be able to temporarily map / then invoke the directory chooser dialog. */
  
  char newLetter = 'C'; /* we skip A and B, they are historically floppy drives */
  long mask = ~drive_available_mask(0); /* the mask is now which drives aren't available */
  char *sectionName;
  
  while (mask & (1 << (newLetter - 'A'))) {
    newLetter++;
    if (newLetter > 'Z') {
      MessageBox(NULL, "You cannot add any more drives.\n\nEach drive must have a letter, from A to Z, so you cannot have more than 26", "", MB_OK | MB_ICONEXCLAMATION);
      return;
    }
  }
  WINE_TRACE("allocating drive letter %c\n", newLetter);
  
  sectionName = malloc(strlen("Drive X") + 1);
  sprintf(sectionName, "Drive %c", newLetter);
  if (newLetter == 'C') {
    addTransaction(sectionName, "Path", ACTION_SET, "fake_windows");
    addTransaction(sectionName, "Label", ACTION_SET, "System Drive");
  } else
    addTransaction(sectionName, "Path", ACTION_SET, "/"); /* default to root path */
  addTransaction(sectionName, "Type", ACTION_SET, "hd");
  processTransQueue(); /* make sure the drive has been added, even if we are not in instant apply mode */
  free(sectionName);

  refreshDriveDlg(driveDlgHandle);

  DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_DRIVE_EDIT), NULL, (DLGPROC) DriveEditDlgProc, (LPARAM) newLetter);
}


INT_PTR CALLBACK
DriveDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int nItem;
  char letter;
  
  switch (uMsg) {
      case WM_COMMAND:
	switch (LOWORD(wParam)) {
	    case IDC_LIST_DRIVES:
	      /* double click should open the edit window for the chosen drive */
	      if (HIWORD(wParam) == LBN_DBLCLK)
	        SendMessageA(hDlg, WM_COMMAND, IDC_BUTTON_EDIT, 0);

	      if (HIWORD(wParam) == LBN_SELCHANGE) lastSel = SendDlgItemMessage(hDlg, IDC_LIST_DRIVES, LB_GETCURSEL, 0, 0);
	      break;

	    case IDC_BUTTON_ADD:
	      onAddDriveClicked(hDlg);
	      break;

	    case IDC_BUTTON_REMOVE:
	      if (HIWORD(wParam) != BN_CLICKED) break;
	      nItem = SendDlgItemMessage(hDlg, IDC_LIST_DRIVES,  LB_GETCURSEL, 0, 0);
	      letter = SendDlgItemMessage(hDlg, IDC_LIST_DRIVES, LB_GETITEMDATA, nItem, 0);
	      removeDrive(letter);
	      refreshDriveDlg(driveDlgHandle);
	      break;
	      
	    case IDC_BUTTON_EDIT:
	      if (HIWORD(wParam) != BN_CLICKED) break;
	      nItem = SendMessage(GetDlgItem(hDlg, IDC_LIST_DRIVES),  LB_GETCURSEL, 0, 0);
	      letter = SendMessage(GetDlgItem(hDlg, IDC_LIST_DRIVES), LB_GETITEMDATA, nItem, 0);
	      DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_DRIVE_EDIT), NULL, (DLGPROC) DriveEditDlgProc, (LPARAM) letter);
	      break;

	    case IDC_BUTTON_AUTODETECT:
	      WRITEME(hDlg);
	      break;
	}
	break;
	
      case WM_NOTIFY: switch(((LPNMHDR)lParam)->code) {
	    case PSN_KILLACTIVE:
	      SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
	      break;
	    case PSN_APPLY:
	      SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
	      break;
	    case PSN_SETACTIVE:
	      driveDlgHandle = hDlg;
	      refreshDriveDlg (driveDlgHandle);
	      break;
	}
	break;
  }

  return FALSE;
}
