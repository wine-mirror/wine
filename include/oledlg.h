/*
 *	oledlg.h	-	Declarations for OLEDLG
 */

#ifndef __WINE_OLEDLG_H
#define __WINE_OLEDLG_H

#include "commdlg.h"
#include "prsht.h"
#include "windef.h"
#include "oleidl.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct IOleUILinkContainerA IOleUILinkContainerA, *POLEUILINKCONTAINERA, *LPOLEUILINKCONTAINERA;
typedef struct IOleUILinkContainerW IOleUILinkContainerW, *POLEUILINKCONTAINERW, *LPOLEUILINKCONTAINERW;

typedef struct IOleUILinkInfoA IOleUILinkInfoA, *POLEUILINKINFOA, *LPOLEUILINKINFOA;
typedef struct IOleUILinkInfoW IOleUILinkInfoW, *POLEUILINKINFOW, *LPOLEUILINKINFOW;

typedef struct IOleUIObjInfoA IOleUIObjInfoA, *POLEUIOBJINFOA, *LPOLEUIOBJINFOA;
typedef struct IOleUIObjInfoW IOleUIObjInfoW, *POLEUIOBJINFOW, *LPOLEUIOBJINFOW;

#define CSF_SHOWHELP                    0x00000001L
#define CSF_VALIDSOURCE                 0x00000002L
#define CSF_ONLYGETSOURCE               0x00000004L
#define CSF_EXPLORER                    0x00000008L

#define PSF_SHOWHELP                    0x00000001L
#define PSF_SELECTPASTE                 0x00000002L
#define PSF_SELECTPASTELINK             0x00000004L
#define PSF_CHECKDISPLAYASICON          0x00000008L
#define PSF_DISABLEDISPLAYASICON        0x00000010L
#define PSF_HIDECHANGEICON              0x00000020L
#define PSF_STAYONCLIPBOARDCHANGE       0x00000040L
#define PSF_NOREFRESHDATAOBJECT         0x00000080L

#define IOF_SHOWHELP                    0x00000001L
#define IOF_SELECTCREATENEW             0x00000002L
#define IOF_SELECTCREATEFROMFILE        0x00000004L
#define IOF_CHECKLINK                   0x00000008L
#define IOF_CHECKDISPLAYASICON          0x00000010L
#define IOF_CREATENEWOBJECT             0x00000020L
#define IOF_CREATEFILEOBJECT            0x00000040L
#define IOF_CREATELINKOBJECT            0x00000080L
#define IOF_DISABLELINK                 0x00000100L
#define IOF_VERIFYSERVERSEXIST          0x00000200L
#define IOF_DISABLEDISPLAYASICON        0x00000400L
#define IOF_HIDECHANGEICON              0x00000800L
#define IOF_SHOWINSERTCONTROL           0x00001000L
#define IOF_SELECTCREATECONTROL         0x00002000L

#define CF_SHOWHELPBUTTON               0x00000001L
#define CF_SETCONVERTDEFAULT            0x00000002L
#define CF_SETACTIVATEDEFAULT           0x00000004L
#define CF_SELECTCONVERTTO              0x00000008L
#define CF_SELECTACTIVATEAS             0x00000010L
#define CF_DISABLEDISPLAYASICON         0x00000020L
#define CF_DISABLEACTIVATEAS            0x00000040L
#define CF_HIDECHANGEICON               0x00000080L
#define CF_CONVERTONLY                  0x00000100L

#define CIF_SHOWHELP                    0x00000001L
#define CIF_SELECTCURRENT               0x00000002L
#define CIF_SELECTDEFAULT               0x00000004L
#define CIF_SELECTFROMFILE              0x00000008L
#define CIF_USEICONEXE                  0x00000010L

#define OLEUI_FALSE   0
#define OLEUI_SUCCESS 1 /* Same as OLEUI_OK */
#define OLEUI_OK      1 /* OK button pressed */ 
#define OLEUI_CANCEL  2 /* Cancel button pressed */


typedef UINT (CALLBACK *LPFNOLEUIHOOK)(HWND, UINT, WPARAM, LPARAM);

/*****************************************************************************
 * INSERT OBJECT DIALOG
 */
typedef struct tagOLEUIINSERTOBJECTA
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    HWND            hWndOwner;      
    LPCSTR          lpszCaption;    
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    HINSTANCE       hInstance;      
    LPCSTR          lpszTemplate;   
    HRSRC           hResource;      
    CLSID           clsid;          
    LPSTR           lpszFile;       
    UINT            cchFile;        
    UINT            cClsidExclude;  
    LPCLSID         lpClsidExclude; 
    IID             iid;            
    DWORD           oleRender;      
    LPFORMATETC     lpFormatEtc;    
    LPOLECLIENTSITE lpIOleClientSite;   
    LPSTORAGE       lpIStorage;     
    LPVOID          *ppvObj;        
    SCODE           sc;             
    HGLOBAL         hMetaPict;      
} OLEUIINSERTOBJECTA, *POLEUIINSERTOBJECTA, *LPOLEUIINSERTOBJECTA;

typedef struct tagOLEUIINSERTOBJECTW
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    HWND            hWndOwner;      
    LPCWSTR         lpszCaption;    
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    HINSTANCE       hInstance;      
    LPCWSTR         lpszTemplate;   
    HRSRC           hResource;      
    CLSID           clsid;          
    LPWSTR          lpszFile;       
    UINT            cchFile;        
    UINT            cClsidExclude;  
    LPCLSID         lpClsidExclude; 
    IID             iid;            
    DWORD           oleRender;      
    LPFORMATETC     lpFormatEtc;    
    LPOLECLIENTSITE lpIOleClientSite;   
    LPSTORAGE       lpIStorage;     
    LPVOID          *ppvObj;        
    SCODE           sc;             
    HGLOBAL         hMetaPict;      
} OLEUIINSERTOBJECTW, *POLEUIINSERTOBJECTW, *LPOLEUIINSERTOBJECTW;

DECL_WINELIB_TYPE_AW(OLEUIINSERTOBJECT)
DECL_WINELIB_TYPE_AW(POLEUIINSERTOBJECT)
DECL_WINELIB_TYPE_AW(LPOLEUIINSERTOBJECT)


/*****************************************************************************
 * CONVERT DIALOG
 */
typedef struct tagOLEUICONVERTA
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    HWND            hWndOwner;      
    LPCSTR          lpszCaption;    
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    HINSTANCE       hInstance;      
    LPCSTR          lpszTemplate;   
    HRSRC           hResource;      
    CLSID           clsid;          
    CLSID           clsidConvertDefault;    
    CLSID           clsidActivateDefault;   
    CLSID           clsidNew;       
    DWORD           dvAspect;       
    WORD            wFormat;        
    BOOL            fIsLinkedObject;
    HGLOBAL         hMetaPict;      
    LPSTR           lpszUserType;   
    BOOL            fObjectsIconChanged; 
    LPSTR           lpszDefLabel;   
    UINT            cClsidExclude;  
    LPCLSID         lpClsidExclude; 
} OLEUICONVERTA, *POLEUICONVERTA, *LPOLEUICONVERTA;

typedef struct tagOLEUICONVERTW
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    HWND            hWndOwner;      
    LPCWSTR         lpszCaption;    
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    HINSTANCE       hInstance;      
    LPCWSTR         lpszTemplate;   
    HRSRC           hResource;      
    CLSID           clsid;          
    CLSID           clsidConvertDefault;    
    CLSID           clsidActivateDefault;   
    CLSID           clsidNew;       
    DWORD           dvAspect;       
    WORD            wFormat;        
    BOOL            fIsLinkedObject;
    HGLOBAL         hMetaPict;      
    LPWSTR          lpszUserType;   
    BOOL            fObjectsIconChanged; 
    LPWSTR          lpszDefLabel;   
    UINT            cClsidExclude;  
    LPCLSID         lpClsidExclude; 
} OLEUICONVERTW, *POLEUICONVERTW, *LPOLEUICONVERTW;

DECL_WINELIB_TYPE_AW(OLEUICONVERT)
DECL_WINELIB_TYPE_AW(POLEUICONVERT)
DECL_WINELIB_TYPE_AW(LPOLEUICONVERT)

/*****************************************************************************
 * CHANGE ICON DIALOG
 */
typedef struct tagOLEUICHANGEICONA
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    HWND            hWndOwner;      
    LPCSTR          lpszCaption;    
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    HINSTANCE       hInstance;      
    LPCSTR          lpszTemplate;   
    HRSRC           hResource;      
    HGLOBAL         hMetaPict;      
    CLSID           clsid;          
    CHAR            szIconExe[MAX_PATH];    
    INT             cchIconExe;     
} OLEUICHANGEICONA, *POLEUICHANGEICONA, *LPOLEUICHANGEICONA;

typedef struct tagOLEUICHANGEICONW
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    HWND            hWndOwner;      
    LPCWSTR         lpszCaption;    
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    HINSTANCE       hInstance;      
    LPCWSTR         lpszTemplate;   
    HRSRC           hResource;      
    HGLOBAL         hMetaPict;      
    CLSID           clsid;          
    WCHAR           szIconExe[MAX_PATH];    
    INT             cchIconExe;     
} OLEUICHANGEICONW, *POLEUICHANGEICONW, *LPOLEUICHANGEICONW;

DECL_WINELIB_TYPE_AW(OLEUICHANGEICON)
DECL_WINELIB_TYPE_AW(POLEUICHANGEICON)
DECL_WINELIB_TYPE_AW(LPOLEUICHANGEICON)


/*****************************************************************************
 * PASTE SPECIAL DIALOG
 */

typedef enum tagOLEUIPASTEFLAG
{
   OLEUIPASTE_ENABLEICON    = 2048,     
   OLEUIPASTE_PASTEONLY     = 0,
   OLEUIPASTE_PASTE         = 512,
   OLEUIPASTE_LINKANYTYPE   = 1024,
   OLEUIPASTE_LINKTYPE1     = 1,
   OLEUIPASTE_LINKTYPE2     = 2,
   OLEUIPASTE_LINKTYPE3     = 4,
   OLEUIPASTE_LINKTYPE4     = 8,
   OLEUIPASTE_LINKTYPE5     = 16,
   OLEUIPASTE_LINKTYPE6     = 32,
   OLEUIPASTE_LINKTYPE7     = 64,
   OLEUIPASTE_LINKTYPE8     = 128
} OLEUIPASTEFLAG;    

typedef struct tagOLEUIPASTEENTRYA
{
   FORMATETC        fmtetc;         
   LPCSTR           lpstrFormatName;
   LPCSTR           lpstrResultText;
   DWORD            dwFlags;        
   DWORD            dwScratchSpace; 
} OLEUIPASTEENTRYA, *POLEUIPASTEENTRYA, *LPOLEUIPASTEENTRYA;

typedef struct tagOLEUIPASTEENTRYW
{
   FORMATETC        fmtetc;         
   LPCWSTR          lpstrFormatName;
   LPCWSTR          lpstrResultText;
   DWORD            dwFlags;        
   DWORD            dwScratchSpace; 
} OLEUIPASTEENTRYW, *POLEUIPASTEENTRYW, *LPOLEUIPASTEENTRYW;

DECL_WINELIB_TYPE_AW(OLEUIPASTEENTRY)
DECL_WINELIB_TYPE_AW(POLEUIPASTEENTRY)
DECL_WINELIB_TYPE_AW(LPOLEUIPASTEENTRY)

typedef struct tagOLEUIPASTESPECIALA
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    HWND            hWndOwner;      
    LPCSTR          lpszCaption;    
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    HINSTANCE       hInstance;      
    LPCSTR          lpszTemplate;   
    HRSRC           hResource;      
    LPDATAOBJECT    lpSrcDataObj;   
    LPOLEUIPASTEENTRYA arrPasteEntries;
    INT             cPasteEntries;  
    UINT*           arrLinkTypes;   
    INT             cLinkTypes;     
    UINT            cClsidExclude;  
    LPCLSID         lpClsidExclude; 
    INT             nSelectedIndex; 
    BOOL            fLink;          
    HGLOBAL         hMetaPict;      
    SIZEL           sizel;          
} OLEUIPASTESPECIALA, *POLEUIPASTESPECIALA, *LPOLEUIPASTESPECIALA;

typedef struct tagOLEUIPASTESPECIALW
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    HWND            hWndOwner;      
    LPCWSTR         lpszCaption;    
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    HINSTANCE       hInstance;      
    LPCWSTR         lpszTemplate;   
    HRSRC           hResource;      
    LPDATAOBJECT    lpSrcDataObj;   
    LPOLEUIPASTEENTRYW arrPasteEntries;
    INT             cPasteEntries;  
    UINT*           arrLinkTypes;   
    INT             cLinkTypes;     
    UINT            cClsidExclude;  
    LPCLSID         lpClsidExclude; 
    INT             nSelectedIndex; 
    BOOL            fLink;          
    HGLOBAL         hMetaPict;      
    SIZEL           sizel;          
} OLEUIPASTESPECIALW, *POLEUIPASTESPECIALW, *LPOLEUIPASTESPECIALW;

DECL_WINELIB_TYPE_AW(OLEUIPASTESPECIAL)
DECL_WINELIB_TYPE_AW(POLEUIPASTESPECIAL)
DECL_WINELIB_TYPE_AW(LPOLEUIPASTESPECIAL)

#define PSF_SHOWHELP                    0x00000001L
#define PSF_SELECTPASTE                 0x00000002L
#define PSF_SELECTPASTELINK             0x00000004L

#define PSF_CHECKDISPLAYASICON          0x00000008L
#define PSF_DISABLEDISPLAYASICON        0x00000010L
#define PSF_HIDECHANGEICON              0x00000020L
#define PSF_STAYONCLIPBOARDCHANGE       0x00000040L
#define PSF_NOREFRESHDATAOBJECT         0x00000080L

#define OLEUI_IOERR_SRCDATAOBJECTINVALID    (OLEUI_ERR_STANDARDMAX+0)
#define OLEUI_IOERR_ARRPASTEENTRIESINVALID  (OLEUI_ERR_STANDARDMAX+1)
#define OLEUI_IOERR_ARRLINKTYPESINVALID     (OLEUI_ERR_STANDARDMAX+2)
#define OLEUI_PSERR_CLIPBOARDCHANGED        (OLEUI_ERR_STANDARDMAX+3)
#define OLEUI_PSERR_GETCLIPBOARDFAILED      (OLEUI_ERR_STANDARDMAX+4)

typedef struct tagOLEUIEDITLINKSW
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    HWND            hWndOwner;      
    LPCWSTR         lpszCaption;    
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    HINSTANCE       hInstance;      
    LPCWSTR         lpszTemplate;   
    HRSRC           hResource;      
    LPOLEUILINKCONTAINERW lpOleUILinkContainer;  
} OLEUIEDITLINKSW, *POLEUIEDITLINKSW, *LPOLEUIEDITLINKSW;

typedef struct tagOLEUIEDITLINKSA
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    HWND            hWndOwner;      
    LPCSTR          lpszCaption;    
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    HINSTANCE       hInstance;      
    LPCSTR          lpszTemplate;   
    HRSRC           hResource;      
    LPOLEUILINKCONTAINERA lpOleUILinkContainer;  
} OLEUIEDITLINKSA, *POLEUIEDITLINKSA, *LPOLEUIEDITLINKSA;

DECL_WINELIB_TYPE_AW(OLEUIEDITLINKS)
DECL_WINELIB_TYPE_AW(POLEUIEDITLINKS)
DECL_WINELIB_TYPE_AW(LPOLEUIEDITLINKS)


/***********************************************************************************
 * BUSY DIALOG
 */
typedef struct tagOLEUIBUSYA
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    HWND            hWndOwner;      
    LPCSTR          lpszCaption;    
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    HINSTANCE       hInstance;      
    LPCSTR          lpszTemplate;   
    HRSRC           hResource;      
    HTASK           hTask;          
    HWND*           lphWndDialog;   
} OLEUIBUSYA, *POLEUIBUSYA, *LPOLEUIBUSYA;

typedef struct tagOLEUIBUSYW
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    HWND            hWndOwner;      
    LPCWSTR         lpszCaption;    
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    HINSTANCE       hInstance;      
    LPCWSTR         lpszTemplate;   
    HRSRC           hResource;      
    HTASK           hTask;          
    HWND*           lphWndDialog;   
} OLEUIBUSYW, *POLEUIBUSYW, *LPOLEUIBUSYW;

DECL_WINELIB_TYPE_AW(OLEUIBUSY)
DECL_WINELIB_TYPE_AW(POLEUIBUSY)
DECL_WINELIB_TYPE_AW(LPOLEUIBUSY)


struct tagOLEUIOBJECTPROPSW;
struct tagOLEUIOBJECTPROPSA;

typedef struct tagOLEUIGNRLPROPSA
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    DWORD           dwReserved1[2];
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    DWORD           dwReserved2[3];
    struct tagOLEUIOBJECTPROPSA* lpOP;   

} OLEUIGNRLPROPSA, *POLEUIGNRLPROPSA, *LPOLEUIGNRLPROPSA;

typedef struct tagOLEUIGNRLPROPSW
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    DWORD           dwReserved1[2];
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    DWORD           dwReserved2[3];
    struct tagOLEUIOBJECTPROPSW* lpOP;   
} OLEUIGNRLPROPSW, *POLEUIGNRLPROPSW, *LPOLEUIGNRLPROPSW;

DECL_WINELIB_TYPE_AW(OLEUIGNRLPROPS)
DECL_WINELIB_TYPE_AW(POLEUIGNRLPROPS)
DECL_WINELIB_TYPE_AW(LPOLEUIGNRLPROPS)

typedef struct tagOLEUIVIEWPROPSA
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    DWORD           dwReserved1[2];
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    DWORD           dwReserved2[3];
    struct tagOLEUIOBJECTPROPSA* lpOP; 
    INT             nScaleMin;      
    INT             nScaleMax;
} OLEUIVIEWPROPSA, *POLEUIVIEWPROPSA, *LPOLEUIVIEWPROPSA;

typedef struct tagOLEUIVIEWPROPSW
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    DWORD           dwReserved1[2];
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    DWORD           dwReserved2[3];
    struct tagOLEUIOBJECTPROPSW* lpOP; 
    INT             nScaleMin;      
    INT             nScaleMax;
} OLEUIVIEWPROPSW, *POLEUIVIEWPROPSW, *LPOLEUIVIEWPROPSW;

DECL_WINELIB_TYPE_AW(OLEUIVIEWPROPS)
DECL_WINELIB_TYPE_AW(POLEUIVIEWPROPS)
DECL_WINELIB_TYPE_AW(LPOLEUIVIEWPROPS)


#define VPF_SELECTRELATIVE          0x00000001L 
#define VPF_DISABLERELATIVE         0x00000002L 
#define VPF_DISABLESCALE            0x00000004L 

typedef struct tagOLEUILINKPROPSA
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    DWORD           dwReserved1[2];
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    DWORD           dwReserved2[3];
    struct tagOLEUIOBJECTPROPSA* lpOP; 
} OLEUILINKPROPSA, *POLEUILINKPROPSA, *LPOLEUILINKPROPSA;

typedef struct tagOLEUILINKPROPSW
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    DWORD           dwReserved1[2];
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    DWORD           dwReserved2[3];
    struct tagOLEUIOBJECTPROPSW* lpOP; 
} OLEUILINKPROPSW, *POLEUILINKPROPSW, *LPOLEUILINKPROPSW;

DECL_WINELIB_TYPE_AW(OLEUILINKPROPS)
DECL_WINELIB_TYPE_AW(POLEUILINKPROPS)
DECL_WINELIB_TYPE_AW(LPOLEUILINKPROPS)

typedef struct tagOLEUIOBJECTPROPSA
{
    DWORD                cbStruct;       
    DWORD                dwFlags;        
    LPPROPSHEETHEADERA   lpPS;         
    DWORD                dwObject;       
    LPOLEUIOBJINFOA      lpObjInfo;      
    DWORD                dwLink;         
    LPOLEUILINKINFOA     lpLinkInfo;     
    LPOLEUIGNRLPROPSA    lpGP;          
    LPOLEUIVIEWPROPSA    lpVP;          
    LPOLEUILINKPROPSA    lpLP;          
} OLEUIOBJECTPROPSA, *POLEUIOBJECTPROPSA, *LPOLEUIOBJECTPROPSA;

typedef struct tagOLEUIOBJECTPROPSW
{
    DWORD                cbStruct;       
    DWORD                dwFlags;        
    LPPROPSHEETHEADERW   lpPS;         
    DWORD                dwObject;       
    LPOLEUIOBJINFOW      lpObjInfo;      
    DWORD                dwLink;         
    LPOLEUILINKINFOW     lpLinkInfo;     
    LPOLEUIGNRLPROPSW    lpGP;          
    LPOLEUIVIEWPROPSW    lpVP;          
    LPOLEUILINKPROPSW    lpLP;          
} OLEUIOBJECTPROPSW, *POLEUIOBJECTPROPSW, *LPOLEUIOBJECTPROPSW;

DECL_WINELIB_TYPE_AW(OLEUIOBJECTPROPS)
DECL_WINELIB_TYPE_AW(POLEUIOBJECTPROPS)
DECL_WINELIB_TYPE_AW(LPOLEUIOBJECTPROPS)

/************************************************************************************
 * CHANGE SOURCE DIALOG
 */


typedef struct tagOLEUICHANGESOURCEW
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    HWND            hWndOwner;      
    LPCWSTR         lpszCaption;    
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    HINSTANCE       hInstance;      
    LPCWSTR         lpszTemplate;   
    HRSRC           hResource;      
    OPENFILENAMEW*lpOFN;          
    DWORD           dwReserved1[4]; 
    LPOLEUILINKCONTAINERW lpOleUILinkContainer;  
    DWORD           dwLink;         
    LPWSTR          lpszDisplayName;
    ULONG           nFileLength;    
    LPWSTR          lpszFrom;       
    LPWSTR          lpszTo;         
} OLEUICHANGESOURCEW, *POLEUICHANGESOURCEW, *LPOLEUICHANGESOURCEW;


typedef struct tagOLEUICHANGESOURCEA
{
    DWORD           cbStruct;       
    DWORD           dwFlags;        
    HWND            hWndOwner;      
    LPCSTR          lpszCaption;    
    LPFNOLEUIHOOK   lpfnHook;       
    LPARAM          lCustData;      
    HINSTANCE       hInstance;      
    LPCSTR          lpszTemplate;   
    HRSRC           hResource;      
    OPENFILENAMEA*  lpOFN;          
    DWORD           dwReserved1[4]; 
    LPOLEUILINKCONTAINERA lpOleUILinkContainer;  
    DWORD           dwLink;         
    LPSTR           lpszDisplayName;
    ULONG           nFileLength;    
    LPSTR           lpszFrom;       
    LPSTR           lpszTo;         
} OLEUICHANGESOURCEA, *POLEUICHANGESOURCEA, *LPOLEUICHANGESOURCEA;


DECL_WINELIB_TYPE_AW(OLEUICHANGESOURCE)
DECL_WINELIB_TYPE_AW(POLEUICHANGESOURCE)
DECL_WINELIB_TYPE_AW(LPOLEUICHANGESOURCE)


/*****************************************************************************
 * IOleUILinkContainer interface
 */
#define ICOM_INTERFACE   IOleUILinkContainerA
#define IOleUILinkContainerA_METHODS \
    ICOM_METHOD1(DWORD,GetNextLink,               DWORD,dwLink) \
    ICOM_METHOD2(HRESULT,SetLinkUpdateOptions,    DWORD,dwLink,  DWORD,dwUpdateOpt) \
    ICOM_METHOD2(HRESULT,GetLinkUpdateOptions,    DWORD,dwLink,  DWORD*,lpdwUpdateOpt) \
    ICOM_METHOD5(HRESULT,SetLinkSource,           DWORD,dwLink,  LPSTR,lpszDisplayName, \
                ULONG,lenFileName,  ULONG*,pchEaten,  BOOL,fValidateSource) \
    ICOM_METHOD7(HRESULT,GetLinkSource,           DWORD,dwLink, \
                LPSTR*,lplpszDisplayName,  ULONG*,lplenFileName, \
                LPSTR*,lplpszFullLinkType,  LPSTR*,lplpszShortLinkType, \
                BOOL*,lpfSourceAvailable,  BOOL*,lpfIsSelected) \
    ICOM_METHOD1(HRESULT,OpenLinkSource,          DWORD,dwLink) \
    ICOM_METHOD3(HRESULT,UpdateLink,              DWORD,dwLink, \
                BOOL,fErrorMessage,  BOOL,fReserved) \
    ICOM_METHOD1(HRESULT,CancelLink,							DWORD,dwLink)
#define IOleUILinkContainerA_IMETHODS \
    IUnknown_IMETHODS \
    IOleUILinkContainerA_METHODS
ICOM_DEFINE(IOleUILinkContainerA, IUnknown)
#undef ICOM_INTERFACE

#define ICOM_INTERFACE   IOleUILinkContainerW
#define IOleUILinkContainerW_METHODS \
    ICOM_METHOD1(DWORD,GetNextLink,               DWORD,dwLink) \
    ICOM_METHOD2(HRESULT,SetLinkUpdateOptions,    DWORD,dwLink,  DWORD,dwUpdateOpt) \
    ICOM_METHOD2(HRESULT,GetLinkUpdateOptions,    DWORD,dwLink,  DWORD*,lpdwUpdateOpt) \
    ICOM_METHOD5(HRESULT,SetLinkSource,           DWORD,dwLink,  LPWSTR,lpszDisplayName, \
                ULONG,lenFileName,  ULONG*,pchEaten,  BOOL,fValidateSource) \
    ICOM_METHOD7(HRESULT,GetLinkSource,           DWORD,dwLink, \
                LPWSTR*,lplpszDisplayName,  ULONG*,lplenFileName, \
                LPWSTR*,lplpszFullLinkType,  LPWSTR*,lplpszShortLinkType, \
                BOOL*,lpfSourceAvailable,  BOOL*,lpfIsSelected) \
    ICOM_METHOD1(HRESULT,OpenLinkSource,          DWORD,dwLink) \
    ICOM_METHOD3(HRESULT,UpdateLink,              DWORD,dwLink, \
                BOOL,fErrorMessage,  BOOL,fReserved) \
    ICOM_METHOD1(HRESULT,CancelLink,							DWORD,dwLink)
#define IOleUILinkContainerW_IMETHODS \
    IUnknown_IMETHODS \
    IOleUILinkContainerW_METHODS 
ICOM_DEFINE(IOleUILinkContainerW, IUnknown)
#undef ICOM_INTERFACE

DECL_WINELIB_TYPE_AW(IOleUILinkContainer)
DECL_WINELIB_TYPE_AW(POLEUILINKCONTAINER)
DECL_WINELIB_TYPE_AW(LPOLEUILINKCONTAINER)

#if ICOM_INTERFACE
/*** IUnknown methods ***/
#define IOleUILinkContainer_QueryInterface(p,a,b)          ICOM_CALL2(QueryInterface,p,a,b)
#define IOleUILinkContainer_AddRef(p)                      ICOM_CALL (AddRef,p)
#define IOleUILinkContainer_Release(p)                     ICOM_CALL (Release,p)
/*** IOleUILinkContainer methods ***/
#define IOleUILinkContainer_GetNextLink(p,a)               ICOM_CALL1(GetNextLink,p,a)
#define IOleUILinkContainer_SetLinkUpdateOptions(p,a,b)    ICOM_CALL2(SetLinkUpdateOptions,p,a,b)
#define IOleUILinkContainer_GetLinkUpdateOptions(p,a,b)    ICOM_CALL2(GetLinkUpdateOptions,p,a,b)
#define IOleUILinkContainer_SetLinkSource(p,a,b,c,d,e)     ICOM_CALL5(SetLinkSource,p,a,b,c,d,e)
#define IOleUILinkContainer_GetLinkSource(p,a,b,c,d,e,f,g) ICOM_CALL7(GetLinkSource,p,a,b,c,d,e,f,g)
#define IOleUILinkContainer_OpenLinkSource(p,a)            ICOM_CALL1(OpenLinkSource,p,a)
#define IOleUILinkContainer_UpdateLink(p,a,b,c)            ICOM_CALL3(UpdateLink,p,a,b,c)        
#define IOleUILinkContainer_CancelLink(p,a)                ICOM_CALL1(CancelLink,p,a)           
#endif


/*****************************************************************************
 * IOleUILinkInfo interface
 */
#define ICOM_INTERFACE   IOleUILinkInfoA
#define IOleUILinkInfoA_METHODS \
    ICOM_METHOD2(HRESULT,GetLastUpdate,           DWORD,dwLink,  FILETIME*,lpLastUpdate)
#define IOleUILinkInfoA_IMETHODS \
    IOleUILinkContainerA_IMETHODS \
    IOleUILinkInfoA_METHODS 
ICOM_DEFINE(IOleUILinkInfoA, IOleUILinkContainerA)
#undef ICOM_INTERFACE

#define ICOM_INTERFACE   IOleUILinkInfoW
#define IOleUILinkInfoW_METHODS \
    ICOM_METHOD2(HRESULT,GetLastUpdate,           DWORD,dwLink,  FILETIME*,lpLastUpdate)
#define IOleUILinkInfoW_IMETHODS \
    IOleUILinkContainerW_IMETHODS \
    IOleUILinkInfoW_METHODS 
ICOM_DEFINE(IOleUILinkInfoW, IOleUILinkContainerW)
#undef  ICOM_INTERFACE

DECL_WINELIB_TYPE_AW(IOleUILinkInfo)
DECL_WINELIB_TYPE_AW(POLEUILINKINFO)
DECL_WINELIB_TYPE_AW(LPOLEUILINKINFO)

#ifdef ICOM_CINTERFACE
/*** IUnknown methods ***/
#define IOleUILinkInfo_QueryInterface(p,a,b)          ICOM_CALL2(QueryInterface,p,a,b)
#define IOleUILinkInfo_AddRef(p)                      ICOM_CALL (AddRef,p)
#define IOleUILinkInfo_Release(p)                     ICOM_CALL (Release,p)
/*** IOleUILinkContainer methods ***/
#define IOleUILinkInfo_GetNextLink(p,a)               ICOM_CALL1(GetNextLink,p,a)
#define IOleUILinkInfo_SetLinkUpdateOptions(p,a,b)    ICOM_CALL2(SetLinkUpdateOptions,p,a,b)
#define IOleUILinkInfo_GetLinkUpdateOptions(p,a,b)    ICOM_CALL2(GetLinkUpdateOptions,p,a,b)
#define IOleUILinkInfo_SetLinkSource(p,a,b,c,d,e)     ICOM_CALL5(SetLinkSource,p,a,b,c,d,e)
#define IOleUILinkInfo_GetLinkSource(p,a,b,c,d,e,f,g) ICOM_CALL7(GetLinkSource,p,a,b,c,d,e,f,g)
#define IOleUILinkInfo_OpenLinkSource(p,a)            ICOM_CALL1(OpenLinkSource,p,a)
#define IOleUILinkInfo_UpdateLink(p,a,b,c)            ICOM_CALL3(UpdateLink,p,a,b,c)
#define IOleUILinkInfo_CancelLink(p,a)                ICOM_CALL1(CancelLink,p,a)
/*** IOleUILinkInfo methods ***/                                                                 
#define IOleUILinkInfo_GetLastUpdate(p,a,b)           ICOM_CALL2(GetLastUpdate,p,a,b)                        
#endif


/*****************************************************************************
 * IOleUIObjInfo interface
 */
#define ICOM_INTERFACE   IOleUIObjInfoA
#define IOleUIObjInfoA_METHODS \
    ICOM_METHOD6(HRESULT,GetObjectInfo,            DWORD,dwObject, DWORD*,lpdwObjSize, LPSTR*,lplpszLabel, \
                LPSTR*,lplpszType, LPSTR*,lplpszShortType, LPSTR*,lplpszLocation) \
    ICOM_METHOD6(HRESULT,GetConvertInfo,           DWORD,dwObject, CLSID*,lpClassID, WORD*,lpwFormat, \
                CLSID*,lpConvertDefaultClassID, LPCLSID*,lplpClsidExclude, UINT*,lpcClsidExclude) \
    ICOM_METHOD2(HRESULT,ConvertObject,            DWORD,dwObject,  REFCLSID,clsidNew) \
    ICOM_METHOD4(HRESULT,GetViewInfo,              DWORD,dwObject, \
                HGLOBAL*,phMetaPict,  DWORD*,pdvAspect,  INT*,pnCurrentScale) \
    ICOM_METHOD5(HRESULT,SetViewInfo,              DWORD,dwObject, \
                HGLOBAL,hMetaPict,  DWORD,dvAspect, \
                INT,nCurrentScale,  BOOL,bRelativeToOrig)
#define IOleUIObjInfoA_IMETHODS \
    IUnknown_IMETHODS \
    IOleUIObjInfoA_METHODS 
ICOM_DEFINE(IOleUIObjInfoA, IUnknown)
#undef ICOM_INTERFACE

#define ICOM_INTERFACE   IOleUIObjInfoW
#define IOleUIObjInfoW_METHODS \
    ICOM_METHOD6(HRESULT,GetObjectInfo,            DWORD,dwObject, DWORD*,lpdwObjSize, LPWSTR*,lplpszLabel, \
                LPWSTR*,lplpszType,  LPWSTR*,lplpszShortType,  LPWSTR*,lplpszLocation) \
    ICOM_METHOD6(HRESULT,GetConvertInfo,           DWORD,dwObject, CLSID*,lpClassID, WORD*,lpwFormat, \
                CLSID*,lpConvertDefaultClassID, LPCLSID*,lplpClsidExclude, UINT*,lpcClsidExclude) \
    ICOM_METHOD2(HRESULT,ConvertObject,            DWORD,dwObject,  REFCLSID,clsidNew) \
    ICOM_METHOD4(HRESULT,GetViewInfo,              DWORD,dwObject, \
                HGLOBAL*,phMetaPict,  DWORD*,pdvAspect,  INT*,pnCurrentScale) \
    ICOM_METHOD5(HRESULT,SetViewInfo,              DWORD,dwObject, \
                HGLOBAL,hMetaPict,  DWORD,dvAspect, \
                INT,nCurrentScale,  BOOL,bRelativeToOrig)
#define IOleUIObjInfoW_IMETHODS \
    IUnknown_IMETHODS \
    IOleUIObjInfoW_METHODS 
ICOM_DEFINE(IOleUIObjInfoW, IUnknown)
#undef ICOM_INTERFACE

DECL_WINELIB_TYPE_AW(IOleUIObjInfo)
DECL_WINELIB_TYPE_AW(POLEUIOBJINFO)
DECL_WINELIB_TYPE_AW(LPOLEUIOBJINFO)

#ifdef ICOM_CINTERFACE
/*** IUnknown methods ***/
#define IOleUIObjInfo_QueryInterface(p,a,b)            ICOM_CALL2(QueryInterface,p,a,b)
#define IOleUIObjInfo_AddRef(p)                        ICOM_CALL (AddRef,p)
#define IOleUIObjInfo_Release(p)                       ICOM_CALL (Release,p)
/*** IOleUIObjInfo methods ***/
#define IOleUIObjInfo_GetObjectInfo(p,a,b,c,d,e,f)     ICOM_CALL6(GetObjectInfo,p,a,b,c,d,e,f)
#define IOleUIObjInfo_GetConvertInfo(p,a,b,c,d,e,f)    ICOM_CALL6(GetConvertInfo,p,a,b,c,d,e,f)
#define IOleUIObjInfo_ConvertObject(p,a,b)             ICOM_CALL2(ConvertObject,p,a,b)
#define IOleUIObjInfo_GetViewInfo(p,a,b,c,d)           ICOM_CALL4(GetViewInfo,p,a,b,c,d)
#define IOleUIObjInfo_SetViewInfo(p,a,b,c,d,e)         ICOM_CALL5(SetViewInfo,p,a,b,c,d,e)
#endif        

UINT WINAPI  OleUIInsertObjectW(LPOLEUIINSERTOBJECTW);
UINT WINAPI  OleUIInsertObjectA(LPOLEUIINSERTOBJECTA);
#define      OleUIInsertObject WINELIB_NAME_AW(OleUIInsertObject)
UINT WINAPI  OleUIConvertA(LPOLEUICONVERTA);
UINT WINAPI  OleUIConvertW(LPOLEUICONVERTW);
#define      OleUIConvert WINELIB_NAME_AW(OleUIConvert)
UINT WINAPI  OleUIChangeIconA(LPOLEUICHANGEICONA);
UINT WINAPI  OleUIChangeIconW(LPOLEUICHANGEICONW);
#define      OleUIChangeIcon WINELIB_NAME_AW(OleUIChangeIcon)
UINT WINAPI  OleUIBusyA(LPOLEUIBUSYA);
UINT WINAPI  OleUIBusyW(LPOLEUIBUSYW);
#define      OleUIBusy WINELIB_NAME_AW(OleUIBusy)
UINT WINAPI  OleUIObjectPropertiesA(LPOLEUIOBJECTPROPSA);
UINT WINAPI  OleUIObjectPropertiesW(LPOLEUIOBJECTPROPSW);
#define      OleUIObjectProperties WINELIB_NAME_AW(OleUIObjectProperties)
UINT WINAPI  OleUIChangeSourceW(LPOLEUICHANGESOURCEW);
UINT WINAPI  OleUIChangeSourceA(LPOLEUICHANGESOURCEA);
#define      OleUIChangeSource WINELIB_NAME_AW(OleUIChangeSource)
UINT WINAPI  OleUIEditLinksA(LPOLEUIEDITLINKSA lpOleUIEditLinks);
UINT WINAPI  OleUIEditLinksW(LPOLEUIEDITLINKSW lpOleUIEditLinks);
#define      OleUIEditLinks WINELIB_NAME_AW(OleUIEditLinks)
BOOL WINAPI  OleUIUpdateLinksA(LPOLEUILINKCONTAINERA lpOleUILinkCntr, HWND hwndParent, LPSTR lpszTitle, INT cLinks);
BOOL WINAPI  OleUIUpdateLinksW(LPOLEUILINKCONTAINERW lpOleUILinkCntr, HWND hwndParent, LPWSTR lpszTitle, INT cLinks);
#define      OleUIUpdateLinks WINELIB_NAME_AW(OleUIUpdateLinks)
BOOL WINAPI  OleUIAddVerbMenuA(LPOLEOBJECT lpOleObj, LPCSTR lpszShortType, HMENU hMenu, UINT uPos, UINT uIDVerbMin, UINT uIDVerbMax,
  BOOL bAddConvert, UINT idConvert, HMENU *lphMenu);
BOOL WINAPI  OleUIAddVerbMenuW(LPOLEOBJECT lpOleObj, LPCWSTR lpszShortType, HMENU hMenu, UINT uPos, UINT uIDVerbMin, UINT uIDVerbMax,
  BOOL bAddConvert, UINT idConvert, HMENU *lphMenu);
#define      OleUIAddVerbMenu WINELIB_NAME_AW(OleUIAddVerbMenu)
UINT WINAPI  OleUIPasteSpecialA(LPOLEUIPASTESPECIALA lpOleUIPasteSpecial);
UINT WINAPI  OleUIPasteSpecialW(LPOLEUIPASTESPECIALW lpOleUIPasteSpecial);
#define      OleUIPasteSpecial WINELIB_NAME_AW(OleUIPasteSpecial) 


#ifdef __cplusplus
} /* Extern "C" */
#endif


#endif  /* __WINE_OLEDLG_H */





