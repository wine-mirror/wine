name	mouse
type	win16

1 pascal16 Inquire(ptr) MouseInquire
2 pascal16 Enable(segptr) MouseEnable
3 pascal16 Disable() MouseDisable
4 stub MOUSEGETINTVECT
5 stub GETSETMOUSEDATA
#Control Panel thinks this is implemented if it is available
#6 stub CPLAPPLET
7 stub POWEREVENTPROC
