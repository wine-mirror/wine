name dplayx
type win32

  1 stdcall DirectPlayCreate(ptr ptr ptr ptr) DirectPlayCreate
  2 stdcall DirectPlayEnumerateA(ptr ptr) DirectPlayEnumerateA
  3 stdcall DirectPlayEnumerateW(ptr ptr) DirectPlayEnumerateW
  4 stdcall DirectPlayLobbyCreateA(ptr ptr ptr ptr long) DirectPlayLobbyCreateA
  5 stdcall DirectPlayLobbyCreateW(ptr ptr ptr ptr long) DirectPlayLobbyCreateW
  9 stub DirectPlayEnumerate
