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


enum {
    WineApplicationEventWakeQuery,
};


@class WineEventQueue;
@class WineWindow;
@protocol WineClipCursorHandler;


@interface WineApplicationController : NSObject <NSApplicationDelegate>
{
    CFRunLoopSourceRef requestSource;
    NSMutableArray* requests;
    dispatch_queue_t requestsManipQueue;

    NSMutableArray* eventQueues;
    NSLock*         eventQueuesLock;

    NSTimeInterval eventTimeAdjustment;

    NSMutableArray* keyWindows;
    NSMutableSet* triedWindows;
    unsigned long windowFocusSerial;

    TISInputSourceRef lastKeyboardInputSource;
    TISInputSourceRef lastKeyboardLayoutInputSource;
    CGEventSourceKeyboardType keyboardType;
    NSEvent* lastFlagsChanged;
    BOOL inputSourceIsInputMethod;
    uint32_t pressedKeyCodes[128 / 32];

    CGFloat primaryScreenHeight;
    BOOL primaryScreenHeightValid;
    NSMutableData* screenFrameCGRects;

    WineWindow* lastTargetWindow;
    WineWindow* mouseCaptureWindow;
    BOOL forceNextMouseMoveAbsolute;
    double mouseMoveDeltaX, mouseMoveDeltaY;
    NSUInteger unmatchedMouseDowns;

    NSTimeInterval lastScrollTime;
    double accumScrollX, accumScrollY;

    NSMutableDictionary* originalDisplayModes;
    NSMutableDictionary* latentDisplayModes;
    BOOL displaysCapturedForFullscreen;

    NSArray*    cursorFrames;
    int         cursorFrame;
    NSTimer*    cursorTimer;
    NSCursor*   cursor;
    BOOL        cursorIsCurrent;
    BOOL        cursorHidden;
    BOOL        clientWantsCursorHidden;

    NSTimeInterval lastSetCursorPositionTime;

    id<WineClipCursorHandler> clipCursorHandler;

    NSImage* applicationIcon;

    BOOL beenActive;

    NSMutableSet* windowsBeingDragged;
}

@property (nonatomic) CGEventSourceKeyboardType keyboardType;
@property (readonly, copy, nonatomic) NSEvent* lastFlagsChanged;
@property (readonly, nonatomic) BOOL areDisplaysCaptured;

@property (readonly) BOOL clippingCursor;
@property (nonatomic) NSTimeInterval lastSetCursorPositionTime;

    + (WineApplicationController*) sharedController;

    - (void) transformProcessToForeground:(BOOL)activateIfTransformed;
    - (void) tryToActivateIgnoringOtherApps:(BOOL)ignore;

    - (BOOL) registerEventQueue:(WineEventQueue*)queue;
    - (void) unregisterEventQueue:(WineEventQueue*)queue;

    - (void) computeEventTimeAdjustmentFromTicks:(unsigned long long)tickcount uptime:(uint64_t)uptime_ns;
    - (double) ticksForEventTime:(NSTimeInterval)eventTime;

    - (void) windowGotFocus:(WineWindow*)window;

    - (BOOL) waitUntilQueryDone:(int*)done timeout:(NSDate*)timeout processEvents:(BOOL)processEvents;

    - (void) noteKey:(uint16_t)keyCode pressed:(BOOL)pressed;

    - (void) window:(WineWindow*)window isBeingDragged:(BOOL)dragged;
    - (void) windowWillOrderOut:(WineWindow*)window;

    - (void) flipRect:(NSRect*)rect;
    - (NSPoint) flippedMouseLocation:(NSPoint)point;

    - (WineWindow*) frontWineWindow;
    - (void) adjustWindowLevels;
    - (void) updateFullscreenWindows;

    - (BOOL) handleEvent:(NSEvent*)anEvent;
    - (void) didSendEvent:(NSEvent*)anEvent;

@end


@interface WineApplication : NSApplication
{
    WineApplicationController* wineController;
}

@property (readwrite, assign, nonatomic) WineApplicationController* wineController;

@end


void OnMainThreadAsync(dispatch_block_t block);

void LogError(const char* func, NSString* format, ...);
void LogErrorv(const char* func, NSString* format, va_list args);
