# $Id: commdlg.spec,v 1.3 1994/20/08 04:04:21 root Exp root $
#
name	commdlg
id	14
length	31

  1   pascal  GETOPENFILENAME(ptr) GetOpenFileName
  2   pascal  GETSAVEFILENAME(ptr) GetSaveFileName
  5   pascal  CHOOSECOLOR(ptr) ChooseColor
  6   pascal  FILEOPENDLGPROC(word word word long) FileOpenDlgProc
  7   pascal  FILESAVEDLGPROC(word word word long) FileSaveDlgProc
  8   pascal  COLORDLGPROC(word word word long) ColorDlgProc
#  9   pascal  LOADALTERBITMAP exported, shared data
 11   pascal  FINDTEXT(ptr) FindText
 12   pascal  REPLACETEXT(ptr) ReplaceText
 13   pascal  FINDTEXTDLGPROC(word word word long) FindTextDlgProc
 14   pascal  REPLACETEXTDLGPROC(word word word long) ReplaceTextDlgProc
# 15   pascal  CHOOSEFONT exported, shared data
# 16   pascal  FORMATCHARDLGPROC exported, shared data
# 18   pascal  FONTSTYLEENUMPROC exported, shared data
# 19   pascal  FONTFAMILYENUMPROC exported, shared data
 20   pascal  PRINTDLG(ptr) PrintDlg
 21   pascal  PRINTDLGPROC(word word word long) PrintDlgProc
 22   pascal  PRINTSETUPDLGPROC(word word word long) PrintSetupDlgProc
# 23   pascal  EDITINTEGERONLY exported, shared data
# 25   pascal  WANTARROWS exported, shared data
 26   pascal  COMMDLGEXTENDEDERROR() CommDlgExtendError
 27   pascal  GETFILETITLE(ptr ptr word) GetFileTitle
# 28   pascal  WEP exported, shared data
# 29   pascal  DWLBSUBCLASS exported, shared data
# 30   pascal  DWUPARROWHACK exported, shared data
# 31   pascal  DWOKSUBCLASS exported, shared data
