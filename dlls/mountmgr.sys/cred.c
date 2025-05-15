/*
 * Mount manager macOS credentials support
 *
 * Copyright 2020 Hans Leidekker
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <CoreFoundation/CFString.h>
#define LoadResource mac_LoadResource
#define GetCurrentThread mac_GetCurrentThread
#include <CoreServices/CoreServices.h>
#undef LoadResource
#undef GetCurrentThread
#endif

#include "mountmgr.h"
#include "unixlib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mountmgr);

#ifdef __APPLE__

#define TICKSPERSEC        10000000
#define SECSPERDAY         86400
/* 1601 to 1970 is 369 years plus 89 leap days */
#define SECS_1601_TO_1970  ((369 * 365 + 89) * (ULONGLONG)SECSPERDAY)
#define TICKS_1601_TO_1970 (SECS_1601_TO_1970 * TICKSPERSEC)

/* implementation of Wine extension to use host APIs to find symbol file by GUID */
NTSTATUS query_symbol_file( void *args )
{
    const struct ioctl_params *params = args;
    char *result = params->buff;
    CFStringRef query_cfstring;
    MDQueryRef mdquery;
    const GUID *id = params->buff;
    NTSTATUS status = STATUS_NO_MEMORY;

    if (!(query_cfstring = CFStringCreateWithFormat(kCFAllocatorDefault, NULL,
                                                    CFSTR("com_apple_xcode_dsym_uuids == \"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\""),
                                                    id->Data1, id->Data2, id->Data3, id->Data4[0],
                                                    id->Data4[1], id->Data4[2], id->Data4[3], id->Data4[4],
                                                    id->Data4[5], id->Data4[6], id->Data4[7] )))
        return STATUS_NO_MEMORY;

    mdquery = MDQueryCreate(NULL, query_cfstring, NULL, NULL);
    CFRelease(query_cfstring);
    if (!mdquery) return STATUS_NO_MEMORY;

    MDQuerySetMaxCount(mdquery, 1);
    if (MDQueryExecute(mdquery, kMDQuerySynchronous))
    {
        if (MDQueryGetResultCount(mdquery) >= 1)
        {
            MDItemRef item = (MDItemRef)MDQueryGetResultAtIndex(mdquery, 0);
            CFStringRef item_path = MDItemCopyAttribute(item, kMDItemPath);

            if (item_path)
            {
                if (CFStringGetCString( item_path, result, params->outsize, kCFStringEncodingUTF8 ))
                {
                    *params->info = strlen( result ) + 1;
                    status = STATUS_SUCCESS;
                    TRACE("found %s\n", debugstr_a(result));
                }
                else status = STATUS_BUFFER_OVERFLOW;
                CFRelease(item_path);
            }
        }
        else status = STATUS_NO_MORE_ENTRIES;
    }
    CFRelease(mdquery);
    return status;
}

static inline BOOL check_credential_string( const void *buf, ULONG buflen, ULONG size, ULONG offset )
{
    const WCHAR *ptr = buf;
    if (!size || size % sizeof(WCHAR) || offset + size > buflen || ptr[(offset + size) / sizeof(WCHAR) - 1]) return FALSE;
    return TRUE;
}

static WCHAR *cred_umbstowcs( const char *src, ULONG srclen, ULONG *retlen )
{
    WCHAR *ret = malloc( (srclen + 1) * sizeof(WCHAR) );
    if (ret)
    {
        *retlen = ntdll_umbstowcs( src, srclen, ret, srclen );
        ret[*retlen] = 0;
    }
    return ret;
}

static char *cred_wcstoumbs( const WCHAR *src, ULONG srclen, ULONG *retlen )
{
    char *ret = malloc( srclen * 3 + 1 );
    if (ret)
    {
        *retlen = ntdll_wcstoumbs( src, srclen, ret, srclen * 3, FALSE );
        ret[*retlen] = 0;
    }
    return ret;
}

static SecKeychainItemRef find_credential( const WCHAR *name )
{
    int status;
    SecKeychainSearchRef search;
    SecKeychainItemRef item;

    status = SecKeychainSearchCreateFromAttributes( NULL, kSecGenericPasswordItemClass, NULL, &search );
    if (status != noErr) return NULL;

    while (SecKeychainSearchCopyNext( search, &item ) == noErr)
    {
        SecKeychainAttributeInfo info;
        SecKeychainAttributeList *attr_list;
        UInt32 info_tags[] = { kSecServiceItemAttr };
        WCHAR *itemname;
        ULONG len;

        info.count = ARRAY_SIZE(info_tags);
        info.tag = info_tags;
        info.format = NULL;
        status = SecKeychainItemCopyAttributesAndData( item, &info, NULL, &attr_list, NULL, NULL );
        if (status != noErr)
        {
            WARN( "SecKeychainItemCopyAttributesAndData returned status %d\n", status );
            continue;
        }
        if (attr_list->count != 1 || attr_list->attr[0].tag != kSecServiceItemAttr)
        {
            CFRelease( item );
            continue;
        }
        itemname = cred_umbstowcs( attr_list->attr[0].data, attr_list->attr[0].length, &len );
        if (!itemname)
        {
            CFRelease( item );
            CFRelease( search );
            return NULL;
        }
        if (wcsicmp( itemname, name ))
        {
            CFRelease( item );
            free( itemname );
            continue;
        }
        free( itemname );
        SecKeychainItemFreeAttributesAndData( attr_list, NULL );
        CFRelease( search );
        return item;
    }

    CFRelease( search );
    return NULL;
}

static NTSTATUS fill_credential( SecKeychainItemRef item, BOOL require_password, void *buf, ULONG data_offset,
                                 ULONG buflen, ULONG *retlen )
{
    struct mountmgr_credential *cred = buf;
    int status;
    ULONG size, len;
    UInt32 i, cred_blob_len = 0;
    void *cred_blob;
    WCHAR *str;
    BOOL user_name_present = FALSE;
    SecKeychainAttributeInfo info;
    SecKeychainAttributeList *attr_list = NULL;
    UInt32 info_tags[] = { kSecServiceItemAttr, kSecAccountItemAttr, kSecCommentItemAttr, kSecCreationDateItemAttr };

    info.count  = ARRAY_SIZE(info_tags);
    info.tag    = info_tags;
    info.format = NULL;
    status = SecKeychainItemCopyAttributesAndData( item, &info, NULL, &attr_list, &cred_blob_len, &cred_blob );
    if (status == errSecAuthFailed && !require_password)
    {
        cred_blob_len = 0;
        cred_blob     = NULL;
        status = SecKeychainItemCopyAttributesAndData( item, &info, NULL, &attr_list, &cred_blob_len, NULL );
    }
    if (status != noErr)
    {
        WARN( "SecKeychainItemCopyAttributesAndData returned status %d\n", status );
        return STATUS_NOT_FOUND;
    }
    for (i = 0; i < attr_list->count; i++)
    {
        if (attr_list->attr[i].tag == kSecAccountItemAttr && attr_list->attr[i].data)
        {
            user_name_present = TRUE;
            break;
        }
    }
    if (!user_name_present)
    {
        WARN( "no kSecAccountItemAttr for item\n" );
        SecKeychainItemFreeAttributesAndData( attr_list, cred_blob );
        return STATUS_NOT_FOUND;
    }

    *retlen = sizeof(*cred);
    for (i = 0; i < attr_list->count; i++)
    {
        switch (attr_list->attr[i].tag)
        {
            case kSecServiceItemAttr:
                TRACE( "kSecServiceItemAttr: %.*s\n", (int)attr_list->attr[i].length, (char *)attr_list->attr[i].data );
                if (cred) cred->targetname_offset = cred->targetname_size = 0;
                if (!attr_list->attr[i].data) continue;

                if (!(str = cred_umbstowcs( attr_list->attr[i].data, attr_list->attr[i].length, &len )))
                    continue;
                size = (len + 1) * sizeof(WCHAR);
                if (cred && *retlen + size <= buflen)
                {
                    cred->targetname_offset = data_offset;
                    cred->targetname_size   = size;
                    memcpy( (char *)cred + cred->targetname_offset, str, size );
                    data_offset += size;
                }
                free( str );
                *retlen += size;
                break;
            case kSecAccountItemAttr:
            {
                TRACE( "kSecAccountItemAttr: %.*s\n", (int)attr_list->attr[i].length, (char *)attr_list->attr[i].data );
                if (cred) cred->username_offset = cred->username_size = 0;
                if (!attr_list->attr[i].data) continue;

                if (!(str = cred_umbstowcs( attr_list->attr[i].data, attr_list->attr[i].length, &len )))
                    continue;
                size = (len + 1) * sizeof(WCHAR);
                if (cred && *retlen + size <= buflen)
                {
                    cred->username_offset = data_offset;
                    cred->username_size   = size;
                    memcpy( (char *)cred + cred->username_offset, str, size );
                    data_offset += size;
                }
                free( str );
                *retlen += size;
                break;
            }
            case kSecCommentItemAttr:
                TRACE( "kSecCommentItemAttr: %.*s\n", (int)attr_list->attr[i].length, (char *)attr_list->attr[i].data );
                if (cred) cred->comment_offset = cred->comment_size = 0;
                if (!attr_list->attr[i].data) continue;

                if (!(str = cred_umbstowcs( attr_list->attr[i].data, attr_list->attr[i].length, &len )))
                    continue;
                size = (len + 1) * sizeof(WCHAR);
                if (cred && *retlen + size <= buflen)
                {
                    cred->comment_offset = data_offset;
                    cred->comment_size   = size;
                    memcpy( (char *)cred + cred->comment_offset, str, size );
                    data_offset += size;
                }
                free( str );
                *retlen += size;
                break;
            case kSecCreationDateItemAttr:
            {
                ULONGLONG ticks;
                struct tm tm;
                time_t time;

                TRACE( "kSecCreationDateItemAttr: %.*s\n", (int)attr_list->attr[i].length, (char *)attr_list->attr[i].data );
                if (cred) cred->last_written.dwLowDateTime = cred->last_written.dwHighDateTime = 0;
                if (!attr_list->attr[i].data) continue;

                if (cred)
                {
                    memset( &tm, 0, sizeof(tm) );
                    strptime( attr_list->attr[i].data, "%Y%m%d%H%M%SZ", &tm );
                    time = mktime( &tm );
                    ticks = time * (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970;
                    cred->last_written.dwLowDateTime = ticks;
                    cred->last_written.dwHighDateTime = ticks >> 32;
                }
                break;
            }
            default:
                FIXME( "unhandled attribute %u\n", (unsigned)attr_list->attr[i].tag );
                break;
        }
    }

    if (cred) cred->blob_offset = cred->blob_size = 0;
    str = cred_umbstowcs( cred_blob, cred_blob_len, &len );
    size = len * sizeof(WCHAR);
    if (cred && *retlen + size <= buflen)
    {
        cred->blob_offset = data_offset;
        cred->blob_size   = size;
        memcpy( (char *)cred + cred->blob_offset, str, size );
    }
    free( str );
    *retlen += size;

    if (attr_list) SecKeychainItemFreeAttributesAndData( attr_list, cred_blob );
    return STATUS_SUCCESS;
}

NTSTATUS read_credential( void *args )
{
    const struct ioctl_params *params = args;
    struct mountmgr_credential *cred = params->buff;
    const WCHAR *targetname;
    SecKeychainItemRef item;
    ULONG size;
    NTSTATUS status;

    if (!check_credential_string( params->buff, params->insize, cred->targetname_size, cred->targetname_offset ))
        return STATUS_INVALID_PARAMETER;
    targetname = (const WCHAR *)((const char *)cred + cred->targetname_offset);

    if (!(item = find_credential( targetname ))) return STATUS_NOT_FOUND;

    status = fill_credential( item, TRUE, cred, sizeof(*cred), params->outsize, &size );
    CFRelease( item );
    if (status != STATUS_SUCCESS) return status;

    *params->info = size;
    return (size > params->outsize) ? STATUS_BUFFER_OVERFLOW : STATUS_SUCCESS;
}

NTSTATUS write_credential( void *args )
{
    const struct ioctl_params *params = args;
    const struct mountmgr_credential *cred = params->buff;
    int status;
    ULONG len, len_password = 0;
    const WCHAR *ptr;
    SecKeychainItemRef keychain_item;
    char *targetname, *username = NULL, *password = NULL;
    SecKeychainAttribute attrs[1];
    SecKeychainAttributeList attr_list;
    NTSTATUS ret = STATUS_NO_MEMORY;

    if (!check_credential_string( params->buff, params->insize, cred->targetname_size, cred->targetname_offset ) ||
        !check_credential_string( params->buff, params->insize, cred->username_size, cred->username_offset ) ||
        ((cred->blob_size && cred->blob_size % sizeof(WCHAR)) || cred->blob_offset + cred->blob_size > params->insize) ||
        (cred->comment_size && !check_credential_string( params->buff, params->insize, cred->comment_size, cred->comment_offset )) ||
        sizeof(*cred) + cred->targetname_size + cred->username_size + cred->blob_size + cred->comment_size > params->insize)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ptr = (const WCHAR *)((const char *)cred + cred->targetname_offset);
    if (!(targetname = cred_wcstoumbs( ptr, cred->targetname_size / sizeof(WCHAR), &len ))) goto error;

    ptr = (const WCHAR *)((const char *)cred + cred->username_offset);
    if (!(username = cred_wcstoumbs( ptr, cred->username_size / sizeof(WCHAR), &len ))) goto error;

    if (cred->blob_size)
    {
        ptr = (const WCHAR *)((const char *)cred + cred->blob_offset);
        if (!(password = cred_wcstoumbs( ptr, cred->blob_size / sizeof(WCHAR), &len_password ))) goto error;
    }

    TRACE("adding target %s, username %s using Keychain\n", targetname, username );
    status = SecKeychainAddGenericPassword( NULL, strlen(targetname), targetname, strlen(username), username,
                                            len_password, password, &keychain_item );
    if (status != noErr) ERR( "SecKeychainAddGenericPassword returned %d\n", status );
    if (status == errSecDuplicateItem)
    {
        status = SecKeychainFindGenericPassword( NULL, strlen(targetname), targetname, strlen(username), username, NULL,
                                                 NULL, &keychain_item );
        if (status != noErr) ERR( "SecKeychainFindGenericPassword returned %d\n", status );
    }
    free( username );
    free( targetname );
    if (status != noErr)
    {
        free( password );
        return STATUS_UNSUCCESSFUL;
    }
    if (cred->comment_size)
    {
        attr_list.count = 1;
        attr_list.attr  = attrs;
        attrs[0].tag    = kSecCommentItemAttr;
        ptr = (const WCHAR *)((const char *)cred + cred->comment_offset);
        if (!(attrs[0].data = cred_wcstoumbs( ptr, cred->comment_size / sizeof(WCHAR), &len ))) goto error;
        attrs[0].length = len - 1;
    }
    else
    {
        attr_list.count = 0;
        attr_list.attr  = NULL;
    }
    status = SecKeychainItemModifyAttributesAndData( keychain_item, &attr_list, cred->blob_preserve ? 0 : len_password,
                                                     cred->blob_preserve ? NULL : password );

    if (cred->comment_size) free( attrs[0].data );
    free( password );
    /* FIXME: set TargetAlias attribute */
    CFRelease( keychain_item );
    if (status != noErr) return STATUS_UNSUCCESSFUL;
    return STATUS_SUCCESS;

error:
    free( username );
    free( targetname );
    free( password );
    return ret;
}

NTSTATUS delete_credential( void *args )
{
    const struct ioctl_params *params = args;
    const struct mountmgr_credential *cred = params->buff;
    const WCHAR *targetname;
    SecKeychainItemRef item;

    if (!check_credential_string( params->buff, params->insize, cred->targetname_size, cred->targetname_offset ))
        return STATUS_INVALID_PARAMETER;
    targetname = (const WCHAR *)((const char *)cred + cred->targetname_offset);

    if (!(item = find_credential( targetname ))) return STATUS_NOT_FOUND;

    SecKeychainItemDelete( item );
    CFRelease( item );
    return STATUS_SUCCESS;
}

static BOOL match_credential( void *data, UInt32 data_len, const WCHAR *filter )
{
    ULONG len;
    WCHAR *targetname;
    const WCHAR *p;
    BOOL ret;

    if (!*filter) return TRUE;
    if (!(targetname = cred_umbstowcs( data, data_len, &len ))) return FALSE;

    TRACE( "comparing filter %s to target name %s\n", debugstr_w(filter), debugstr_w(targetname) );

    p = wcschr( filter, '*' );
    if (*p && !p[1]) ret = !wcsnicmp( filter, targetname, p - filter );
    else ret = !wcsicmp( filter, targetname );
    free( targetname );
    return ret;
}

static NTSTATUS search_credentials( const WCHAR *filter, struct mountmgr_credential_list *list, ULONG *ret_count, ULONG *ret_size )
{
    SecKeychainSearchRef search;
    SecKeychainItemRef item;
    int status;
    ULONG i = 0;
    ULONG data_offset, data_size = 0, size;
    NTSTATUS ret = STATUS_NOT_FOUND;

    status = SecKeychainSearchCreateFromAttributes( NULL, kSecGenericPasswordItemClass, NULL, &search );
    if (status != noErr)
    {
        ERR( "SecKeychainSearchCreateFromAttributes returned status %d\n", status );
        return STATUS_INTERNAL_ERROR;
    }

    while (SecKeychainSearchCopyNext( search, &item ) == noErr)
    {
        SecKeychainAttributeInfo info;
        SecKeychainAttributeList *attr_list;
        UInt32 info_tags[] = { kSecServiceItemAttr };
        BOOL match;

        info.count  = ARRAY_SIZE(info_tags);
        info.tag    = info_tags;
        info.format = NULL;
        status = SecKeychainItemCopyAttributesAndData( item, &info, NULL, &attr_list, NULL, NULL );
        if (status != noErr)
        {
            WARN( "SecKeychainItemCopyAttributesAndData returned status %d\n", status );
            CFRelease( item );
            continue;
        }
        if (attr_list->count != 1 || attr_list->attr[0].tag != kSecServiceItemAttr)
        {
            SecKeychainItemFreeAttributesAndData( attr_list, NULL );
            CFRelease( item );
            continue;
        }
        TRACE( "service item: %.*s\n", (int)attr_list->attr[0].length, (char *)attr_list->attr[0].data );

        match = match_credential( attr_list->attr[0].data, attr_list->attr[0].length, filter );
        SecKeychainItemFreeAttributesAndData( attr_list, NULL );
        if (!match)
        {
            CFRelease( item );
            continue;
        }

        if (!list) ret = fill_credential( item, FALSE, NULL, 0, 0, &size );
        else
        {
            data_offset = FIELD_OFFSET( struct mountmgr_credential_list, creds[list->count] ) -
                          FIELD_OFFSET( struct mountmgr_credential_list, creds[i] ) + data_size;
            ret = fill_credential( item, FALSE, &list->creds[i], data_offset, list->size - data_offset, &size );
        }

        CFRelease( item );
        if (ret == STATUS_NOT_FOUND) continue;
        if (ret != STATUS_SUCCESS) break;
        data_size += size - sizeof(struct mountmgr_credential);
        i++;
    }

    if (ret_count) *ret_count = i;
    if (ret_size) *ret_size = FIELD_OFFSET( struct mountmgr_credential_list, creds[i] ) + data_size;

    CFRelease( search );
    return ret;
}

NTSTATUS enumerate_credentials( void *args )
{
    const struct ioctl_params *params = args;
    struct mountmgr_credential_list *list = params->buff;
    WCHAR *filter;
    ULONG size, count;
    Boolean saved_user_interaction_allowed;
    NTSTATUS status;

    if (!check_credential_string( params->buff, params->insize, list->filter_size, list->filter_offset ))
        return STATUS_INVALID_PARAMETER;
    if (!(filter = malloc( list->filter_size ))) return STATUS_NO_MEMORY;
    memcpy( filter, (const char *)list + list->filter_offset, list->filter_size );

    SecKeychainGetUserInteractionAllowed( &saved_user_interaction_allowed );
    SecKeychainSetUserInteractionAllowed( false );

    if ((status = search_credentials( filter, NULL, &count, &size )) == STATUS_SUCCESS)
    {

        if (size > params->outsize)
        {
            if (size >= sizeof(list->size)) list->size = size;
            *params->info = sizeof(list->size);
            status = STATUS_BUFFER_OVERFLOW;
        }
        else
        {
            list->size  = size;
            list->count = count;
            *params->info = size;
            status = search_credentials( filter, list, NULL, NULL );
        }
    }

    SecKeychainSetUserInteractionAllowed( saved_user_interaction_allowed );
    free( filter );
    return status;
}

#else /* __APPLE__ */

NTSTATUS query_symbol_file( void *args )
{
    FIXME( "not supported\n" );
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS read_credential( void *args )
{
    FIXME( "not supported\n" );
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS write_credential( void *args )
{
    FIXME( "not supported\n" );
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS delete_credential( void *args )
{
    FIXME( "not supported\n" );
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS enumerate_credentials( void *args )
{
    FIXME( "not supported\n" );
    return STATUS_NOT_SUPPORTED;
}

#endif /* __APPLE__ */
