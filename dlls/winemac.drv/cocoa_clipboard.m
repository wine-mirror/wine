/*
 * MACDRV Cocoa clipboard code
 *
 * Copyright 2012, 2013 Ken Thomases for CodeWeavers Inc.
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

#include "macdrv_cocoa.h"
#import "cocoa_app.h"
#import "cocoa_event.h"
#import "cocoa_window.h"

#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

#if !defined(MAC_OS_X_VERSION_10_14) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_14
/* For older SDKs, #define the new names of constants deprecated/renamed in macOS 10.14. */
#define NSBitmapImageFileTypeBMP        NSBMPFileType
#define NSBitmapImageFileTypeGIF        NSGIFFileType
#define NSBitmapImageFileTypeJPEG       NSJPEGFileType
#define NSBitmapImageFileTypePNG        NSPNGFileType
#define NSBitmapImageFileTypeTIFF       NSTIFFFileType
#endif

static int owned_change_count = -1;
static int change_count = -1;

static NSDictionary<NSString *, NSNumber *> *BitmapOutputTypeMap;
static dispatch_once_t BitmapOutputTypesInitOnce;

static NSString* const OwnershipSentinel = @"org.winehq.wine.winemac.pasteboard-ownership-sentinel";


/***********************************************************************
 *              macdrv_is_pasteboard_owner
 */
int macdrv_is_pasteboard_owner(macdrv_window w)
{
    __block int ret;
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        NSPasteboard* pb = [NSPasteboard generalPasteboard];
        ret = ([pb changeCount] == owned_change_count);

        [window.queue discardEventsMatchingMask:event_mask_for_type(LOST_PASTEBOARD_OWNERSHIP)
                                      forWindow:window];
    });

    return ret;
}

/***********************************************************************
 *              macdrv_has_pasteboard_changed
 */
int macdrv_has_pasteboard_changed(void)
{
    __block int new_change_count;
    int ret;

    OnMainThread(^{
        NSPasteboard* pb = [NSPasteboard generalPasteboard];
        new_change_count = [pb changeCount];
    });

    ret = (change_count != new_change_count);
    change_count = new_change_count;
    return ret;
}

/***********************************************************************
 *              macdrv_copy_pasteboard_types
 *
 * Returns an array of UTI strings for the types of data available on
 * the pasteboard, or NULL on error.  The caller is responsible for
 * releasing the returned array with CFRelease().
 */
CFArrayRef macdrv_copy_pasteboard_types(CFTypeRef pasteboard)
{
@autoreleasepool
{
    NSPasteboard* pb = (NSPasteboard*)pasteboard;
    __block CFArrayRef ret = NULL;

    dispatch_once(&BitmapOutputTypesInitOnce, ^{
        BitmapOutputTypeMap =
        @{
                   @"public.tiff" : @(NSBitmapImageFileTypeTIFF),
                    @"public.png" : @(NSBitmapImageFileTypePNG),
             @"com.microsoft.bmp" : @(NSBitmapImageFileTypeBMP),
            @"com.compuserve.gif" : @(NSBitmapImageFileTypeGIF),
                   @"public.jpeg" : @(NSBitmapImageFileTypeJPEG),
        };
        [BitmapOutputTypeMap retain];
    });

    OnMainThread(^{
        @try
        {
            NSPasteboard* local_pb = pb;
            NSArray* types;

            if (!local_pb) local_pb = [NSPasteboard generalPasteboard];
            types = [local_pb types];

            // If there are any types understood by NSBitmapImageRep, then we
            // can offer all of the types that it can output, too.  For example,
            // if TIFF is on the pasteboard, we can offer PNG, BMP, etc. to the
            // Windows program.  We'll convert on demand.
            if ([types firstObjectCommonWithArray:[NSBitmapImageRep imageTypes]] ||
                [types firstObjectCommonWithArray:[NSBitmapImageRep imagePasteboardTypes]])
            {
                NSMutableArray<NSString *> *newTypes = [[BitmapOutputTypeMap allKeys] mutableCopy];
                [newTypes removeObjectsInArray:types];
                types = [types arrayByAddingObjectsFromArray:newTypes];
                [newTypes release];
            }

            ret = (CFArrayRef)[types copy];
        }
        @catch (id e)
        {
            ERR(@"Exception discarded while copying pasteboard types: %@\n", e);
        }
    });

    return ret;
}
}


/***********************************************************************
 *              macdrv_copy_pasteboard_data
 *
 * Returns the pasteboard data for a specified type, or NULL on error or
 * if there's no such type on the pasteboard.  The caller is responsible
 * for releasing the returned data object with CFRelease().
 */
CFDataRef macdrv_copy_pasteboard_data(CFTypeRef pasteboard, CFStringRef type)
{
    NSPasteboard* pb = (NSPasteboard*)pasteboard;
    __block NSData* ret = nil;

    OnMainThread(^{
        @try
        {
            NSPasteboard* local_pb = pb;
            if (!local_pb) local_pb = [NSPasteboard generalPasteboard];
            if ([local_pb availableTypeFromArray:@[(NSString*)type]])
                ret = [[local_pb dataForType:(NSString*)type] copy];
            else
            {
                NSNumber* bitmapType = BitmapOutputTypeMap[(NSString*)type];
                if (bitmapType)
                {
                    NSArray* reps = [NSBitmapImageRep imageRepsWithPasteboard:local_pb];
                    ret = [NSBitmapImageRep representationOfImageRepsInArray:reps
                                                                   usingType:[bitmapType unsignedIntegerValue]
                                                                  properties:nil];
                    ret = [ret copy];
                }
            }
        }
        @catch (id e)
        {
            ERR(@"Exception discarded while copying pasteboard types: %@\n", e);
        }
    });

    return (CFDataRef)ret;
}


/***********************************************************************
 *              macdrv_clear_pasteboard
 *
 * Takes ownership of the Mac pasteboard and clears it of all data types.
 */
void macdrv_clear_pasteboard(macdrv_window w)
{
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        @try
        {
            NSPasteboard* pb = [NSPasteboard generalPasteboard];
            owned_change_count = [pb declareTypes:@[OwnershipSentinel]
                                            owner:window];
            [window.queue discardEventsMatchingMask:event_mask_for_type(LOST_PASTEBOARD_OWNERSHIP)
                                          forWindow:window];
        }
        @catch (id e)
        {
            ERR(@"Exception discarded while clearing pasteboard: %@\n", e);
        }
    });
}


/***********************************************************************
 *              macdrv_set_pasteboard_data
 *
 * Sets the pasteboard data for a specified type.  Replaces any data of
 * that type already on the pasteboard.  If data is NULL, promises the
 * type.
 *
 * Returns 0 on error, non-zero on success.
 */
int macdrv_set_pasteboard_data(CFStringRef type, CFDataRef data, macdrv_window w)
{
    __block int ret = 0;
    WineWindow* window = (WineWindow*)w;

    OnMainThread(^{
        @try
        {
            NSPasteboard* pb = [NSPasteboard generalPasteboard];
            NSInteger change_count = [pb addTypes:@[(NSString*)type]
                                            owner:window];
            if (change_count)
            {
                owned_change_count = change_count;
                if (data)
                    ret = [pb setData:(NSData*)data forType:(NSString*)type];
                else
                    ret = 1;
            }
        }
        @catch (id e)
        {
            ERR(@"Exception discarded while copying pasteboard types: %@\n", e);
        }
    });

    return ret;
}
