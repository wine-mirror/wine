#ifndef __WINE_SHLGUID_H
#define __WINE_SHLGUID_H

#include "wine/obj_base.h"

DEFINE_SHLGUID(CLSID_ShellDesktop,      0x00021400L, 0, 0);
DEFINE_SHLGUID(CLSID_ShellLink,         0x00021401L, 0, 0);

/* shell32 formatids */
DEFINE_SHLGUID(FMTID_Intshcut,          0x000214A0L, 0, 0);
DEFINE_SHLGUID(FMTID_InternetSite,      0x000214A1L, 0, 0);
/* command group ids */
DEFINE_SHLGUID(CGID_Explorer,           0x000214D0L, 0, 0);
DEFINE_SHLGUID(CGID_ShellDocView,       0x000214D1L, 0, 0);

 /* shell32interface ids */
DEFINE_SHLGUID(IID_INewShortcutHookA,   0x000214E1L, 0, 0);
DEFINE_SHLGUID(IID_IShellIcon,          0x000214E5L, 0, 0);
DEFINE_SHLGUID(IID_IShellPropSheetExt,  0x000214E9L, 0, 0);
DEFINE_SHLGUID(IID_IShellCopyHook,      0x000214EFL, 0, 0);
DEFINE_SHLGUID(IID_IFileViewer,         0x000214F0L, 0, 0);
DEFINE_SHLGUID(IID_IFileViewerSite,     0x000214F3L, 0, 0);
DEFINE_SHLGUID(IID_IShellExecuteHookA,  0x000214F5L, 0, 0);
DEFINE_SHLGUID(IID_IPropSheetPage,      0x000214F6L, 0, 0);
DEFINE_SHLGUID(IID_INewShortcutHookW,   0x000214F7L, 0, 0);
DEFINE_SHLGUID(IID_IFileViewerW,        0x000214F8L, 0, 0);
DEFINE_SHLGUID(IID_IShellExecuteHookW,  0x000214FBL, 0, 0);
DEFINE_SHLGUID(IID_IShellCopyHookW,     0x000214FCL, 0, 0);

DEFINE_GUID (IID_IDockingWindow,	0x012dd920L, 0x7B26, 0x11D0, 0x8C, 0xA9, 0x00, 0xA0, 0xC9, 0x2D, 0xBF, 0xE8);
DEFINE_GUID (IID_IDockingWindowSite,	0x2A342FC2L, 0x7B26, 0x11D0, 0x8C, 0xA9, 0x00, 0xA0, 0xC9, 0x2D, 0xBF, 0xE8);

/****************************************************************************
*  undocumented stuff
*/
/* the next IID's are the namespace elements of the pidls */
DEFINE_GUID (IID_MyComputer,	0x20D04FE0L, 0x3AEA, 0x1069, 0xA2, 0xD8, 0x08, 0x00, 0x2B, 0x30, 0x30, 0x9D);
DEFINE_GUID (IID_IExplore,	0x871C5380L, 0x42A0, 0x1069, 0xA2, 0xEA, 0x08, 0x00, 0x2B, 0x30, 0x30, 0x9D);
DEFINE_GUID (IID_Control,	0x23EC2020L, 0x3AEA, 0x1069, 0xA2, 0xDD, 0x08, 0x00, 0x2B, 0x30, 0x30, 0x9D);
DEFINE_GUID (IID_Printer,	0x2227A280L, 0x3AEA, 0x1069, 0xA2, 0xDE, 0x08, 0x00, 0x2B, 0x30, 0x30, 0x9D);
DEFINE_GUID (IID_Network,	0x208D2C60L, 0x3AEA, 0x1069, 0xA2, 0xD7, 0x08, 0x00, 0x2B, 0x30, 0x30, 0x9D);
DEFINE_GUID (IID_BitBucket,	0x645FF040L, 0x5081, 0x101B, 0x9F, 0x08, 0x00, 0xAA, 0x00, 0x2F, 0x95, 0x4E);

DEFINE_GUID (CLSID_PaperBin,	0x645FF040L, 0x5081, 0x101B, 0x9F, 0x08, 0x00, 0xAA, 0x00, 0x2F, 0x95, 0x4E);
#endif /* __WINE_SHLGUID_H */
