name	commdlg
id	14

1   pascal16 GetOpenFileName(ptr) GetOpenFileName
2   pascal16 GetSaveFileName(ptr) GetSaveFileName
5   pascal16 ChooseColor(ptr) ChooseColor
6   pascal   FileOpenDlgProc(word word word long) FileOpenDlgProc
7   pascal   FileSaveDlgProc(word word word long) FileSaveDlgProc
8   pascal   ColorDlgProc(word word word long) ColorDlgProc
#9   pascal  LOADALTERBITMAP exported, shared data
11  pascal16 FindText(ptr) FindText
12  pascal16 ReplaceText(ptr) ReplaceText
13  pascal   FindTextDlgProc(word word word long) FindTextDlgProc
14  pascal   ReplaceTextDlgProc(word word word long) ReplaceTextDlgProc
15  stub ChooseFont
#16  pascal  FORMATCHARDLGPROC exported, shared data
#18  pascal  FONTSTYLEENUMPROC exported, shared data
#19  pascal  FONTFAMILYENUMPROC exported, shared data
20  pascal16 PrintDlg(ptr) PrintDlg
21  pascal   PrintDlgProc(word word word long) PrintDlgProc
22  pascal   PrintSetupDlgProc(word word word long) PrintSetupDlgProc
#23  pascal  EDITINTEGERONLY exported, shared data
#25  pascal  WANTARROWS exported, shared data
26  pascal   CommDlgExtendedError() CommDlgExtendedError
27  pascal16 GetFileTitle(ptr ptr word) GetFileTitle
#28  pascal  WEP exported, shared data
#29  pascal  DWLBSUBCLASS exported, shared data
#30  pascal  DWUPARROWHACK exported, shared data
#31  pascal  DWOKSUBCLASS exported, shared data
