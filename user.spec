# $Id: user.spec,v 1.2 1993/06/30 14:24:33 root Exp root $
#
name	user
id	2
length	540

5   pascal InitApp(word) USER_InitApp(1)
175 pascal LoadBitmap(word ptr) RSC_LoadBitmap(1 2)
176 pascal LoadString(word word ptr s_word) RSC_LoadString(1 2 3 4)
