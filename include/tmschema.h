/*
 * Copyright (C) 2003 Kevin Koltzau
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

#ifndef __WINE_TMSCHEMA_H
#define __WINE_TMSCHEMA_H

/* PARTS & STATES */

/* BUTTON parts */
#define BP_PUSHBUTTON   1
#define BP_RADIOBUTTON  2
#define BP_CHECKBOX     3
#define BP_GROUPBOX     4
#define BP_USERBUTTON   5

/* BUTTON PUSHBUTTON states */
#define PBS_NORMAL      1
#define PBS_HOT         2
#define PBS_PRESSED     3
#define PBS_DISABLED    4
#define PBS_DEFAULTED   5

/* BUTTON RADIOBUTTON states */
#define RBS_UNCHECKEDNORMAL   1
#define RBS_UNCHECEDHOT       2
#define RBS_UNCHECKEDPRESSED  3
#define RBS_UNCHECKEDDISABLED 4
#define RBS_CHECKEDNORMAL     5
#define RBS_CHECKEDHOT        6
#define RBS_CHECKEDPRESSED    7
#define RBS_CHECKEDDISABLED   8

/* BUTTON CHECBOX states */
#define CBS_UNCHECKEDNORMAL   1
#define CBS_UNCHECKEDHOT      2
#define CBS_UNCHECKEDPRESSED  3
#define CBS_UNCHECKEDDISABLED 4
#define CBS_CHECKEDNORMAL     5
#define CBS_CHECKEDHOT        6
#define CBS_CHECKEDPRESSED    7
#define CBS_CHECKEDDISABLED   8
#define CBS_MIXEDNORMAL       9
#define CBS_MIXEDHOT          10
#define CBS_MIXEDPRESSED      11
#define CBS_MIXEDDISABLED     12

/* BUTTON GROUPBOX states */
#define GBS_NORMAL   1
#define GBS_DISABLED 2

/* CLOCK parts */
#define CLP_TIME 1

/* CLOCK TIME states */
#define CLS_NORMAL 1

/* COMBOBOX parts */
#define CB_DROPDOWNBUTTON 1

/* COMBOBOX DROPDOWNBUTTON states */
#define CBXS_NORMAL   1
#define CBXS_HOT      2
#define CBXS_PRESSED  3
#define CBXS_DISABLED 4

/* EDIT parts */
#define EP_EDITTEXT 1
#define EP_CARET    1

/* EDIT EDITTEXT states */
#define ETS_NORMAL   1
#define ETS_HOT      2
#define ETS_SELECTED 3
#define ETS_DISABLED 4
#define ETS_FOCUSED  5
#define ETS_READONLY 6
#define ETS_ASSIST   7

/* EXPLORERBAR parts */
#define EBP_HEADERBACKGROUND       1
#define EBP_HEADERCLOSE            2
#define EBP_HEADERPIN              3
#define EBP_IEBARMENU              4
#define EBP_NORMALGROUPBACKGROUND  5
#define EBP_NORMALGROUPCOLLAPSE    6
#define EBP_NORMALGROUPEXPAND      7
#define EBP_NORMALGROUPHEAD        8
#define EBP_SPECIALGROUPBACKGROUND 9
#define EBP_SPECIALGROUPCOLLAPSE   10
#define EBP_SPECIALGROUPEXPAND     11
#define EBP_SPECIALGROUPHEAD       12

/* EXPLORERBAR HEADERCLOSE states */
#define EBHC_NORMAL  1
#define EBHC_HOT     2
#define EBHC_PRESSED 3

/* EXPLORERBAR HEADERPIN states */
#define EBHP_NORMAL          1
#define EBHP_HOT             2
#define EBHP_PRESSED         3
#define EBHP_SELECTEDNORMAL  4
#define EBHP_SELECTEDHOT     5
#define EBHP_SELECTEDPRESSED 6

/* EXPLORERBAR IEBARMENU states */
#define EBM_NORMAL  1
#define EBM_HOT     2
#define EBM_PRESSED 3

/* EXPLORERBAR NORMALGROUPCOLLAPSE states */
#define EBNGC_NORMAL  1
#define EBNGC_HOT     2
#define EBNGC_PRESSED 3

/* EXPLORERBAR NORMALGROUPEXPAND states */
#define EBNGE_NORMAL  1
#define EBNGE_HOT     2
#define EBNGE_PRESSED 3

/* EXPLORERBAR SPECIALGROUPCOLLAPSE states */
#define EBSGC_NORMAL  1
#define EBSGC_HOT     2
#define EBSGC_PRESSED 3

/* EXPLORERBAR SPECIALGROUPEXPAND states */
#define EBSGE_NORMAL  1
#define EBSGE_HOT     2
#define EBSGE_PRESSED 3

/* GLOBALS parts */
#define GP_BORDER   1
#define GP_LINEHORZ 2
#define GP_LINEVERT 3

/* GLOBALS BORDER states */
#define BSS_FLAT   1
#define BSS_RAISED 2
#define BSS_SUNKEN 3

/* GLOBALS LINEHORZ states */
#define LHS_FLAT   1
#define LHS_RAISED 2
#define LHS_SUNKEN 3

/* GLOBAL LINEVERT states */
#define LVS_FLAT   1
#define LVS_RAISED 2
#define LVS_SUNKEN 3

/* HEADER parts */
#define HP_HEADERITEM      1
#define HP_HEADERITEMLEFT  2
#define HP_HEADERITEMRIGHT 3
#define HP_HEADERSORTARROW 4

/* HEADER HEADERITEM states */
#define HIS_NORMAL  1
#define HIS_HOT     2
#define HIS_PRESSED 3

/* HEADER HEADERITEMLEFT states */
#define HILS_NORMAL  1
#define HILS_HOT     2
#define HILS_PRESSED 3

/* HEADER HEADERITEMRIGHT states */
#define HIRS_NORMAL  1
#define HIRS_HOT     2
#define HIRS_PRESSED 3

/* HEADER HEADERSORTARROW states */
#define HSAS_NORMAL  1
#define HSAS_HOT     2
#define HSAS_PRESSED 3

/* LISTVIEW parts */
#define LVP_LISTITEM         1
#define LVP_LISTGROUP        2
#define LVP_LISTDETAIL       3
#define LVP_LISTSORTEDDETAIL 4
#define LVP_EMPTYTEXT        5

/* LISTVIEW LISTITEM states */
#define LIS_NORMAL           1
#define LIS_HOT              2
#define LIS_SELECTED         3
#define LIS_DISABLED         4
#define LIS_SELECTEDNOTFOCUS 5

/* MENU parts */
#define MP_MENUITEM        1
#define MP_MENUDROPDOWN    2
#define MP_MENUBARITEM     3
#define MP_MENUBARDROPDOWN 4
#define MP_CHEVRON         5
#define MP_SEPARATOR       6

/* MENU * states */
#define MS_NORMAL    1
#define MS_SELECTED  2
#define MS_DEMOTED   3

/* MENUBAND parts */
#define MDP_NEWAPPBUTTON 1
#define MDP_SEPERATOR    2

/* MENUBAND NEWAPPBUTTON parts */
#define MDS_NORMAL     1
#define MDS_HOT        2
#define MDS_PRESSED    3
#define MDS_DISABLED   4
#define MDS_CHECKED    5
#define MDS_HOTCHECKED 6

/* PAGE parts */
#define PGRP_UP       1
#define PGRP_DOWN     2
#define PGRP_UPHORZ   3
#define PGRP_DOWNHORZ 4

/* PAGE UP states */
#define UPS_NORMAL   1
#define UPS_HOT      2
#define UPS_PRESSED  3
#define UPS_DISABLED 4

/* PAGE DOWN states  */
#define DNS_NORMAL   1
#define DNS_HOT      2
#define DNS_PRESSED  3
#define DNS_DISABLED 4

/* PAGE UPHORZ states */
#define UPHZS_NORMAL   1
#define UPHZS_HOT      2
#define UPHZS_PRESSED  3
#define UPHZS_DISABLED 4

/* PAGE DOWNHORZ states */
#define DNHZS_NORMAL   1
#define DNHZS_HOT      2
#define DNHZS_PRESSED  3
#define DNHZS_DISABLED 4

/* PROGRESS parts */
#define PP_BAR       1
#define PP_BARVERT   2
#define PP_CHUNK     3
#define PP_CHUNKVERT 4

/* REBAR parts */
#define RP_GRIPPER     1
#define RP_GRIPPERVERT 2
#define RP_BAND        3
#define RP_CHEVRON     4
#define RP_CHEVRONVERT 5

/* REBAR CHEVRON states */
#define CHEVS_NORMAL  1
#define CHEVS_HOT     2
#define CHEVS_PRESSED 3

/* SCROLLBAR parts */
#define SBP_ARROWBTN       1
#define SBP_THUMBBTNHORZ   2
#define SBP_THUMBBTNVERT   3
#define SBP_LOWERTRACKHORZ 4
#define SBP_UPPERTRACKHORZ 5
#define SBP_LOWERTRACKVERT 6
#define SBP_UPPERTRACKVERT 7
#define SBP_GRIPPERHORZ    8
#define SBP_GRIPPERVERT    9
#define SBP_SIZEBOX        10

/* SCROLLBAR ARROWBTN states */
#define ABS_UPNORMAL      1
#define ABS_UPHOT         2
#define ABS_UPPRESSED     3
#define ABS_UPDISABLED    4
#define ABS_DOWNNORMAL    5
#define ABS_DOWNHOT       6
#define ABS_DOWNPRESSED   7
#define ABS_DOWNDISABLED  8
#define ABS_LEFTNORMAL    9
#define ABS_LEFTHOT       10
#define ABS_LEFTPRESSED   11
#define ABS_LEFTDISABLED  12
#define ABS_RIGHTNORMAL   13
#define ABS_RIGHTHOT      14
#define ABS_RIGHTPRESSED  15
#define ABS_RIGHTDISABLED 16

/* SCROLLBAR LOWER* / THUMB* / UPPER* states */
#define SCRBS_NORMAL   1
#define SCRBS_HOT      2
#define SCRBS_PRESSED  3
#define SCRBS_DISABLED 4

/* SCROLLBAR SIZEBOX states */
#define SZB_RIGHTALIGN 1
#define SZB_LEFTALIGN  2

/* SPIN parts */
#define SPNP_UP       1
#define SPNP_DOWN     2
#define SPNP_UPHORZ   3
#define SPNP_DOWNHORZ 4

/* SPIN * states */
/* See PAGE states */

/* STARTPANEL parts */
#define SPP_USERPANE            1
#define SPP_MOREPROGRAMS        2
#define SPP_MOREPROGRAMSARROW   3
#define SPP_PROGLIST            4
#define SPP_PROGLISTSEPARATOR   5
#define SPP_PLACESLIST          6
#define SPP_PLACESLISTSEPARATOR 7
#define SPP_LOGOFF              8
#define SPP_LOGOFFBUTTONS       9
#define SPP_USERPICTURE         10
#define SPP_PREVIEW             11

/* STARTPANEL MOREPROGRAMSARROW states */
#define SPS_NORMAL  1
#define SPS_HOT     2
#define SPS_PRESSED 3

/* STARTPANEL LOGOFFBUTTONS states */
#define SPLS_NORMAL  1
#define SPLS_HOT     2
#define SPLS_PRESSED 3

/* STATUS parts */
#define SP_PANE        1
#define SP_GRIPPERPANE 2
#define SP_GRIPPER     3

/* TAB parts */
#define TABP_TABITEM             1
#define TABP_TABITEMLEFTEDGE     2
#define TABP_TABITEMRIGHTEDGE    3
#define TABP_TABITEMBOTHEDGE     4
#define TABP_TOPTABITEM          5
#define TABP_TOPTABITEMLEFTEDGE  6
#define TABP_TOPTABITEMRIGHTEDGE 7
#define TABP_TOPTABITEMBOTHEDGE  8
#define TABP_PANE                9
#define TABP_BODY                10

/* TAB TABITEM states */
#define TIS_NORMAL   1
#define TIS_HOT      2
#define TIS_SELECTED 3
#define TIS_DISABLED 4
#define TIS_FOCUSED  5

/* TAB TABITEMLEFTEDGE states */
#define TILES_NORMAL   1
#define TILES_HOT      2
#define TILES_SELECTED 3
#define TILES_DISABLED 4
#define TILES_FOCUSED  5

/* TAB TABITEMRIGHTEDGE states */
#define TIRES_NORMAL   1
#define TIRES_HOT      2
#define TIRES_SELECTED 3
#define TIRES_DISABLED 4
#define TIRES_FOCUSED  5

/* TAB TABITEMBOTHEDGES states */
#define TIBES_NORMAL   1
#define TIBES_HOT      2
#define TIBES_SELECTED 3
#define TIBES_DISABLED 4
#define TIBES_FOCUSED  5

/* TAB TOPTABITEM states */
#define TTIS_NORMAL   1
#define TTIS_HOT      2
#define TTIS_SELECTED 3
#define TTIS_DISABLED 4
#define TTIS_FOCUSED  5

/* TAB TOPTABITEMLEFTEDGE states */
#define TTILES_NORMAL   1
#define TTILES_HOT      2
#define TTILES_SELECTED 3
#define TTILES_DISABLED 4
#define TTILES_FOCUSED  5

/* TAB TOPTABITEMRIGHTEDGE states */
#define TTIRES_NORMAL   1
#define TTIRES_HOT      2
#define TTIRES_SELECTED 3
#define TTIRES_DISABLED 4
#define TTIRES_FOCUSED  5

/* TAB TOPTABITEMBOTHEDGES states */
#define TTIBES_NORMAL   1
#define TTIBES_HOT      2
#define TTIBES_SELECTED 3
#define TTIBES_DISABLED 4
#define TTIBES_FOCUSED  5

/* TASKBAND parts */
#define TDP_GROUPCOUNT           1
#define TDP_FLASHBUTTON          2
#define TDP_FLASHBUTTONGROUPMENU 3

/* TASKBAR parts */
#define TBP_BACKGROUNDBOTTOM 1
#define TBP_BACKGROUNDRIGHT  2
#define TBP_BACKGROUNDTOP    3
#define TBP_BACKGROUNDLEFT   4
#define TBP_SIZINGBARBOTTOM  5
#define TBP_SIZINGBARRIGHT   6
#define TBP_SIZINGBARTOP     7
#define TBP_SIZINGBARLEFT    8

/* TOOLBAR parts */
#define TP_BUTTON              1
#define TP_DROPDOWNBUTTON      2
#define TP_SPLITBUTTON         3
#define TP_SPLITBUTTONDROPDOWN 4
#define TP_SEPARATOR           5
#define TP_SEPARATORVERT       6

/* TOOLBAR * states */
#define TS_NORMAL     1
#define TS_HOT        2
#define TS_PRESSED    3
#define TS_DISABLED   4
#define TS_CHECKED    5
#define TS_HOTCHECKED 6

/* TOOLTIP parts */
#define TTP_STANDARD      1
#define TTP_STANDARDTITLE 2
#define TTP_BALLOON       3
#define TTP_BALLOONTITLE  4
#define TTP_CLOSE         5

/* TOOLTIP STANDARD states */
#define TTSS_NORMAL 1
#define TTSS_LINK   2

/* TOOLTIP STANDARDTITLE states */
/* See TOOLTIP STANDARD  */

/* TOOLTIP BALLOON states */
#define TTBS_NORMAL 1
#define TTBS_LINK   2

/* TOOLTIP BALOONTITLE states */
/* See TOOLTIP BALOON */

/* TOOLTIP CLOSE states */
#define TTCS_NORMAL  1
#define TTCS_HOT     2
#define TTCS_PRESSED 3

/* TRACKBAR parts */
#define TKP_TRACK       1
#define TKP_TRACKVERT   2
#define TKP_THUMB       3
#define TKP_THUMBBOTTOM 4
#define TKP_THUMBTOP    5
#define TKP_THUMBVERT   6
#define TKP_THUMBLEFT   7
#define TKP_THUMBRIGHT  8
#define TKP_TICS        9
#define TKP_TICSVERT    10

/* TRACKBAR TRACK states */
#define TRS_NORMAL 1

/* TRACKBAR TRACKVERT states */
#define TRVS_NORMAL 1

/* TRACKBAR THUMB states */
#define TUS_NORMAL   1
#define TUS_HOT      2
#define TUS_PRESSED  3
#define TUS_FOCUSED  4
#define TUS_DISABLED 5

/* TRACKBAR THUMBBOTTOM states */
#define TUBS_NORMAL   1
#define TUBS_HOT      2
#define TUBS_PRESSED  3
#define TUBS_FOCUSED  4
#define TUBS_DISABLED 5

/* TRACKBAR THUMBTOP states */
#define TUTS_NORMAL   1
#define TUTS_HOT      2
#define TUTS_PRESSED  3
#define TUTS_FOCUSED  4
#define TUTS_DISABLED 5

/* TRACKBAR THUMBVERT states */
#define TUVS_NORMAL   1
#define TUVS_HOT      2
#define TUVS_PRESSED  3
#define TUVS_FOCUSED  4
#define TUVS_DISABLED 5

/* TRACKBAR THUMBLEFT states */
#define TUVLS_NORMAL   1
#define TUVLS_HOT      2
#define TUVLS_PRESSED  3
#define TUVLS_FOCUSED  4
#define TUVLS_DISABLED 5

/* TRACKBAR THUMBRIGHT states */
#define TUVRS_NORMAL   1
#define TUVRS_HOT      2
#define TUVRS_PRESSED  3
#define TUVRS_FOCUSED  4
#define TUVRS_DISABLED 5

/* TRACKBAR TICS states */
#define TSS_NORMAL 1

/* TRACKBAR TICSVERT states */
#define TSVS_NORMAL 1

/* TRAYNOTIFY parts */
#define TNP_BACKGROUND     1
#define TNP_ANIMBACKGROUND 2

/* TREEVIEW parts */
#define TVP_TREEITEM 1
#define TVP_GLYPH    2
#define TVP_BRANCH   3

/* TREEVIEW TREEITEM states */
#define TREIS_NORMAL           1
#define TREIS_HOT              2
#define TREIS_SELECTED         3
#define TREIS_DISABLED         4
#define TREIS_SELECTEDNOTFOCUS 5

/* TREEVIEW GLYPH states */
#define GLPS_CLOSED 1
#define GLPS_OPENED 2

/* WINDOW parts */
#define WP_CAPTION                        1
#define WP_SMALLCAPTION                   2
#define WP_MINCAPTION                     3
#define WP_SMALLMINCAPTION                4
#define WP_MAXCAPTION                     5
#define WP_SMALLMAXCAPTION                6
#define WP_FRAMELEFT                      7
#define WP_FRAMERIGHT                     8
#define WP_FRAMEBOTTOM                    9
#define WP_SMALLFRAMELEFT                 10
#define WP_SMALLFRAMERIGHT                11
#define WP_SMALLFRAMEBOTTOM               12
#define WP_SYSBUTTON                      13
#define WP_MDISYSBUTTON                   14
#define WP_MINBUTTON                      15
#define WP_MDIMINBUTTON                   16
#define WP_MAXBUTTON                      17
#define WP_CLOSEBUTTON                    18
#define WP_SMALLCLOSEBUTTON               19
#define WP_MDICLOSEBUTTON                 20
#define WP_RESTOREBUTTON                  21
#define WP_MDIRESTOREBUTTON               22
#define WP_HELPBUTTON                     23
#define WP_MDIHELPBUTTON                  24
#define WP_HORZSCROLL                     25
#define WP_HORZTHUMB                      26
#define WP_VERTSCROLL                     27
#define WP_VERTTHUMB                      28
#define WP_DIALOG                         29
#define WP_CAPTIONSIZINGTEMPLATE          30
#define WP_SMALLCAPTIONSIZINGTEMPLATE     31
#define WP_FRAMELEFTSIZINGTEMPLATE        32
#define WP_SMALLFRAMELEFTSIZINGTEMPLATE   33
#define WP_FRAMERIGHTSIZINGTEMPLATE       34
#define WP_SMALLFRAMERIGHTSIZINGTEMPLATE  35
#define WP_FRAMEBOTTOMSIZINGTEMPLATE      36
#define WP_SMALLFRAMEBOTTOMSIZINGTEMPLATE 37

/* WINDOW CAPTION / SMALLCAPTION state */
#define CS_ACTIVE   1
#define CS_INACTIVE 2
#define CS_DISABLED 3

/* WINDOW MINCAPTION / SMALLMINCAPTION state */
#define MNCS_ACTIVE   1
#define MNCS_INACTIVE 2
#define MNCS_DISABLED 3

/* WINDOW MAXCAPTION / SMALLMAXCAPTION state */
#define MXCS_ACTIVE   1
#define MXCS_INACTIVE 2
#define MXCS_DISABLED 3

/* WINDOW FRAME* / SMALLFRAME* state */
#define FS_ACTIVE   1
#define FS_INACTIVE 2

/* WINDOW SYSBUTTON / MDISYSBUTTON state */
#define SBS_NORMAL   1
#define SBS_HOT      2
#define SBS_PUSHED   3
#define SBS_DISABLED 4

/* WINDOW MINBUTTON / MDIMINBUTTON state */
#define MINBS_NORMAL   1
#define MINBS_HOT      2
#define MINBS_PUSHED   3
#define MINBS_DISABLED 4

/* WINDOW MAXBUTTON state */
#define MAXBS_NORMAL   1
#define MAXBS_HOT      2
#define MAXBS_PUSHED   3
#define MAXBS_DISABLED 4

/* WINDOW CLOSEBUTTON / SMALLCLOSEBUTTON / MDICLOSEBUTTON state */
#define CBS_NORMAL   1
#define CBS_HOT      2
#define CBS_PUSHED   3
#define CBS_DISABLED 4

/* WINDOW RESTOREBUTTON / MDIRESTOREBUTTON state */
#define RBS_NORMAL   1
#define RBS_HOT      2
#define RBS_PUSHED   3
#define RBS_DISABLED 4

/* WINDOW HELPBUTTON / MDIHELPBUTTON state */
#define HBS_NORMAL   1
#define HBS_HOT      2
#define HBS_PUSHED   3
#define HBS_DISABLED 4

/* WINDOW HORZSCROLL state */
#define HSS_NORMAL   1
#define HSS_HOT      2
#define HSS_PUSHED   3
#define HSS_DISABLED 4

/* WINDOW HORZTHUMB state */
#define HTS_NORMAL   1
#define HTS_HOT      2
#define HTS_PUSHED   3
#define HTS_DISABLED 4

/* WINDOW VERTSCROLL state */
#define VSS_NORMAL   1
#define VSS_HOT      2
#define VSS_PUSHED   3
#define VSS_DISABLED 4

/* WINDOW VERTTHUMB state */
#define VTS_NORMAL   1
#define VTS_HOT      2
#define VTS_PUSHED   3
#define VTS_DISABLED 4

#endif
