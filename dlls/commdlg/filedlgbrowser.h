/*
 *  Implementation of IShellBrowser for the File Open common dialog
 * 
 *
 */

#ifndef SHBROWSER_H
#define SHBROWSER_H TRUE

#include "shlobj.h"
#include "winbase.h"
#include "objbase.h"
#include "commdlg.h"

/***********************************************************************
 * Defines and global variables
 */
#define _ICommDlgBrowser_Offset ((int)(&(((IShellBrowserImpl*)0)->lpVtblCommDlgBrowser)))
#define _ICOM_THIS_FromICommDlgBrowser(class, name) class* This = (class*)(((char*)name)-_ICommDlgBrowser_Offset);

#define _IServiceProvider_Offset ((int)(&(((IShellBrowserImpl*)0)->lpVtblServiceProvider)))
#define _ICOM_THIS_FromIServiceProvider(class, name) class* This = (class*)(((char*)name)-_IServiceProvider_Offset);

/* dialog internal property */

#define FODPROP_SAVEDLG 0x0001  /* File dialog is a Save file dialog */
#define FODPROP_USEVIEW 0x0002  /* Indicates the user selection must be taken 
				   from the IShellView */

/***********************************************************************
 * Data structure
 */


typedef struct
{
    
    LPOPENFILENAMEA ofnInfos;
    struct {
        IShellBrowser *FOIShellBrowser; 
        IShellFolder *FOIShellFolder;
        IShellView *FOIShellView;
	IDataObject *FOIDataObject;
    } Shell;

    struct {
        HWND hwndOwner;
        HWND hwndView;
        RECT rectView;
        FOLDERSETTINGS folderSettings;
        LPITEMIDLIST pidlAbsCurrent;
        LPWSTR lpstrCurrentFilter;
    } ShellInfos;

    struct {
        HWND hwndFileTypeCB;
        HWND hwndLookInCB;
        HWND hwndFileName;
	HWND hwndTB;
        HWND hwndCustomDlg;
	DWORD dwDlgProp;
    } DlgInfos;

    struct {
	UINT fileokstring;
	UINT lbselchstring;
	UINT helpmsgstring;
	UINT sharevistring;
    } HookMsg;

} FileOpenDlgInfos;

/***********************************************************************
 * Control ID's
 */
#define IDS_ABOUTBOX                    101
#define IDS_DOCUMENTFOLDERS             102
#define IDS_PERSONAL                    103
#define IDS_FAVORITES                   104
#define IDS_PATH                        105
#define IDS_DESKTOP                     106

#define IDS_FONTS                       108
#define IDS_MYCOMPUTER                  110
#define IDS_SYSTEMFOLDERS               112
#define IDS_LOCALHARDRIVES              113
#define IDS_FILENOTFOUND                114
#define IDS_VERIFYFILE                  115
#define IDS_CREATEFILE                  116
#define IDS_CREATEFOLDER_DENIED         117
#define IDS_FILEOPEN_CAPTION            118
#define IDL_OVERWRITEFILE		119
#define IDS_INVALID_FILENAME_TITLE	120
#define IDS_INVALID_FILENAME		121
#define IDS_PATHNOTEXISTING		122
#define IDS_FILENOTEXISTING		123

/* File Dialog Tooltips string IDs */

#define IDS_UPFOLDER                    150
#define IDS_NEWFOLDER                   151
#define IDS_LISTVIEW                    152
#define IDS_REPORTVIEW                  153
#define IDS_TODESKTOP                   154

#define IDC_OPENREADONLY                chx1

#define IDC_TOOLBARSTATIC		stc1
#define IDC_FILETYPESTATIC              stc2
#define IDC_FILENAMESTATIC              stc3
#define IDC_LOOKINSTATIC                stc4

#define IDC_SHELLSTATIC                 lst1

#define IDC_FILETYPE                    cmb1
#define IDC_LOOKIN                      cmb2

#define IDC_FILENAME                    edt1

#define IDC_TOOLBAR			ctl1

/***********************************************************************
 * Prototypes for the methods of the IShellBrowserImpl class
 */
/* Constructor */
IShellBrowser * IShellBrowserImpl_Construct(HWND hwndOwner);


LPITEMIDLIST GetPidlFromDataObject ( IDataObject *doSelected, UINT nPidlIndex);
UINT GetNumSelected(IDataObject *doSelected);

/* pidl handling */
BOOL IsPidlFolder (LPSHELLFOLDER psf, LPITEMIDLIST pidl);

/* Functions used by the EDIT box */
void FILEDLG95_FILENAME_FillFromSelection (HWND hwnd);
int FILEDLG95_FILENAME_GetFileNames (HWND hwnd, LPSTR * lpstrFileList, UINT * sizeUsed);

#endif /*SHBROWSER_H*/
