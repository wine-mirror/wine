/*
 * MSCMS - Color Management System for Wine
 *
 * Copyright 2004 Hans Leidekker
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "mscms_priv.h"

#ifndef LCMS_API_FUNCTION
#error "This file should be included with LCMS_API_FUNCTION defined!"
#endif

#ifdef HAVE_LCMS_H

LCMS_API_FUNCTION(cmsCloseProfile)
LCMS_API_FUNCTION(cmsIsTag)
LCMS_API_FUNCTION(cmsOpenProfileFromFile)
LCMS_API_FUNCTION(cmsOpenProfileFromMem)

#ifndef LCMS_API_NO_REDEFINE
#define cmsCloseProfile pcmsCloseProfile
#define cmsIsTag pcmsIsTag
#define cmsOpenProfileFromFile pcmsOpenProfileFromFile
#define cmsOpenProfileFromMem pcmsOpenProfileFromMem
#endif /* LCMS_API_NO_REDEFINE */

#endif /* HAVE_LCMS_H */
