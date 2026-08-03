/*
 * Unix library interface for gphoto
 *
 * Copyright 2000 Corel Corporation
 * Copyright 2006 Marcus Meissner
 * Copyright 2021 Alexandre Julliard
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
/* Hack for gphoto2, which changes behaviour when WIN32 is set. */
#undef WIN32
#include <gphoto2/gphoto2-camera.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "unixlib.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

static Camera *camera;
static GPContext *context;

static char **files;
static unsigned int files_count;
static unsigned int files_size;

static void load_filesystem( const char *folder )
{
    const char *name, *ext;
    char *fullname;
    int		i, count, ret;
    CameraList	*list;

    ERR("%s\n",folder);
    ret = gp_list_new (&list);
    if (ret < GP_OK)
	return;
    ret = gp_camera_folder_list_files (camera, folder, list, context);
    if (ret < GP_OK) {
        ERR("list %d %p %p\n",ret, camera, context);
	gp_list_free (list);
	return;
    }
    count = gp_list_count (list);
    if (count < GP_OK) {
	gp_list_free (list);
	return;
    }
    for (i = 0; i < count; i++) {
	ret = gp_list_get_name (list, i, &name);
	if (ret < GP_OK)
	    continue;
        if (!(ext = strrchr( name, '.' ))) continue;
        if (strcmp( ext, ".jpg" ) && strcmp( ext, ".JPG" )) continue;

        if (files_count == files_size)
        {
            unsigned int size = max( 64, files_size * 2 );
            char **new = realloc( files, size * sizeof(*files) );
            if (!new) return;
            files = new;
            files_size = size;
        }
        fullname = malloc( strlen(folder) + 1 + strlen(name) + 1 );
        sprintf( fullname, "%s/%s", folder[1] ? folder : "", name );
	files[files_count++] = fullname;
	TRACE("adding %s\n", fullname);
    }
    gp_list_reset (list);

    ret = gp_camera_folder_list_folders (camera, folder, list, context);
    if (ret < GP_OK) {
	FIXME("list_folders failed\n");
	gp_list_free (list);
	return;
    }
    count = gp_list_count (list);
    if (count < GP_OK) {
	FIXME("list_folders failed\n");
	gp_list_free (list);
	return;
    }
    for (i = 0; i < count; i++) {
	ret = gp_list_get_name (list, i, &name);
	if (ret < GP_OK)
	    continue;
	TRACE("recursing into %s\n", name);
        fullname = malloc( strlen(folder) + 1 + strlen(name) + 1 );
        sprintf( fullname, "%s/%s", folder[1] ? folder : "", name );
	load_filesystem( fullname );
        free( fullname );
    }
    gp_list_free (list);
}

static NTSTATUS load_file_list( void *args )
{
    const struct load_file_list_params *params = args;

    if (!context) context = gp_context_new ();
    load_filesystem( params->root );
    *params->count = files_count;
    return STATUS_SUCCESS;
}

static void free_file_list(void)
{
    unsigned int i;

    for (i = 0; i < files_count; i++) free( files[i] );
    files_count = 0;
}

static NTSTATUS get_file_name( void *args )
{
    const struct get_file_name_params *params = args;
    char *name;
    unsigned int len;

    if (params->idx >= files_count) return STATUS_NO_MORE_FILES;
    name = strrchr( files[params->idx], '/' ) + 1;
    len = min( strlen(name) + 1, params->size );
    if (len)
    {
        memcpy( params->buffer, name, len - 1 );
        params->buffer[len - 1] = 0;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS open_file( void *args )
{
    const struct open_file_params *params = args;
    CameraFileType type = params->preview ? GP_FILE_TYPE_PREVIEW : GP_FILE_TYPE_NORMAL;
    CameraFile *file;
    char *folder, *filename;
    const char *filedata;
    unsigned long filesize;
    int ret;

    if (params->idx >= files_count) return STATUS_NO_MORE_FILES;
    folder = strdup( files[params->idx] );
    filename = strrchr( folder, '/' );
    *filename++ = 0;

    gp_file_new( &file );
    ret = gp_camera_file_get( camera, folder, filename, type, file, context );
    free( folder );
    if (ret < GP_OK)
    {
	FIXME( "Failed to get %s\n", files[params->idx] );
	gp_file_unref( file );
	return STATUS_NO_SUCH_FILE;
    }
    ret = gp_file_get_data_and_size( file, &filedata, &filesize );
    if (ret < GP_OK)
    {
	gp_file_unref( file );
	return STATUS_NO_SUCH_FILE;
    }
    *params->handle = (ULONG_PTR)file;
    *params->size = filesize;
    return STATUS_SUCCESS;
}

static NTSTATUS get_file_data( void *args )
{
    const struct get_file_data_params *params = args;
    CameraFile *file = (CameraFile *)(ULONG_PTR)params->handle;
    const char *filedata;
    unsigned long filesize;
    int ret;

    ret = gp_file_get_data_and_size( file, &filedata, &filesize );
    if (ret < GP_OK) return STATUS_NO_SUCH_FILE;
    if (filesize > params->size) return STATUS_BUFFER_TOO_SMALL;
    memcpy( params->data, filedata, filesize );
    return STATUS_SUCCESS;
}

static NTSTATUS close_file( void *args )
{
    const struct close_file_params *params = args;
    CameraFile *file = (CameraFile *)(ULONG_PTR)params->handle;

    gp_file_unref( file );
    return STATUS_SUCCESS;
}

#ifdef HAVE_GPHOTO2_PORT

static GPPortInfoList *port_list;
static int curcamera;
static CameraList *detected_cameras;
static CameraAbilitiesList *abilities_list;

static BOOL gphoto2_auto_detect(void)
{
    int result, count;

    if (detected_cameras && (gp_list_count (detected_cameras) == 0)) {
	/* Reload if previously no cameras, we might detect new ones. */
	TRACE("Reloading portlist trying to detect cameras.\n");
	if (port_list) {
	    gp_port_info_list_free (port_list);
	    port_list = NULL;
	}
    }
    if (!port_list) {
	TRACE("Auto detecting gphoto cameras.\n");
	TRACE("Loading ports...\n");
	if (gp_port_info_list_new (&port_list) < GP_OK)
	    return FALSE;
	result = gp_port_info_list_load (port_list);
	if (result < 0) {
	    gp_port_info_list_free (port_list);
	    return FALSE;
	}
	count = gp_port_info_list_count (port_list);
	if (count <= 0)
	    return FALSE;
	if (gp_list_new (&detected_cameras) < GP_OK)
	    return FALSE;
	if (!abilities_list) { /* Load only once per program start */
	    gp_abilities_list_new (&abilities_list);
	    TRACE("Loading cameras...\n");
	    gp_abilities_list_load (abilities_list, NULL);
	}
	TRACE("Detecting cameras...\n");
	gp_abilities_list_detect (abilities_list, port_list, detected_cameras, NULL);
        curcamera = 0;
        TRACE("%d cameras detected\n", gp_list_count(detected_cameras));
    }
    return TRUE;
}

static NTSTATUS get_identity( void *args )
{
    TW_IDENTITY *id = args;
    int count;
    const char *cname, *pname;

    if (!gphoto2_auto_detect()) return STATUS_DEVICE_NOT_CONNECTED;

    count = gp_list_count (detected_cameras);
    if (count < GP_OK) {
	gp_list_free (detected_cameras);
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    TRACE("%d cameras detected.\n", count);
    id->ProtocolMajor = TWON_PROTOCOLMAJOR;
    id->ProtocolMinor = TWON_PROTOCOLMINOR;
    id->SupportedGroups = DG_CONTROL | DG_IMAGE | DF_DS2;
    lstrcpynA (id->Manufacturer, "The Wine Team", sizeof(id->Manufacturer) - 1);
    lstrcpynA (id->ProductFamily, "GPhoto2 Camera", sizeof(id->ProductFamily) - 1);

    if (!count) { /* No camera detected. But we need to return an IDENTITY anyway. */
	lstrcpynA (id->ProductName, "GPhoto2 Camera", sizeof(id->ProductName) - 1);
        return STATUS_SUCCESS;
    }
    gp_list_get_name  (detected_cameras, curcamera, &cname);
    gp_list_get_value (detected_cameras, curcamera, &pname);
    if (count == 1) /* Normal case, only one camera. */
	snprintf (id->ProductName, sizeof(id->ProductName), "%s", cname);
    else
	snprintf (id->ProductName, sizeof(id->ProductName), "%s@%s", cname, pname);
    curcamera = (curcamera+1) % count;
    return STATUS_SUCCESS;
}

static NTSTATUS open_ds( void *args )
{
    TW_IDENTITY *id = args;
    int ret, m, p, count, i;
    CameraAbilities a;
    GPPortInfo info;
    const char	*model, *port;

    if (!gphoto2_auto_detect()) return STATUS_DEVICE_NOT_CONNECTED;

    if (strcmp(id->ProductFamily,"GPhoto2 Camera")) {
	FIXME("identity passed is not a gphoto camera, but %s!?!\n", id->ProductFamily);
	return STATUS_DEVICE_UNRESPONSIVE;
    }
    count = gp_list_count (detected_cameras);
    if (!count) {
	ERR("No camera found by autodetection. Returning failure.\n");
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    if (!strcmp (id->ProductName, "GPhoto2 Camera")) {
	TRACE("Potential undetected camera. Just using the first autodetected one.\n");
	i = 0;
    } else {
	for (i=0;i<count;i++) {
	    const char *cname, *pname;
	    TW_STR32	name;

	    gp_list_get_name  (detected_cameras, i, &cname);
	    gp_list_get_value (detected_cameras, i, &pname);
	    if (!strcmp(id->ProductName,cname))
		break;
	    snprintf(name, sizeof(name), "%s", cname);
	    if (!strcmp(id->ProductName,name))
		break;
	    snprintf(name, sizeof(name), "%s@%s", cname, pname);
	    if (!strcmp(id->ProductName,name))
		break;
        }
        if (i == count) {
	    TRACE("Camera %s not found in autodetected list. Using first entry.\n", id->ProductName);
	    i=0;
        }
    }
    gp_list_get_name  (detected_cameras, i, &model);
    gp_list_get_value  (detected_cameras, i, &port);
    TRACE("model %s, port %s\n", model, port);
    ret = gp_camera_new (&camera);
    if (ret < GP_OK) {
	ERR("gp_camera_new: %d\n", ret);
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    m = gp_abilities_list_lookup_model (abilities_list, model);
    if (m < GP_OK) {
	FIXME("Model %s not found, %d!\n", model, m);
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    ret = gp_abilities_list_get_abilities (abilities_list, m, &a);
    if (ret < GP_OK) {
	FIXME("gp_camera_list_get_abilities failed? %d\n", ret);
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    ret = gp_camera_set_abilities (camera, a);
    if (ret < GP_OK) {
	FIXME("gp_camera_set_abilities failed? %d\n", ret);
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    p = gp_port_info_list_lookup_path (port_list, port);
    if (p < GP_OK) {
	FIXME("port %s not in portlist?\n", port);
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    ret = gp_port_info_list_get_info (port_list, p, &info);
    if (ret < GP_OK) {
	FIXME("could not get portinfo for port %s?\n", port);
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    ret = gp_camera_set_port_info (camera, info);
    if (ret < GP_OK) {
	FIXME("could not set portinfo for port %s to camera?\n", port);
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS close_ds( void *args )
{
    if (!camera) return STATUS_SUCCESS;
    gp_camera_free (camera);
    free_file_list();
    return STATUS_SUCCESS;
}

#else  /* HAVE_GPHOTO2_PORT */

static NTSTATUS get_identity( void *args )
{
    return STATUS_DEVICE_NOT_CONNECTED;
}

static NTSTATUS open_ds( void *args )
{
    return STATUS_DEVICE_NOT_CONNECTED;
}

static NTSTATUS close_ds( void *args )
{
    return STATUS_SUCCESS;
}

#endif  /* HAVE_GPHOTO2_PORT */

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    get_identity,
    open_ds,
    close_ds,
    load_file_list,
    get_file_name,
    open_file,
    get_file_data,
    close_file,
};

#ifdef _WIN64

typedef ULONG PTR32;

static NTSTATUS wow64_load_file_list( void *args )
{
    struct
    {
        PTR32 root;
        PTR32 count;
    } const *params32 = args;

    struct load_file_list_params params =
    {
        ULongToPtr(params32->root),
        ULongToPtr(params32->count)
    };

    return load_file_list( &params );
}

static NTSTATUS wow64_get_file_name( void *args )
{
    struct
    {
        unsigned int  idx;
        unsigned int  size;
        PTR32         buffer;
    } const *params32 = args;

    struct get_file_name_params params =
    {
        params32->idx,
        params32->size,
        ULongToPtr(params32->buffer)
    };

    return get_file_name( &params );
}

static NTSTATUS wow64_open_file( void *args )
{
    struct
    {
        unsigned int  idx;
        BOOL          preview;
        PTR32         handle;
        PTR32         size;
    } const *params32 = args;

    struct open_file_params params =
    {
        params32->idx,
        params32->preview,
        ULongToPtr(params32->handle),
        ULongToPtr(params32->size)
    };

    return open_file( &params );
}

static NTSTATUS wow64_get_file_data( void *args )
{
    struct
    {
        UINT64       handle;
        PTR32        data;
        unsigned int size;
    } const *params32 = args;

    struct get_file_data_params params =
    {
        params32->handle,
        ULongToPtr(params32->data),
        params32->size
    };

    return get_file_data( &params );
}

static NTSTATUS wow64_close_file( void *args )
{
    struct
    {
        UINT64       handle;
    } const *params32 = args;

    struct close_file_params params = { params32->handle };

    return close_file( &params );
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    get_identity,
    open_ds,
    close_ds,
    wow64_load_file_list,
    wow64_get_file_name,
    wow64_open_file,
    wow64_get_file_data,
    wow64_close_file,
};

#endif  /* _WIN64 */
