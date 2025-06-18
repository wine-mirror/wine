/*
 * WineCfg resource definitions
 *
 * Copyright 2002 Jaco Greeff
 * Copyright 2003 Dimitrie O. Paun
 * Copyright 2003 Mark Westcott
 * Copyright 2004 Mike Hearn
 * Copyright 2005 Raphael Junqueira
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <windef.h>
#include <winuser.h>
#include <commctrl.h>

#define IDC_STATIC                     -1
#define IDS_TAB_APPLICATIONS            2
#define IDS_TAB_DLLS                    3
#define IDS_TAB_DRIVES                  4
#define IDS_CHOOSE_PATH                 5
#define IDS_SHOW_ADVANCED               6
#define IDS_HIDE_ADVANCED               7
#define IDS_NOTHEME                     8
#define IDS_TAB_GRAPHICS                9
#define IDS_TAB_DESKTOP_INTEGRATION     10
#define IDS_TAB_AUDIO                   11
#define IDS_TAB_ABOUT                   12
#define IDS_WINECFG_TITLE               13
#define IDS_THEMEFILE                   14
#define IDS_THEMEFILE_SELECT            15
#define IDS_SHELL_FOLDER                16
#define IDS_LINKS_TO                    17
#define IDS_WINECFG_TITLE_APP           18   /* App specific title */
#define IDI_WINECFG                     100
#define IDI_LOGO                        102
#define IDD_ABOUTCFG                    107
#define IDD_APPCFG                      108
#define IDD_AUDIOCFG                    109
#define IDD_GRAPHCFG                    110
#define IDD_DLLCFG                      111
#define IDD_DRIVECFG                    112
#define IDD_DESKTOP_INTEGRATION         115
#define IDC_WINVER                      1012
#define IDC_DESKTOP_WIDTH               1023
#define IDC_DESKTOP_HEIGHT              1024
#define IDC_DESKTOP_SIZE                1025
#define IDC_DESKTOP_BY                  1026

/* dll editing  */
#define IDC_RAD_BUILTIN                 1029
#define IDC_RAD_NATIVE                  1030
#define IDC_RAD_BUILTIN_NATIVE          1031
#define IDC_RAD_NATIVE_BUILTIN          1032
#define IDC_RAD_DISABLE                 1033
#define IDC_DLLS_LIST                   1034
#define IDC_DLLS_ADDDLL                 8001
#define IDC_DLLS_EDITDLL                8002
#define IDC_DLLS_REMOVEDLL              8003
#define IDC_DLLCOMBO                    8004
#define IDD_LOADORDER                   8005
#define IDS_DLL_WARNING                 8010
#define IDS_DLL_WARNING_CAPTION         8011
#define IDS_DLL_NATIVE                  8012
#define IDS_DLL_BUILTIN                 8013
#define IDS_DLL_NATIVE_BUILTIN          8014
#define IDS_DLL_BUILTIN_NATIVE          8015
#define IDS_DLL_DISABLED                8016
#define IDS_DEFAULT_SETTINGS            8017
#define IDS_EXECUTABLE_FILTER           8018
#define IDS_USE_GLOBAL_SETTINGS         8019
#define IDS_SELECT_EXECUTABLE           8020

/* drive editing */
#define IDC_STATIC_MOUNTMGR_ERROR       1041
#define IDC_LIST_DRIVES                 1042
#define IDC_BUTTON_ADD                  1043
#define IDC_BUTTON_REMOVE               1044
#define IDC_BUTTON_EDIT                 1045
#define IDC_EDIT_LABEL                  1060
#define IDC_EDIT_PATH                   1061
#define IDC_EDIT_SERIAL                 1062
#define IDC_STATIC_PATH                 1063
#define IDC_COMBO_TYPE                  1065
#define IDC_EDIT_DEVICE                 1066
#define IDC_BUTTON_BROWSE_PATH          1067
#define IDC_STATIC_DEVICE               1068
#define IDC_BUTTON_BROWSE_DEVICE        1070
#define IDC_STATIC_SERIAL               1072
#define IDC_STATIC_LABEL                1073
#define IDC_ENABLE_DESKTOP              1074
#define IDS_DRIVE_NO_C                  1075
#define IDC_BUTTON_SHOW_HIDE_ADVANCED   1076
#define IDC_STATIC_TYPE                 1077
#define IDC_SHOW_DOT_FILES              1080

#define IDS_DRIVE_UNKNOWN               8200
#define IDS_DRIVE_FIXED                 8201
#define IDS_DRIVE_REMOTE                8202
#define IDS_DRIVE_REMOVABLE             8203
#define IDS_DRIVE_CDROM                 8204
#define IDS_DRIVE_LETTERS_EXCEEDED      8205
#define IDS_SYSTEM_DRIVE_LABEL          8206
#define IDS_CONFIRM_DELETE_C            8207
#define IDS_COL_DRIVELETTER             8208
#define IDS_COL_DRIVEMAPPING            8209
#define IDS_NO_DRIVE_C                  8210
#define IDD_DRIVECHOOSE                 8211
#define IDC_DRIVESA2Z                   8212

/* graphics */
#define IDC_ENABLE_MANAGED              1100
#define IDC_ENABLE_DECORATED            1101
#define IDC_FULLSCREEN_GRAB             1102

#define IDC_RES_TRACKBAR                1107
#define IDC_RES_DPIEDIT                 1108
#define IDC_RES_FONT_PREVIEW            1109

/* applications tab */
#define IDC_APP_LISTVIEW                1200
#define IDC_APP_ADDAPP                  1201
#define IDC_APP_REMOVEAPP               1202

/* audio tab */
#define IDC_AUDIO_TEST                  1300
#define IDC_AUDIO_DRIVER                1301
#define IDS_AUDIO_DRIVER                1302
#define IDS_AUDIO_DRIVER_NONE           1303
#define IDC_AUDIOOUT_DEVICE             1304
#define IDC_VOICEOUT_DEVICE             1305
#define IDC_AUDIOIN_DEVICE              1306
#define IDC_VOICEIN_DEVICE              1307
#define IDW_TESTSOUND                   1308
#define IDS_AUDIO_TEST_FAILED           1309
#define IDS_AUDIO_TEST_FAILED_TITLE     1310
#define IDS_AUDIO_SYSDEFAULT            1311
#define IDS_AUDIO_DEVICE                1312
#define IDC_SPEAKERCONFIG_SPEAKERS      1313
#define IDS_AUDIO_SPEAKER_5POINT1       1314
#define IDS_AUDIO_SPEAKER_QUAD          1315
#define IDS_AUDIO_SPEAKER_STEREO        1316
#define IDS_AUDIO_SPEAKER_MONO          1317
#define IDC_LIST_AUDIO_DEVICES          1318
#define IDS_AUDIO_SPEAKER_CONFIG        1319

/* desktop integration tab */
#define IDC_THEME_COLORCOMBO            1401
#define IDC_THEME_COLORTEXT             1402
#define IDC_THEME_SIZECOMBO             1403
#define IDC_THEME_SIZETEXT              1404
#define IDC_THEME_THEMECOMBO            1405
#define IDC_THEME_APPCOMBO              1406
#define IDC_THEME_APPCOMBO_LIGHT        1407
#define IDC_THEME_APPCOMBO_DARK         1408
#define IDC_THEME_INSTALL               1409
#define IDC_LIST_SFPATHS                1410
#define IDC_LINK_SFPATH                 1411
#define IDC_EDIT_SFPATH                 1412
#define IDC_BROWSE_SFPATH               1413
#define IDC_SYSPARAM_COMBO              1414
#define IDC_SYSPARAM_SIZE_TEXT          1415
#define IDC_SYSPARAM_SIZE               1416
#define IDC_SYSPARAM_SIZE_UD            1417
#define IDC_SYSPARAM_COLOR_TEXT         1418
#define IDC_SYSPARAM_COLOR              1419
#define IDC_SYSPARAM_FONT               1420
#define IDC_ENABLE_FILE_ASSOCIATIONS    1421

#define IDC_SYSPARAMS_BUTTON            8400
#define IDC_SYSPARAMS_BUTTON_TEXT       8401
#define IDC_SYSPARAMS_DESKTOP           8402
#define IDC_SYSPARAMS_MENU              8403
#define IDC_SYSPARAMS_MENU_TEXT         8404
#define IDC_SYSPARAMS_SCROLLBAR         8405
#define IDC_SYSPARAMS_SELECTION         8406
#define IDC_SYSPARAMS_SELECTION_TEXT    8407
#define IDC_SYSPARAMS_TOOLTIP           8408
#define IDC_SYSPARAMS_TOOLTIP_TEXT      8409
#define IDC_SYSPARAMS_WINDOW            8410
#define IDC_SYSPARAMS_WINDOW_TEXT       8411
#define IDC_SYSPARAMS_ACTIVE_TITLE      8412
#define IDC_SYSPARAMS_ACTIVE_TITLE_TEXT 8413
#define IDC_SYSPARAMS_INACTIVE_TITLE    8414
#define IDC_SYSPARAMS_INACTIVE_TITLE_TEXT 8415
#define IDC_SYSPARAMS_MSGBOX_TEXT       8416
#define IDC_SYSPARAMS_APPWORKSPACE      8417
#define IDC_SYSPARAMS_WINDOW_FRAME      8418
#define IDC_SYSPARAMS_ACTIVE_BORDER     8419
#define IDC_SYSPARAMS_INACTIVE_BORDER   8420
#define IDC_SYSPARAMS_BUTTON_SHADOW     8421
#define IDC_SYSPARAMS_GRAY_TEXT         8422
#define IDC_SYSPARAMS_BUTTON_HIGHLIGHT  8423
#define IDC_SYSPARAMS_BUTTON_DARK_SHADOW 8424
#define IDC_SYSPARAMS_BUTTON_LIGHT      8425
#define IDC_SYSPARAMS_BUTTON_ALTERNATE 8426
#define IDC_SYSPARAMS_HOT_TRACKING      8427
#define IDC_SYSPARAMS_ACTIVE_TITLE_GRADIENT 8428
#define IDC_SYSPARAMS_INACTIVE_TITLE_GRADIENT 8429
#define IDC_SYSPARAMS_MENU_HIGHLIGHT    8430
#define IDC_SYSPARAMS_MENUBAR           8431

/* About tab */
#define IDC_ABT_OWNER                8432
#define IDC_ABT_ORG                  8433
#define IDC_ABT_PANEL                8434
#define IDC_ABT_PANEL_TEXT           8435
#define IDC_ABT_TITLE_TEXT           8436
#define IDC_ABT_WEB_LINK             8437
#define IDC_ABT_LICENSE_TEXT         8438
