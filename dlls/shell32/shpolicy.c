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
 * Up to date as of SHELL32 v5.00 (W2K)
 */

#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winerror.h"
#include "winreg.h"

#include "shell32_main.h"
#include "undocshell.h"
#include "wine/winuser16.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define SHELL_NO_POLICY 0xffffffff

typedef struct tagPOLICYDAT
{
  DWORD policy;          /* policy value passed to SHRestricted */
  LPCSTR appstr;         /* application str such as "Explorer" */
  LPCSTR keystr;         /* name of the actual registry key / policy */
  DWORD cache;           /* cached value or 0xffffffff for invalid */
} POLICYDATA, *LPPOLICYDATA;

/* registry strings */
static const CHAR strRegistryPolicyA[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Policies";
static const WCHAR strRegistryPolicyW[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o',
                                           's','o','f','t','\\','W','i','n','d','o','w','s','\\',
                                           'C','u','r','r','e','n','t','V','e','r','s','i','o','n',
                                           '\\','P','o','l','i','c','i','e','s',0};
static const CHAR strPolicyA[] = "Policy";
static const WCHAR strPolicyW[] = {'P','o','l','i','c','y',0};

/* application strings */

static const char strExplorer[] = {"Explorer"};
static const char strActiveDesk[] = {"ActiveDesktop"};
static const char strWinOldApp[] = {"WinOldApp"};
static const char strAddRemoveProgs[] = {"AddRemoveProgs"};

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
    0x800000,
    strExplorer,
    "SeparateProcess",
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
  },
  {
    0x40000022,
    strExplorer,
    "FindComputers",
    SHELL_NO_POLICY
  },
  {
    0x40000023,
    strExplorer,
    "IntelliMenus",
    SHELL_NO_POLICY
  },
  {
    0x40000024,
    strExplorer,
    "MemCheckBoxInRunDlg",
    SHELL_NO_POLICY
  },
  {
    0x40000025,
    strAddRemoveProgs,
    "ShowPostSetup",
    SHELL_NO_POLICY
  },
  {
    0x40000026,
    strExplorer,
    "NoSyncAll",
    SHELL_NO_POLICY
  },
  {
    0x40000027,
    strExplorer,
    "NoControlPanel",
    SHELL_NO_POLICY
  },
  {
    0x40000028,
    strExplorer,
    "EnumWorkgroup",
    SHELL_NO_POLICY
  },
  {
    0x40000029,
    strAddRemoveProgs,
    "NoAddRemovePrograms",
    SHELL_NO_POLICY
  },
  {
    0x4000002A,
    strAddRemoveProgs,
    "NoRemovePage",
    SHELL_NO_POLICY
  },
  {
    0x4000002B,
    strAddRemoveProgs,
    "NoAddPage",
    SHELL_NO_POLICY
  },
  {
    0x4000002C,
    strAddRemoveProgs,
    "NoWindowsSetupPage",
    SHELL_NO_POLICY
  },
  {
    0x4000002E,
    strExplorer,
    "NoChangeMappedDriveLabel",
    SHELL_NO_POLICY
  },
  {
    0x4000002F,
    strExplorer,
    "NoChangeMappedDriveComment",
    SHELL_NO_POLICY
  },
  {
    0x40000030,
    strExplorer,
    "MaxRecentDocs",
    SHELL_NO_POLICY
  },
  {
    0x40000031,
    strExplorer,
    "NoNetworkConnections",
    SHELL_NO_POLICY
  },
  {
    0x40000032,
    strExplorer,
    "ForceStartMenuLogoff",
    SHELL_NO_POLICY
  },
  {
    0x40000033,
    strExplorer,
     "NoWebView",
    SHELL_NO_POLICY
  },
  {
    0x40000034,
    strExplorer,
    "NoCustomizeThisFolder",
    SHELL_NO_POLICY
  },
  {
    0x40000035,
    strExplorer,
    "NoEncryption",
    SHELL_NO_POLICY
  },
  {
    0x40000036,
    strExplorer,
    "AllowFrenchEncryption",
    SHELL_NO_POLICY
  },
  {
    0x40000037,
    strExplorer,
    "DontShowSuperHidden",
    SHELL_NO_POLICY
  },
  {
    0x40000038,
    strExplorer,
    "NoShellSearchButton",
    SHELL_NO_POLICY
  },
  {
    0x40000039,
    strExplorer,
    "NoHardwareTab",
    SHELL_NO_POLICY
  },
  {
    0x4000003A,
    strExplorer,
    "NoRunasInstallPrompt",
    SHELL_NO_POLICY
  },
  {
    0x4000003B,
    strExplorer,
    "PromptRunasInstallNetPath",
    SHELL_NO_POLICY
  },
  {
    0x4000003C,
    strExplorer,
    "NoManageMyComputerVerb",
    SHELL_NO_POLICY
  },
  {
    0x4000003D,
    strExplorer,
    "NoRecentDocsNetHood",
    SHELL_NO_POLICY
  },
  {
    0x4000003E,
    strExplorer,
    "DisallowRun",
    SHELL_NO_POLICY
  },
  {
    0x4000003F,
    strExplorer,
    "NoWelcomeScreen",
    SHELL_NO_POLICY
  },
  {
    0x40000040,
    strExplorer,
    "RestrictCpl",
    SHELL_NO_POLICY
  },
  {
    0x40000041,
    strExplorer,
    "DisallowCpl",
    SHELL_NO_POLICY
  },
  {
    0x40000042,
    strExplorer,
    "NoSMBalloonTip",
    SHELL_NO_POLICY
  },
  {
    0x40000043,
    strExplorer,
    "NoSMHelp",
    SHELL_NO_POLICY
  },
  {
    0x40000044,
    strExplorer,
    "NoWinKeys",
    SHELL_NO_POLICY
  },
  {
    0x40000045,
    strExplorer,
    "NoEncryptOnMove",
    SHELL_NO_POLICY
  },
  {
    0x40000046,
    strExplorer,
    "DisableLocalMachineRun",
    SHELL_NO_POLICY
  },
  {
    0x40000047,
    strExplorer,
    "DisableCurrentUserRun",
    SHELL_NO_POLICY
  },
  {
    0x40000048,
    strExplorer,
    "DisableLocalMachineRunOnce",
    SHELL_NO_POLICY
  },
  {
    0x40000049,
    strExplorer,
    "DisableCurrentUserRunOnce",
    SHELL_NO_POLICY
  },
  {
    0x4000004A,
    strExplorer,
    "ForceActiveDesktopOn",
    SHELL_NO_POLICY
  },
  {
    0x4000004B,
    strExplorer,
    "NoComputersNearMe",
    SHELL_NO_POLICY
  },
  {
    0x4000004C,
    strExplorer,
    "NoViewOnDrive",
    SHELL_NO_POLICY
  },
  {
    0x4000004F,
    strExplorer,
    "NoSMMyDocs",
    SHELL_NO_POLICY
  },
  {
    0x40000061,
    strExplorer,
    "StartRunNoHOMEPATH",
    SHELL_NO_POLICY
  },
  {
    0x41000001,
    strExplorer,
    "NoDisconnect",
    SHELL_NO_POLICY
  },
  {
    0x41000002,
    strExplorer,
    "NoNTSecurity",
    SHELL_NO_POLICY
  },
  {
    0x41000003,
    strExplorer,
    "NoFileAssociate",
    SHELL_NO_POLICY
  },
  {
    0,
    0,
    0,
    SHELL_NO_POLICY
	}
};

/*************************************************************************
 * SHRestricted				 [SHELL32.100]
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
DWORD WINAPI SHRestricted (DWORD policy)
{
	char regstr[256];
	HKEY    xhkey;
	DWORD   retval, datsize = 4;
	LPPOLICYDATA p;

	TRACE("(%08lx)\n", policy);

	/* scan to see if we know this policy ID */
	for (p = sh32_policy_table; p->policy; p++)
	{
	  if (policy == p->policy)
	  {
	    break;
	  }
	}

	if (p->policy == 0)
	{
	    /* we don't know this policy, return 0 */
	    TRACE("unknown policy: (%08lx)\n", policy);
		return 0;
	}

	/* we have a known policy */

	/* first check if this policy has been cached, return it if so */
	if (p->cache != SHELL_NO_POLICY)
	{
	    return p->cache;
	}

	lstrcpyA(regstr, strRegistryPolicyA);
	lstrcatA(regstr, p->appstr);

	/* return 0 and don't set the cache if any registry errors occur */
	retval = 0;
	if (RegOpenKeyA(HKEY_CURRENT_USER, regstr, &xhkey) == ERROR_SUCCESS)
	{
	  if (RegQueryValueExA(xhkey, p->keystr, NULL, NULL, (LPBYTE)&retval, &datsize) == ERROR_SUCCESS)
	  {
	    p->cache = retval;
	  }
	  RegCloseKey(xhkey);
	}
	return retval;
}

/*************************************************************************
 * SHInitRestricted          [SHELL32.244]
 *
 * Initialise the policy cache to speed up calls to SHRestricted().
 *
 * PARAMS
 *  unused    [I] Reserved.
 *  inpRegKey [I] Registry key to scan.
 *
 * RETURNS
 *  Success: -1. The policy cache is initialised.
 *  Failure: 0, if inpRegKey is any value other than NULL, "Policy", or
 *           "Software\Microsoft\Windows\CurrentVersion\Policies".
 *
 * NOTES
 *  Exported by ordinal. Introduced in Win98.
 */
BOOL WINAPI SHInitRestricted(LPCVOID unused, LPCVOID inpRegKey)
{
	LPPOLICYDATA p;

	TRACE("(%p, %p)\n", unused, inpRegKey);

	/* first check - if input is non-NULL and points to the secret
	   key string, then pass.  Otherwise return 0.
	 */
	if (inpRegKey != NULL)
	{
	  if (SHELL_OsIsUnicode())
	  {
	    if (lstrcmpiA((LPSTR)inpRegKey, strRegistryPolicyA) ||
	        lstrcmpiA((LPSTR)inpRegKey, strPolicyA))
	      /* doesn't match, fail */
	      return 0;
	  }
	  else
	  {
	    if (lstrcmpiW((LPWSTR)inpRegKey, strRegistryPolicyW) ||
	        lstrcmpiW((LPWSTR)inpRegKey, strPolicyW))
	      /* doesn't match, fail */
	      return 0;
	  }
	}

	/* check passed, init all policy cache entries with SHELL_NO_POLICY */
	for (p = sh32_policy_table; p->policy; p++)
	{
	  p->cache = SHELL_NO_POLICY;
	}
	return SHELL_NO_POLICY;
}
