/* OLE DB error codes.
 *
 * Copyright 2009 Huw Davies
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */


#ifndef __WINE_OLEDBERR_H
#define __WINE_OLEDBERR_H

#define DB_E_BADBINDINFO                    0x80040e08

#define DB_E_NOTFOUND                       0x80040e19

#define DB_E_UNSUPPORTEDCONVERSION          0x80040e1d

#define DB_E_ERRORSOCCURRED                 0x80040e21

#define DB_E_BADSTORAGEFLAG                 0x80040e26
#define DB_E_BADCOMPAREOP                   0x80040e27
#define DB_E_BADSTATUSVALUE                 0x80040e28

#define DB_E_CONCURRENCYVIOLATION           0x80040e38

#define DB_E_TABLEINUSE                     0x80040e40
#define DB_E_NOLOCALE                       0x80040e41
#define DB_E_BADRECORDNUM                   0x80040e42
#define DB_E_BOOKMARKSKIPPED                0x80040e43
#define DB_E_BADPROPERTYVALUE               0x80040e44
#define DB_E_INVALID                        0x80040e45

#define DB_E_ALREADYINITIALIZED             0x80040e52
#define DB_E_DATAOVERFLOW                   0x80040e57

#define DB_E_MISMATCHEDPROVIDER             0x80040e75

#define DB_S_ERRORSOCCURRED                 0x00040eda

#endif /* __WINE_OLEDBERR_H */
