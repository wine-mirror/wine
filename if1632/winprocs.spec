name	winprocs
id	24

1  pascal ButtonWndProc(word word word long) ButtonWndProc
2  pascal StaticWndProc(word word word long) StaticWndProc
3  pascal ScrollBarWndProc(word word word long) ScrollBarWndProc
4  pascal ListBoxWndProc(word word word long) ListBoxWndProc
5  pascal ComboBoxWndProc(word word word long) ComboBoxWndProc
6  pascal EditWndProc(word word word long) EditWndProc
7  pascal PopupMenuWndProc(word word word long) PopupMenuWndProc
8  pascal DesktopWndProc(word word word long) DesktopWndProc
9  pascal DefDlgProc(word word word long) DefDlgProc
10 pascal MDIClientWndProc(word word word long) MDIClientWndProc
11 pascal DefWindowProc(word word word long) DefWindowProc
12 pascal DefMDIChildProc(word word word long) DefMDIChildProc
13 pascal SystemMessageBoxProc(word word word long) SystemMessageBoxProc
14 pascal FileOpenDlgProc(word word word long) FileOpenDlgProc
15 pascal FileSaveDlgProc(word word word long) FileSaveDlgProc
16 pascal ColorDlgProc(word word word long) ColorDlgProc
17 pascal FindTextDlgProc(word word word long) FindTextDlgProc
18 pascal ReplaceTextDlgProc(word word word long) ReplaceTextDlgProc
19 pascal PrintSetupDlgProc(word word word long) PrintSetupDlgProc
20 pascal PrintDlgProc(word word word long) PrintDlgProc
21 pascal AboutDlgProc(word word word long) AboutDlgProc
22 pascal ComboLBoxWndProc(word word word long) ComboLBoxWndProc
23 pascal16 CARET_Callback(word word word long) CARET_Callback
24 pascal16 TASK_Reschedule() TASK_Reschedule

# Interrupt vectors 0-255 are ordinals 100-355
# Undefined functions are mapped to the dummy handler (ordinal 356)
# The 'word' parameter are the flags pushed on the stack by the interrupt

116 register INT_Int10Handler(word) INT_Int10Handler
119 register INT_Int13Handler(word) INT_Int13Handler
121 register INT_Int15Handler(word) INT_Int15Handler
122 register INT_Int16Handler(word) INT_Int16Handler
126 register INT_Int1aHandler(word) INT_Int1aHandler
133 register INT_Int21Handler(word) INT_Int21Handler
137 register INT_Int25Handler(word) INT_Int25Handler
138 register INT_Int26Handler(word) INT_Int26Handler
142 register INT_Int2aHandler(word) INT_Int2aHandler
147 register INT_Int2fHandler(word) INT_Int2fHandler
149 register INT_Int31Handler(word) INT_Int31Handler
192 register INT_Int5cHandler(word) INT_Int5cHandler

# Dummy interrupt vector
356 register INT_DummyHandler(word) INT_DummyHandler
