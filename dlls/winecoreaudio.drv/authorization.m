/*
 * CoreAudio audio capture authorization functions
 *
 * Copyright 2020 Brendan Shanks for CodeWeavers
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

#ifdef HAVE_AVFOUNDATION_AVFOUNDATION_H
#import <AVFoundation/AVFoundation.h>
#endif

#include "coreaudio_cocoa.h"

#ifdef HAVE_AVFOUNDATION_AVFOUNDATION_H

#if !defined(MAC_OS_X_VERSION_10_14) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_14

@interface AVCaptureDevice (WineCaptureAuthorizationExtensions)
    + (NSInteger)authorizationStatusForMediaType:(AVMediaType)mediaType;
    + (void)requestAccessForMediaType:(AVMediaType)mediaType completionHandler:(void (^)(BOOL granted))handler;
@end

#endif

static NSString* const WineWillShowPermissionDialogNotification = @"WineWillShowPermissionDialogNotification";
static NSString* const WineDidShowPermissionDialogNotification = @"WineDidShowPermissionDialogNotification";

/***********************************************************************
 *              CoreAudio_get_capture_authorization_status
 *
 * Returns the authorization status for audio capture as an
 * AVAuthorizationStatus enum value.
 * On pre-10.14 OS versions that don't support authorization, always
 * returns AUTHORIZED.
 */
int CoreAudio_get_capture_authorization_status(void)
{
    int ret = AUTHORIZED;
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    if ([AVCaptureDevice respondsToSelector:@selector(authorizationStatusForMediaType:)])
        ret = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];

    [pool release];
    return ret;
}

/***********************************************************************
 *              CoreAudio_request_capture_authorization
 *
 * Requests audio capture/microphone permissions from the user, and posts
 * in-process NSNotifications before/after the request.
 *
 * Returns: 1 if access was granted
 *          0 if access was denied
 *         -1 if requesting permission is unsupported (older macOS)
 */
int CoreAudio_request_capture_authorization(void)
{
    __block int ret = -1;
    dispatch_semaphore_t semaphore;
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    if ([AVCaptureDevice respondsToSelector:@selector(requestAccessForMediaType:completionHandler:)])
    {
        [[NSNotificationCenter defaultCenter] postNotificationName:WineWillShowPermissionDialogNotification
                                                            object:nil];
        semaphore = dispatch_semaphore_create(0);
        [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio
                                 completionHandler:^(BOOL granted){
            dispatch_semaphore_signal(semaphore);
            ret = granted ? 1 : 0;
        }];

        dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
        dispatch_release(semaphore);
        [[NSNotificationCenter defaultCenter] postNotificationName:WineDidShowPermissionDialogNotification
                                                            object:nil];
    }

    [pool release];
    return ret;
}

#else

int CoreAudio_get_capture_authorization_status(void)
{
    return AUTHORIZED;
}

int CoreAudio_request_capture_authorization(void)
{
    return -1;
}

#endif
