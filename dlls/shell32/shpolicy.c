/*
 * shpolicy.c - Data for shell/system policies.
 *
 * Created 1999 by Ian Schmidt <ischmidt@cfl.rr.com>
 *
 * Some of these policies can be tweaked via the System Policy
 * Editor which came with the Win95 Migration Guide, although
 * there doesn't appear to be an updated Win98 version that
 * would handle the many new policies introduced since then.
 * You could easily write one with the information in 
 * this file...
 * 
 * Up to date as of SHELL32 v4.72 (Win98, Win95 with MSIE 5)
 */

#include <stdlib.h>
#include <string.h>

#include "wine/winuser16.h"
#include "shpolicy.h"

/* application strings */

static char strExplorer[] = {"Explorer"};
static char strActiveDesk[] = {"ActiveDesktop"};
static char strWinOldApp[] = {"WinOldApp"};

/* key strings */

static char strNoFileURL[] = {"NoFileUrl"};
static char strNoFolderOptions[] = {"NoFolderOptions"};
static char strNoChangeStartMenu[] = {"NoChangeStartMenu"};
static char strNoWindowsUpdate[] = {"NoWindowsUpdate"};
static char strNoSetActiveDesktop[] = {"NoSetActiveDesktop"};
static char strNoForgetSoftwareUpdate[] = {"NoForgetSoftwareUpdate"};
static char strNoMSAppLogo[] = {"NoMSAppLogo5ChannelNotify"};
static char strForceCopyACLW[] = {"ForceCopyACLWithFile"};
static char strNoResolveTrk[] = {"NoResolveTrack"};
static char strNoResolveSearch[] = {"NoResolveSearch"};
static char strNoEditComponent[] = {"NoEditingComponents"};
static char strNoMovingBand[] = {"NoMovingBands"};
static char strNoCloseDragDrop[] = {"NoCloseDragDropBands"};
static char strNoCloseComponent[] = {"NoClosingComponents"};
static char strNoDelComponent[] = {"NoDeletingComponents"};
static char strNoAddComponent[] = {"NoAddingComponents"};
static char strNoComponent[] = {"NoComponents"};
static char strNoChangeWallpaper[] = {"NoChangingWallpaper"};
static char strNoHTMLWallpaper[] = {"NoHTMLWallpaper"};
static char strNoCustomWebView[] = {"NoCustomizeWebView"};
static char strClassicShell[] = {"ClassicShell"};
static char strClearRecentDocs[] = {"ClearRecentDocsOnExit"};
static char strNoFavoritesMenu[] = {"NoFavoritesMenu"};
static char strNoActiveDesktopChanges[] = {"NoActiveDesktopChanges"};
static char strNoActiveDesktop[] = {"NoActiveDesktop"};
static char strNoRecentDocMenu[] = {"NoRecentDocsMenu"};
static char strNoRecentDocHistory[] = {"NoRecentDocsHistory"};
static char strNoInetIcon[] = {"NoInternetIcon"};
static char strNoStngsWizard[] = {"NoSettingsWizards"};
static char strNoLogoff[] = {"NoLogoff"};
static char strNoNetConDis[] = {"NoNetConnectDisconnect"};
static char strNoContextMenu[] = {"NoViewContextMenu"};
static char strNoTryContextMenu[] = {"NoTrayContextMenu"};
static char strNoWebMenu[] = {"NoWebMenu"};
static char strLnkResolveIgnoreLnkInfo[] = {"LinkResolveIgnoreLinkInfo"};
static char strNoCommonGroups[] = {"NoCommonGroups"};
static char strEnforceShlExtSecurity[] = {"EnforceShellExtensionSecurity"};
static char strNoRealMode[] = {"NoRealMode"};
static char strMyDocsOnNet[] = {"MyDocsOnNet"};
static char strNoStartMenuSubfolder[] = {"NoStartMenuSubFolders"};
static char strNoAddPrinters[] = {"NoAddPrinter"};
static char strNoDeletePrinters[] = {"NoDeletePrinter"};
static char strNoPrintTab[] = {"NoPrinterTabs"};
static char strRestrictRun[] = {"RestrictRun"};
static char strNoStartBanner[] = {"NoStartBanner"};
static char strNoNetworkNeighborhood[] = {"NoNetHood"};
static char strNoDriveTypeAtRun[] = {"NoDriveTypeAutoRun"};
static char strNoDrivesAutoRun[] = {"NoDriveAutoRun"};
static char strNoDrives[] = {"NoDrives"};
static char strNoFind[] = {"NoFind"};
static char strNoDesktop[] = {"NoDesktop"};
static char strNoSetTaskBar[] = {"NoSetTaskbar"};
static char strNoSetFld[] = {"NoSetFolders"};
static char strNoFileMenu[] = {"NoFileMenu"};
static char strNoSavSetng[] = {"NoSaveSettings"};
static char strNoClose[] = {"NoClose"};
static char strNoRun[] = {"NoRun"};

/* policy data array */

POLICYDATA sh32_policy_table[] = 
{
  {
    0x1,
    strExplorer,
    strNoRun,
    SHELL_NO_POLICY
  },
  {
    0x2,
    strExplorer,
    strNoClose,
    SHELL_NO_POLICY
  },
  {
    0x4,
    strExplorer,
    strNoSavSetng,
    SHELL_NO_POLICY
  },
  {
    0x8,
    strExplorer,
    strNoFileMenu,
    SHELL_NO_POLICY
  },
  {
    0x10,
    strExplorer,
    strNoSetFld,
    SHELL_NO_POLICY
  },
  {
    0x20,
    strExplorer,
    strNoSetTaskBar,
    SHELL_NO_POLICY
  },
  {
    0x40,
    strExplorer,
    strNoDesktop,
    SHELL_NO_POLICY
  },
  {
    0x80,
    strExplorer,
    strNoFind,
    SHELL_NO_POLICY
  },
  {
    0x100,
    strExplorer,
    strNoDrives,
    SHELL_NO_POLICY
  },
  {
    0x200,
    strExplorer,
    strNoDrivesAutoRun,
    SHELL_NO_POLICY
  },
  {
    0x400,
    strExplorer,
    strNoDriveTypeAtRun,
    SHELL_NO_POLICY
  },
  {
    0x800,
    strExplorer,
    strNoNetworkNeighborhood,
    SHELL_NO_POLICY
  },
  {
    0x1000,
    strExplorer,
    strNoStartBanner,
    SHELL_NO_POLICY
  },
  {
    0x2000,
    strExplorer,
    strRestrictRun,
    SHELL_NO_POLICY
  },
  {
    0x4000,
    strExplorer,
    strNoPrintTab,
    SHELL_NO_POLICY
  },
  {
    0x8000,
    strExplorer,
    strNoDeletePrinters,
    SHELL_NO_POLICY
  },
  {
    0x10000,
    strExplorer,
    strNoAddPrinters,
    SHELL_NO_POLICY
  },
  {
    0x20000,
    strExplorer,
    strNoStartMenuSubfolder,
    SHELL_NO_POLICY
  },
  {
    0x40000,
    strExplorer,
    strMyDocsOnNet,
    SHELL_NO_POLICY
  },
  {
    0x80000,
    strWinOldApp,
    strNoRealMode,
    SHELL_NO_POLICY
  },
  {
    0x100000,
    strExplorer,
    strEnforceShlExtSecurity,
    SHELL_NO_POLICY
  },
  {
    0x200000,
    strExplorer,
    strLnkResolveIgnoreLnkInfo,
    SHELL_NO_POLICY
  },
  {
    0x400000,
    strExplorer,
    strNoCommonGroups,
    SHELL_NO_POLICY
  },
  {
    0x1000000,
    strExplorer,
    strNoWebMenu,
    SHELL_NO_POLICY
  },
  {
    0x2000000,
    strExplorer,
    strNoTryContextMenu,
    SHELL_NO_POLICY
  },
  {
    0x4000000,
    strExplorer,
    strNoContextMenu,
    SHELL_NO_POLICY
  },
  {
    0x8000000,
    strExplorer,
    strNoNetConDis,
    SHELL_NO_POLICY
  },
  {
    0x10000000,
    strExplorer,
    strNoLogoff,
    SHELL_NO_POLICY
  },
  {
    0x20000000,
    strExplorer,
    strNoStngsWizard,
    SHELL_NO_POLICY
  },
  {
    0x40000001,
    strExplorer,
    strNoInetIcon,
    SHELL_NO_POLICY
  },
  {
    0x40000002,
    strExplorer,
    strNoRecentDocHistory,
    SHELL_NO_POLICY
  },
  {
    0x40000003,
    strExplorer,
    strNoRecentDocMenu,
    SHELL_NO_POLICY
  },
  {
    0x40000004,
    strExplorer,
    strNoActiveDesktop,
    SHELL_NO_POLICY
  },
  {
    0x40000005,
    strExplorer,
    strNoActiveDesktopChanges,
    SHELL_NO_POLICY
  },
  {
    0x40000006,
    strExplorer,
    strNoFavoritesMenu,
    SHELL_NO_POLICY
  },
  {
    0x40000007,
    strExplorer,
    strClearRecentDocs,
    SHELL_NO_POLICY
  },
  {
    0x40000008,
    strExplorer,
    strClassicShell,
    SHELL_NO_POLICY
  },
  {
    0x40000009,
    strExplorer,
    strNoCustomWebView,
    SHELL_NO_POLICY
  },
  {
    0x40000010,
    strActiveDesk,
    strNoHTMLWallpaper,
    SHELL_NO_POLICY
  },
  {
    0x40000011,
    strActiveDesk,
    strNoChangeWallpaper,
    SHELL_NO_POLICY
  },
  {
    0x40000012,
    strActiveDesk,
    strNoComponent,
    SHELL_NO_POLICY
  },
  {
    0x40000013,
    strActiveDesk,
    strNoAddComponent,
    SHELL_NO_POLICY
  },
  {
    0x40000014,
    strActiveDesk,
    strNoDelComponent,
    SHELL_NO_POLICY
  },
  {
    0x40000015,
    strActiveDesk,
    strNoCloseComponent,
    SHELL_NO_POLICY
  },
  {
    0x40000016,
    strActiveDesk,
    strNoCloseDragDrop,
    SHELL_NO_POLICY
  },
  {
    0x40000017,
    strActiveDesk,
    strNoMovingBand,
    SHELL_NO_POLICY
  },
  {
    0x40000018,
    strActiveDesk,
    strNoEditComponent,
    SHELL_NO_POLICY
  },
  {
    0x40000019,
    strExplorer,
    strNoResolveSearch,
    SHELL_NO_POLICY
  },
  {
    0x4000001a,
    strExplorer,
    strNoResolveTrk,
    SHELL_NO_POLICY
  },
  {
    0x4000001b,
    strExplorer,
    strForceCopyACLW,
    SHELL_NO_POLICY
  },
  {
    0x4000001c,
    strExplorer,
    strNoMSAppLogo,
    SHELL_NO_POLICY
  },
  {
    0x4000001d,
    strExplorer,
    strNoForgetSoftwareUpdate,
    SHELL_NO_POLICY
  },
  {
    0x4000001e,
    strExplorer,
    strNoSetActiveDesktop,
    SHELL_NO_POLICY
  },
  {
    0x4000001f,
    strExplorer,
    strNoWindowsUpdate,
    SHELL_NO_POLICY
  },
  {
    0x40000020,
    strExplorer,
    strNoChangeStartMenu,
    SHELL_NO_POLICY
  },
  {
    0x40000021,
    strExplorer,
    strNoFolderOptions,
    SHELL_NO_POLICY
  },
  {
    0x50000024,
    strExplorer,
    strNoFileURL,
    SHELL_NO_POLICY
  }
};
