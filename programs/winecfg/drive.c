/*
 * Drive management UI code
 *
 * Copyright 2003 Mark Westcott
 * Copyright 2003 Mike Hearn
 * Copyright 2004 Chris Morgan
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

/* TODO: */
/* - Support for devices(not sure what this means) */
/* - Various autodetections */

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

typedef struct drive_entry_s
{
    char letter;
    char *unixpath;
    char *label;
    char *serial;
    uint type;

    BOOL in_use;
} drive_entry_t;

static BOOL updatingUI = FALSE;
static drive_entry_t* editDriveEntry;
static int lastSel = 0; /* the last drive selected in the property sheet */
drive_entry_t drives[26]; /* one for each drive letter */

int getDrive(char letter)
{
    return (toupper(letter) - 'A');
}

BOOL addDrive(char letter, char *targetpath, char *label, char *serial, uint type)
{
    int driveIndex = getDrive(letter);

    if(drives[driveIndex].in_use)
        return FALSE;

    WINE_TRACE("letter == '%c', unixpath == '%s', label == '%s', serial == '%s', type == %d\n",
               letter, targetpath, label, serial, type);

    drives[driveIndex].letter   = toupper(letter);
    drives[driveIndex].unixpath = strdup(targetpath);
    drives[driveIndex].label    = strdup(label);
    drives[driveIndex].serial   = strdup(serial);
    drives[driveIndex].type     = type;
    drives[driveIndex].in_use   = TRUE;

    return TRUE;
}

/* frees up the memory associated with a drive and returns the */
/* pNext of the given drive entry */
void freeDrive(drive_entry_t *pDrive)
{
    free(pDrive->unixpath);
    free(pDrive->label);
    free(pDrive->serial);

    pDrive->in_use = FALSE;
}

void setDriveLabel(drive_entry_t *pDrive, char *label)
{
    WINE_TRACE("pDrive->letter '%c', label = '%s'\n", pDrive->letter, label);
    free(pDrive->label);
    pDrive->label = strdup(label);
}

void setDriveSerial(drive_entry_t *pDrive, char *serial)
{
    WINE_TRACE("pDrive->letter '%c', serial = '%s'\n", pDrive->letter, serial);
    free(pDrive->serial);
    pDrive->serial = strdup(serial);
}

void setDrivePath(drive_entry_t *pDrive, char *path)
{
    WINE_TRACE("pDrive->letter '%c', path = '%s'\n", pDrive->letter, path);
    free(pDrive->unixpath);
    pDrive->unixpath = strdup(path);
}

BOOL copyDrive(drive_entry_t *pSrc, drive_entry_t *pDst)
{
    if(pDst->in_use)
    {
        WINE_TRACE("pDst already in use\n");
        return FALSE;
    }

    if(!pSrc->unixpath) WINE_TRACE("!pSrc->unixpath\n");
    if(!pSrc->label) WINE_TRACE("!pSrc->label\n");
    if(!pSrc->serial) WINE_TRACE("!pSrc->serial\n");

    pDst->unixpath = strdup(pSrc->unixpath);
    pDst->label = strdup(pSrc->label);
    pDst->serial = strdup(pSrc->serial);
    pDst->type = pSrc->type;
    pDst->in_use = TRUE;

    return TRUE;
}

BOOL moveDrive(drive_entry_t *pSrc, drive_entry_t *pDst)
{
    WINE_TRACE("pSrc->letter == %c, pDst->letter == %c\n", pSrc->letter, pDst->letter);

    if(!copyDrive(pSrc, pDst))
    {
        WINE_TRACE("copyDrive failed\n");
        return FALSE;
    }

    freeDrive(pSrc);
    return TRUE;
}

int refreshDriveDlg (HWND dialog)
{
  int driveCount = 0;
  int doesDriveCExist = FALSE;
  int i;

  WINE_TRACE("\n");

  updatingUI = TRUE;

  /* Clear the listbox */
  SendMessageA(GetDlgItem(dialog, IDC_LIST_DRIVES), LB_RESETCONTENT, 0, 0);

  for(i = 0; i < 26; i++)
  {
      char *title = 0;
      int titleLen;
      int itemIndex;

      /* skip over any unused drives */
      if(!drives[i].in_use)
          continue;

      if(drives[i].letter == 'C')
          doesDriveCExist = TRUE;

      titleLen = snprintf(title, 0, "Drive %c:\\ %s", 'A' + i,
                          drives[i].unixpath);
      titleLen++; /* add a byte for the trailing null */

      title = malloc(titleLen);

      /* the %s in the item label will be replaced by the drive letter, so -1, then
         -2 for the second %s which will be expanded to the label, finally + 1 for terminating #0 */
      snprintf(title, titleLen, "Drive %c:\\ %s", 'A' + i,
               drives[i].unixpath);

      WINE_TRACE("title is '%s'\n", title);

      /* the first SendMessage call adds the string and returns the index, the second associates that index with it */
      itemIndex = SendMessageA(GetDlgItem(dialog, IDC_LIST_DRIVES), LB_ADDSTRING ,(WPARAM) -1, (LPARAM) title);
      SendMessageA(GetDlgItem(dialog, IDC_LIST_DRIVES), LB_SETITEMDATA, itemIndex, (LPARAM) &drives[i]);

      free(title);
      driveCount++;
  }

  WINE_TRACE("loaded %d drives\n", driveCount);
  SendDlgItemMessage(dialog, IDC_LIST_DRIVES, LB_SETSEL, TRUE, lastSel);

  /* show the warning if there is no Drive C */
  if (!doesDriveCExist)
    ShowWindow(GetDlgItem(dialog, IDS_DRIVE_NO_C), SW_NORMAL);
  else
    ShowWindow(GetDlgItem(dialog, IDS_DRIVE_NO_C), SW_HIDE);
  
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
  const uint sCode;
  const char *sDesc;
} code_desc_pair;

static code_desc_pair type_pairs[] = {
  {DRIVE_FIXED,      "Local hard disk"},
  {DRIVE_REMOTE,     "Network share" },
  {DRIVE_REMOVABLE,  "Floppy disk"},
  {DRIVE_CDROM,      "CD-ROM"}
};
#define DRIVE_TYPE_DEFAULT 1


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
/*	enable(IDC_RADIO_AUTODETECT); */
	enable(IDC_RADIO_ASSIGN);
	disable(IDC_EDIT_DEVICE);
	disable(IDC_BUTTON_BROWSE_DEVICE);
	enable(IDC_EDIT_SERIAL);
	enable(IDC_EDIT_LABEL);
	enable(IDC_STATIC_SERIAL);
	enable(IDC_STATIC_LABEL);
	break;
	
      case BOX_MODE_CD_AUTODETECT:
/*	enable(IDC_RADIO_AUTODETECT); */
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
  int i;

  WINE_TRACE("\n");

  
  for(i = 0; i < 26; i++)
  {
      if(!drives[i].in_use) continue;
      result |= (1 << (toupper(drives[i].letter) - 'A'));
  }

  result = ~result;
  if (letter) result |= DRIVE_MASK_BIT(letter);
  
  WINE_TRACE( "finished drive letter loop with %lx\n", result );
  return result;
}

void advancedDriveEditDialog(HWND hDlg, BOOL showAdvanced)
{
#define ADVANCED_DELTA      120

    static RECT okpos;
    static BOOL got_initial_ok_position = FALSE;

    static RECT windowpos; /* we only use the height of this rectangle */
    static BOOL got_initial_window_position = FALSE;

    static RECT current_window;

    INT state;
    INT offset;
    char *text;

    if(!got_initial_ok_position)
    {
        POINT pt;
        GetWindowRect(GetDlgItem(hDlg, ID_BUTTON_OK), &okpos);
        pt.x = okpos.left;
        pt.y = okpos.top;
        ScreenToClient(hDlg, &pt);
        okpos.right+= (pt.x - okpos.left);
        okpos.bottom+= (pt.y - okpos.top);
        okpos.left = pt.x;
        okpos.top = pt.y;
        got_initial_ok_position = TRUE;
    }

    if(!got_initial_window_position)
    {
        GetWindowRect(hDlg, &windowpos);
        got_initial_window_position = TRUE;
    }
          
    if(showAdvanced)
    {
        state = SW_NORMAL;
        offset = 0;
        text = "Hide Advanced";
    } else
    {
        state = SW_HIDE;
        offset = ADVANCED_DELTA;
        text = "Show Advanced";
    }

    ShowWindow(GetDlgItem(hDlg, IDC_STATIC_TYPE), state);
    ShowWindow(GetDlgItem(hDlg, IDC_COMBO_TYPE), state);
    ShowWindow(GetDlgItem(hDlg, IDC_BOX_LABELSERIAL), state);
    ShowWindow(GetDlgItem(hDlg, IDC_RADIO_AUTODETECT), state);
    ShowWindow(GetDlgItem(hDlg, IDC_RADIO_ASSIGN), state);
    ShowWindow(GetDlgItem(hDlg, IDC_EDIT_LABEL), state);
    ShowWindow(GetDlgItem(hDlg, IDC_EDIT_DEVICE), state);
    ShowWindow(GetDlgItem(hDlg, IDC_STATIC_LABEL), state);
    ShowWindow(GetDlgItem(hDlg, IDC_BUTTON_BROWSE_DEVICE), state);
    SetWindowPos(GetDlgItem(hDlg, ID_BUTTON_OK),
                 HWND_TOP,
                 okpos.left, okpos.top - offset, okpos.right - okpos.left,
                 okpos.bottom - okpos.top,
                 0);

    /* resize the parent window */
    GetWindowRect(hDlg, &current_window);
    SetWindowPos(hDlg, 
                 HWND_TOP,
                 current_window.left,
                 current_window.top,
                 windowpos.right - windowpos.left,
                 windowpos.bottom - windowpos.top - offset,
                 0);

    /* update the button text based on the state */
    SetWindowText(GetDlgItem(hDlg, IDC_BUTTON_SHOW_HIDE_ADVANCED),
                  text);
}


void refreshDriveEditDialog(HWND dialog) {
  char *path;
  uint type;
  char *label;
  char *serial;
  char *device;
  unsigned int i;
  int selection = -1;

  updatingUI = TRUE;

  WINE_TRACE("\n");
  
  /* Drive letters */
  fill_drive_droplist( drive_available_mask( editDriveEntry->letter ), editDriveEntry->letter, dialog );

  /* path */
  path = editDriveEntry->unixpath;
  if (path) {
    WINE_TRACE("set path control text to '%s'\n", path);
    SetWindowText(GetDlgItem(dialog, IDC_EDIT_PATH), path);
  } else WINE_WARN("no Path field?\n");
  
  /* drive type */
  type = editDriveEntry->type;
  if (type) {
    for(i = 0; i < sizeof(type_pairs)/sizeof(code_desc_pair); i++) {
      SendDlgItemMessage(dialog, IDC_COMBO_TYPE, CB_ADDSTRING, 0,
			 (LPARAM) type_pairs[i].sDesc);
      if(type_pairs[i].sCode ==  type){
          selection = i;
      }
    }
  
    if( selection == -1 ) selection = DRIVE_TYPE_DEFAULT;
    SendDlgItemMessage(dialog, IDC_COMBO_TYPE, CB_SETCURSEL, selection, 0);
  } else WINE_WARN("no Type field?\n");

  
  /* removeable media properties */
  label = editDriveEntry->label;
  if (label) {
    SendDlgItemMessage(dialog, IDC_EDIT_LABEL, WM_SETTEXT, 0,(LPARAM)label);
  } else WINE_WARN("no Label field?\n");

  /* set serial edit text */
  serial = editDriveEntry->serial;
  if (serial) {
    SendDlgItemMessage(dialog, IDC_EDIT_SERIAL, WM_SETTEXT, 0,(LPARAM)serial);
  } else WINE_WARN("no Serial field?\n");

  /* TODO: get the device here to put into the edit box */
  device = "Not implemented yet";
  if (device) {
    SendDlgItemMessage(dialog, IDC_EDIT_DEVICE, WM_SETTEXT, 0,(LPARAM)device);
  } else WINE_WARN("no Device field?\n");

  selection = IDC_RADIO_ASSIGN;
  if ((type == DRIVE_CDROM) || (type == DRIVE_REMOVABLE)) {
#if 0
    if (device) {
      selection = IDC_RADIO_AUTODETECT;
      enable_labelserial_box(dialog, BOX_MODE_CD_AUTODETECT);
    } else {
#endif
      selection = IDC_RADIO_ASSIGN;
      enable_labelserial_box(dialog, BOX_MODE_CD_ASSIGN);
#if 0
    }
#endif
  } else {
    enable_labelserial_box(dialog, BOX_MODE_NORMAL);
    selection = IDC_RADIO_ASSIGN;
  }

  CheckRadioButton( dialog, IDC_RADIO_AUTODETECT, IDC_RADIO_ASSIGN, selection );
  
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
        if(!label) label = strdup("");
        setDriveLabel(editDriveEntry, label);
        refreshDriveDlg(driveDlgHandle);
        if (label) free(label);
        break;
      }
      case IDC_EDIT_PATH: {
          char *path = getDialogItemText(hDlg, controlID);
          if (!path) path = strdup("fake_windows"); /* default to assuming fake_windows in the .wine directory */
          WINE_TRACE("got path from control of '%s'\n", path);
          setDrivePath(editDriveEntry, path);
          if(path) free(path);
          break;
      }
      case IDC_EDIT_SERIAL: {
          char *serial = getDialogItemText(hDlg, controlID);
          if(!serial) serial = strdup("");
          setDriveSerial(editDriveEntry, serial);
          if (serial) free (serial);
          break;
      }
      case IDC_EDIT_DEVICE: {
          char *device = getDialogItemText(hDlg,controlID);
          /* TODO: handle device if/when it makes sense to do so.... */
          if (device) free(device);
          refreshDriveDlg(driveDlgHandle);
          break;
      }
  }
}

/* edit a drive entry */
INT_PTR CALLBACK DriveEditDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int selection;
  static BOOL advanced = FALSE;

  switch (uMsg)  {
      case WM_CLOSE:
	    EndDialog(hDlg, wParam);
	    return TRUE;

      case WM_INITDIALOG: {
          enable_labelserial_box(hDlg, BOX_MODE_NORMAL);
          advancedDriveEditDialog(hDlg, advanced);
          editDriveEntry = (drive_entry_t*)lParam;
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
        editDriveEntry->type = type_pairs[selection].sCode;
	    break;

	  case IDC_COMBO_LETTER: {
	    int item = SendDlgItemMessage(hDlg, IDC_COMBO_LETTER, CB_GETCURSEL, 0, 0);
	    char newLetter[4];
	    SendDlgItemMessage(hDlg, IDC_COMBO_LETTER, CB_GETLBTEXT, item, (LPARAM) newLetter);

	    if (HIWORD(wParam) != CBN_SELCHANGE) break;
	    if (newLetter[0] == editDriveEntry->letter) break;

	    WINE_TRACE("changing drive letter to %c\n", newLetter[0]);
            moveDrive(editDriveEntry, &drives[getDrive(newLetter[0])]);
            editDriveEntry = &drives[getDrive(newLetter[0])];
	    refreshDriveDlg(driveDlgHandle);
	    break;
	  }

	  case IDC_BUTTON_BROWSE_PATH:
	    WRITEME(hDlg);
	    break;

	  case IDC_RADIO_AUTODETECT: {
        /* TODO: */
        WINE_FIXME("Implement autodetection\n");
	    enable_labelserial_box(hDlg, BOX_MODE_CD_AUTODETECT);
        refreshDriveDlg(driveDlgHandle);
	    break;
	  }
	    
	  case IDC_RADIO_ASSIGN:
      {
        char *edit, *serial;
        edit = getDialogItemText(hDlg, IDC_EDIT_LABEL);
        if(!edit) edit = strdup("");
        setDriveLabel(editDriveEntry, edit);
        free(edit);

        serial = getDialogItemText(hDlg, IDC_EDIT_SERIAL);
        if(!serial) serial = strdup("");
        setDriveSerial(editDriveEntry, serial);
        free(serial);

        /* TODO: we don't have a device at this point */
/*      setDriveValue(editWindowLetter, "Device", NULL); */
	    enable_labelserial_box(hDlg, BOX_MODE_CD_ASSIGN);
	    refreshDriveDlg(driveDlgHandle);
	    break;
      }
	    
      case IDC_BUTTON_SHOW_HIDE_ADVANCED:
          advanced = (advanced == TRUE) ? FALSE : TRUE; /* toggle state */
          advancedDriveEditDialog(hDlg, advanced);
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
  
  while (mask & (1 << (newLetter - 'A'))) {
    newLetter++;
    if (newLetter > 'Z') {
      MessageBox(NULL, "You cannot add any more drives.\n\nEach drive must have a letter, from A to Z, so you cannot have more than 26", "", MB_OK | MB_ICONEXCLAMATION);
      return;
    }
  }
  WINE_TRACE("allocating drive letter %c\n", newLetter);

  if(newLetter == 'C') {
      addDrive(newLetter, "fake_windows", "System Drive", "", DRIVE_FIXED);
  } else {
      addDrive(newLetter, "/", "", "", DRIVE_FIXED);
  }

  refreshDriveDlg(driveDlgHandle);

  DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_DRIVE_EDIT), NULL, (DLGPROC) DriveEditDlgProc, (LPARAM) &(drives[getDrive(newLetter)]));
}

void onDriveInitDialog(void)
{
  char *pDevices, *pDev;
  int ret;
  int i;
  int retval;

  WINE_TRACE("\n");

  /* setup the drives array */
  pDev = pDevices = malloc(512);
  ret = GetLogicalDriveStrings(512, pDevices);

  /* make all devices unused */
  for(i = 0; i < 26; i++)
  {
      drives[i].letter = 'A' + i;
      drives[i].in_use = FALSE;
  }

  i = 0;

  while(ret)
  {
    CHAR volumeNameBuffer[512];
    DWORD serialNumber;
    CHAR serialNumberString[256];
    DWORD maxComponentLength;
    DWORD fileSystemFlags;
    CHAR fileSystemName[128];
    char rootpath[256];
    char simplepath[3];
    int pathlen;
    char targetpath[256];

    *pDevices = toupper(*pDevices);

    WINE_TRACE("pDevices == '%s'\n", pDevices);

    volumeNameBuffer[0] = 0;
    
    retval = GetVolumeInformation(pDevices,
                         volumeNameBuffer,
                         sizeof(volumeNameBuffer),
                         &serialNumber,
                         &maxComponentLength,
                         &fileSystemFlags,
                         fileSystemName,
                         sizeof(fileSystemName));
    if(!retval)
    {
        WINE_TRACE("GetVolumeInformation() for '%s' failed, setting serialNumber to 0\n", pDevices);
        PRINTERROR();
        serialNumber = 0;
    }

    WINE_TRACE("serialNumber: '0x%lX'\n", serialNumber);

    /* build rootpath for GetDriveType() */
    strncpy(rootpath, pDevices, sizeof(rootpath));
    pathlen = strlen(rootpath);
    /* ensure that we have a backslash on the root path */
    if((rootpath[pathlen - 1] != '\\') &&
       (pathlen < sizeof(rootpath)))
     {
         rootpath[pathlen] = '\\';
         rootpath[pathlen + 1] = 0;
     }

    strncpy(simplepath, pDevices, 2); /* QueryDosDevice() requires no trailing backslash */
    simplepath[2] = 0;
    QueryDosDevice(simplepath, targetpath, sizeof(targetpath)); 
    
    snprintf(serialNumberString, sizeof(serialNumberString), "%lX", serialNumber);
    WINE_TRACE("serialNumberString: '%s'\n", serialNumberString);
    addDrive(*pDevices, targetpath, volumeNameBuffer, serialNumberString, GetDriveType(rootpath));

    ret-=strlen(pDevices);
    pDevices+=strlen(pDevices);

    /* skip over any nulls */
    while((*pDevices == 0) && (ret))
    {
        ret--;
        pDevices++;
    }

    i++;
  }

  WINE_TRACE("found %d drives\n", i);

  free(pDev);
}


void applyDriveChanges(void)
{
    int i;
    CHAR devicename[4];
    CHAR targetpath[256];
    BOOL foundDrive;
    CHAR volumeNameBuffer[512];
    DWORD serialNumber;
    DWORD maxComponentLength;
    DWORD fileSystemFlags;
    CHAR fileSystemName[128];
    int retval;
    BOOL defineDevice;

    WINE_TRACE("\n");

    /* add each drive and remove as we go */
    for(i = 0; i < 26; i++)
    {
        defineDevice = FALSE;
        foundDrive = FALSE;
        snprintf(devicename, sizeof(devicename), "%c:", 'A' + i); 

        /* get a drive */
        if(QueryDosDevice(devicename, targetpath, sizeof(targetpath)))
        {
            foundDrive = TRUE;
        }

        /* if we found a drive and have a drive then compare things */
        if(foundDrive && drives[i].in_use)
        {
            char newSerialNumberText[256];

            volumeNameBuffer[0] = 0;

            WINE_TRACE("drives[i].letter: '%c'\n", drives[i].letter);

            snprintf(devicename, sizeof(devicename), "%c:\\", 'A' + i);
            retval = GetVolumeInformation(devicename,
                         volumeNameBuffer,
                         sizeof(volumeNameBuffer),
                         &serialNumber,
                         &maxComponentLength,
                         &fileSystemFlags,
                         fileSystemName,
                         sizeof(fileSystemName));
            if(!retval)
            {
                WINE_TRACE("  GetVolumeInformation() for '%s' failed\n", devicename);
                WINE_TRACE("  Skipping this drive\n");
                PRINTERROR();
                continue; /* skip this drive */
            }

            snprintf(newSerialNumberText, sizeof(newSerialNumberText), "%lX", serialNumber);

            WINE_TRACE("  current path:   '%s', new path:   '%s'\n",
                       targetpath, drives[i].unixpath);
            WINE_TRACE("  current label:  '%s', new label:  '%s'\n",
                       volumeNameBuffer, drives[i].label);
            WINE_TRACE("  current serial: '%s', new serial: '%s'\n",
                       newSerialNumberText, drives[i].serial);

            /* compare to what we have */
            /* do we have the same targetpath? */
            if(strcmp(drives[i].unixpath, targetpath) ||
               strcmp(drives[i].label, volumeNameBuffer) ||
               strcmp(drives[i].serial, newSerialNumberText))
            {
                defineDevice = TRUE;
                WINE_TRACE("  making changes to drive '%s'\n", devicename);
            } else
            {
                WINE_TRACE("  no changes to drive '%s'\n", devicename);
            }
        } else if(foundDrive && !drives[i].in_use)
        {
            /* remove this drive */
            if(!DefineDosDevice(DDD_REMOVE_DEFINITION, devicename, drives[i].unixpath))
            {
                WINE_ERR("unable to remove devicename of '%s', targetpath of '%s'\n",
                    devicename, drives[i].unixpath);
                PRINTERROR();
            } else
            {
                WINE_TRACE("removed devicename of '%s', targetpath of '%s'\n",
                           devicename, drives[i].unixpath);
            }
        } else if(drives[i].in_use) /* foundDrive must be false from the above check */
        {
            defineDevice = TRUE;
        }

        /* adding and modifying are the same steps */
        if(defineDevice)
        {
            char filename[256];
            HANDLE hFile;

            HKEY hKey;
            char *typeText;
            char driveValue[256];

            /* define this drive */
            /* DefineDosDevice() requires that NO trailing slash be present */
            snprintf(devicename, sizeof(devicename), "%c:", 'A' + i); 
            if(!DefineDosDevice(DDD_RAW_TARGET_PATH, devicename, drives[i].unixpath))
            {
                WINE_ERR("  unable to define devicename of '%s', targetpath of '%s'\n",
                    devicename, drives[i].unixpath);
                PRINTERROR();
            } else
            {
                WINE_TRACE("  added devicename of '%s', targetpath of '%s'\n",
                           devicename, drives[i].unixpath);
                
                /* SetVolumeLabel() requires a trailing slash */
                snprintf(devicename, sizeof(devicename), "%c:\\", 'A' + i);
                if(!SetVolumeLabel(devicename, drives[i].label))
                {
                    WINE_ERR("unable to set volume label for devicename of '%s', label of '%s'\n",
                        devicename, drives[i].label);
                    PRINTERROR();
                } else
                {
                    WINE_TRACE("  set volume label for devicename of '%s', label of '%s'\n",
                        devicename, drives[i].label);
                }
            }

            /* Set the drive type in the registry */
            if(drives[i].type == DRIVE_FIXED)
                typeText = "hd";
            else if(drives[i].type == DRIVE_REMOTE)
                typeText = "network";
            else if(drives[i].type == DRIVE_REMOVABLE)
                typeText = "floppy";
            else /* must be DRIVE_CDROM */
                typeText = "cdrom";
            

            snprintf(driveValue, sizeof(driveValue), "%c:", toupper(drives[i].letter));

            retval = RegOpenKey(HKEY_LOCAL_MACHINE,
                       "Software\\Wine\\Drives",
                       &hKey);

            if(retval != ERROR_SUCCESS)
            {
                WINE_TRACE("  Unable to open '%s'\n", "Software\\Wine\\Drives");
            } else
            {
                retval = RegSetValueEx(
                              hKey,
                              driveValue,
                              0,
                              REG_SZ,
                              typeText,
                              strlen(typeText) + 1);
                if(retval != ERROR_SUCCESS)
                {
                    WINE_TRACE("  Unable to set value of '%s' to '%s'\n",
                               driveValue, typeText);
                } else
                {
                    WINE_TRACE("  Finished setting value of '%s' to '%s'\n",
                               driveValue, typeText);
                    RegCloseKey(hKey);
                }
            }

            /* Set the drive serial number via a .windows-serial file in */
            /* the targetpath directory */
            snprintf(filename, sizeof(filename), "%c:\\.windows-serial", drives[i].letter);
            WINE_TRACE("  Putting serial number of '%ld' into file '%s'\n",
                       serialNumber, filename);
            hFile = CreateFile(filename,
                       GENERIC_WRITE,
                       FILE_SHARE_READ,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
            if(hFile)
            {
                WINE_TRACE("  writing serial number of '%s'\n", drives[i].serial);
                WriteFile(hFile,
                          drives[i].serial,
                          strlen(drives[i].serial),
                          NULL,
                          NULL); 
                WriteFile(hFile,
                          "\n",
                          strlen("\n"),
                          NULL,
                          NULL); 
                CloseHandle(hFile);
            } else
            {
                WINE_TRACE("  CreateFile() error with file '%s'\n", filename);
            }
        }

        /* if this drive is in use we should free it up */
        if(drives[i].in_use)
        {
            freeDrive(&drives[i]); /* free up the string memory */
        }
    }
}


INT_PTR CALLBACK
DriveDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int nItem;
  drive_entry_t *pDrive;

  switch (uMsg) {
      case WM_INITDIALOG:
          onDriveInitDialog();
          break;
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
	      pDrive = (drive_entry_t*)SendDlgItemMessage(hDlg, IDC_LIST_DRIVES, LB_GETITEMDATA, nItem, 0);
          freeDrive(pDrive);
	      refreshDriveDlg(driveDlgHandle);
	      break;
	      
	    case IDC_BUTTON_EDIT:
	      if (HIWORD(wParam) != BN_CLICKED) break;
	      nItem = SendMessage(GetDlgItem(hDlg, IDC_LIST_DRIVES),  LB_GETCURSEL, 0, 0);
	      pDrive = (drive_entry_t*)SendMessage(GetDlgItem(hDlg, IDC_LIST_DRIVES), LB_GETITEMDATA, nItem, 0);
	      DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_DRIVE_EDIT), NULL, (DLGPROC) DriveEditDlgProc, (LPARAM) pDrive);
	      break;

	    case IDC_BUTTON_AUTODETECT:
	      WRITEME(hDlg);
	      break;
	}
	break;
	
      case WM_NOTIFY: switch(((LPNMHDR)lParam)->code) {
	    case PSN_KILLACTIVE:
          WINE_TRACE("PSN_KILLACTIVE\n");
	      SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
	      break;
	    case PSN_APPLY:
          applyDriveChanges();
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
