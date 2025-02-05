/*
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

#ifndef _SECURITY_H
#define _SECURITY_H

#ifndef NEGOSSP_NAME
#define NEGOSSP_NAME_A "Negotiate"
#if defined(_MSC_VER) || defined(__MINGW32__)
#define NEGOSSP_NAME_W L"Negotiate"
#else
static const WCHAR NEGOSSP_NAME_W[] = { 'N','e','g','o','t','i','a','t','e',0 };
#endif
#define NEGOSSP_NAME WINELIB_NAME_AW(NEGOSSP_NAME_)
#endif /* NEGOSSP_NAME */

#include <sspi.h>

#if defined(SECURITY_WIN32) || defined(SECURITY_KERNEL)
#include <secext.h>
#endif

#endif /* _SECURITY_H */
