/******************************************************************************
 * winspool internal include file
 *
 *
 * Copyright 2005  Huw Davies
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

#include <windef.h>
#include <winuser.h>
#include <winternl.h>

extern HINSTANCE WINSPOOL_hInstance;

extern PRINTPROVIDOR * backend;
extern BOOL load_backend(void);

extern void WINSPOOL_LoadSystemPrinters(void);

#define IDS_CAPTION       10
#define IDS_FILE_EXISTS   11
#define IDS_CANNOT_OPEN   12

#define FILENAME_DIALOG  100
#define EDITBOX 201

struct printer_info
{
    WCHAR *name;
    WCHAR *comment;
    WCHAR *location;
    BOOL is_default;
};

struct enum_printers_params
{
    struct printer_info *printers;
    unsigned int *size;
    unsigned int *num;
};

struct get_default_page_size_params
{
    WCHAR *name;
    unsigned int *name_size;
};

struct get_ppd_params
{
    const WCHAR *printer; /* set to NULL to unlink */
    const WCHAR *ppd;
};

#define UNIX_CALL( func, params ) WINE_UNIX_CALL( unix_ ## func, params )

enum cups_funcs
{
    unix_process_attach,
    unix_enum_printers,
    unix_get_default_page_size,
    unix_get_ppd,
    unix_funcs_count,
};
