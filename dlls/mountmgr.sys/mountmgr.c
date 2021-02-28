/*
 * Mount manager service implementation
 *
 * Copyright 2008 Alexandre Julliard
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

#ifdef __APPLE__
#include <CoreFoundation/CFString.h>
#define LoadResource mac_LoadResource
#define GetCurrentThread mac_GetCurrentThread
#include <CoreServices/CoreServices.h>
#undef LoadResource
#undef GetCurrentThread
#endif

#include <stdarg.h>
#include <unistd.h>

#define NONAMELESSUNION

#include "mountmgr.h"
#include "winreg.h"
#include "wine/list.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mountmgr);

#define MIN_ID_LEN     4

struct mount_point
{
    struct list    entry;   /* entry in mount points list */
    DEVICE_OBJECT *device;  /* disk device */
    UNICODE_STRING name;    /* device name */
    UNICODE_STRING link;    /* DOS device symlink */
    void          *id;      /* device unique id */
    unsigned int   id_len;
};

static struct list mount_points_list = LIST_INIT(mount_points_list);
static HKEY mount_key;

void set_mount_point_id( struct mount_point *mount, const void *id, unsigned int id_len )
{
    RtlFreeHeap( GetProcessHeap(), 0, mount->id );
    mount->id_len = max( MIN_ID_LEN, id_len );
    if ((mount->id = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, mount->id_len )))
    {
        memcpy( mount->id, id, id_len );
        RegSetValueExW( mount_key, mount->link.Buffer, 0, REG_BINARY, mount->id, mount->id_len );
    }
    else mount->id_len = 0;
}

static struct mount_point *add_mount_point( DEVICE_OBJECT *device, UNICODE_STRING *device_name,
                                            const WCHAR *link )
{
    struct mount_point *mount;
    WCHAR *str;
    UINT len = (strlenW(link) + 1) * sizeof(WCHAR) + device_name->Length + sizeof(WCHAR);

    if (!(mount = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*mount) + len ))) return NULL;

    str = (WCHAR *)(mount + 1);
    strcpyW( str, link );
    RtlInitUnicodeString( &mount->link, str );
    str += strlenW(str) + 1;
    memcpy( str, device_name->Buffer, device_name->Length );
    str[device_name->Length / sizeof(WCHAR)] = 0;
    mount->name.Buffer = str;
    mount->name.Length = device_name->Length;
    mount->name.MaximumLength = device_name->Length + sizeof(WCHAR);
    mount->device = device;
    mount->id = NULL;
    list_add_tail( &mount_points_list, &mount->entry );

    IoCreateSymbolicLink( &mount->link, device_name );

    TRACE( "created %s id %s for %s\n", debugstr_w(mount->link.Buffer),
           debugstr_a(mount->id), debugstr_w(mount->name.Buffer) );
    return mount;
}

/* create the DosDevices mount point symlink for a new device */
struct mount_point *add_dosdev_mount_point( DEVICE_OBJECT *device, UNICODE_STRING *device_name, int drive )
{
    static const WCHAR driveW[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\','%','c',':',0};
    WCHAR link[sizeof(driveW)];

    sprintfW( link, driveW, 'A' + drive );
    return add_mount_point( device, device_name, link );
}

/* create the Volume mount point symlink for a new device */
struct mount_point *add_volume_mount_point( DEVICE_OBJECT *device, UNICODE_STRING *device_name,
                                            const GUID *guid )
{
    static const WCHAR volumeW[] = {'\\','?','?','\\','V','o','l','u','m','e','{',
                                    '%','0','8','x','-','%','0','4','x','-','%','0','4','x','-',
                                    '%','0','2','x','%','0','2','x','-','%','0','2','x','%','0','2','x',
                                    '%','0','2','x','%','0','2','x','%','0','2','x','%','0','2','x','}',0};
    WCHAR link[sizeof(volumeW)];

    sprintfW( link, volumeW, guid->Data1, guid->Data2, guid->Data3,
              guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
              guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
    return add_mount_point( device, device_name, link );
}

/* delete the mount point symlinks when a device goes away */
void delete_mount_point( struct mount_point *mount )
{
    TRACE( "deleting %s\n", debugstr_w(mount->link.Buffer) );
    list_remove( &mount->entry );
    RegDeleteValueW( mount_key, mount->link.Buffer );
    IoDeleteSymbolicLink( &mount->link );
    RtlFreeHeap( GetProcessHeap(), 0, mount->id );
    RtlFreeHeap( GetProcessHeap(), 0, mount );
}

/* check if a given mount point matches the requested specs */
static BOOL matching_mount_point( const struct mount_point *mount, const MOUNTMGR_MOUNT_POINT *spec )
{
    if (spec->SymbolicLinkNameOffset)
    {
        const WCHAR *name = (const WCHAR *)((const char *)spec + spec->SymbolicLinkNameOffset);
        if (spec->SymbolicLinkNameLength != mount->link.Length) return FALSE;
        if (strncmpiW( name, mount->link.Buffer, mount->link.Length/sizeof(WCHAR)))
            return FALSE;
    }
    if (spec->DeviceNameOffset)
    {
        const WCHAR *name = (const WCHAR *)((const char *)spec + spec->DeviceNameOffset);
        if (spec->DeviceNameLength != mount->name.Length) return FALSE;
        if (strncmpiW( name, mount->name.Buffer, mount->name.Length/sizeof(WCHAR)))
            return FALSE;
    }
    if (spec->UniqueIdOffset)
    {
        const void *id = ((const char *)spec + spec->UniqueIdOffset);
        if (spec->UniqueIdLength != mount->id_len) return FALSE;
        if (memcmp( id, mount->id, mount->id_len )) return FALSE;
    }
    return TRUE;
}

/* implementation of IOCTL_MOUNTMGR_QUERY_POINTS */
static NTSTATUS query_mount_points( void *buff, SIZE_T insize,
                                    SIZE_T outsize, IO_STATUS_BLOCK *iosb )
{
    UINT count, pos, size;
    MOUNTMGR_MOUNT_POINT *input = buff;
    MOUNTMGR_MOUNT_POINTS *info;
    struct mount_point *mount;

    /* sanity checks */
    if (input->SymbolicLinkNameOffset + input->SymbolicLinkNameLength > insize ||
        input->UniqueIdOffset + input->UniqueIdLength > insize ||
        input->DeviceNameOffset + input->DeviceNameLength > insize ||
        input->SymbolicLinkNameOffset + input->SymbolicLinkNameLength < input->SymbolicLinkNameOffset ||
        input->UniqueIdOffset + input->UniqueIdLength < input->UniqueIdOffset ||
        input->DeviceNameOffset + input->DeviceNameLength < input->DeviceNameOffset)
        return STATUS_INVALID_PARAMETER;

    count = size = 0;
    LIST_FOR_EACH_ENTRY( mount, &mount_points_list, struct mount_point, entry )
    {
        if (!matching_mount_point( mount, input )) continue;
        size += mount->name.Length;
        size += mount->link.Length;
        size += mount->id_len;
        size = (size + sizeof(WCHAR) - 1) & ~(sizeof(WCHAR) - 1);
        count++;
    }
    pos = FIELD_OFFSET( MOUNTMGR_MOUNT_POINTS, MountPoints[count] );
    size += pos;

    if (size > outsize)
    {
        info = buff;
        if (size >= sizeof(info->Size)) info->Size = size;
        iosb->Information = sizeof(info->Size);
        return STATUS_MORE_ENTRIES;
    }

    input = HeapAlloc( GetProcessHeap(), 0, insize );
    if (!input)
        return STATUS_NO_MEMORY;
    memcpy( input, buff, insize );
    info = buff;

    info->NumberOfMountPoints = count;
    count = 0;
    LIST_FOR_EACH_ENTRY( mount, &mount_points_list, struct mount_point, entry )
    {
        if (!matching_mount_point( mount, input )) continue;

        info->MountPoints[count].DeviceNameOffset = pos;
        info->MountPoints[count].DeviceNameLength = mount->name.Length;
        memcpy( (char *)buff + pos, mount->name.Buffer, mount->name.Length );
        pos += mount->name.Length;

        info->MountPoints[count].SymbolicLinkNameOffset = pos;
        info->MountPoints[count].SymbolicLinkNameLength = mount->link.Length;
        memcpy( (char *)buff + pos, mount->link.Buffer, mount->link.Length );
        pos += mount->link.Length;

        info->MountPoints[count].UniqueIdOffset = pos;
        info->MountPoints[count].UniqueIdLength = mount->id_len;
        memcpy( (char *)buff + pos, mount->id, mount->id_len );
        pos += mount->id_len;
        pos = (pos + sizeof(WCHAR) - 1) & ~(sizeof(WCHAR) - 1);
        count++;
    }
    info->Size = pos;
    iosb->Information = pos;
    HeapFree( GetProcessHeap(), 0, input );
    return STATUS_SUCCESS;
}

/* implementation of IOCTL_MOUNTMGR_DEFINE_UNIX_DRIVE */
static NTSTATUS define_unix_drive( const void *in_buff, SIZE_T insize )
{
    const struct mountmgr_unix_drive *input = in_buff;
    const char *mount_point = NULL, *device = NULL;
    unsigned int i;
    WCHAR letter = tolowerW( input->letter );

    if (letter < 'a' || letter > 'z') return STATUS_INVALID_PARAMETER;
    if (input->type > DRIVE_RAMDISK) return STATUS_INVALID_PARAMETER;
    if (input->mount_point_offset > insize || input->device_offset > insize)
        return STATUS_INVALID_PARAMETER;

    /* make sure string are null-terminated */
    if (input->mount_point_offset)
    {
        mount_point = (const char *)in_buff + input->mount_point_offset;
        for (i = input->mount_point_offset; i < insize; i++)
            if (!*((const char *)in_buff + i)) break;
        if (i >= insize) return STATUS_INVALID_PARAMETER;
    }
    if (input->device_offset)
    {
        device = (const char *)in_buff + input->device_offset;
        for (i = input->device_offset; i < insize; i++)
            if (!*((const char *)in_buff + i)) break;
        if (i >= insize) return STATUS_INVALID_PARAMETER;
    }

    if (input->type != DRIVE_NO_ROOT_DIR)
    {
        enum device_type type = DEVICE_UNKNOWN;

        TRACE( "defining %c: dev %s mount %s type %u\n",
               letter, debugstr_a(device), debugstr_a(mount_point), input->type );
        switch (input->type)
        {
        case DRIVE_REMOVABLE: type = (letter >= 'c') ? DEVICE_HARDDISK : DEVICE_FLOPPY; break;
        case DRIVE_REMOTE:    type = DEVICE_NETWORK; break;
        case DRIVE_CDROM:     type = DEVICE_CDROM; break;
        case DRIVE_RAMDISK:   type = DEVICE_RAMDISK; break;
        case DRIVE_FIXED:     type = DEVICE_HARDDISK_VOL; break;
        }
        return add_dos_device( letter - 'a', NULL, device, mount_point, type, NULL, NULL );
    }
    else
    {
        TRACE( "removing %c:\n", letter );
        return remove_dos_device( letter - 'a', NULL );
    }
}

/* implementation of IOCTL_MOUNTMGR_QUERY_DHCP_REQUEST_PARAMS */
static NTSTATUS query_dhcp_request_params( void *buff, SIZE_T insize,
                                           SIZE_T outsize, IO_STATUS_BLOCK *iosb )
{
    struct mountmgr_dhcp_request_params *query = buff;
    ULONG i, offset;

    /* sanity checks */
    if (FIELD_OFFSET(struct mountmgr_dhcp_request_params, params[query->count]) > insize ||
        !memchrW( query->adapter, 0, ARRAY_SIZE(query->adapter) )) return STATUS_INVALID_PARAMETER;
    for (i = 0; i < query->count; i++)
        if (query->params[i].offset + query->params[i].size > insize) return STATUS_INVALID_PARAMETER;

    offset = FIELD_OFFSET(struct mountmgr_dhcp_request_params, params[query->count]);
    for (i = 0; i < query->count; i++)
    {
        offset += get_dhcp_request_param( query->adapter, &query->params[i], buff, offset, outsize - offset );
        if (offset > outsize)
        {
            if (offset >= sizeof(query->size)) query->size = offset;
            iosb->Information = sizeof(query->size);
            return STATUS_MORE_ENTRIES;
        }
    }

    iosb->Information = offset;
    return STATUS_SUCCESS;
}

/* implementation of Wine extension to use host APIs to find symbol file by GUID */
#ifdef __APPLE__
static void WINAPI query_symbol_file( TP_CALLBACK_INSTANCE *instance, void *context )
{
    IRP *irp = context;
    MOUNTMGR_TARGET_NAME *result;
    CFStringRef query_cfstring;
    WCHAR *filename, *unix_buf = NULL;
    ANSI_STRING unix_path;
    UNICODE_STRING path;
    MDQueryRef mdquery;
    const GUID *id;
    size_t size;
    NTSTATUS status = STATUS_NO_MEMORY;

    static const WCHAR formatW[] = { 'c','o','m','_','a','p','p','l','e','_','x','c','o','d','e',
                                     '_','d','s','y','m','_','u','u','i','d','s',' ','=','=',' ',
                                     '"','%','0','8','X','-','%','0','4','X','-',
                                     '%','0','4','X','-','%','0','2','X','%','0','2','X','-',
                                     '%','0','2','X','%','0','2','X','%','0','2','X','%','0','2','X',
                                     '%','0','2','X','%','0','2','X','"',0 };
    WCHAR query_string[ARRAY_SIZE(formatW)];

    id = irp->AssociatedIrp.SystemBuffer;
    sprintfW( query_string, formatW, id->Data1, id->Data2, id->Data3,
              id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
              id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7] );
    if (!(query_cfstring = CFStringCreateWithCharacters(NULL, query_string, lstrlenW(query_string)))) goto done;

    mdquery = MDQueryCreate(NULL, query_cfstring, NULL, NULL);
    CFRelease(query_cfstring);
    if (!mdquery) goto done;

    MDQuerySetMaxCount(mdquery, 1);
    TRACE("Executing %s\n", debugstr_w(query_string));
    if (MDQueryExecute(mdquery, kMDQuerySynchronous))
    {
        if (MDQueryGetResultCount(mdquery) >= 1)
        {
            MDItemRef item = (MDItemRef)MDQueryGetResultAtIndex(mdquery, 0);
            CFStringRef item_path = MDItemCopyAttribute(item, kMDItemPath);

            if (item_path)
            {
                CFIndex item_path_len = CFStringGetLength(item_path);
                if ((unix_buf = HeapAlloc(GetProcessHeap(), 0, (item_path_len + 1) * sizeof(WCHAR))))
                {
                    CFStringGetCharacters(item_path, CFRangeMake(0, item_path_len), unix_buf);
                    unix_buf[item_path_len] = 0;
                    TRACE("found %s\n", debugstr_w(unix_buf));
                }
                CFRelease(item_path);
            }
        }
        else status = STATUS_NO_MORE_ENTRIES;
    }
    CFRelease(mdquery);
    if (!unix_buf) goto done;

    RtlInitUnicodeString( &path, unix_buf );
    status = RtlUnicodeStringToAnsiString( &unix_path, &path, TRUE );
    HeapFree( GetProcessHeap(), 0, unix_buf );
    if (status) goto done;

    filename = wine_get_dos_file_name( unix_path.Buffer );
    RtlFreeAnsiString( &unix_path );
    if (!filename)
    {
        status = STATUS_NO_SUCH_FILE;
        goto done;
    }
    result = irp->AssociatedIrp.SystemBuffer;
    result->DeviceNameLength = lstrlenW(filename) * sizeof(WCHAR);
    size = FIELD_OFFSET(MOUNTMGR_TARGET_NAME, DeviceName[lstrlenW(filename)]);
    if (size <= IoGetCurrentIrpStackLocation(irp)->Parameters.DeviceIoControl.OutputBufferLength)
    {
        memcpy( result->DeviceName, filename, lstrlenW(filename) * sizeof(WCHAR) );
        irp->IoStatus.Information = size;
        status = STATUS_SUCCESS;
    }
    else
    {
        irp->IoStatus.Information = sizeof(*result);
        status = STATUS_BUFFER_OVERFLOW;
    }
    RtlFreeHeap( GetProcessHeap(), 0, filename );

done:
    irp->IoStatus.u.Status = status;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
}

static inline BOOL check_credential_string( const void *buf, SIZE_T buflen, ULONG size, ULONG offset )
{
    const WCHAR *ptr = buf;
    if (!size || size % sizeof(WCHAR) || offset + size > buflen || ptr[(offset + size) / sizeof(WCHAR) - 1]) return FALSE;
    return TRUE;
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
        int len;

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
        len = MultiByteToWideChar( CP_UTF8, 0, attr_list->attr[0].data, attr_list->attr[0].length, NULL, 0 );
        if (!(itemname = RtlAllocateHeap( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) )))
        {
            CFRelease( item );
            CFRelease( search );
            return NULL;
        }
        MultiByteToWideChar( CP_UTF8, 0, attr_list->attr[0].data, attr_list->attr[0].length, itemname, len );
        itemname[len] = 0;
        if (strcmpiW( itemname, name ))
        {
            CFRelease( item );
            RtlFreeHeap( GetProcessHeap(), 0, itemname );
            continue;
        }
        RtlFreeHeap( GetProcessHeap(), 0, itemname );
        SecKeychainItemFreeAttributesAndData( attr_list, NULL );
        CFRelease( search );
        return item;
    }

    CFRelease( search );
    return NULL;
}

static NTSTATUS fill_credential( SecKeychainItemRef item, BOOL require_password, void *buf, SIZE_T data_offset,
                                 SIZE_T buflen, SIZE_T *retlen )
{
    struct mountmgr_credential *cred = buf;
    int status, len;
    SIZE_T size;
    UInt32 i, cred_blob_len = 0;
    void *cred_blob;
    WCHAR *ptr;
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

                len = MultiByteToWideChar( CP_UTF8, 0, attr_list->attr[i].data, attr_list->attr[i].length, NULL, 0 );
                size = (len + 1) * sizeof(WCHAR);
                if (cred && *retlen + size <= buflen)
                {
                    cred->targetname_offset = data_offset;
                    cred->targetname_size   = size;
                    ptr = (WCHAR *)((char *)cred + cred->targetname_offset);
                    MultiByteToWideChar( CP_UTF8, 0, attr_list->attr[i].data, attr_list->attr[i].length, ptr, len );
                    ptr[len] = 0;
                    data_offset += size;
                }
                *retlen += size;
                break;
            case kSecAccountItemAttr:
            {
                TRACE( "kSecAccountItemAttr: %.*s\n", (int)attr_list->attr[i].length, (char *)attr_list->attr[i].data );
                if (cred) cred->username_offset = cred->username_size = 0;
                if (!attr_list->attr[i].data) continue;

                len = MultiByteToWideChar( CP_UTF8, 0, attr_list->attr[i].data, attr_list->attr[i].length, NULL, 0 );
                size = (len + 1) * sizeof(WCHAR);
                if (cred && *retlen + size <= buflen)
                {
                    cred->username_offset = data_offset;
                    cred->username_size   = size;
                    ptr = (WCHAR *)((char *)cred + cred->username_offset);
                    MultiByteToWideChar( CP_UTF8, 0, attr_list->attr[i].data, attr_list->attr[i].length, ptr, len );
                    ptr[len] = 0;
                    data_offset += size;
                }
                *retlen += size;
                break;
            }
            case kSecCommentItemAttr:
                TRACE( "kSecCommentItemAttr: %.*s\n", (int)attr_list->attr[i].length, (char *)attr_list->attr[i].data );
                if (cred) cred->comment_offset = cred->comment_size = 0;
                if (!attr_list->attr[i].data) continue;

                len = MultiByteToWideChar( CP_UTF8, 0, attr_list->attr[i].data, attr_list->attr[i].length, NULL, 0 );
                size = (len + 1) * sizeof(WCHAR);
                if (cred && *retlen + size <= buflen)
                {
                    cred->comment_offset = data_offset;
                    cred->comment_size   = size;
                    ptr = (WCHAR *)((char *)cred + cred->comment_offset);
                    len = MultiByteToWideChar( CP_UTF8, 0, attr_list->attr[i].data, attr_list->attr[i].length, ptr, len );
                    ptr[len] = 0;
                    data_offset += size;
                }
                *retlen += size;
                break;
            case kSecCreationDateItemAttr:
            {
                LARGE_INTEGER wintime;
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
                    RtlSecondsSince1970ToTime( time, &wintime );
                    cred->last_written.dwLowDateTime = wintime.u.LowPart;
                    cred->last_written.dwHighDateTime = wintime.u.HighPart;
                }
                break;
            }
            default:
                FIXME( "unhandled attribute %u\n", (unsigned)attr_list->attr[i].tag );
                break;
        }
    }

    if (cred)
    {
        if (*retlen + cred_blob_len <= buflen)
        {
            len = MultiByteToWideChar( CP_UTF8, 0, cred_blob, cred_blob_len, NULL, 0 );
            cred->blob_offset = data_offset;
            cred->blob_size   = len * sizeof(WCHAR);
            ptr = (WCHAR *)((char *)cred + cred->blob_offset);
            MultiByteToWideChar( CP_UTF8, 0, cred_blob, cred_blob_len, ptr, len );
        }
        else cred->blob_offset = cred->blob_size = 0;
    }
    *retlen += cred_blob_len;

    if (attr_list) SecKeychainItemFreeAttributesAndData( attr_list, cred_blob );
    return STATUS_SUCCESS;
}

static NTSTATUS read_credential( void *buff, SIZE_T insize, SIZE_T outsize, IO_STATUS_BLOCK *iosb )
{
    struct mountmgr_credential *cred = buff;
    const WCHAR *targetname;
    SecKeychainItemRef item;
    SIZE_T size;
    NTSTATUS status;

    if (!check_credential_string( buff, insize, cred->targetname_size, cred->targetname_offset ))
        return STATUS_INVALID_PARAMETER;
    targetname = (const WCHAR *)((const char *)cred + cred->targetname_offset);

    if (!(item = find_credential( targetname ))) return STATUS_NOT_FOUND;

    status = fill_credential( item, TRUE, cred, sizeof(*cred), outsize, &size );
    CFRelease( item );
    if (status != STATUS_SUCCESS) return status;

    iosb->Information = size;
    return (size > outsize) ? STATUS_BUFFER_OVERFLOW : STATUS_SUCCESS;
}

static NTSTATUS write_credential( void *buff, SIZE_T insize, SIZE_T outsize, IO_STATUS_BLOCK *iosb )
{
    const struct mountmgr_credential *cred = buff;
    int status, len, len_password = 0;
    const WCHAR *ptr;
    SecKeychainItemRef keychain_item;
    char *targetname, *username = NULL, *password = NULL;
    SecKeychainAttribute attrs[1];
    SecKeychainAttributeList attr_list;
    NTSTATUS ret = STATUS_NO_MEMORY;

    if (!check_credential_string( buff, insize, cred->targetname_size, cred->targetname_offset ) ||
        !check_credential_string( buff, insize, cred->username_size, cred->username_offset ) ||
        ((cred->blob_size && cred->blob_size % sizeof(WCHAR)) || cred->blob_offset + cred->blob_size > insize) ||
        (cred->comment_size && !check_credential_string( buff, insize, cred->comment_size, cred->comment_offset )) ||
        sizeof(*cred) + cred->targetname_size + cred->username_size + cred->blob_size + cred->comment_size > insize)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ptr = (const WCHAR *)((const char *)cred + cred->targetname_offset);
    len = WideCharToMultiByte( CP_UTF8, 0, ptr, -1, NULL, 0, NULL, NULL );
    if (!(targetname = RtlAllocateHeap( GetProcessHeap(), 0, len ))) goto error;
    WideCharToMultiByte( CP_UTF8, 0, ptr, -1, targetname, len, NULL, NULL );

    ptr = (const WCHAR *)((const char *)cred + cred->username_offset);
    len = WideCharToMultiByte( CP_UTF8, 0, ptr, -1, NULL, 0, NULL, NULL );
    if (!(username = RtlAllocateHeap( GetProcessHeap(), 0, len ))) goto error;
    WideCharToMultiByte( CP_UTF8, 0, ptr, -1, username, len, NULL, NULL );

    if (cred->blob_size)
    {
        ptr = (const WCHAR *)((const char *)cred + cred->blob_offset);
        len_password = WideCharToMultiByte( CP_UTF8, 0, ptr, cred->blob_size / sizeof(WCHAR), NULL, 0, NULL, NULL );
        if (!(password = RtlAllocateHeap( GetProcessHeap(), 0, len_password ))) goto error;
        WideCharToMultiByte( CP_UTF8, 0, ptr, cred->blob_size / sizeof(WCHAR), password, len_password, NULL, NULL );
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
    RtlFreeHeap( GetProcessHeap(), 0, username );
    RtlFreeHeap( GetProcessHeap(), 0, targetname );
    if (status != noErr)
    {
        RtlFreeHeap( GetProcessHeap(), 0, password );
        return STATUS_UNSUCCESSFUL;
    }
    if (cred->comment_size)
    {
        attr_list.count = 1;
        attr_list.attr  = attrs;
        attrs[0].tag    = kSecCommentItemAttr;
        ptr = (const WCHAR *)((const char *)cred + cred->comment_offset);
        attrs[0].length = WideCharToMultiByte( CP_UTF8, 0, ptr, -1, NULL, 0, NULL, NULL );
        if (attrs[0].length) attrs[0].length--;
        if (!(attrs[0].data = RtlAllocateHeap( GetProcessHeap(), 0, attrs[0].length ))) goto error;
        WideCharToMultiByte( CP_UTF8, 0, ptr, -1, attrs[0].data, attrs[0].length, NULL, NULL );
    }
    else
    {
        attr_list.count = 0;
        attr_list.attr  = NULL;
    }
    status = SecKeychainItemModifyAttributesAndData( keychain_item, &attr_list, cred->blob_preserve ? 0 : len_password,
                                                     cred->blob_preserve ? NULL : password );

    if (cred->comment_size) RtlFreeHeap( GetProcessHeap(), 0, attrs[0].data );
    RtlFreeHeap( GetProcessHeap(), 0, password );
    /* FIXME: set TargetAlias attribute */
    CFRelease( keychain_item );
    if (status != noErr) return STATUS_UNSUCCESSFUL;
    return STATUS_SUCCESS;

error:
    RtlFreeHeap( GetProcessHeap(), 0, username );
    RtlFreeHeap( GetProcessHeap(), 0, targetname );
    RtlFreeHeap( GetProcessHeap(), 0, password );
    return ret;
}

static NTSTATUS delete_credential( void *buff, SIZE_T insize, SIZE_T outsize, IO_STATUS_BLOCK *iosb )
{
    const struct mountmgr_credential *cred = buff;
    const WCHAR *targetname;
    SecKeychainItemRef item;

    if (!check_credential_string( buff, insize, cred->targetname_size, cred->targetname_offset ))
        return STATUS_INVALID_PARAMETER;
    targetname = (const WCHAR *)((const char *)cred + cred->targetname_offset);

    if (!(item = find_credential( targetname ))) return STATUS_NOT_FOUND;

    SecKeychainItemDelete( item );
    CFRelease( item );
    return STATUS_SUCCESS;
}

static BOOL match_credential( void *data, UInt32 data_len, const WCHAR *filter )
{
    int len;
    WCHAR *targetname;
    const WCHAR *p;
    BOOL ret;

    if (!*filter) return TRUE;

    len = MultiByteToWideChar( CP_UTF8, 0, data, data_len, NULL, 0 );
    if (!(targetname = RtlAllocateHeap( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) ))) return FALSE;
    MultiByteToWideChar( CP_UTF8, 0, data, data_len, targetname, len );
    targetname[len] = 0;

    TRACE( "comparing filter %s to target name %s\n", debugstr_w(filter), debugstr_w(targetname) );

    p = strchrW( filter, '*' );
    ret = CompareStringW( GetThreadLocale(), NORM_IGNORECASE, filter,
                          (p && !p[1]) ? p - filter : -1, targetname, (p && !p[1]) ? p - filter : -1 ) == CSTR_EQUAL;
    RtlFreeHeap( GetProcessHeap(), 0, targetname );
    return ret;
}

static NTSTATUS search_credentials( const WCHAR *filter, struct mountmgr_credential_list *list, SIZE_T *ret_count, SIZE_T *ret_size )
{
    SecKeychainSearchRef search;
    SecKeychainItemRef item;
    int status;
    ULONG i = 0;
    SIZE_T data_offset, data_size = 0, size;
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

static NTSTATUS enumerate_credentials( void *buff, SIZE_T insize, SIZE_T outsize, IO_STATUS_BLOCK *iosb )
{
    struct mountmgr_credential_list *list = buff;
    WCHAR *filter;
    SIZE_T size, count;
    Boolean saved_user_interaction_allowed;
    NTSTATUS status;

    if (!check_credential_string( buff, insize, list->filter_size, list->filter_offset )) return STATUS_INVALID_PARAMETER;
    if (!(filter = strdupW( (const WCHAR *)((const char *)list + list->filter_offset) ))) return STATUS_NO_MEMORY;

    SecKeychainGetUserInteractionAllowed( &saved_user_interaction_allowed );
    SecKeychainSetUserInteractionAllowed( false );

    if ((status = search_credentials( filter, NULL, &count, &size )) == STATUS_SUCCESS)
    {

        if (size > outsize)
        {
            if (size >= sizeof(list->size)) list->size = size;
            iosb->Information = sizeof(list->size);
            status = STATUS_MORE_ENTRIES;
        }
        else
        {
            list->size  = size;
            list->count = count;
            iosb->Information = size;
            status = search_credentials( filter, list, NULL, NULL );
        }
    }

    SecKeychainSetUserInteractionAllowed( saved_user_interaction_allowed );
    RtlFreeHeap( GetProcessHeap(), 0, filter );
    return status;
}
#endif /* __APPLE__ */

/* handler for ioctls on the mount manager device */
static NTSTATUS WINAPI mountmgr_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    NTSTATUS status;

    TRACE( "ioctl %x insize %u outsize %u\n",
           irpsp->Parameters.DeviceIoControl.IoControlCode,
           irpsp->Parameters.DeviceIoControl.InputBufferLength,
           irpsp->Parameters.DeviceIoControl.OutputBufferLength );

    switch(irpsp->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_MOUNTMGR_QUERY_POINTS:
        if (irpsp->Parameters.DeviceIoControl.InputBufferLength < sizeof(MOUNTMGR_MOUNT_POINT))
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        status = query_mount_points( irp->AssociatedIrp.SystemBuffer,
                                     irpsp->Parameters.DeviceIoControl.InputBufferLength,
                                     irpsp->Parameters.DeviceIoControl.OutputBufferLength,
                                     &irp->IoStatus );
        break;
    case IOCTL_MOUNTMGR_DEFINE_UNIX_DRIVE:
        if (irpsp->Parameters.DeviceIoControl.InputBufferLength < sizeof(struct mountmgr_unix_drive))
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        irp->IoStatus.Information = 0;
        status = define_unix_drive( irp->AssociatedIrp.SystemBuffer,
                                    irpsp->Parameters.DeviceIoControl.InputBufferLength );
        break;
    case IOCTL_MOUNTMGR_QUERY_UNIX_DRIVE:
        if (irpsp->Parameters.DeviceIoControl.InputBufferLength < sizeof(struct mountmgr_unix_drive))
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        status = query_unix_drive( irp->AssociatedIrp.SystemBuffer,
                                   irpsp->Parameters.DeviceIoControl.InputBufferLength,
                                   irpsp->Parameters.DeviceIoControl.OutputBufferLength,
                                   &irp->IoStatus );
        break;
    case IOCTL_MOUNTMGR_QUERY_DHCP_REQUEST_PARAMS:
        if (irpsp->Parameters.DeviceIoControl.InputBufferLength < sizeof(struct mountmgr_dhcp_request_params))
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        status = query_dhcp_request_params( irp->AssociatedIrp.SystemBuffer,
                                            irpsp->Parameters.DeviceIoControl.InputBufferLength,
                                            irpsp->Parameters.DeviceIoControl.OutputBufferLength,
                                            &irp->IoStatus );
        break;
#ifdef __APPLE__
    case IOCTL_MOUNTMGR_QUERY_SYMBOL_FILE:
        if (irpsp->Parameters.DeviceIoControl.InputBufferLength != sizeof(GUID)
            || irpsp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUNTMGR_TARGET_NAME))
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        if (TrySubmitThreadpoolCallback( query_symbol_file, irp, NULL ))
            return (irp->IoStatus.u.Status = STATUS_PENDING);
        status = STATUS_NO_MEMORY;
        break;
    case IOCTL_MOUNTMGR_READ_CREDENTIAL:
        if (irpsp->Parameters.DeviceIoControl.InputBufferLength < sizeof(struct mountmgr_credential))
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        status = read_credential( irp->AssociatedIrp.SystemBuffer,
                                  irpsp->Parameters.DeviceIoControl.InputBufferLength,
                                  irpsp->Parameters.DeviceIoControl.OutputBufferLength,
                                  &irp->IoStatus );
        break;
    case IOCTL_MOUNTMGR_WRITE_CREDENTIAL:
        if (irpsp->Parameters.DeviceIoControl.InputBufferLength < sizeof(struct mountmgr_credential))
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        status = write_credential( irp->AssociatedIrp.SystemBuffer,
                                   irpsp->Parameters.DeviceIoControl.InputBufferLength,
                                   irpsp->Parameters.DeviceIoControl.OutputBufferLength,
                                   &irp->IoStatus );
        break;
    case IOCTL_MOUNTMGR_DELETE_CREDENTIAL:
        if (irpsp->Parameters.DeviceIoControl.InputBufferLength < sizeof(struct mountmgr_credential))
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        status = delete_credential( irp->AssociatedIrp.SystemBuffer,
                                    irpsp->Parameters.DeviceIoControl.InputBufferLength,
                                    irpsp->Parameters.DeviceIoControl.OutputBufferLength,
                                    &irp->IoStatus );
        break;
    case IOCTL_MOUNTMGR_ENUMERATE_CREDENTIALS:
        if (irpsp->Parameters.DeviceIoControl.InputBufferLength < sizeof(struct mountmgr_credential_list))
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        status = enumerate_credentials( irp->AssociatedIrp.SystemBuffer,
                                        irpsp->Parameters.DeviceIoControl.InputBufferLength,
                                        irpsp->Parameters.DeviceIoControl.OutputBufferLength,
                                        &irp->IoStatus );
        break;
#endif
    default:
        FIXME( "ioctl %x not supported\n", irpsp->Parameters.DeviceIoControl.IoControlCode );
        status = STATUS_NOT_SUPPORTED;
        break;
    }
    irp->IoStatus.u.Status = status;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return status;
}

/* main entry point for the mount point manager driver */
NTSTATUS WINAPI DriverEntry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    static const WCHAR mounted_devicesW[] = {'S','y','s','t','e','m','\\','M','o','u','n','t','e','d','D','e','v','i','c','e','s',0};
    static const WCHAR device_mountmgrW[] = {'\\','D','e','v','i','c','e','\\','M','o','u','n','t','P','o','i','n','t','M','a','n','a','g','e','r',0};
    static const WCHAR link_mountmgrW[] = {'\\','?','?','\\','M','o','u','n','t','P','o','i','n','t','M','a','n','a','g','e','r',0};
    static const WCHAR harddiskW[] = {'\\','D','r','i','v','e','r','\\','H','a','r','d','d','i','s','k',0};
    static const WCHAR driver_serialW[] = {'\\','D','r','i','v','e','r','\\','S','e','r','i','a','l',0};
    static const WCHAR driver_parallelW[] = {'\\','D','r','i','v','e','r','\\','P','a','r','a','l','l','e','l',0};
    static const WCHAR devicemapW[] = {'H','A','R','D','W','A','R','E','\\','D','E','V','I','C','E','M','A','P','\\','S','c','s','i',0};

#ifdef _WIN64
    static const WCHAR qualified_ports_keyW[] = {'\\','R','E','G','I','S','T','R','Y','\\',
                                                 'M','A','C','H','I','N','E','\\','S','o','f','t','w','a','r','e','\\',
                                                 'W','i','n','e','\\','P','o','r','t','s'}; /* no null terminator */
    static const WCHAR wow64_ports_keyW[] = {'S','o','f','t','w','a','r','e','\\',
                                             'W','o','w','6','4','3','2','N','o','d','e','\\','W','i','n','e','\\',
                                             'P','o','r','t','s',0};
    static const WCHAR symbolic_link_valueW[] = {'S','y','m','b','o','l','i','c','L','i','n','k','V','a','l','u','e',0};
    HKEY wow64_ports_key = NULL;
#endif

    UNICODE_STRING nameW, linkW;
    DEVICE_OBJECT *device;
    HKEY devicemap_key;
    NTSTATUS status;

    TRACE( "%s\n", debugstr_w(path->Buffer) );

    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = mountmgr_ioctl;

    RtlInitUnicodeString( &nameW, device_mountmgrW );
    RtlInitUnicodeString( &linkW, link_mountmgrW );
    if (!(status = IoCreateDevice( driver, 0, &nameW, 0, 0, FALSE, &device )))
        status = IoCreateSymbolicLink( &linkW, &nameW );
    if (status)
    {
        FIXME( "failed to create device error %x\n", status );
        return status;
    }

    RegCreateKeyExW( HKEY_LOCAL_MACHINE, mounted_devicesW, 0, NULL,
                     REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &mount_key, NULL );

    if (!RegCreateKeyExW( HKEY_LOCAL_MACHINE, devicemapW, 0, NULL, REG_OPTION_VOLATILE,
                          KEY_ALL_ACCESS, NULL, &devicemap_key, NULL ))
        RegCloseKey( devicemap_key );

    RtlInitUnicodeString( &nameW, harddiskW );
    status = IoCreateDriver( &nameW, harddisk_driver_entry );

    initialize_dbus();
    initialize_diskarbitration();

#ifdef _WIN64
    /* create a symlink so that the Wine port overrides key can be edited with 32-bit reg or regedit */
    RegCreateKeyExW( HKEY_LOCAL_MACHINE, wow64_ports_keyW, 0, NULL, REG_OPTION_CREATE_LINK,
                     KEY_SET_VALUE, NULL, &wow64_ports_key, NULL );
    RegSetValueExW( wow64_ports_key, symbolic_link_valueW, 0, REG_LINK,
                    (BYTE *)qualified_ports_keyW, sizeof(qualified_ports_keyW) );
    RegCloseKey( wow64_ports_key );
#endif

    RtlInitUnicodeString( &nameW, driver_serialW );
    IoCreateDriver( &nameW, serial_driver_entry );

    RtlInitUnicodeString( &nameW, driver_parallelW );
    IoCreateDriver( &nameW, parallel_driver_entry );

    return status;
}
