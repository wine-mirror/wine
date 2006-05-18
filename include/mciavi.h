/*
 * Copyright (C) 2001 Eric Pouech
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

#define MCI_MCIAVI_PLAY_WINDOW		0x01000000L
#define MCI_MCIAVI_PLAY_FULLSCREEN	0x02000000L
#define MCI_MCIAVI_PLAY_FULLBY2		0x04000000L

#define MCI_AVI_STATUS_FRAMES_SKIPPED	0x8001L

#define MCI_AVI_STATUS_LAST_PLAY_SPEED		0x8002L

#define MCI_AVI_STATUS_AUDIO_BREAKS		0x8003L

#define MCI_AVI_SETVIDEO_DRAW_PROCEDURE		0x8000L

#define MCI_AVI_SETVIDEO_PALETTE_COLOR		0x8100L

#define MCI_AVI_SETVIDEO_PALETTE_HALFTONE       0x0000FFFFL

#define MCIERR_AVI_OLDAVIFORMAT		(MCIERR_CUSTOM_DRIVER_BASE + 100)
#define MCIERR_AVI_NOTINTERLEAVED	(MCIERR_CUSTOM_DRIVER_BASE + 101)
#define MCIERR_AVI_NODISPDIB		(MCIERR_CUSTOM_DRIVER_BASE + 102)
#define MCIERR_AVI_CANTPLAYFULLSCREEN	(MCIERR_CUSTOM_DRIVER_BASE + 103)
#define MCIERR_AVI_TOOBIGFORVGA		(MCIERR_CUSTOM_DRIVER_BASE + 104)
#define MCIERR_AVI_NOCOMPRESSOR         (MCIERR_CUSTOM_DRIVER_BASE + 105)
#define MCIERR_AVI_DISPLAYERROR         (MCIERR_CUSTOM_DRIVER_BASE + 106)
#define MCIERR_AVI_AUDIOERROR		(MCIERR_CUSTOM_DRIVER_BASE + 107)
#define MCIERR_AVI_BADPALETTE		(MCIERR_CUSTOM_DRIVER_BASE + 108)
