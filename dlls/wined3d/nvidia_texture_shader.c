/*
 * Fixed function pipeline replacement using GL_NV_register_combiners
 * and GL_NV_texture_shader
 *
 * Copyright 2006 Henri Verbeet
 * Copyright 2008 Stefan Dösinger(for CodeWeavers)
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
#include "config.h"

#include <math.h>
#include <stdio.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

#define GLINFO_LOCATION (*gl_info)
static void nvrc_enable(IWineD3DDevice *iface, BOOL enable) { }

static void nvts_enable(IWineD3DDevice *iface, BOOL enable) {
    if(enable) {
        glEnable(GL_TEXTURE_SHADER_NV);
        checkGLcall("glEnable(GL_TEXTURE_SHADER_NV)");
    } else {
        glDisable(GL_TEXTURE_SHADER_NV);
        checkGLcall("glDisable(GL_TEXTURE_SHADER_NV)");
    }
}

static void nvrc_fragment_get_caps(WINED3DDEVTYPE devtype, WineD3D_GL_Info *gl_info, struct fragment_caps *pCaps) {
    pCaps->TextureOpCaps =  WINED3DTEXOPCAPS_ADD                        |
                            WINED3DTEXOPCAPS_ADDSIGNED                  |
                            WINED3DTEXOPCAPS_ADDSIGNED2X                |
                            WINED3DTEXOPCAPS_MODULATE                   |
                            WINED3DTEXOPCAPS_MODULATE2X                 |
                            WINED3DTEXOPCAPS_MODULATE4X                 |
                            WINED3DTEXOPCAPS_SELECTARG1                 |
                            WINED3DTEXOPCAPS_SELECTARG2                 |
                            WINED3DTEXOPCAPS_DISABLE                    |
                            WINED3DTEXOPCAPS_BLENDDIFFUSEALPHA          |
                            WINED3DTEXOPCAPS_BLENDTEXTUREALPHA          |
                            WINED3DTEXOPCAPS_BLENDFACTORALPHA           |
                            WINED3DTEXOPCAPS_BLENDCURRENTALPHA          |
                            WINED3DTEXOPCAPS_LERP                       |
                            WINED3DTEXOPCAPS_SUBTRACT                   |
                            WINED3DTEXOPCAPS_ADDSMOOTH                  |
                            WINED3DTEXOPCAPS_MULTIPLYADD                |
                            WINED3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR     |
                            WINED3DTEXOPCAPS_MODULATECOLOR_ADDALPHA     |
                            WINED3DTEXOPCAPS_BLENDTEXTUREALPHAPM        |
                            WINED3DTEXOPCAPS_DOTPRODUCT3                |
                            WINED3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR  |
                            WINED3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA;

    if(GL_SUPPORT(NV_TEXTURE_SHADER2)) {
        /* Bump mapping is supported already in NV_TEXTURE_SHADER, but that extension does
         * not support 3D textures. This asks for trouble if an app uses both bump mapping
         * and 3D textures. It also allows us to keep the code simpler by having texture
         * shaders constantly enabled.
         */
        pCaps->TextureOpCaps |= WINED3DTEXOPCAPS_BUMPENVMAP;
        /* TODO: Luminance bump map? */
    }

#if 0
    /* FIXME: Add
            pCaps->TextureOpCaps |= WINED3DTEXOPCAPS_BUMPENVMAPLUMINANCE
            WINED3DTEXOPCAPS_PREMODULATE */
#endif

    pCaps->MaxTextureBlendStages   = GL_LIMITS(texture_stages);
    pCaps->MaxSimultaneousTextures = GL_LIMITS(textures);

    pCaps->PrimitiveMiscCaps |=  WINED3DPMISCCAPS_TSSARGTEMP;

    /* The caps below can be supported but aren't handled yet in utils.c 'd3dta_to_combiner_input', disable them until support is fixed */
#if 0
    if (GL_SUPPORT(NV_REGISTER_COMBINERS2))
    pCaps->PrimitiveMiscCaps |=  WINED3DPMISCCAPS_PERSTAGECONSTANT;
#endif
}

static HRESULT nvrc_fragment_alloc(IWineD3DDevice *iface) { return WINED3D_OK; }
static void nvrc_fragment_free(IWineD3DDevice *iface) {}

/* Two fixed function pipeline implementations using GL_NV_register_combiners and
 * GL_NV_texture_shader. The nvts_fragment_pipeline assumes that both extensions
 * are available(geforce 3 and newer), while nvrc_fragment_pipeline uses only the
 * register combiners extension(Pre-GF3).
 */
const struct fragment_pipeline nvts_fragment_pipeline = {
    nvts_enable,
    nvrc_fragment_get_caps,
    nvrc_fragment_alloc,
    nvrc_fragment_free,
    ffp_fragmentstate_template
};

const struct fragment_pipeline nvrc_fragment_pipeline = {
    nvrc_enable,
    nvrc_fragment_get_caps,
    nvrc_fragment_alloc,
    nvrc_fragment_free,
    ffp_fragmentstate_template
};
