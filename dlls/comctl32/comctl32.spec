# Functions exported by the Win95 comctl32.dll
# (these need to have these exact ordinals, because some win95 dlls
#  import comctl32.dll by ordinal)
#   This list was created from a comctl32.dll v5.81 (IE5.01).

  2 stdcall MenuHelp(long long long long long long ptr) MenuHelp
  3 stdcall ShowHideMenuCtl(long long ptr) ShowHideMenuCtl
  4 stdcall GetEffectiveClientRect(long long long) GetEffectiveClientRect
  5 stdcall DrawStatusTextA(long ptr str long) DrawStatusTextA
  6 stdcall CreateStatusWindowA(long str long long) CreateStatusWindowA
  7 stdcall CreateToolbar(long long long long long long ptr long) CreateToolbar
  8 stdcall CreateMappedBitmap(long long long ptr long) CreateMappedBitmap
  9 stdcall -noname DPA_LoadStream(ptr ptr ptr long) DPA_LoadStream
 10 stdcall -noname DPA_SaveStream(ptr ptr ptr long) DPA_SaveStream
 11 stdcall -noname DPA_Merge(ptr ptr long ptr ptr long) DPA_Merge
#12 stub Cctl1632_ThunkData32
 13 stdcall MakeDragList(long) MakeDragList
 14 stdcall LBItemFromPt(long long long long) LBItemFromPt
 15 stdcall DrawInsert(long long long) DrawInsert
 16 stdcall CreateUpDownControl(long long long long long long long long long long long long) CreateUpDownControl
 17 stdcall InitCommonControls() InitCommonControls
 71 stdcall -noname Alloc(long) COMCTL32_Alloc
 72 stdcall -noname ReAlloc(ptr long) COMCTL32_ReAlloc
 73 stdcall -noname Free(ptr) COMCTL32_Free
 74 stdcall -noname GetSize(ptr) COMCTL32_GetSize
151 stdcall -noname CreateMRUListA(ptr) CreateMRUListA
152 stdcall -noname FreeMRUList(long) FreeMRUList
153 stdcall -noname AddMRUStringA(long str) AddMRUStringA
154 stdcall -noname EnumMRUListA(long long ptr long) EnumMRUListA
155 stdcall -noname FindMRUStringA(long str ptr) FindMRUStringA
156 stdcall -noname DelMRUString(long long) DelMRUString
157 stdcall -noname CreateMRUListLazyA(ptr long long long) CreateMRUListLazyA
163 stub -noname CreatePage
164 stub -noname CreateProxyPage
167 stdcall -noname AddMRUData(long ptr long) AddMRUData
169 stdcall -noname FindMRUData(long ptr long ptr) FindMRUData
233 stdcall -noname Str_GetPtrA(str str long) Str_GetPtrA
234 stdcall -noname Str_SetPtrA(str str) Str_SetPtrA
235 stdcall -noname Str_GetPtrW(wstr wstr long) Str_GetPtrW
236 stdcall -noname Str_SetPtrW(wstr wstr) Str_SetPtrW
320 stdcall -noname DSA_Create(long long) DSA_Create
321 stdcall -noname DSA_Destroy(ptr) DSA_Destroy
322 stdcall -noname DSA_GetItem(ptr long long) DSA_GetItem
323 stdcall -noname DSA_GetItemPtr(ptr long) DSA_GetItemPtr
324 stdcall -noname DSA_InsertItem(ptr long long) DSA_InsertItem
325 stdcall -noname DSA_SetItem (ptr long long) DSA_SetItem
326 stdcall -noname DSA_DeleteItem(ptr long) DSA_DeleteItem
327 stdcall -noname DSA_DeleteAllItems(ptr) DSA_DeleteAllItems
328 stdcall -noname DPA_Create(long) DPA_Create
329 stdcall -noname DPA_Destroy(ptr) DPA_Destroy
330 stdcall -noname DPA_Grow(ptr long) DPA_Grow
331 stdcall -noname DPA_Clone(ptr ptr) DPA_Clone
332 stdcall -noname DPA_GetPtr(ptr long) DPA_GetPtr
333 stdcall -noname DPA_GetPtrIndex(ptr ptr) DPA_GetPtrIndex
334 stdcall -noname DPA_InsertPtr(ptr long ptr) DPA_InsertPtr
335 stdcall -noname DPA_SetPtr(ptr long ptr) DPA_SetPtr
336 stdcall -noname DPA_DeletePtr(ptr long) DPA_DeletePtr
337 stdcall -noname DPA_DeleteAllPtrs(ptr) DPA_DeleteAllPtrs
338 stdcall -noname DPA_Sort(ptr ptr long) DPA_Sort
339 stdcall -noname DPA_Search(ptr ptr long ptr long long) DPA_Search
340 stdcall -noname DPA_CreateEx(long long) DPA_CreateEx
341 stdcall -noname SendNotify(long long long ptr) COMCTL32_SendNotify
342 stdcall -noname SendNotifyEx(long long long ptr long) COMCTL32_SendNotifyEx
350 stdcall -noname StrChrA(str str) COMCTL32_StrChrA
351 stdcall -noname StrRChrA(str str long) COMCTL32_StrRChrA
352 stdcall -noname StrCmpNA(str str long) COMCTL32_StrCmpNA
353 stdcall -noname StrCmpNIA(str str long) COMCTL32_StrCmpNIA
354 stdcall -noname StrStrA(str str) COMCTL32_StrStrA
355 stdcall -noname StrStrIA(str str) COMCTL32_StrStrIA
356 stdcall -noname StrCSpnA(str str) COMCTL32_StrCSpnA
357 stdcall -noname StrToIntA(str) COMCTL32_StrToIntA
358 stdcall -noname StrChrW(wstr long) COMCTL32_StrChrW
359 stdcall -noname StrRChrW(wstr wstr long) COMCTL32_StrRChrW
360 stdcall -noname StrCmpNW(wstr wstr long) COMCTL32_StrCmpNW
361 stdcall -noname StrCmpNIW(wstr wstr long) COMCTL32_StrCmpNIW
362 stdcall -noname StrStrW(wstr wstr) COMCTL32_StrStrW
363 stdcall -noname StrStrIW(wstr wstr) COMCTL32_StrStrIW
364 stdcall -noname StrSpnW(wstr wstr) COMCTL32_StrSpnW
365 stdcall -noname StrToIntW(wstr) COMCTL32_StrToIntW
366 stub -noname StrChrIA
367 stub -noname StrChrIW
368 stub -noname StrRChrIA
369 stub -noname StrRChrIW
372 stub -noname StrRStrIA
373 stub -noname StrRStrIW
374 stub -noname StrCSpnIA
375 stub -noname StrCSpnIW
376 stub -noname IntlStrEqWorkerA
377 stub -noname IntlStrEqWorkerW
382 stdcall -noname SmoothScrollWindow(ptr) SmoothScrollWindow
383 stub -noname DoReaderMode
384 stub -noname SetPathWordBreakProc
385 stdcall -noname DPA_EnumCallback(long long long) DPA_EnumCallback
386 stdcall -noname DPA_DestroyCallback(ptr ptr long) DPA_DestroyCallback
387 stdcall -noname DSA_EnumCallback(ptr ptr long) DSA_EnumCallback
388 stdcall -noname DSA_DestroyCallback(ptr ptr long) DSA_DestroyCallback
389 stub @
390 stub @
400 stdcall -noname CreateMRUListW(ptr) CreateMRUListW
401 stdcall -noname AddMRUStringW(long wstr) AddMRUStringW
402 stdcall -noname FindMRUStringW(long wstr ptr) FindMRUStringW
403 stdcall -noname EnumMRUListW(long long ptr long) EnumMRUListW
404 stdcall -noname CreateMRUListLazyW(ptr long long long) CreateMRUListLazyW
410 stdcall @(long long long long) COMCTL32_410
411 stdcall @(long long long) COMCTL32_411
412 stdcall @(long long long) COMCTL32_412
413 stdcall @(long long long long) COMCTL32_413
414 stub @
415 stdcall @(long long long long long) COMCTL32_415
416 stub @
417 stdcall @(long long long long ptr wstr long ptr) COMCTL32_417
418 stub @
419 stdcall @(long long long long) COMCTL32_419
420 stub @
421 stub @

# Functions imported by name

@ stdcall CreatePropertySheetPage(ptr) CreatePropertySheetPageA
@ stdcall CreatePropertySheetPageA(ptr) CreatePropertySheetPageA
@ stdcall CreatePropertySheetPageW(ptr) CreatePropertySheetPageW
@ stdcall CreateStatusWindow(long str long long) CreateStatusWindowA
@ stdcall CreateStatusWindowW(long wstr long long) CreateStatusWindowW
@ stdcall CreateToolbarEx(long long long long long long ptr long long long long long long) CreateToolbarEx
@ stdcall DestroyPropertySheetPage(long) DestroyPropertySheetPage
@ stdcall DllGetVersion(ptr) COMCTL32_DllGetVersion
@ stdcall DllInstall(long ptr) COMCTL32_DllInstall
@ stdcall DrawStatusText(long ptr ptr long) DrawStatusTextA
@ stdcall DrawStatusTextW(long ptr wstr long) DrawStatusTextW
@ stdcall FlatSB_EnableScrollBar (long long long) FlatSB_EnableScrollBar
@ stdcall FlatSB_GetScrollInfo (long long ptr) FlatSB_GetScrollInfo
@ stdcall FlatSB_GetScrollPos (long long) FlatSB_GetScrollPos
@ stdcall FlatSB_GetScrollProp (long long ptr) FlatSB_GetScrollProp
@ stdcall FlatSB_GetScrollRange (long long ptr ptr) FlatSB_GetScrollRange
@ stdcall FlatSB_SetScrollInfo (long long ptr long) FlatSB_SetScrollInfo
@ stdcall FlatSB_SetScrollPos (long long long long) FlatSB_SetScrollPos
@ stdcall FlatSB_SetScrollProp (long long long long) FlatSB_SetScrollProp
@ stdcall FlatSB_SetScrollRange (long long long long long) FlatSB_SetScrollRange
@ stdcall FlatSB_ShowScrollBar (long long long) FlatSB_ShowScrollBar
@ stdcall GetMUILanguage() GetMUILanguage
@ stdcall ImageList_Add(ptr long long) ImageList_Add
@ stdcall ImageList_AddIcon(ptr long) ImageList_AddIcon
@ stdcall ImageList_AddMasked(ptr long long) ImageList_AddMasked
@ stdcall ImageList_BeginDrag(ptr long long long) ImageList_BeginDrag
@ stdcall ImageList_Copy(ptr long ptr long long) ImageList_Copy
@ stdcall ImageList_Create(long long long long long) ImageList_Create
@ stdcall ImageList_Destroy(ptr) ImageList_Destroy
@ stdcall ImageList_DragEnter(long long long) ImageList_DragEnter
@ stdcall ImageList_DragLeave(long) ImageList_DragLeave
@ stdcall ImageList_DragMove(long long) ImageList_DragMove
@ stdcall ImageList_DragShowNolock(long) ImageList_DragShowNolock
@ stdcall ImageList_Draw(ptr long long long long long) ImageList_Draw
@ stdcall ImageList_DrawEx(ptr long long long long long long long long long) ImageList_DrawEx
@ stdcall ImageList_DrawIndirect(ptr) ImageList_DrawIndirect
@ stdcall ImageList_Duplicate(ptr) ImageList_Duplicate
@ stdcall ImageList_EndDrag() ImageList_EndDrag
@ stdcall ImageList_GetBkColor(ptr) ImageList_GetBkColor
@ stdcall ImageList_GetDragImage(ptr ptr) ImageList_GetDragImage
@ stdcall ImageList_GetFlags(ptr) ImageList_GetFlags
@ stdcall ImageList_GetIcon(ptr long long) ImageList_GetIcon
@ stdcall ImageList_GetIconSize(ptr ptr ptr) ImageList_GetIconSize
@ stdcall ImageList_GetImageCount(ptr) ImageList_GetImageCount
@ stdcall ImageList_GetImageInfo(ptr long ptr) ImageList_GetImageInfo
@ stdcall ImageList_GetImageRect(ptr long ptr) ImageList_GetImageRect
@ stdcall ImageList_LoadImage(long str long long long long long) ImageList_LoadImageA
@ stdcall ImageList_LoadImageA(long str long long long long long) ImageList_LoadImageA
@ stdcall ImageList_LoadImageW(long wstr long long long long long) ImageList_LoadImageW
@ stdcall ImageList_Merge(ptr long ptr long long long) ImageList_Merge
@ stdcall ImageList_Read(ptr) ImageList_Read
@ stdcall ImageList_Remove(ptr long) ImageList_Remove
@ stdcall ImageList_Replace(ptr long long long) ImageList_Replace
@ stdcall ImageList_ReplaceIcon(ptr long long) ImageList_ReplaceIcon
@ stdcall ImageList_SetBkColor(ptr long) ImageList_SetBkColor
@ stdcall ImageList_SetDragCursorImage(ptr long long long) ImageList_SetDragCursorImage
@ stdcall ImageList_SetFilter(ptr long long) ImageList_SetFilter
@ stdcall ImageList_SetFlags(ptr long) ImageList_SetFlags
@ stdcall ImageList_SetIconSize(ptr long long) ImageList_SetIconSize
@ stdcall ImageList_SetImageCount(ptr long) ImageList_SetImageCount
@ stdcall ImageList_SetOverlayImage(ptr long long) ImageList_SetOverlayImage
@ stdcall ImageList_Write(ptr ptr) ImageList_Write
@ stdcall InitCommonControlsEx(ptr) InitCommonControlsEx
@ stdcall InitMUILanguage(long) InitMUILanguage
@ stdcall InitializeFlatSB(long) InitializeFlatSB
@ stdcall PropertySheet(ptr) PropertySheetA
@ stdcall PropertySheetA(ptr) PropertySheetA
@ stdcall PropertySheetW(ptr) PropertySheetW
@ stdcall UninitializeFlatSB(long) UninitializeFlatSB
@ stdcall _TrackMouseEvent(ptr) _TrackMouseEvent

# These are only available in comctrl 6
@ stdcall DefSubclassProc(long long long long) DefSubclassProc
@ stdcall GetWindowSubclass(long ptr long ptr) GetWindowSubclass
@ stdcall RemoveWindowSubclass(long ptr long) RemoveWindowSubclass
@ stdcall SetWindowSubclass(long ptr long long) SetWindowSubclass
