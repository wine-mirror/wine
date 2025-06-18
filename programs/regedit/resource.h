/*
 * Regedit resource definitions
 *
 * Copyright 2002 Robert Dickenson
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
 */

#include <windef.h>
#include <winuser.h>

#define ID_REGISTRY_MENU                0
#define ID_EDIT_MENU                    1
#define ID_VIEW_MENU                    2
#define ID_FAVORITES_MENU               3
#define ID_HELP_MENU                    4
#define ID_EDIT_NEW_MENU                5
#define IDS_LIST_COLUMN_FIRST           91
#define IDS_LIST_COLUMN_NAME            91
#define IDS_LIST_COLUMN_TYPE            92
#define IDS_LIST_COLUMN_DATA            93
#define IDS_LIST_COLUMN_LAST            93
#define IDS_APP_TITLE                   103
#define IDI_REGEDIT                     100
#define IDI_SMALL                       108
#define IDC_REGEDIT                     109
#define IDC_REGEDIT_FRAME               110
#define IDR_REGEDIT_MENU                130
#define IDD_EXPORT_TEMPLATE             131
#define IDI_OPEN_FILE                   132
#define IDI_CLOSED_FILE                 133
#define IDD_ADDFAVORITE                 133
#define IDI_ROOT                        134
#define IDD_DELFAVORITE                 134
#define IDI_STRING                      135
#define IDD_FIND                        135
#define IDI_BIN                         136
#define IDR_POPUP_MENUS                 137
#define IDS_FILEDIALOG_IMPORT_TITLE     144
#define IDS_FILEDIALOG_EXPORT_TITLE     145
#define IDS_FILEDIALOG_FILTER_REG       146
#define IDS_FILEDIALOG_FILTER_REG4      147
#define IDS_FILEDIALOG_FILTER_ALL       148
#define IDS_REGISTRY_ROOT_NAME          160
#define IDS_REGISTRY_DEFAULT_VALUE      161
#define IDS_REGISTRY_VALUE_NOT_SET      162
#define IDS_REGISTRY_VALUE_CANT_DISPLAY 164
#define IDS_REGISTRY_UNKNOWN_TYPE       165
#define IDC_LICENSE_EDIT                1029
#define ID_REGISTRY_EXIT                32770
#define ID_FAVORITES_ADDTOFAVORITES     32772
#define ID_FAVORITES_REMOVEFAVORITE     32773
#define ID_VIEW_STATUSBAR               32774
#define ID_VIEW_SPLIT                   32775
#define ID_VIEW_REFRESH                 32776
#define ID_EDIT_DELETE                  32778
#define ID_EDIT_RENAME                  32779
#define ID_EDIT_COPYKEYNAME             32781
#define ID_EDIT_FIND                    32782
#define ID_EDIT_FINDNEXT                32783
#define ID_EDIT_MODIFY                  32784
#define ID_EDIT_NEW_KEY                 32785
#define ID_EDIT_NEW_STRINGVALUE         32786
#define ID_EDIT_NEW_BINARYVALUE         32787
#define ID_EDIT_NEW_DWORDVALUE          32788
#define ID_REGISTRY_IMPORTREGISTRYFILE  32789
#define ID_REGISTRY_EXPORTREGISTRYFILE  32790
#define ID_REGISTRY_PRINT               32793
#define ID_HELP_HELPTOPICS              32794
#define ID_HELP_ABOUT                   32795
#define ID_WINDOW_CASCADE               32797
#define ID_WINDOW_TILE                  32798
#define ID_WINDOW_ARRANGEICONS          32799
#define ID_OPTIONS_FONT                 32800
#define ID_OPTIONS_AUTOREFRESH          32801
#define ID_OPTIONS_READONLYMODE         32802
#define ID_OPTIONS_CONFIRMONDELETE      32803
#define ID_OPTIONS_SAVESETTINGSONEXIT   32804
#define ID_SECURITY_PERMISSIONS         32805
#define ID_VIEW_TREEANDDATA             32806
#define ID_VIEW_TREEONLY                32807
#define ID_VIEW_DATAONLY                32808
#define ID_VIEW_DISPLAYBINARYDATA       32810
#define ID_VIEW_REFRESHALL              32811
#define ID_VIEW_REFRESHACTIVE           32812
#define ID_VIEW_FINDKEY                 32813
#define ID_TREE_EXPANDONELEVEL          32814
#define ID_TREE_EXPANDBRANCH            32815
#define ID_TREE_EXPANDALL               32816
#define ID_TREE_COLLAPSEBRANCH          32817
#define ID_EDIT_ADDKEY                  32818
#define ID_EDIT_ADDVALUE                32819
#define ID_EDIT_BINARY                  32821
#define ID_EDIT_STRING                  32822
#define ID_EDIT_DWORD                   32823
#define ID_EDIT_MULTISTRING             32824
#define ID_REGISTRY_OPENLOCAL           32825
#define ID_REGISTRY_CLOSE               32826
#define ID_REGISTRY_LOADHIVE            32827
#define ID_REGISTRY_UNLOADHIVE          32828
#define ID_REGISTRY_RESTORE             32829
#define ID_REGISTRY_SAVEKEY             32830
#define ID_REGISTRY_SELECTCOMPUTER      32831
#define ID_REGISTRY_PRINTSUBTREE        32832
#define ID_REGISTRY_PRINTERSETUP        32833
#define ID_REGISTRY_SAVESUBTREEAS       32834
#define IDS_LICENSE                     32835
#define IDS_ERROR                       32836
#define IDS_BAD_VALUE			32837
#define IDS_UNSUPPORTED_TYPE		32838
#define IDS_TOO_BIG_VALUE		32839
#define IDS_DELETE_VALUE_TITLE		32840
#define IDS_DELETE_VALUE_TEXT		32841
#define IDS_NOTFOUND    		32842
#define IDS_DELETE_VALUE_TEXT_MULTIPLE	32843
#define IDD_EDIT_DWORD			32850
#define IDC_DWORD_BASE			32852
#define IDC_DWORD_HEX			32853
#define IDC_DWORD_DEC			32854
#define IDS_NEWKEY			32860
#define IDS_NEWVALUE			32861
#define IDS_BAD_KEY			32862
#define ID_EDIT_MODIFY_BIN		32870
#define ID_SWITCH_PANELS                32871
#define ID_FAVORITE_FIRST               33000
#define ID_FAVORITE_LAST                33099
#define ID_EDIT_NEW_MULTI_STRINGVALUE   33100
#define ID_EDIT_EXPORT                  33101
#define ID_EDIT_NEW_EXPANDVALUE         33102
#define IDS_DELETE_KEY_TITLE            33103
#define IDS_DELETE_KEY_TEXT             33104
#define ID_TREE_EXPAND_COLLAPSE         33105
#define IDS_EXPAND                      33106
#define IDS_COLLAPSE                    33107
#define IDS_EDIT_MODIFY                 33108
#define IDS_EDIT_MODIFY_BIN             33109
#define ID_EDIT_NEW_QWORDVALUE          33110
#define IDS_EDIT_QWORD                  33111

#define IDD_EDIT_STRING			2000
#define IDC_VALUE_NAME			2001
#define IDC_VALUE_DATA			2002
#define IDD_EDIT_BINARY                 2003
#define IDC_NAME_LIST                   2004
#define IDC_FIND_KEYS                   2005
#define IDC_FIND_VALUES                 2006
#define IDC_FIND_CONTENT                2007
#define IDC_FIND_WHOLE                  2008
#define IDD_EDIT_MULTI_STRING           2009

#define IDS_SET_VALUE_FAILED            2010
#define IDS_CREATE_KEY_FAILED           2011
#define IDS_CREATE_VALUE_FAILED         2012
#define IDS_KEY_EXISTS                  2013
#define IDS_VALUE_EXISTS                2014
#define IDS_DELETE_KEY_FAILED           2015
#define IDS_RENAME_KEY_FAILED           2016
#define IDS_RENAME_VALUE_FAILED         2017
#define IDS_IMPORT_SUCCESSFUL           2018
#define IDS_IMPORT_FAILED               2019

#define IDC_EXPORT_BASE                 100
#define IDC_EXPORT_ALL                  101
#define IDC_EXPORT_SELECTED             102
#define IDC_EXPORT_PATH                 103

#define IDC_STATIC                      -1

/* Command-line strings */
#define STRING_USAGE                    3001
#define STRING_INVALID_SWITCH           3002
#define STRING_HELP                     3003
#define STRING_NO_FILENAME              3004
#define STRING_NO_REG_KEY               3005
#define STRING_FILE_NOT_FOUND           3006
#define STRING_CANNOT_OPEN_FILE         3007
#define STRING_UNHANDLED_ACTION         3008
#define STRING_OUT_OF_MEMORY            3009
#define STRING_INVALID_HEX              3010
#define STRING_CSV_HEX_ERROR            3011
#define STRING_ESCAPE_SEQUENCE          3012
#define STRING_UNKNOWN_DATA_FORMAT      3013
#define STRING_UNEXPECTED_EOL           3014
#define STRING_UNRECOGNIZED_LINE        3015
#define STRING_SETVALUE_FAILED          3016
#define STRING_OPEN_KEY_FAILED          3017
#define STRING_UNSUPPORTED_TYPE         3018
#define STRING_EXPORT_AS_BINARY         3019
#define STRING_INVALID_SYSTEM_KEY       3020
#define STRING_REG_KEY_NOT_FOUND        3021
#define STRING_DELETE_FAILED            3022
#define STRING_UNKNOWN_TYPE             3023
#define STRING_INVALID_LINE_SYNTAX      3024
