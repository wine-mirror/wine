name	comctl32
type	win32

# Functions exported by the Win95 comctl32.dll 
# (these need to have these exact ordinals, because some win95 dlls 
#  import comctl32.dll by ordinal)

  2 stub MenuHelp
  3 stub ShowHideMenuCtl
  4 stub GetEffectiveClientRect
  5 stdcall DrawStatusTextA(long ptr ptr long) DrawStatusText32A
  6 stdcall CreateStatusWindowA(long ptr long long) CreateStatusWindow32A
  7 stub CreateToolbar
  8 stub CreateMappedBitmap
  9 stub Cctl1632_ThunkData32
 10 stub CreatePropertySheetPage
 11 stub CreatePropertySheetPageA
 12 stub CreatePropertySheetPageW
 13 stub MakeDragList
 14 stub LBItemFromPt
 15 stub DrawInsert
 16 stdcall CreateUpDownControl(long long long long long long long long long long long long) CreateUpDownControl
 17 stdcall InitCommonControls() InitCommonControls
# 18 pascal16 CreateStatusWindow(word ptr word word) CreateStatusWindow16
 18 stub CreateStatusWindow
 19 stdcall CreateStatusWindowW(long ptr long long) CreateStatusWindow32W
 20 stub CreateToolbarEx
 21 stub DestroyPropertySheetPage
 22 stub DllGetVersion
# 23 pascal16 DrawStatusText(word ptr ptr word) DrawStatusText16
 23 stub DrawStatusText
 24 stdcall DrawStatusTextW(long ptr ptr long) DrawStatusText32W
 25 stdcall ImageList_Add(ptr long long) ImageList_Add
 26 stub ImageList_AddIcon
 27 stdcall ImageList_AddMasked(ptr long long) ImageList_AddMasked
 28 stub ImageList_BeginDrag
 29 stub ImageList_Copy
 30 stdcall ImageList_Create(long long long long long) ImageList_Create
 31 stdcall ImageList_Destroy(ptr) ImageList_Destroy
 32 stub ImageList_DragEnter
 33 stub ImageList_DragLeave
 34 stub ImageList_DragMove
 35 stub ImageList_DragShowNolock
 36 stdcall ImageList_Draw(ptr long long long long long) ImageList_Draw
 37 stub ImageList_DrawEx
 38 stub ImageList_EndDrag
 39 stdcall ImageList_GetBkColor(ptr) ImageList_GetBkColor
 40 stub ImageList_GetDragImage
 41 stub ImageList_GetIcon
 42 stdcall ImageList_GetIconSize(ptr ptr ptr) ImageList_GetIconSize
 43 stdcall ImageList_GetImageCount(ptr) ImageList_GetImageCount
 44 stdcall ImageList_GetImageInfo(ptr long ptr) ImageList_GetImageInfo
 45 stub ImageList_GetImageRect
 46 stub ImageList_LoadImage
 47 stdcall ImageList_LoadImageA(long ptr long long long long long) ImageList_LoadImage32A
 48 stdcall ImageList_LoadImageW(long ptr long long long long long) ImageList_LoadImage32W
 49 stdcall ImageList_Merge(ptr long ptr long long long) ImageList_Merge
 50 stub ImageList_Read
 51 stub ImageList_Remove
 52 stdcall ImageList_Replace(ptr long long long) ImageList_Replace
 53 stdcall ImageList_ReplaceIcon(ptr long long) ImageList_ReplaceIcon
 54 stdcall ImageList_SetBkColor(ptr long) ImageList_SetBkColor
 55 stub ImageList_SetDragCursorImage
 56 stub ImageList_SetFilter
 57 stub ImageList_SetIconSize
 58 stub ImageList_SetImageCount
 59 stdcall ImageList_SetOverlayImage(ptr long long) ImageList_SetOverlayImage
 60 stub ImageList_Write
 61 stub InitCommonControlsEx
 62 stub PropertySheet
 63 stub PropertySheetA
 64 stub PropertySheetW
 65 stub _TrackMouseEvent
