# First DirectPlay dll. Replaced by dplayx.dll.
name dplay
type win32

import  dplayx.dll

1 forward DirectPlayCreate DPLAYX.DirectPlayCreate
2 forward DirectPlayEnumerate DPLAYX.DirectPlayEnumerate
