name	oledlg
type	win32

  1 stdcall OleUIAddVerbMenuA(ptr str long long long long long long ptr) OleUIAddVerbMenu32A
  2 stdcall OleUICanConvertOrActivateAs(ptr long long) OleUICanConvertOrActivateAs32
  3 stdcall OleUIInsertObjectA(ptr) OleUIInsertObject32A
  4 stdcall OleUIPasteSpecialA(ptr) OleUIPasteSpecial32A
  5 stdcall OleUIEditLinksA(ptr) OleUIEditLinks32A
  6 stdcall OleUIChangeIconA(ptr) OleUIChangeIcon32A
  7 stdcall OleUIConvertA(ptr) OleUIConvert32A
  8 stdcall OleUIBusyA(ptr) OleUIBusy32A
  9 stdcall OleUIUpdateLinksA(ptr long str long) OleUIUpdateLinks32A
 10 varargs OleUIPromptUserA() OleUIPromptUser32A
 11 stdcall OleUIObjectPropertiesA(ptr) OleUIObjectProperties32A
 12 stdcall OleUIChangeSourceA(ptr) OleUIChangeSource32A
 13 varargs OleUIPromptUserW() OleUIPromptUser32W
 14 stdcall OleUIAddVerbMenuW(ptr wstr long long long long long long ptr) OleUIAddVerbMenu32W
 15 stdcall OleUIBusyW(ptr) OleUIBusy32W
 16 stdcall OleUIChangeIconW(ptr) OleUIChangeIcon32W
 17 stdcall OleUIChangeSourceW(ptr) OleUIChangeSource32W
 18 stdcall OleUIConvertW(ptr) OleUIConvert32W
 19 stdcall OleUIEditLinksW(ptr) OleUIEditLinks32W
 20 stdcall OleUIInsertObjectW(ptr) OleUIInsertObject32W
 21 stdcall OleUIObjectPropertiesW(ptr) OleUIObjectProperties32W
 22 stdcall OleUIPasteSpecialW(ptr) OleUIPasteSpecial32W

