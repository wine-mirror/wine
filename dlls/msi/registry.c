/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Mike McCormak for CodeWeavers
 * Copyright 2005 Aric Stewart for CodeWeavers
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "msi.h"
#include "msipriv.h"
#include "wincrypt.h"
#include "wine/unicode.h"
#include "winver.h"
#include "winuser.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);


/* 
 * This module will be all the helper functions for registry access by the
 * installer bits. 
 */
static const WCHAR szUserFeatures_fmt[] = {
'S','o','f','t','w','a','r','e','\\',
'M','i','c','r','o','s','o','f','t','\\',
'I','n','s','t','a','l','l','e','r','\\',
'F','e','a','t','u','r','e','s','\\',
'%','s',0};

static const WCHAR szInstaller_Features[] = {
'S','o','f','t','w','a','r','e','\\',
'M','i','c','r','o','s','o','f','t','\\',
'W','i','n','d','o','w','s','\\',
'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
'I','n','s','t','a','l','l','e','r','\\',
'F','e','a','t','u','r','e','s',0 };

static const WCHAR szInstaller_Features_fmt[] = {
'S','o','f','t','w','a','r','e','\\',
'M','i','c','r','o','s','o','f','t','\\',
'W','i','n','d','o','w','s','\\',
'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
'I','n','s','t','a','l','l','e','r','\\',
'F','e','a','t','u','r','e','s','\\',
'%','s',0};

static const WCHAR szInstaller_Components[] = {
'S','o','f','t','w','a','r','e','\\',
'M','i','c','r','o','s','o','f','t','\\',
'W','i','n','d','o','w','s','\\',
'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
'I','n','s','t','a','l','l','e','r','\\',
'C','o','m','p','o','n','e','n','t','s',0 };

static const WCHAR szInstaller_Components_fmt[] = {
'S','o','f','t','w','a','r','e','\\',
'M','i','c','r','o','s','o','f','t','\\',
'W','i','n','d','o','w','s','\\',
'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
'I','n','s','t','a','l','l','e','r','\\',
'C','o','m','p','o','n','e','n','t','s','\\',
'%','s',0};

static const WCHAR szUninstall_fmt[] = {
'S','o','f','t','w','a','r','e','\\',
'M','i','c','r','o','s','o','f','t','\\',
'W','i','n','d','o','w','s','\\',
'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
'U','n','i','n','s','t','a','l','l','\\',
'%','s',0 };

static const WCHAR szUserProduct_fmt[] = {
'S','o','f','t','w','a','r','e','\\',
'M','i','c','r','o','s','o','f','t','\\',
'I','n','s','t','a','l','l','e','r','\\',
'P','r','o','d','u','c','t','s','\\',
'%','s',0};

static const WCHAR szInstaller_Products[] = {
'S','o','f','t','w','a','r','e','\\',
'M','i','c','r','o','s','o','f','t','\\',
'W','i','n','d','o','w','s','\\',
'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
'I','n','s','t','a','l','l','e','r','\\',
'P','r','o','d','u','c','t','s',0};

static const WCHAR szInstaller_Products_fmt[] = {
'S','o','f','t','w','a','r','e','\\',
'M','i','c','r','o','s','o','f','t','\\',
'W','i','n','d','o','w','s','\\',
'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
'I','n','s','t','a','l','l','e','r','\\',
'P','r','o','d','u','c','t','s','\\',
'%','s',0};

BOOL unsquash_guid(LPCWSTR in, LPWSTR out)
{
    DWORD i,n=0;

    out[n++]='{';
    for(i=0; i<8; i++)
        out[n++] = in[7-i];
    out[n++]='-';
    for(i=0; i<4; i++)
        out[n++] = in[11-i];
    out[n++]='-';
    for(i=0; i<4; i++)
        out[n++] = in[15-i];
    out[n++]='-';
    for(i=0; i<2; i++)
    {
        out[n++] = in[17+i*2];
        out[n++] = in[16+i*2];
    }
    out[n++]='-';
    for( ; i<8; i++)
    {
        out[n++] = in[17+i*2];
        out[n++] = in[16+i*2];
    }
    out[n++]='}';
    out[n]=0;
    return TRUE;
}

BOOL squash_guid(LPCWSTR in, LPWSTR out)
{
    DWORD i,n=0;

    if(in[n++] != '{')
        return FALSE;
    for(i=0; i<8; i++)
        out[7-i] = in[n++];
    if(in[n++] != '-')
        return FALSE;
    for(i=0; i<4; i++)
        out[11-i] = in[n++];
    if(in[n++] != '-')
        return FALSE;
    for(i=0; i<4; i++)
        out[15-i] = in[n++];
    if(in[n++] != '-')
        return FALSE;
    for(i=0; i<2; i++)
    {
        out[17+i*2] = in[n++];
        out[16+i*2] = in[n++];
    }
    if(in[n++] != '-')
        return FALSE;
    for( ; i<8; i++)
    {
        out[17+i*2] = in[n++];
        out[16+i*2] = in[n++];
    }
    out[32]=0;
    if(in[n++] != '}')
        return FALSE;
    if(in[n])
        return FALSE;
    return TRUE;
}


/* tables for encoding and decoding base85 */
static const unsigned char table_dec85[0x80] = {
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0x00,0xff,0xff,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0xff,
0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0xff,0xff,0xff,0x16,0xff,0x17,
0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0xff,0x34,0x35,0x36,
0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,
0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0xff,0x53,0x54,0xff,
};

static const char table_enc85[] =
"!$%&'()*+,-.0123456789=?@ABCDEFGHIJKLMNO"
"PQRSTUVWXYZ[]^_`abcdefghijklmnopqrstuvwx"
"yz{}~";

/*
 *  Converts a base85 encoded guid into a GUID pointer
 *  Base85 encoded GUIDs should be 20 characters long.
 *
 *  returns TRUE if successful, FALSE if not
 */
BOOL decode_base85_guid( LPCWSTR str, GUID *guid )
{
    DWORD i, val = 0, base = 1, *p;

    p = (DWORD*) guid;
    for( i=0; i<20; i++ )
    {
        if( (i%5) == 0 )
        {
            val = 0;
            base = 1;
        }
        val += table_dec85[str[i]] * base;
        if( str[i] >= 0x80 )
            return FALSE;
        if( table_dec85[str[i]] == 0xff )
            return FALSE;
        if( (i%5) == 4 )
            p[i/5] = val;
        base *= 85;
    }
    return TRUE;
}

/*
 *  Encodes a base85 guid given a GUID pointer
 *  Caller should provide a 21 character buffer for the encoded string.
 *
 *  returns TRUE if successful, FALSE if not
 */
BOOL encode_base85_guid( GUID *guid, LPWSTR str )
{
    unsigned int x, *p, i;

    p = (unsigned int*) guid;
    for( i=0; i<4; i++ )
    {
        x = p[i];
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
    }
    *str = 0;

    return TRUE;
}


UINT MSIREG_OpenUninstallKey(LPCWSTR szProduct, HKEY* key, BOOL create)
{
    UINT rc;
    WCHAR keypath[0x200];
    TRACE("%s\n",debugstr_w(szProduct));

    sprintfW(keypath,szUninstall_fmt,szProduct);

    if (create)
        rc = RegCreateKeyW(HKEY_LOCAL_MACHINE, keypath, key);
    else
        rc = RegOpenKeyW(HKEY_LOCAL_MACHINE, keypath, key);

    return rc;
}

UINT MSIREG_OpenUserProductsKey(LPCWSTR szProduct, HKEY* key, BOOL create)
{
    UINT rc;
    WCHAR squished_pc[GUID_SIZE];
    WCHAR keypath[0x200];

    TRACE("%s\n",debugstr_w(szProduct));
    squash_guid(szProduct,squished_pc);
    TRACE("squished (%s)\n", debugstr_w(squished_pc));

    sprintfW(keypath,szUserProduct_fmt,squished_pc);

    if (create)
        rc = RegCreateKeyW(HKEY_CURRENT_USER,keypath,key);
    else
        rc = RegOpenKeyW(HKEY_CURRENT_USER,keypath,key);

    return rc;
}

UINT MSIREG_OpenUserFeaturesKey(LPCWSTR szProduct, HKEY* key, BOOL create)
{
    UINT rc;
    WCHAR squished_pc[GUID_SIZE];
    WCHAR keypath[0x200];

    TRACE("%s\n",debugstr_w(szProduct));
    squash_guid(szProduct,squished_pc);
    TRACE("squished (%s)\n", debugstr_w(squished_pc));

    sprintfW(keypath,szUserFeatures_fmt,squished_pc);

    if (create)
        rc = RegCreateKeyW(HKEY_CURRENT_USER,keypath,key);
    else
        rc = RegOpenKeyW(HKEY_CURRENT_USER,keypath,key);

    return rc;
}

UINT MSIREG_OpenFeatures(HKEY* key)
{
    return RegCreateKeyW(HKEY_LOCAL_MACHINE,szInstaller_Features,key);
}

UINT MSIREG_OpenFeaturesKey(LPCWSTR szProduct, HKEY* key, BOOL create)
{
    UINT rc;
    WCHAR squished_pc[GUID_SIZE];
    WCHAR keypath[0x200];

    TRACE("%s\n",debugstr_w(szProduct));
    squash_guid(szProduct,squished_pc);
    TRACE("squished (%s)\n", debugstr_w(squished_pc));

    sprintfW(keypath,szInstaller_Features_fmt,squished_pc);

    if (create)
        rc = RegCreateKeyW(HKEY_LOCAL_MACHINE,keypath,key);
    else
        rc = RegOpenKeyW(HKEY_LOCAL_MACHINE,keypath,key);

    return rc;
}

UINT MSIREG_OpenComponents(HKEY* key)
{
    return RegCreateKeyW(HKEY_LOCAL_MACHINE,szInstaller_Components,key);
}

UINT MSIREG_OpenComponentsKey(LPCWSTR szComponent, HKEY* key, BOOL create)
{
    UINT rc;
    WCHAR squished_cc[GUID_SIZE];
    WCHAR keypath[0x200];

    TRACE("%s\n",debugstr_w(szComponent));
    squash_guid(szComponent,squished_cc);
    TRACE("squished (%s)\n", debugstr_w(squished_cc));

    sprintfW(keypath,szInstaller_Components_fmt,squished_cc);

    if (create)
        rc = RegCreateKeyW(HKEY_LOCAL_MACHINE,keypath,key);
    else
        rc = RegOpenKeyW(HKEY_LOCAL_MACHINE,keypath,key);

    return rc;
}

UINT MSIREG_OpenProductsKey(LPCWSTR szProduct, HKEY* key, BOOL create)
{
    UINT rc;
    WCHAR squished_pc[GUID_SIZE];
    WCHAR keypath[0x200];

    TRACE("%s\n",debugstr_w(szProduct));
    squash_guid(szProduct,squished_pc);
    TRACE("squished (%s)\n", debugstr_w(squished_pc));

    sprintfW(keypath,szInstaller_Products_fmt,squished_pc);

    if (create)
        rc = RegCreateKeyW(HKEY_LOCAL_MACHINE,keypath,key);
    else
        rc = RegOpenKeyW(HKEY_LOCAL_MACHINE,keypath,key);

    return rc;
}
