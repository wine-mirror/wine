# $Id: gdi.spec,v 1.3 1993/07/04 04:04:21 root Exp root $
#
name	gdi
id	3
length	490

27  pascal Rectangle(word word word word word) Rectangle(1 2 3 4 5)
82  pascal GetObject(word word ptr) RSC_GetObject(1 2 3)
87  pascal GetStockObject(word) GetStockObject(1)
