/*
 * MACDRV Cocoa window declarations
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


@class WineEventQueue;


@interface WineWindow : NSPanel <NSWindowDelegate>
{
    BOOL disabled;
    BOOL noForeground;
    BOOL preventsAppActivation;
    BOOL floating;
    BOOL resizable;
    BOOL maximized;
    BOOL fullscreen;
    BOOL pendingMinimize;
    BOOL pendingOrderOut;
    BOOL savedVisibleState;
    BOOL drawnSinceShown;
    BOOL closing;
    WineWindow* latentParentWindow;
    NSMutableArray* latentChildWindows;

    void* hwnd;
    WineEventQueue* queue;

    void* surface;
    pthread_mutex_t* surface_mutex;

    CGDirectDisplayID _lastDisplayID;
    NSTimeInterval _lastDisplayTime;

    NSRect wineFrame;
    NSRect roundedWineFrame;

    BOOL shapeChangedSinceLastDraw;

    BOOL colorKeyed;
    CGFloat colorKeyRed, colorKeyGreen, colorKeyBlue;

    BOOL usePerPixelAlpha;

    NSUInteger lastModifierFlags;

    NSRect frameAtResizeStart;
    BOOL resizingFromLeft, resizingFromTop;

    void* himc;
    BOOL commandDone;

    NSSize savedContentMinSize;
    NSSize savedContentMaxSize;

    BOOL enteringFullScreen;
    BOOL exitingFullScreen;
    NSRect nonFullscreenFrame;
    NSTimeInterval enteredFullScreenTime;

    int draggingPhase;
    NSPoint dragStartPosition;
    NSPoint dragWindowStartPosition;

    NSTimeInterval lastDockIconSnapshot;

    BOOL allowKeyRepeats;

    BOOL ignore_windowDeminiaturize;
    BOOL ignore_windowResize;
    BOOL fakingClose;
}

@property (retain, readonly, nonatomic) WineEventQueue* queue;
@property (readonly, nonatomic) BOOL disabled;
@property (readonly, nonatomic) BOOL noForeground;
@property (readonly, nonatomic) BOOL preventsAppActivation;
@property (readonly, nonatomic) BOOL floating;
@property (readonly, getter=isFullscreen, nonatomic) BOOL fullscreen;
@property (readonly, getter=isFakingClose, nonatomic) BOOL fakingClose;
@property (readonly, nonatomic) NSRect wine_fractionalFrame;

    - (NSInteger) minimumLevelForActive:(BOOL)active;
    - (void) updateFullscreen;

    - (void) postKeyEvent:(NSEvent *)theEvent;
    - (void) postBroughtForwardEvent;

    - (WineWindow*) ancestorWineWindow;

    - (void) updateForCursorClipping;

    - (void) setRetinaMode:(int)mode;

@end
