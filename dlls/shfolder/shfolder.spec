name shfolder
type win32

import shell32

@ forward SHGetFolderPathA shell32.SHGetFolderPathA
@ forward SHGetFolderPathW shell32.SHGetFolderPathW
