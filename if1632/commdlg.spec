# $Id: commdlg.spec,v 1.3 1994/20/08 04:04:21 root Exp root $
#
name	commdlg
id	15
length	31

  1   pascal  GETOPENFILENAME(ptr) GetOpenFileName(1)
  2   pascal  GETSAVEFILENAME(ptr) GetSaveFileName(1)
  5   pascal  CHOOSECOLOR(ptr) ChooseColor(1)
  6   pascal  FILEOPENDLGPROC(word word word long) FileOpenDlgProc(1 2 3 4)
  7   pascal  FILESAVEDLGPROC(word word word long) FileSaveDlgProc(1 2 3 4)
  8   pascal  COLORDLGPROC(word word word long) ColorDlgProc(1 2 3 4)
#  9   pascal  LOADALTERBITMAP exported, shared data
 11   pascal  FINDTEXT(ptr) FindText(1)
 12   pascal  REPLACETEXT(ptr) ReplaceText(1)
 13   pascal  FINDTEXTDLGPROC(word word word long) FindTextDlgProc(1 2 3 4)
 14   pascal  REPLACETEXTDLGPROC(word word word long) ReplaceTextDlgProc(1 2 3 4)
# 15   pascal  CHOOSEFONT exported, shared data
# 16   pascal  FORMATCHARDLGPROC exported, shared data
# 18   pascal  FONTSTYLEENUMPROC exported, shared data
# 19   pascal  FONTFAMILYENUMPROC exported, shared data
 20   pascal  PRINTDLG(ptr) PrintDlg(1)
 21   pascal  PRINTDLGPROC(word word word long) PrintDlgProc(1 2 3 4)
 22   pascal  PRINTSETUPDLGPROC(word word word long) PrintSetupDlgProc(1 2 3 4)
# 23   pascal  EDITINTEGERONLY exported, shared data
# 25   pascal  WANTARROWS exported, shared data
 26   pascal  COMMDLGEXTENDEDERROR() CommDlgExtendError()
 27   pascal  GETFILETITLE(ptr ptr word) GetFileTitle(1 2 3)
# 28   pascal  WEP exported, shared data
# 29   pascal  DWLBSUBCLASS exported, shared data
# 30   pascal  DWUPARROWHACK exported, shared data
# 31   pascal  DWOKSUBCLASS exported, shared data
