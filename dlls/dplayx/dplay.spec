# First DirectPlay dll. Replaced by dplayx.dll.
name dplay
type win32

import  dplayx.dll

@ forward DirectPlayCreate DPLAYX.DirectPlayCreate
@ forward DirectPlayEnumerate DPLAYX.DirectPlayEnumerate
