/*
 * Definitions for Common Property Sheet User Interface
 *
 * Copyright 2006 Detlef Riekenberg
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

#ifndef _COMPSTUI_
#define _COMPSTUI_

#ifdef __cplusplus
extern "C" {
#endif

/* DEFINES */
#define PROPSHEETUI_REASON_INIT             0
#define PROPSHEETUI_REASON_GET_INFO_HEADER  1
#define PROPSHEETUI_REASON_DESTROY          2
#define PROPSHEETUI_REASON_SET_RESULT       3
#define PROPSHEETUI_REASON_GET_ICON         4
#define PROPSHEETUI_REASON_BEFORE_INIT      5
#define MAX_PROPSHEETUI_REASON_INDEX        5

#define PROPSHEETUI_INFO_VERSION    0x0100

#define PSUIINFO_UNICODE    1

#define CPSFUNC_ADD_HPROPSHEETPAGE      0
#define CPSFUNC_ADD_PROPSHEETPAGEW      1
#define CPSFUNC_ADD_PCOMPROPSHEETUIA    2
#define CPSFUNC_ADD_PCOMPROPSHEETUIW    3
#define CPSFUNC_ADD_PFNPROPSHEETUIA     4
#define CPSFUNC_ADD_PFNPROPSHEETUIW     5
#define CPSFUNC_DELETE_HCOMPROPSHEET    6
#define CPSFUNC_SET_HSTARTPAGE          7
#define CPSFUNC_GET_PAGECOUNT           8
#define CPSFUNC_SET_RESULT              9
#define CPSFUNC_GET_HPSUIPAGES          10
#define CPSFUNC_LOAD_CPSUI_STRINGA      11
#define CPSFUNC_LOAD_CPSUI_STRINGW      12
#define CPSFUNC_LOAD_CPSUI_ICON         13
#define CPSFUNC_GET_PFNPROPSHEETUI_ICON 14
#define CPSFUNC_ADD_PROPSHEETPAGEA      15
#define CPSFUNC_INSERT_PSUIPAGEA        16
#define CPSFUNC_INSERT_PSUIPAGEW        17
#define CPSFUNC_SET_PSUIPAGE_TITLEA     18
#define CPSFUNC_SET_PSUIPAGE_TITLEW     19
#define CPSFUNC_SET_PSUIPAGE_ICON       20
#define CPSFUNC_SET_DATABLOCK           21
#define CPSFUNC_QUERY_DATABLOCK         22
#define CPSFUNC_SET_DMPUB_HIDEBITS      23
#define CPSFUNC_IGNORE_CPSUI_PSN_APPLY  24
#define CPSFUNC_DO_APPLY_CPSUI          25
#define CPSFUNC_SET_FUSION_CONTEXT      26
#define MAX_CPSFUNC_INDEX               26

#define PSUIHDRF_OBSOLETE       0x01
#define PSUIHDRF_NOAPPLYNOW     0x02
#define PSUIHDRF_PROPTITLE      0x04
#define PSUIHDRF_USEHICON       0x08
#define PSUIHDRF_DEFTITLE       0x10
#define PSUIHDRF_EXACT_PTITLE   0x20

/* return-values for CommonPropertySheetUI on success */
#define CPSUI_CANCEL            0
#define CPSUI_OK                1
#define CPSUI_RESTARTWINDOWS    2
#define CPSUI_REBOOTSYSTEM      3

#define ERR_CPSUI_GETLASTERROR              -1
#define ERR_CPSUI_ALLOCMEM_FAILED           -2
#define ERR_CPSUI_INVALID_PDATA             -3
#define ERR_CPSUI_INVALID_LPARAM            -4
#define ERR_CPSUI_NULL_HINST                -5
#define ERR_CPSUI_NULL_CALLERNAME           -6
#define ERR_CPSUI_NULL_OPTITEMNAME          -7
#define ERR_CPSUI_NO_PROPSHEETPAGE          -8
#define ERR_CPSUI_TOO_MANY_PROPSHEETPAGES   -9
#define ERR_CPSUI_CREATEPROPPAGE_FAILED     -10
#define ERR_CPSUI_MORE_THAN_ONE_TVPAGE      -11
#define ERR_CPSUI_MORE_THAN_ONE_STDPAGE     -12
#define ERR_CPSUI_INVALID_PDLGPAGE          -13
#define ERR_CPSUI_INVALID_DLGPAGE_CBSIZE    -14
#define ERR_CPSUI_TOO_MANY_DLGPAGES         -15
#define ERR_CPSUI_INVALID_DLGPAGEIDX        -16
#define ERR_CPSUI_SUBITEM_DIFF_DLGPAGEIDX   -17
#define ERR_CPSUI_NULL_POPTITEM             -18
#define ERR_CPSUI_INVALID_OPTITEM_CBSIZE    -19
#define ERR_CPSUI_INVALID_OPTTYPE_CBSIZE    -20
#define ERR_CPSUI_INVALID_OPTTYPE_COUNT     -21
#define ERR_CPSUI_NULL_POPTPARAM            -22
#define ERR_CPSUI_INVALID_OPTPARAM_CBSIZE   -23
#define ERR_CPSUI_INVALID_EDITBOX_PSEL      -24
#define ERR_CPSUI_INVALID_EDITBOX_BUF_SIZE  -25
#define ERR_CPSUI_INVALID_ECB_CBSIZE        -26
#define ERR_CPSUI_NULL_ECB_PTITLE           -27
#define ERR_CPSUI_NULL_ECB_PCHECKEDNAME     -28
#define ERR_CPSUI_INVALID_DMPUBID           -29
#define ERR_CPSUI_INVALID_DMPUB_TVOT        -30
#define ERR_CPSUI_CREATE_TRACKBAR_FAILED    -31
#define ERR_CPSUI_CREATE_UDARROW_FAILED     -32
#define ERR_CPSUI_CREATE_IMAGELIST_FAILED   -33
#define ERR_CPSUI_INVALID_TVOT_TYPE         -34
#define ERR_CPSUI_INVALID_LBCB_TYPE         -35
#define ERR_CPSUI_SUBITEM_DIFF_OPTIF_HIDE   -36
#define ERR_CPSUI_INVALID_PUSHBUTTON_TYPE   -38
#define ERR_CPSUI_INVALID_EXTPUSH_CBSIZE    -39
#define ERR_CPSUI_NULL_EXTPUSH_DLGPROC      -40
#define ERR_CPSUI_NO_EXTPUSH_DLGTEMPLATEID  -41
#define ERR_CPSUI_NULL_EXTPUSH_CALLBACK     -42
#define ERR_CPSUI_DMCOPIES_USE_EXTPUSH      -43
#define ERR_CPSUI_ZERO_OPTITEM              -44

#define ERR_CPSUI_FUNCTION_NOT_IMPLEMENTED  -9999
#define ERR_CPSUI_INTERNAL_ERROR            -10000

#define IDI_CPSUI_ICONID_FIRST              64000
#define IDI_CPSUI_EMPTY                     64000
#define IDI_CPSUI_SEL_NONE                  64001
#define IDI_CPSUI_WARNING                   64002
#define IDI_CPSUI_NO                        64003
#define IDI_CPSUI_YES                       64004
#define IDI_CPSUI_FALSE                     64005
#define IDI_CPSUI_TRUE                      64006
#define IDI_CPSUI_OFF                       64007
#define IDI_CPSUI_ON                        64008
#define IDI_CPSUI_PAPER_OUTPUT              64009
#define IDI_CPSUI_ENVELOPE                  64010
#define IDI_CPSUI_MEM                       64011
#define IDI_CPSUI_FONTCARTHDR               64012
#define IDI_CPSUI_FONTCART                  64013
#define IDI_CPSUI_STAPLER_ON                64014
#define IDI_CPSUI_STAPLER_OFF               64015
#define IDI_CPSUI_HT_HOST                   64016
#define IDI_CPSUI_HT_DEVICE                 64017
#define IDI_CPSUI_TT_PRINTASGRAPHIC         64018
#define IDI_CPSUI_TT_DOWNLOADSOFT           64019
#define IDI_CPSUI_TT_DOWNLOADVECT           64020
#define IDI_CPSUI_TT_SUBDEV                 64021
#define IDI_CPSUI_PORTRAIT                  64022
#define IDI_CPSUI_LANDSCAPE                 64023
#define IDI_CPSUI_ROT_LAND                  64024
#define IDI_CPSUI_AUTOSEL                   64025
#define IDI_CPSUI_PAPER_TRAY                64026
#define IDI_CPSUI_PAPER_TRAY2               64027
#define IDI_CPSUI_PAPER_TRAY3               64028
#define IDI_CPSUI_TRANSPARENT               64029
#define IDI_CPSUI_COLLATE                   64030
#define IDI_CPSUI_DUPLEX_NONE               64031
#define IDI_CPSUI_DUPLEX_HORZ               64032
#define IDI_CPSUI_DUPLEX_VERT               64033
#define IDI_CPSUI_RES_DRAFT                 64034
#define IDI_CPSUI_RES_LOW                   64035
#define IDI_CPSUI_RES_MEDIUM                64036
#define IDI_CPSUI_RES_HIGH                  64037
#define IDI_CPSUI_RES_PRESENTATION          64038
#define IDI_CPSUI_MONO                      64039
#define IDI_CPSUI_COLOR                     64040
#define IDI_CPSUI_DITHER_NONE               64041
#define IDI_CPSUI_DITHER_COARSE             64042
#define IDI_CPSUI_DITHER_FINE               64043
#define IDI_CPSUI_DITHER_LINEART            64044
#define IDI_CPSUI_SCALING                   64045
#define IDI_CPSUI_COPY                      64046
#define IDI_CPSUI_HTCLRADJ                  64047
#define IDI_CPSUI_HALFTONE_SETUP            64048
#define IDI_CPSUI_WATERMARK                 64049
#define IDI_CPSUI_ERROR                     64050
#define IDI_CPSUI_ICM_OPTION                64051
#define IDI_CPSUI_ICM_METHOD                64052
#define IDI_CPSUI_ICM_INTENT                64053
#define IDI_CPSUI_STD_FORM                  64054
#define IDI_CPSUI_OUTBIN                    64055
#define IDI_CPSUI_OUTPUT                    64056
#define IDI_CPSUI_GRAPHIC                   64057
#define IDI_CPSUI_ADVANCE                   64058
#define IDI_CPSUI_DOCUMENT                  64059
#define IDI_CPSUI_DEVICE                    64060
#define IDI_CPSUI_DEVICE2                   64061
#define IDI_CPSUI_PRINTER                   64062
#define IDI_CPSUI_PRINTER2                  64063
#define IDI_CPSUI_PRINTER3                  64064
#define IDI_CPSUI_PRINTER4                  64065
#define IDI_CPSUI_OPTION                    64066
#define IDI_CPSUI_OPTION2                   64067
#define IDI_CPSUI_STOP                      64068
#define IDI_CPSUI_NOTINSTALLED              64069
#define IDI_CPSUI_WARNING_OVERLAY           64070
#define IDI_CPSUI_STOP_WARNING_OVERLAY      64071
#define IDI_CPSUI_GENERIC_OPTION            64072
#define IDI_CPSUI_GENERIC_ITEM              64073
#define IDI_CPSUI_RUN_DIALOG                64074
#define IDI_CPSUI_QUESTION                  64075
#define IDI_CPSUI_FORMTRAYASSIGN            64076
#define IDI_CPSUI_PRINTER_FOLDER            64077
#define IDI_CPSUI_INSTALLABLE_OPTION        64078
#define IDI_CPSUI_PRINTER_FEATURE           64079
#define IDI_CPSUI_DEVICE_FEATURE            64080
#define IDI_CPSUI_FONTSUB                   64081
#define IDI_CPSUI_POSTSCRIPT                64082
#define IDI_CPSUI_TELEPHONE                 64083
#define IDI_CPSUI_DUPLEX_NONE_L             64084
#define IDI_CPSUI_DUPLEX_HORZ_L             64085
#define IDI_CPSUI_DUPLEX_VERT_L             64086
#define IDI_CPSUI_LF_PEN_PLOTTER            64087
#define IDI_CPSUI_SF_PEN_PLOTTER            64088
#define IDI_CPSUI_LF_RASTER_PLOTTER         64089
#define IDI_CPSUI_SF_RASTER_PLOTTER         64090
#define IDI_CPSUI_ROLL_PAPER                64091
#define IDI_CPSUI_PEN_CARROUSEL             64092
#define IDI_CPSUI_PLOTTER_PEN               64093
#define IDI_CPSUI_MANUAL_FEED               64094
#define IDI_CPSUI_FAX                       64095
#define IDI_CPSUI_PAGE_PROTECT              64096
#define IDI_CPSUI_ENVELOPE_FEED             64097
#define IDI_CPSUI_FONTCART_SLOT             64098
#define IDI_CPSUI_LAYOUT_BMP_PORTRAIT       64099
#define IDI_CPSUI_LAYOUT_BMP_ARROWL         64100
#define IDI_CPSUI_LAYOUT_BMP_ARROWS         64101
#define IDI_CPSUI_LAYOUT_BMP_BOOKLETL       64102
#define IDI_CPSUI_LAYOUT_BMP_BOOKLETP       64103
#define IDI_CPSUI_LAYOUT_BMP_ARROWLR        64104
#define IDI_CPSUI_LAYOUT_BMP_ROT_PORT       64105
#define IDI_CPSUI_LAYOUT_BMP_BOOKLETL_NB    64106
#define IDI_CPSUI_LAYOUT_BMP_BOOKLETP_NB    64107
#define IDI_CPSUI_ROT_PORT                  64110
#define IDI_CPSUI_NUP_BORDER                64111
#define IDI_CPSUI_ICONID_LAST               64111

#define IDS_CPSUI_STRID_FIRST           64700
#define IDS_CPSUI_SETUP                 64700
#define IDS_CPSUI_MORE                  64701
#define IDS_CPSUI_CHANGE                64702
#define IDS_CPSUI_OPTION                64703
#define IDS_CPSUI_OF                    64704
#define IDS_CPSUI_RANGE_FROM            64705
#define IDS_CPSUI_TO                    64706
#define IDS_CPSUI_COLON_SEP             64707
#define IDS_CPSUI_LEFT_ANGLE            64708
#define IDS_CPSUI_RIGHT_ANGLE           64709
#define IDS_CPSUI_SLASH_SEP             64710
#define IDS_CPSUI_PERCENT               64711
#define IDS_CPSUI_LBCB_NOSEL            64712
#define IDS_CPSUI_PROPERTIES            64713
#define IDS_CPSUI_DEFAULTDOCUMENT       64714
#define IDS_CPSUI_DOCUMENT              64715
#define IDS_CPSUI_ADVANCEDOCUMENT       64716
#define IDS_CPSUI_PRINTER               64717
#define IDS_CPSUI_AUTOSELECT            64718
#define IDS_CPSUI_PAPER_OUTPUT          64719
#define IDS_CPSUI_GRAPHIC               64720
#define IDS_CPSUI_OPTIONS               64721
#define IDS_CPSUI_ADVANCED              64722
#define IDS_CPSUI_STDDOCPROPTAB         64723
#define IDS_CPSUI_STDDOCPROPTVTAB       64724
#define IDS_CPSUI_DEVICEOPTIONS         64725
#define IDS_CPSUI_FALSE                 64726
#define IDS_CPSUI_TRUE                  64727
#define IDS_CPSUI_NO                    64728
#define IDS_CPSUI_YES                   64729
#define IDS_CPSUI_OFF                   64730
#define IDS_CPSUI_ON                    64731
#define IDS_CPSUI_DEFAULT               64732
#define IDS_CPSUI_ERROR                 64733
#define IDS_CPSUI_NONE                  64734
#define IDS_CPSUI_NOT                   64735
#define IDS_CPSUI_EXIST                 64736
#define IDS_CPSUI_NOTINSTALLED          64737
#define IDS_CPSUI_ORIENTATION           64738
#define IDS_CPSUI_SCALING               64739
#define IDS_CPSUI_NUM_OF_COPIES         64740
#define IDS_CPSUI_SOURCE                64741
#define IDS_CPSUI_PRINTQUALITY          64742
#define IDS_CPSUI_RESOLUTION            64743
#define IDS_CPSUI_COLOR_APPERANCE       64744
#define IDS_CPSUI_DUPLEX                64745
#define IDS_CPSUI_TTOPTION              64746
#define IDS_CPSUI_FORMNAME              64747
#define IDS_CPSUI_ICM                   64748
#define IDS_CPSUI_ICMMETHOD             64749
#define IDS_CPSUI_ICMINTENT             64750
#define IDS_CPSUI_MEDIA                 64751
#define IDS_CPSUI_DITHERING             64752
#define IDS_CPSUI_PORTRAIT              64753
#define IDS_CPSUI_LANDSCAPE             64754
#define IDS_CPSUI_ROT_LAND              64755
#define IDS_CPSUI_COLLATE               64756
#define IDS_CPSUI_COLLATED              64757
#define IDS_CPSUI_PRINTFLDSETTING       64758
#define IDS_CPSUI_DRAFT                 64759
#define IDS_CPSUI_LOW                   64760
#define IDS_CPSUI_MEDIUM                64761
#define IDS_CPSUI_HIGH                  64762
#define IDS_CPSUI_PRESENTATION          64763
#define IDS_CPSUI_COLOR                 64764
#define IDS_CPSUI_GRAYSCALE             64765
#define IDS_CPSUI_MONOCHROME            64766
#define IDS_CPSUI_SIMPLEX               64767
#define IDS_CPSUI_HORIZONTAL            64768
#define IDS_CPSUI_VERTICAL              64769
#define IDS_CPSUI_LONG_SIDE             64770
#define IDS_CPSUI_SHORT_SIDE            64771
#define IDS_CPSUI_TT_PRINTASGRAPHIC     64772
#define IDS_CPSUI_TT_DOWNLOADSOFT       64773
#define IDS_CPSUI_TT_DOWNLOADVECT       64774
#define IDS_CPSUI_TT_SUBDEV             64775
#define IDS_CPSUI_ICM_BLACKWHITE        64776
#define IDS_CPSUI_ICM_NO                64777
#define IDS_CPSUI_ICM_YES               64778
#define IDS_CPSUI_ICM_SATURATION        64779
#define IDS_CPSUI_ICM_CONTRAST          64780
#define IDS_CPSUI_ICM_COLORMETRIC       64781
#define IDS_CPSUI_STANDARD              64782
#define IDS_CPSUI_GLOSSY                64783
#define IDS_CPSUI_TRANSPARENCY          64784
#define IDS_CPSUI_REGULAR               64785
#define IDS_CPSUI_BOND                  64786
#define IDS_CPSUI_COARSE                64787
#define IDS_CPSUI_FINE                  64788
#define IDS_CPSUI_LINEART               64789
#define IDS_CPSUI_ERRDIFFUSE            64790
#define IDS_CPSUI_HALFTONE              64791
#define IDS_CPSUI_HTCLRADJ              64792
#define IDS_CPSUI_USE_HOST_HT           64793
#define IDS_CPSUI_USE_DEVICE_HT         64794
#define IDS_CPSUI_USE_PRINTER_HT        64795
#define IDS_CPSUI_OUTBINASSIGN          64796
#define IDS_CPSUI_WATERMARK             64797
#define IDS_CPSUI_FORMTRAYASSIGN        64798
#define IDS_CPSUI_UPPER_TRAY            64799
#define IDS_CPSUI_ONLYONE               64800
#define IDS_CPSUI_LOWER_TRAY            64801
#define IDS_CPSUI_MIDDLE_TRAY           64802
#define IDS_CPSUI_MANUAL_TRAY           64803
#define IDS_CPSUI_ENVELOPE_TRAY         64804
#define IDS_CPSUI_ENVMANUAL_TRAY        64805
#define IDS_CPSUI_TRACTOR_TRAY          64806
#define IDS_CPSUI_SMALLFMT_TRAY         64807
#define IDS_CPSUI_LARGEFMT_TRAY         64808
#define IDS_CPSUI_LARGECAP_TRAY         64809
#define IDS_CPSUI_CASSETTE_TRAY         64810
#define IDS_CPSUI_DEFAULT_TRAY          64811
#define IDS_CPSUI_FORMSOURCE            64812
#define IDS_CPSUI_MANUALFEED            64813
#define IDS_CPSUI_PRINTERMEM_KB         64814
#define IDS_CPSUI_PRINTERMEM_MB         64815
#define IDS_CPSUI_PAGEPROTECT           64816
#define IDS_CPSUI_HALFTONE_SETUP        64817
#define IDS_CPSUI_INSTFONTCART          64818
#define IDS_CPSUI_SLOT1                 64819
#define IDS_CPSUI_SLOT2                 64820
#define IDS_CPSUI_SLOT3                 64821
#define IDS_CPSUI_SLOT4                 64822
#define IDS_CPSUI_LEFT_SLOT             64823
#define IDS_CPSUI_RIGHT_SLOT            64824
#define IDS_CPSUI_STAPLER               64825
#define IDS_CPSUI_STAPLER_ON            64826
#define IDS_CPSUI_STAPLER_OFF           64827
#define IDS_CPSUI_STACKER               64828
#define IDS_CPSUI_MAILBOX               64829
#define IDS_CPSUI_COPY                  64830
#define IDS_CPSUI_COPIES                64831
#define IDS_CPSUI_TOTAL                 64832
#define IDS_CPSUI_MAKE                  64833
#define IDS_CPSUI_PRINT                 64834
#define IDS_CPSUI_FAX                   64835
#define IDS_CPSUI_PLOT                  64836
#define IDS_CPSUI_SLOW                  64837
#define IDS_CPSUI_FAST                  64838
#define IDS_CPSUI_ROTATED               64839
#define IDS_CPSUI_RESET                 64840
#define IDS_CPSUI_ALL                   64841
#define IDS_CPSUI_DEVICE                64842
#define IDS_CPSUI_SETTINGS              64843
#define IDS_CPSUI_REVERT                64844
#define IDS_CPSUI_CHANGES               64845
#define IDS_CPSUI_CHANGED               64846
#define IDS_CPSUI_WARNING               64847
#define IDS_CPSUI_ABOUT                 64848
#define IDS_CPSUI_VERSION               64849
#define IDS_CPSUI_NO_NAME               64850
#define IDS_CPSUI_SETTING               64851
#define IDS_CPSUI_DEVICE_SETTINGS       64852
#define IDS_CPSUI_STDDOCPROPTAB1        64853
#define IDS_CPSUI_STDDOCPROPTAB2        64854
#define IDS_CPSUI_PAGEORDER             64855
#define IDS_CPSUI_FRONTTOBACK           64856
#define IDS_CPSUI_BACKTOFRONT           64857
#define IDS_CPSUI_QUALITY_SETTINGS      64858
#define IDS_CPSUI_QUALITY_DRAFT         64859
#define IDS_CPSUI_QUALITY_BETTER        64860
#define IDS_CPSUI_QUALITY_BEST          64861
#define IDS_CPSUI_QUALITY_CUSTOM        64862
#define IDS_CPSUI_OUTPUTBIN             64863
#define IDS_CPSUI_NUP                   64864
#define IDS_CPSUI_NUP_NORMAL            64865
#define IDS_CPSUI_NUP_TWOUP             64866
#define IDS_CPSUI_NUP_FOURUP            64867
#define IDS_CPSUI_NUP_SIXUP             64868
#define IDS_CPSUI_NUP_NINEUP            64869
#define IDS_CPSUI_NUP_SIXTEENUP         64870
#define IDS_CPSUI_SIDE1                 64871
#define IDS_CPSUI_SIDE2                 64872
#define IDS_CPSUI_BOOKLET               64873
#define IDS_CPSUI_POSTER                64874
#define IDS_CPSUI_POSTER_2x2            64875
#define IDS_CPSUI_POSTER_3x3            64876
#define IDS_CPSUI_POSTER_4x4            64877
#define IDS_CPSUI_NUP_DIRECTION         64878
#define IDS_CPSUI_RIGHT_THEN_DOWN       64879
#define IDS_CPSUI_DOWN_THEN_RIGHT       64880
#define IDS_CPSUI_LEFT_THEN_DOWN        64881
#define IDS_CPSUI_DOWN_THEN_LEFT        64882
#define IDS_CPSUI_MANUAL_DUPLEX         64883
#define IDS_CPSUI_MANUAL_DUPLEX_ON      64884
#define IDS_CPSUI_MANUAL_DUPLEX_OFF     64885
#define IDS_CPSUI_ROT_PORT              64886
#define IDS_CPSUI_STAPLE                64887
#define IDS_CPSUI_BOOKLET_EDGE          64888
#define IDS_CPSUI_BOOKLET_EDGE_LEFT     64889
#define IDS_CPSUI_BOOKLET_EDGE_RIGHT    64890
#define IDS_CPSUI_NUP_BORDER            64891
#define IDS_CPSUI_NUP_BORDERED          64892
#define IDS_CPSUI_STRID_LAST            64892

/* TYPES */
typedef LONG_PTR (CALLBACK *PFNCOMPROPSHEET)(HANDLE, UINT, LPARAM, LPARAM);


typedef struct _PSPINFO {
    WORD            cbSize;
    WORD            wReserved;
    HANDLE          hComPropSheet;
    HANDLE          hCPSUIPage;
    PFNCOMPROPSHEET pfnComPropSheet;
} PSPINFO, *PPSPINFO;

typedef struct _PROPSHEETUI_INFO {
    WORD            cbSize;
    WORD            Version;
    WORD            Flags;              /* set PSUIINFO_UNICODE for UNICODE */
    WORD            Reason;
    HANDLE          hComPropSheet;
    PFNCOMPROPSHEET pfnComPropSheet;
    LPARAM          lParamInit;
    ULONG_PTR       UserData;
    ULONG_PTR       Result;
} PROPSHEETUI_INFO, *PPROPSHEETUI_INFO;

typedef LONG  (CALLBACK *PFNPROPSHEETUI)(PROPSHEETUI_INFO*, LPARAM);

/* FUNCTIONS */
LONG WINAPI CommonPropertySheetUIA(HWND, PFNPROPSHEETUI, LPARAM, LPDWORD);
LONG WINAPI CommonPropertySheetUIW(HWND, PFNPROPSHEETUI, LPARAM, LPDWORD);
#define CommonPropertySheetUI WINELIB_NAME_AW(CommonPropertySheetUI)

ULONG_PTR WINAPI GetPSTUIUserData(HWND);
BOOL      WINAPI SetPSTUIUserData(HWND, ULONG_PTR);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* _COMPSTUI_ */
