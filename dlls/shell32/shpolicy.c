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

#include "windef.h"
#include "wingdi.h"
#include "winerror.h"
#include "winreg.h"
#include "debugtools.h"
#include "wine/winuser16.h"

DEFAULT_DEBUG_CHANNEL(shell);

#define SHELL_MAX_POLICIES 57

#define SHELL_NO_POLICY 0xffffffff

typedef struct tagPOLICYDAT
{
  DWORD polflags;        /* flags value passed to SHRestricted */
  LPSTR appstr;          /* application str such as "Explorer" */
  LPSTR keystr;          /* name of the actual registry key / policy */
  DWORD cache;           /* cached value or 0xffffffff for invalid */
} POLICYDATA, *LPPOLICYDATA;

//extern POLICYDATA sh32_policy_table[SHELL_MAX_POLICIES];

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

/*************************************************************************
 * SHRestricted				[SHELL32.100]
 *
 * walks through policy table, queries <app> key, <type> value, returns 
 * queried (DWORD) value, and caches it between called to SHInitRestricted
 * to prevent unnecessary registry access.
 *
 * NOTES
 *     exported by ordinal
 *
 * REFERENCES: 
 *     MS System Policy Editor
 *     98Lite 2.0 (which uses many of these policy keys) http://www.98lite.net/
 *     "The Windows 95 Registry", by John Woram, 1996 MIS: Press
 */
DWORD WINAPI SHRestricted (DWORD pol) {
        char regstr[256];
	HKEY	xhkey;
	DWORD   retval, polidx, i, datsize = 4;

	TRACE("(%08lx)\n",pol);

	polidx = -1;

	/* scan to see if we know this policy ID */
	for (i = 0; i < SHELL_MAX_POLICIES; i++)
	{
	     if (pol == sh32_policy_table[i].polflags)
	     {
	         polidx = i;
		 break;
	     }
	}

	if (polidx == -1)
	{
	    /* we don't know this policy, return 0 */
	    TRACE("unknown policy: (%08lx)\n", pol);
		return 0;
	}

	/* we have a known policy */
      	strcpy(regstr, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\");
	strcat(regstr, sh32_policy_table[polidx].appstr);

	/* first check if this policy has been cached, return it if so */
	if (sh32_policy_table[polidx].cache != SHELL_NO_POLICY)
	{
	    return sh32_policy_table[polidx].cache;
	}

	/* return 0 and don't set the cache if any registry errors occur */
	retval = 0;
	if (RegOpenKeyA(HKEY_CURRENT_USER, regstr, &xhkey) == ERROR_SUCCESS)
	{
	    if (RegQueryValueExA(xhkey, sh32_policy_table[polidx].keystr, NULL, NULL, (LPBYTE)&retval, &datsize) == ERROR_SUCCESS)
	    {
	        sh32_policy_table[polidx].cache = retval;
	    }

	RegCloseKey(xhkey);
}

	return retval;
}

/*************************************************************************
 *      SHInitRestricted                         [SHELL32.244]
 *
 * Win98+ by-ordinal only routine called by Explorer and MSIE 4 and 5.
 * Inits the policy cache used by SHRestricted to avoid excess
 * registry access.
 *
 * INPUTS
 * Two inputs: one is a string or NULL.  If non-NULL the pointer
 * should point to a string containing the following exact text:
 * "Software\Microsoft\Windows\CurrentVersion\Policies".
 * The other input is unused.
 *
 * NOTES
 * If the input is non-NULL and does not point to a string containing
 * that exact text the routine will do nothing.
 *
 * If the text does match or the pointer is NULL, then the routine
 * will init SHRestricted()'s policy cache to all 0xffffffff and
 * returns 0xffffffff as well.
 *
 * I haven't yet run into anything calling this with inputs other than
 * (NULL, NULL), so I may have the inputs reversed.
 */

BOOL WINAPI SHInitRestricted(LPSTR inpRegKey, LPSTR parm2)
{
     int i;

     TRACE("(%p, %p)\n", inpRegKey, parm2);

     /* first check - if input is non-NULL and points to the secret
        key string, then pass.  Otherwise return 0.
     */

     if (inpRegKey != (LPSTR)NULL)
     {
         if (lstrcmpiA(inpRegKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies"))
	 {
	     /* doesn't match, fail */
	     return 0;
	 }
     }                               

     /* check passed, init all policy cache entries with SHELL_NO_POLICY */
     for (i = 0; i < SHELL_MAX_POLICIES; i++)
     {
          sh32_policy_table[i].cache = SHELL_NO_POLICY;
     }

     return SHELL_NO_POLICY;
}
