/*
 * shpolicy.c - Data for shell/system policies.
 *
 * Copyright 1999 Ian Schmidt <ischmidt@cfl.rr.com>
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
 * NOTES:
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
#include "winerror.h"
#include "winreg.h"

#include "undocshell.h"
#include "wine/winuser16.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define SHELL_MAX_POLICIES 57

#define SHELL_NO_POLICY 0xffffffff

typedef struct tagPOLICYDAT
{
  DWORD polflags;        /* flags value passed to SHRestricted */
  LPCSTR appstr;         /* application str such as "Explorer" */
  LPCSTR keystr;         /* name of the actual registry key / policy */
  DWORD cache;           /* cached value or 0xffffffff for invalid */
} POLICYDATA, *LPPOLICYDATA;

#if 0
extern POLICYDATA sh32_policy_table[SHELL_MAX_POLICIES];
#endif

/* application strings */

static const char strExplorer[] = {"Explorer"};
static const char strActiveDesk[] = {"ActiveDesktop"};
static const char strWinOldApp[] = {"WinOldApp"};

/* key strings */

static const char strNoFileURL[] = {"NoFileUrl"};
static const char strNoFolderOptions[] = {"NoFolderOptions"};
static const char strNoChangeStartMenu[] = {"NoChangeStartMenu"};
static const char strNoWindowsUpdate[] = {"NoWindowsUpdate"};
static const char strNoSetActiveDesktop[] = {"NoSetActiveDesktop"};
static const char strNoForgetSoftwareUpdate[] = {"NoForgetSoftwareUpdate"};
static const char strNoMSAppLogo[] = {"NoMSAppLogo5ChannelNotify"};
static const char strForceCopyACLW[] = {"ForceCopyACLWithFile"};
static const char strNoResolveTrk[] = {"NoResolveTrack"};
static const char strNoResolveSearch[] = {"NoResolveSearch"};
static const char strNoEditComponent[] = {"NoEditingComponents"};
static const char strNoMovingBand[] = {"NoMovingBands"};
static const char strNoCloseDragDrop[] = {"NoCloseDragDropBands"};
static const char strNoCloseComponent[] = {"NoClosingComponents"};
static const char strNoDelComponent[] = {"NoDeletingComponents"};
static const char strNoAddComponent[] = {"NoAddingComponents"};
static const char strNoComponent[] = {"NoComponents"};
static const char strNoChangeWallpaper[] = {"NoChangingWallpaper"};
static const char strNoHTMLWallpaper[] = {"NoHTMLWallpaper"};
static const char strNoCustomWebView[] = {"NoCustomizeWebView"};
static const char strClassicShell[] = {"ClassicShell"};
static const char strClearRecentDocs[] = {"ClearRecentDocsOnExit"};
static const char strNoFavoritesMenu[] = {"NoFavoritesMenu"};
static const char strNoActiveDesktopChanges[] = {"NoActiveDesktopChanges"};
static const char strNoActiveDesktop[] = {"NoActiveDesktop"};
static const char strNoRecentDocMenu[] = {"NoRecentDocsMenu"};
static const char strNoRecentDocHistory[] = {"NoRecentDocsHistory"};
static const char strNoInetIcon[] = {"NoInternetIcon"};
static const char strNoStngsWizard[] = {"NoSettingsWizards"};
static const char strNoLogoff[] = {"NoLogoff"};
static const char strNoNetConDis[] = {"NoNetConnectDisconnect"};
static const char strNoContextMenu[] = {"NoViewContextMenu"};
static const char strNoTryContextMenu[] = {"NoTrayContextMenu"};
static const char strNoWebMenu[] = {"NoWebMenu"};
static const char strLnkResolveIgnoreLnkInfo[] = {"LinkResolveIgnoreLinkInfo"};
static const char strNoCommonGroups[] = {"NoCommonGroups"};
static const char strEnforceShlExtSecurity[] = {"EnforceShellExtensionSecurity"};
static const char strNoRealMode[] = {"NoRealMode"};
static const char strMyDocsOnNet[] = {"MyDocsOnNet"};
static const char strNoStartMenuSubfolder[] = {"NoStartMenuSubFolders"};
static const char strNoAddPrinters[] = {"NoAddPrinter"};
static const char strNoDeletePrinters[] = {"NoDeletePrinter"};
static const char strNoPrintTab[] = {"NoPrinterTabs"};
static const char strRestrictRun[] = {"RestrictRun"};
static const char strNoStartBanner[] = {"NoStartBanner"};
static const char strNoNetworkNeighborhood[] = {"NoNetHood"};
static const char strNoDriveTypeAtRun[] = {"NoDriveTypeAutoRun"};
static const char strNoDrivesAutoRun[] = {"NoDriveAutoRun"};
static const char strNoDrives[] = {"NoDrives"};
static const char strNoFind[] = {"NoFind"};
static const char strNoDesktop[] = {"NoDesktop"};
static const char strNoSetTaskBar[] = {"NoSetTaskbar"};
static const char strNoSetFld[] = {"NoSetFolders"};
static const char strNoFileMenu[] = {"NoFileMenu"};
static const char strNoSavSetng[] = {"NoSaveSettings"};
static const char strNoClose[] = {"NoClose"};
static const char strNoRun[] = {"NoRun"};

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
 * Get the value associated with a policy Id.
 *
 * PARAMS
 *     pol [I] Policy Id
 *
 * RETURNS
 *     The queried value for the policy.
 *
 * NOTES
 *     Exported by ordinal.
 *     This function caches the retrieved values to prevent unnecessary registry access,
 *     if SHInitRestricted() was previously called.
 *
 * REFERENCES
 *     a: MS System Policy Editor.
 *     b: 98Lite 2.0 (which uses many of these policy keys) http://www.98lite.net/
 *     c: 'The Windows 95 Registry', by John Woram, 1996 MIS: Press
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
 * Initialise the policy cache to speed up calls to SHRestricted().
 *
 * PARAMS
 *  inpRegKey [I] Registry key to scan.
 *  parm2     [I] Reserved.
 *
 * RETURNS
 *  Success: -1. The policy cache is initialsed.
 *  Failure: 0, if inpRegKey is any value other than NULL or
 *           "Software\Microsoft\Windows\CurrentVersion\Policies".
 *
 * NOTES
 *  Exported by ordinal. Introduced in Win98.
 *
 * FIXME
 *  I haven't yet run into anything calling this with inputs other than
 *  (NULL, NULL), so I may have the parameters reversed.
 */
BOOL WINAPI SHInitRestricted(LPSTR inpRegKey, LPSTR parm2)
{
     int i;

     TRACE("(%p, %p)\n", inpRegKey, parm2);

     /* first check - if input is non-NULL and points to the secret
        key string, then pass.  Otherwise return 0.
     */

     if (inpRegKey != NULL)
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
