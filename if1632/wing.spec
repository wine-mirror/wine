name    wing
type    win16

1001 pascal16 WINGCREATEDC() WinGCreateDC16
1002 pascal16 WINGRECOMMENDDIBFORMAT(ptr) WinGRecommendDIBFormat16
1003 pascal16 WINGCREATEBITMAP(word ptr ptr) WinGCreateBitmap16
1004 pascal WINGGETDIBPOINTER(word ptr) WinGGetDIBPointer16
1005 pascal16 WINGGETDIBCOLORTABLE(word word word ptr) WinGGetDIBColorTable16
1006 pascal16 WINGSETDIBCOLORTABLE(word word word ptr) WinGSetDIBColorTable16
1007 pascal16 WINGCREATEHALFTONEPALETTE() WinGCreateHalfTonePalette16
1008 pascal16 WINGCREATEHALFTONEBRUSH(word word word) WinGCreateHalfToneBrush16
1009 pascal16 WINGSTRETCHBLT(word word word word word word word word word word) WinGStretchBlt16
1010 pascal16 WINGBITBLT(word word word word word word word word) WinGBitBlt16

# Seem that 1299 is the limit... weird...
#1500 stub WINGINITIALIZETHUNK16
#1501 stub WINGTHUNK16

#2000 stub REGISTERWINGPAL
#2001 stub EXCEPTIONHANDLER
