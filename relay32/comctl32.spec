name	comctl32
type	win32

# Functions exported by the Win95 comctl32.dll 
# (these need to have these exact ordinals, because some win95 dlls 
#  import comctl32.dll by ordinal)
#   This list was created from a comctl32.dll v4.72 (IE4.01).

  2 stub MenuHelp
  3 stub ShowHideMenuCtl
  4 stub GetEffectiveClientRect
  5 stdcall DrawStatusTextA(long ptr ptr long) DrawStatusText32A
  6 stdcall CreateStatusWindowA(long ptr long long) CreateStatusWindow32A
  7 stub CreateToolbar
  8 stub CreateMappedBitmap
#  9 stub COMCTL32_9
# 10 stub COMCTL32_10
# 11 stub COMCTL32_11
 12 stub Cctl1632_ThunkData32
 13 stub MakeDragList
 14 stub LBItemFromPt
 15 stub DrawInsert
 16 stdcall CreateUpDownControl(long long long long long long long long long long long long) CreateUpDownControl
 17 stdcall InitCommonControls() InitCommonControls
 18 stub CreatePropertySheetPage
 19 stub CreatePropertySheetPageA
 20 stub CreatePropertySheetPageW
# 21 pascal16 CreateStatusWindow(word ptr word word) CreateStatusWindow16
 21 stub CreateStatusWindow
 22 stdcall CreateStatusWindowW(long ptr long long) CreateStatusWindow32W
 23 stub CreateToolbarEx
 24 stub DestroyPropertySheetPage
 25 stub DllGetVersion
 26 stub DllInstall
# 27 pascal16 DrawStatusText(word ptr ptr word) DrawStatusText16
 27 stub DrawStatusText
 28 stdcall DrawStatusTextW(long ptr ptr long) DrawStatusText32W
 29 stub FlatSB_EnableScrollBar
 30 stub FlatSB_GetScrollInfo
 31 stub FlatSB_GetScrollPos
 32 stub FlatSB_GetScrollProp
 33 stub FlatSB_GetScrollRange
 34 stub FlatSB_SetScrollInfo
 35 stub FlatSB_SetScrollPos
 36 stub FlatSB_SetScrollProp
 37 stub FlatSB_SetScrollRange
 38 stub FlatSB_ShowScrollBar
 39 stdcall ImageList_Add(ptr long long) ImageList_Add
 40 stub ImageList_AddIcon
 41 stdcall ImageList_AddMasked(ptr long long) ImageList_AddMasked
 42 stdcall ImageList_BeginDrag(ptr long long long) ImageList_BeginDrag
 43 stdcall ImageList_Copy(ptr long ptr long long) ImageList_Copy
 44 stdcall ImageList_Create(long long long long long) ImageList_Create
 45 stdcall ImageList_Destroy(ptr) ImageList_Destroy
 46 stdcall ImageList_DragEnter(long long long) ImageList_DragEnter
 47 stdcall ImageList_DragLeave(long) ImageList_DragLeave
 48 stdcall ImageList_DragMove(long long) ImageList_DragMove
 49 stdcall ImageList_DragShowNolock(long) ImageList_DragShowNolock
 50 stdcall ImageList_Draw(ptr long long long long long) ImageList_Draw
 51 stdcall ImageList_DrawEx(ptr long long long long long long long long long) ImageList_DrawEx
 52 stdcall ImageList_DrawIndirect(ptr) ImageList_DrawIndirect
 53 stdcall ImageList_Duplicate(ptr) ImageList_Duplicate
 54 stdcall ImageList_EndDrag() ImageList_EndDrag 
 55 stdcall ImageList_GetBkColor(ptr) ImageList_GetBkColor
 56 stdcall ImageList_GetDragImage(ptr ptr) ImageList_GetDragImage
 57 stdcall ImageList_GetIcon(ptr long long) ImageList_GetIcon
 58 stdcall ImageList_GetIconSize(ptr ptr ptr) ImageList_GetIconSize
 59 stdcall ImageList_GetImageCount(ptr) ImageList_GetImageCount
 60 stdcall ImageList_GetImageInfo(ptr long ptr) ImageList_GetImageInfo
 61 stdcall ImageList_GetImageRect(ptr long ptr) ImageList_GetImageRect
 62 stub ImageList_LoadImage
 63 stdcall ImageList_LoadImageA(long ptr long long long long long) ImageList_LoadImage32A
 64 stdcall ImageList_LoadImageW(long ptr long long long long long) ImageList_LoadImage32W
 65 stdcall ImageList_Merge(ptr long ptr long long long) ImageList_Merge
 66 stub ImageList_Read
 67 stdcall ImageList_Remove(ptr long) ImageList_Remove
 68 stdcall ImageList_Replace(ptr long long long) ImageList_Replace
 69 stdcall ImageList_ReplaceIcon(ptr long long) ImageList_ReplaceIcon
 70 stdcall ImageList_SetBkColor(ptr long) ImageList_SetBkColor
# 71
# 72
# 73
# 74 stub GetSize # 1 parameter
 75 stdcall ImageList_SetDragCursorImage(ptr long long long) ImageList_SetDragCursorImage
 76 stub ImageList_SetFilter
 77 stdcall ImageList_SetIconSize(ptr long long) ImageList_SetIconSize
 78 stdcall ImageList_SetImageCount(ptr long) ImageList_SetImageCount
 79 stdcall ImageList_SetOverlayImage(ptr long long) ImageList_SetOverlayImage
 80 stub ImageList_Write
 81 stub InitCommonControlsEx
 82 stub InitializeFlatSB
 83 stub PropertySheet
 84 stub PropertySheetA
 85 stub PropertySheetW
 86 stub UninitializeFlatSB
 87 stub _TrackMouseEvent

#151
#152 stub FreeMRUList  # 1 parameter
#153
#154
#155 stub FindMRUStringA  # 3 parameters
#156
#157
#163
#164
#167
#169 stub FindMRUData
#233 - 236

#320
#321
#322
#323
#324
#325
#326
#327
#328
#329 stub DPA_Create
#330 stub DPA_Destroy
#331
#332 stub DPA_GetPtr
#333
#334 stub DPA_InsertPtr
#335
#336 stub DPA_DeletePtr
#337
#338
#339 stub DPA_Search
#340
#341
#342

#350 stub StrChrA
#351
#352 stub StrCmpA
#353
#354 stub StrStrA
#355
#356
#357 stub StrToIntA
#358 - 369

#372 - 375
#376 stub IntlStrEqWorkerA
#377 stub IntlStrEqWorkerW
#382
#383 stub DoReaderMode
#384 - 390
#400 - 404
#410 - 413
