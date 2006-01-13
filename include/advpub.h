/*
 * Copyright 2004 Huw D M Davies
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
 */
#ifndef __WINE_ADVPUB_H
#define __WINE_ADVPUB_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _CabInfo {
    PSTR  pszCab;
    PSTR  pszInf;
    PSTR  pszSection;
    char  szSrcPath[MAX_PATH];
    DWORD dwFlags;
} CABINFO, *PCABINFO;

typedef struct _StrEntry {
    LPSTR pszName;
    LPSTR pszValue;
} STRENTRY, *LPSTRENTRY;

typedef const STRENTRY CSTRENTRY;
typedef CSTRENTRY *LPCSTRENTRY;

typedef struct _StrTable {
    DWORD cEntries;
    LPSTRENTRY pse;
} STRTABLE, *LPSTRTABLE;

typedef const STRTABLE CSTRTABLE;
typedef CSTRTABLE *LPCSTRTABLE;

/* Flags for AdvInstallFile */
#define AIF_WARNIFSKIP              0x00000001
#define AIF_NOSKIP                  0x00000002
#define AIF_NOVERSIONCHECK          0x00000004
#define AIF_FORCE_FILE_IN_USE       0x00000008
#define AIF_NOOVERWRITE             0x00000010
#define AIF_NO_VERSION_DIALOG       0x00000020
#define AIF_REPLACEONLY             0x00000400
#define AIF_NOLANGUAGECHECK         0x10000000
#define AIF_QUIET                   0x20000000

/* Flags for RunSetupCommand */
#define RSC_FLAG_INF                0x00000001
#define RSC_FLAG_SKIPDISKSPACECHECK 0x00000002
#define RSC_FLAG_QUIET              0x00000004
#define RSC_FLAG_NGCONV             0x00000008
#define RSC_FLAG_UPDHLPDLLS         0x00000010
#define RSC_FLAG_DELAYREGISTEROCX   0x00000200
#define RSC_FLAG_SETUPAPI           0x00000400

/* Flags for DelNode */
#define ADN_DEL_IF_EMPTY            0x00000001
#define ADN_DONT_DEL_SUBDIRS        0x00000002
#define ADN_DONT_DEL_DIR            0x00000004
#define ADN_DEL_UNC_PATHS           0x00000008

/* Flags for RegRestoreAll, RegSaveRestore, RegSaveRestoreOnINF */
#define  IE4_RESTORE                0x00000001
#define  IE4_BACKNEW                0x00000002
#define  IE4_NODELETENEW            0x00000004
#define  IE4_NOMESSAGES             0x00000008
#define  IE4_NOPROGRESS             0x00000010
#define  IE4_NOENUMKEY              0x00000020
#define  IE4_NO_CRC_MAPPING         0x00000040
#define  IE4_REGSECTION             0x00000080
#define  IE4_FRDOALL                0x00000100
#define  IE4_UPDREFCNT              0x00000200
#define  IE4_USEREFCNT              0x00000400
#define  IE4_EXTRAINCREFCNT         0x00000800

/* Flags for file save and restore functions */
#define  AFSR_RESTORE               IE4_RESTORE
#define  AFSR_BACKNEW               IE4_BACKNEW
#define  AFSR_NODELETENEW           IE4_NODELETENEW
#define  AFSR_NOMESSAGES            IE4_NOMESSAGES
#define  AFSR_NOPROGRESS            IE4_NOPROGRESS
#define  AFSR_UPDREFCNT             IE4_UPDREFCNT
#define  AFSR_USEREFCNT             IE4_USEREFCNT
#define  AFSR_EXTRAINCREFCNT        IE4_EXTRAINCREFCNT

HRESULT WINAPI AdvInstallFile(HWND hwnd, LPCSTR lpszSourceDir,
     LPCSTR lpszSourceFile, LPCSTR lpszDestDir, LPCSTR lpszDestFile,
     DWORD dwFlags, DWORD dwReserved);
HRESULT WINAPI DelNode(LPCSTR pszFileOrDirName, DWORD dwFlags);
HRESULT WINAPI DelNodeRunDLL32(HWND,HINSTANCE,LPSTR,INT);
HRESULT WINAPI ExecuteCab( HWND hwnd, PCABINFO pCab, LPVOID pReserved );
HRESULT WINAPI ExtractFiles(LPCSTR,LPCSTR,DWORD,LPCSTR,LPVOID,DWORD);
HRESULT WINAPI FileSaveMarkNotExist(LPSTR pszFileList, LPSTR pszDir, LPSTR pszBaseName);
HRESULT WINAPI FileSaveRestore(HWND hDlg, LPSTR pszFileList, LPSTR pszDir,
     LPSTR pszBaseName, DWORD dwFlags);
HRESULT WINAPI FileSaveRestoreOnINF(HWND hWnd, PCSTR pszTitle, PCSTR pszINF,
     PCSTR pszSection, PCSTR pszBackupDir, PCSTR pszBaseBackupFile, DWORD dwFlags);
HRESULT WINAPI GetVersionFromFile(LPSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer, BOOL bVersion);
HRESULT WINAPI GetVersionFromFileEx(LPSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer, BOOL bVersion);
BOOL WINAPI IsNTAdmin(DWORD,LPDWORD);
INT WINAPI LaunchINFSection(HWND,HINSTANCE,LPSTR,INT);
HRESULT WINAPI LaunchINFSectionEx(HWND,HINSTANCE,LPSTR,INT);
DWORD WINAPI NeedRebootInit(VOID);
BOOL WINAPI NeedReboot(DWORD dwRebootCheck);
HRESULT WINAPI RegInstall(HMODULE hm, LPCSTR pszSection, LPCSTRTABLE pstTable);
HRESULT WINAPI RegRestoreAll(HWND hWnd, PSTR pszTitleString, HKEY hkBackupKey);
HRESULT WINAPI RegSaveRestore(HWND hWnd, PCSTR pszTitleString, HKEY hkBackupKey,
     PCSTR pcszRootKey, PCSTR pcszSubKey, PCSTR pcszValueName, DWORD dwFlags);
HRESULT WINAPI RegSaveRestoreOnINF(HWND hWnd, PCSTR pszTitle, PCSTR pszINF,
     PCSTR pszSection, HKEY hHKLMBackKey, HKEY hHKCUBackKey, DWORD dwFlags);
HRESULT WINAPI RunSetupCommand(HWND hWnd,
     LPCSTR szCmdName, LPCSTR szInfSection, LPCSTR szDir, LPCSTR lpszTitle,
     HANDLE *phEXE, DWORD dwFlags, LPVOID pvReserved);
HRESULT WINAPI TranslateInfString(PCSTR pszInfFilename, PCSTR pszInstallSection,
     PCSTR pszTranslateSection, PCSTR pszTranslateKey, PSTR pszBuffer,
     DWORD dwBufferSize, PDWORD pdwRequiredSize, PVOID pvReserved);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_ADVPUB_H */
