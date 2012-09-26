/*
 * This file is an image processing operation for GEGL
 * sc-context.h
 * Copyright (C) 2012 Barak Itkin <lightningismyname@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GEGL_SC_CONTEXT_H__
#define __GEGL_SC_CONTEXT_H__

#include <poly2tri-c/p2t/poly2tri.h>
#include <poly2tri-c/refine/refine.h>
#include <poly2tri-c/render/mesh-render.h>

#include "sc-common.h"

/**
 * An opaque type representing the context for a seamless cloning
 * operation
 */
typedef struct _ScContext ScContext;

/**
 * Errors generated during the creation of a seamless cloning context
 */
typedef enum {
  /**
   * No error
   */
  SC_CREATION_ERROR_NONE = 0,
  /**
   * The input doesn't contain an opaque area
   */
  SC_CREATION_ERROR_EMPTY,
  /**
   *The opaque area of the input is too small
   */
  SC_CREATION_ERROR_TOO_SMALL,
  /**
   * The opaque area of the input either contains holes or is split
   * to several disconnected areas
   */
  SC_CREATION_ERROR_HOLED_OR_SPLIT
} ScCreationError;

/**
 * Create a new seamless cloning context where the alpha of the
 * given input buffer will be used to determine its outline.
 */
ScContext* sc_context_new            (GeglBuffer          *input,
                                      const GeglRectangle *roi,
                                      gdouble              threshold,
                                      ScCreationError     *error);

gboolean   sc_context_update         (ScContext           *self,
                                      GeglBuffer          *input,
                                      const GeglRectangle *roi,
                                      gdouble              threshold,
                                      ScCreationError     *error);

/**
 * Do the necessary caching so that rendering can happen. This function
 * is not thread-safe, and must be called before each call to the
 * render function (unless none of the ScRenderInfo parts was changed).
 * If this function returns FALSE, it means that a rendering can not be
 * created in the current setup. CURRENTLY THE ONLY REASON FOR SUCH A
 * BEHAVIOUR IS THAT THE FOREGROUND DOES NOT OVERLAP ENOUGH THE
 * BACKGROUND!
 */
gboolean   sc_context_prepare_render (ScContext           *context,
                                      ScRenderInfo        *info);

/**
 * Specifies whether the triangle containing each pixel, along with the
 * UV coordinates of the pixel, should be cached. This requires a memory
 * buffer which is linear in the area of the outline, but it allows to
 * render the result faster.
 * This function takes effect from the next call to prepare_render.
 */
void       sc_context_set_uvt_cache  (ScContext           *context,
                                      gboolean             enabled);

/**
 * Call this function to render the specified area of the seamless
 * cloning composition. This call must be preceeded by a call to
 * the prepare function.
 */
gboolean   sc_context_render         (ScContext           *context,
                                      ScRenderInfo        *info,
                                      const GeglRectangle *part_rect,
                                      GeglBuffer          *part);

void       sc_context_free           (ScContext           *context);

#endif
