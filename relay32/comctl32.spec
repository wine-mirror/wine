name	comctl32
type	win32
init	ComCtl32LibMain

# Functions exported by the Win95 comctl32.dll 
# (these need to have these exact ordinals, because some win95 dlls 
#  import comctl32.dll by ordinal)
#   This list was created from a comctl32.dll v4.72 (IE4.01).

  2 stdcall MenuHelp(long long long long long long ptr) MenuHelp
  3 stdcall ShowHideMenuCtl(long long ptr) ShowHideMenuCtl
  4 stdcall GetEffectiveClientRect(long long long) GetEffectiveClientRect
  5 stdcall DrawStatusTextA(long ptr str long) DrawStatusText32A
  6 stdcall CreateStatusWindowA(long str long long) CreateStatusWindow32A
  7 stdcall CreateToolbar(long long long long long long ptr long) CreateToolbar
  8 stdcall CreateMappedBitmap(long long long ptr long) CreateMappedBitmap
  9 stub COMCTL32_9
 10 stub COMCTL32_10
 11 stdcall COMCTL32_11(ptr ptr long long long long) COMCTL32_11
#12 stub Cctl1632_ThunkData32
 13 stub MakeDragList
 14 stub LBItemFromPt
 15 stub DrawInsert
 16 stdcall CreateUpDownControl(long long long long long long long long long long long long) CreateUpDownControl
 17 stdcall InitCommonControls() InitCommonControls
 18 stub CreatePropertySheetPage
 19 stub CreatePropertySheetPageA
 20 stub CreatePropertySheetPageW
 21 stdcall CreateStatusWindow(long str long long) CreateStatusWindow32A
 22 stdcall CreateStatusWindowW(long wstr long long) CreateStatusWindow32W
 23 stdcall CreateToolbarEx(long long long long long long ptr long long long long long long) CreateToolbarEx
 24 stub DestroyPropertySheetPage
 25 stdcall DllGetVersion(ptr) COMCTL32_DllGetVersion
 26 stub DllInstall
 27 stdcall DrawStatusText(long ptr str long) DrawStatusText32A
 28 stdcall DrawStatusTextW(long ptr wstr long) DrawStatusText32W
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
 40 stdcall ImageList_AddIcon(ptr long) ImageList_AddIcon
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
 63 stdcall ImageList_LoadImage(long str long long long long long) ImageList_LoadImage32A
 63 stdcall ImageList_LoadImageA(long str long long long long long) ImageList_LoadImage32A
 64 stdcall ImageList_LoadImageW(long wstr long long long long long) ImageList_LoadImage32W
 65 stdcall ImageList_Merge(ptr long ptr long long long) ImageList_Merge
 66 stdcall ImageList_Read(ptr) ImageList_Read
 67 stdcall ImageList_Remove(ptr long) ImageList_Remove
 68 stdcall ImageList_Replace(ptr long long long) ImageList_Replace
 69 stdcall ImageList_ReplaceIcon(ptr long long) ImageList_ReplaceIcon
 70 stdcall ImageList_SetBkColor(ptr long) ImageList_SetBkColor
 71 stdcall Alloc(long) COMCTL32_Alloc
 72 stdcall ReAlloc(ptr long) COMCTL32_ReAlloc
 73 stdcall Free(ptr) COMCTL32_Free
 74 stdcall GetSize(ptr) COMCTL32_GetSize
 75 stdcall ImageList_SetDragCursorImage(ptr long long long) ImageList_SetDragCursorImage
 76 stdcall ImageList_SetFilter(ptr long long) ImageList_SetFilter
 77 stdcall ImageList_SetIconSize(ptr long long) ImageList_SetIconSize
 78 stdcall ImageList_SetImageCount(ptr long) ImageList_SetImageCount
 79 stdcall ImageList_SetOverlayImage(ptr long long) ImageList_SetOverlayImage
 80 stdcall ImageList_Write(ptr ptr) ImageList_Write
 81 stdcall InitCommonControlsEx(ptr) InitCommonControlsEx
 82 stub InitializeFlatSB
 83 stub PropertySheet
 84 stub PropertySheetA
 85 stub PropertySheetW
 86 stub UninitializeFlatSB
 87 stub _TrackMouseEvent

151 stdcall CreateMRUListA(ptr) CreateMRUList32A
152 stdcall FreeMRUList(ptr) FreeMRUList32A
153 stub AddMRUStringA@8
154 stub EnumMRUListA@16
155 stub FindMRUStringA@12
156 stub DelMRUString@8
157 stdcall COMCTL32_157(ptr long long long) CreateMRUListEx32A

163 stub CreatePage
164 stub CreateProxyPage

167 stdcall AddMRUData(ptr long long) AddMRUData
169 stdcall FindMRUData(ptr long long long) FindMRUData

233 stdcall Str_GetPtrA(str str long) Str_GetPtr32A
234 stdcall Str_SetPtrA(str str) Str_SetPtr32A
235 stdcall Str_GetPtrW(wstr wstr long) Str_GetPtr32W
236 stdcall Str_SetPtrW(wstr wstr) Str_SetPtr32W

320 stdcall DSA_Create(long long) DSA_Create
321 stdcall DSA_Destroy(ptr) DSA_Destroy
322 stdcall DSA_GetItem(ptr long long) DSA_GetItem
323 stdcall DSA_GetItemPtr(ptr long) DSA_GetItemPtr
324 stdcall DSA_InsertItem(ptr long long) DSA_InsertItem
325 stdcall DSA_SetItem (ptr long long) DSA_SetItem
326 stdcall DSA_DeleteItem(ptr long) DSA_DeleteItem
327 stdcall DSA_DeleteAllItems(ptr) DSA_DeleteAllItems
328 stdcall DPA_Create(long) DPA_Create
329 stdcall DPA_Destroy(ptr) DPA_Destroy
330 stdcall DPA_Grow(ptr long) DPA_Grow
331 stdcall DPA_Clone(ptr ptr) DPA_Clone
332 stdcall DPA_GetPtr(ptr long) DPA_GetPtr
333 stdcall DPA_GetPtrIndex(ptr ptr) DPA_GetPtrIndex
334 stdcall DPA_InsertPtr(ptr long ptr) DPA_InsertPtr
335 stdcall DPA_SetPtr(ptr long ptr) DPA_SetPtr
336 stdcall DPA_DeletePtr(ptr long) DPA_DeletePtr
337 stdcall DPA_DeleteAllPtrs(ptr) DPA_DeleteAllPtrs
338 stdcall DPA_Sort(ptr ptr long) DPA_Sort
339 stdcall DPA_Search(ptr ptr long ptr long long) DPA_Search
340 stdcall DPA_CreateEx(long long) DPA_CreateEx
341 stdcall SendNotify(long long long ptr) COMCTL32_SendNotify
342 stdcall SendNotifyEx(long long long ptr long) COMCTL32_SendNotifyEx

350 stdcall StrChrA(str str) COMCTL32_StrChrA
351 stub StrRChrA
352 stub StrCmpNA
353 stub StrCmpNIA
354 stub StrStrA
355 stdcall StrStrIA(str str) COMCTL32_StrStrIA
356 stub StrCSpnA
357 stdcall StrToIntA(str) COMCTL32_StrToIntA
358 stub StrChrW
359 stub StrRChrW
360 stub StrCmpNW
361 stub StrCmpNIW
362 stub StrStrW
363 stub StrStrIW
364 stub StrSpnW
365 stub StrToIntW
366 stub StrChrIA
367 stub StrChrIW
368 stub StrRChrIA
369 stub StrRChrIW

372 stub StrRStrIA
373 stub StrRStrIW
374 stub StrCSpnIA
375 stub StrCSpnIW
376 stub IntlStrEqWorkerA@16
377 stub IntlStrEqWorkerW@16

382 stub SmoothScrollWindow@4
383 stub DoReaderMode@4
384 stub SetPathWordBreakProc@8
385 stdcall COMCTL32_385(long long long) COMCTL32_385
386 stdcall COMCTL32_386(long long long) COMCTL32_386
387 stdcall COMCTL32_387(long long long) COMCTL32_387
388 stdcall COMCTL32_388(long long long) COMCTL32_388
389 stub COMCTL32_389
390 stub COMCTL32_390

400 stub CreateMRUListW@4
401 stub AddMRUStringW@8
402 stub FindMRUStringW@12
403 stub EnumMRUListW@16
404 stub COMCTL32_404

410 stub COMCTL32_410
411 stub COMCTL32_411
412 stub COMCTL32_412
413 stub COMCTL32_413

