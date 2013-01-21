/*
 * MACDRV Cocoa application class
 *
 * Copyright 2011, 2012, 2013 Ken Thomases for CodeWeavers Inc.
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

#import "cocoa_app.h"


int macdrv_err_on;


@implementation WineApplication

    - (id) init
    {
        self = [super init];
        if (self != nil)
        {
            eventQueues = [[NSMutableArray alloc] init];
            eventQueuesLock = [[NSLock alloc] init];

            if (!eventQueues || !eventQueuesLock)
            {
                [self release];
                return nil;
            }
        }
        return self;
    }

    - (void) dealloc
    {
        [eventQueues release];
        [eventQueuesLock release];
        [super dealloc];
    }

    - (void) transformProcessToForeground
    {
        if ([self activationPolicy] != NSApplicationActivationPolicyRegular)
        {
            NSMenu* mainMenu;
            NSMenu* submenu;
            NSString* bundleName;
            NSString* title;
            NSMenuItem* item;

            [self setActivationPolicy:NSApplicationActivationPolicyRegular];
            [self activateIgnoringOtherApps:YES];

            mainMenu = [[[NSMenu alloc] init] autorelease];

            submenu = [[[NSMenu alloc] initWithTitle:@"Wine"] autorelease];
            bundleName = [[NSBundle mainBundle] objectForInfoDictionaryKey:(NSString*)kCFBundleNameKey];
            if ([bundleName length])
                title = [NSString stringWithFormat:@"Quit %@", bundleName];
            else
                title = @"Quit";
            item = [submenu addItemWithTitle:title action:@selector(terminate:) keyEquivalent:@"q"];
            [item setKeyEquivalentModifierMask:NSCommandKeyMask | NSAlternateKeyMask];
            item = [[[NSMenuItem alloc] init] autorelease];
            [item setTitle:@"Wine"];
            [item setSubmenu:submenu];
            [mainMenu addItem:item];

            submenu = [[[NSMenu alloc] initWithTitle:@"Window"] autorelease];
            [submenu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@""];
            [submenu addItemWithTitle:@"Zoom" action:@selector(performZoom:) keyEquivalent:@""];
            [submenu addItem:[NSMenuItem separatorItem]];
            [submenu addItemWithTitle:@"Bring All to Front" action:@selector(arrangeInFront:) keyEquivalent:@""];
            item = [[[NSMenuItem alloc] init] autorelease];
            [item setTitle:@"Window"];
            [item setSubmenu:submenu];
            [mainMenu addItem:item];

            [self setMainMenu:mainMenu];
            [self setWindowsMenu:submenu];
        }
    }

    - (BOOL) registerEventQueue:(WineEventQueue*)queue
    {
        [eventQueuesLock lock];
        [eventQueues addObject:queue];
        [eventQueuesLock unlock];
        return TRUE;
    }

    - (void) unregisterEventQueue:(WineEventQueue*)queue
    {
        [eventQueuesLock lock];
        [eventQueues removeObjectIdenticalTo:queue];
        [eventQueuesLock unlock];
    }

@end

/***********************************************************************
 *              OnMainThread
 *
 * Run a block on the main thread synchronously.
 */
void OnMainThread(dispatch_block_t block)
{
    dispatch_sync(dispatch_get_main_queue(), block);
}

/***********************************************************************
 *              OnMainThreadAsync
 *
 * Run a block on the main thread asynchronously.
 */
void OnMainThreadAsync(dispatch_block_t block)
{
    dispatch_async(dispatch_get_main_queue(), block);
}

/***********************************************************************
 *              LogError
 */
void LogError(const char* func, NSString* format, ...)
{
    va_list args;
    va_start(args, format);
    LogErrorv(func, format, args);
    va_end(args);
}

/***********************************************************************
 *              LogErrorv
 */
void LogErrorv(const char* func, NSString* format, va_list args)
{
    NSString* message = [[NSString alloc] initWithFormat:format arguments:args];
    fprintf(stderr, "err:%s:%s", func, [message UTF8String]);
    [message release];
}
