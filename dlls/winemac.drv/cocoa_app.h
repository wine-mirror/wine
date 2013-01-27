/*
 * MACDRV Cocoa application class declaration
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

#import <AppKit/AppKit.h>

#include "macdrv_cocoa.h"


#define ERR(...) do { if (macdrv_err_on) LogError(__func__, __VA_ARGS__); } while (false)


@class WineEventQueue;
@class WineWindow;


@interface WineApplication : NSApplication <NSApplicationDelegate>
{
    NSMutableArray* eventQueues;
    NSLock*         eventQueuesLock;

    NSTimeInterval eventTimeAdjustment;

    NSMutableArray* keyWindows;
    NSMutableSet* triedWindows;
    unsigned long windowFocusSerial;
}

    - (void) transformProcessToForeground;

    - (BOOL) registerEventQueue:(WineEventQueue*)queue;
    - (void) unregisterEventQueue:(WineEventQueue*)queue;

    - (void) computeEventTimeAdjustmentFromTicks:(unsigned long long)tickcount uptime:(uint64_t)uptime_ns;
    - (double) ticksForEventTime:(NSTimeInterval)eventTime;

    - (void) windowGotFocus:(WineWindow*)window;

@end

void OnMainThread(dispatch_block_t block);
void OnMainThreadAsync(dispatch_block_t block);

void LogError(const char* func, NSString* format, ...);
void LogErrorv(const char* func, NSString* format, va_list args);
