name    wing
type    win16

1001 pascal16 WinGCreateDC() WinGCreateDC16
1002 pascal16 WinGRecommendDIBFormat(ptr) WinGRecommendDIBFormat16
1003 pascal16 WinGCreateBitmap(word ptr ptr) WinGCreateBitmap16
1004 stub WINGGETDIBPOINTER
1005 stub WINGGETDIBCOLORTABLE
1006 stub WINGSETDIBCOLORTABLE
1007 pascal16 WinGCreateHalfTonePalette() WinGCreateHalfTonePalette16
1008 pascal16 WinGCreateHalfToneBrush(word long word) WinGCreateHalfToneBrush16
1009 stub WINGSTRETCHBLT
# Probably much like BitBlt16... but, without the last field (what value?)
1010 stub WINGBITBLT

# Seem that 1299 is the limit... weird...
#1500 stub WINGINITIALIZETHUNK16
#1501 stub WINGTHUNK16

#2000 stub REGISTERWINGPAL
#2001 stub EXCEPTIONHANDLER
