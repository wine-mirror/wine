name	comctl32
type	win32
base	2

# Functions exported by the Win95 comctl32.dll 
# (these need to have these exact ordinals, because some win95 dlls 
#  import comctl32.dll by ordinal)

00 stub MenuHelp
01 stub ShowHideMenuCtl
02 stub GetEffectiveClientRect
03 stdcall DrawStatusTextA(long ptr ptr long) DrawStatusText32A
04 stdcall CreateStatusWindowA(long ptr long long) CreateStatusWindow32A
05 stub CreateToolbar
06 stub CreateMappedBitmap
07 stub Cctl1632_ThunkData32
08 stub CreatePropertySheetPage
09 stub CreatePropertySheetPageA
10 stub CreatePropertySheetPageW
11 stub MakeDragList
12 stub LBItemFromPt
13 stub DrawInsert
14 stdcall CreateUpDownControl(long long long long long long long long long long long long) CreateUpDownControl
15 stdcall InitCommonControls() InitCommonControls
16 stub CreateStatusWindow
17 stub CreateStatusWindowW
18 stub CreateToolbarEx
19 stub DestroyPropertySheetPage
20 stub DllGetVersion
21 stub DrawStatusText
22 stub DrawStatusTextW
23 stub ImageList_Add
24 stub ImageList_AddIcon
25 stub ImageList_AddMasked
26 stub ImageList_BeginDrag
27 stub ImageList_Copy
28 stub ImageList_Create
29 stub ImageList_Destroy
30 stub ImageList_DragEnter
31 stub ImageList_DragLeave
32 stub ImageList_DragMove
33 stub ImageList_DragShowNolock
34 stub ImageList_Draw
35 stub ImageList_DrawEx
36 stub ImageList_EndDrag
37 stub ImageList_GetBkColor
38 stub ImageList_GetDragImage
39 stub ImageList_GetIcon
40 stub ImageList_GetIconSize
41 stub ImageList_GetImageCount
42 stub ImageList_GetImageInfo
43 stub ImageList_GetImageRect
44 stub ImageList_LoadImage
45 stub ImageList_LoadImageA
46 stub ImageList_LoadImageW
47 stub ImageList_Merge
48 stub ImageList_Read
49 stub ImageList_Remove
50 stub ImageList_Replace
51 stub ImageList_ReplaceIcon
52 stub ImageList_SetBkColor
53 stub ImageList_SetDragCursorImage
54 stub ImageList_SetFilter
55 stub ImageList_SetIconSize
56 stub ImageList_SetImageCount
57 stub ImageList_SetOverlayImage
58 stub ImageList_Write
59 stub InitCommonControlsEx
60 stub PropertySheet
61 stub PropertySheetA
62 stub PropertySheetW
63 stub _TrackMouseEvent
