/*
 * This file is an image processing operation for GEGL
 * sc-common.h
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

#ifndef __GEGL_SC_COMMON_H__
#define __GEGL_SC_COMMON_H__

#include <poly2tri-c/refine/refine.h>
#include <gegl.h>

/**
 * The name of the Babl type used for working on colors.
 * WARNING: THE CODE ASSUMES THAT NO MATTER WHAT THE FORMAT IS, THE
 * ALPHA CHANNEL IS ALWAYS LAST. DO NOT CHANGE THIS WITHOUT UPDATING
 * THE REST OF THE CODE!
 */
#define SC_COLOR_BABL_NAME     "R'G'B'A float"

/**
 * The type used for individual color channels. Note that this should
 * never be used directly - you must get a pointer to this type using
 * the allocation macros below!
 */
typedef gfloat ScColor;

/**
 * The amount of channels per color
 */
#define SC_COLORA_CHANNEL_COUNT 4
#define SC_COLOR_CHANNEL_COUNT  ((SC_COLORA_CHANNEL_COUNT) - 1)

#define sc_color_new()     (g_new (ScColor, SC_COLORA_CHANNEL_COUNT))
#define sc_color_free(mem) (g_free (mem))
/**
 * The index of the alpha channel in the color
 */
#define SC_COLOR_ALPHA_INDEX   SC_COLOR_CHANNEL_COUNT

/**
 * Apply a macro once for each non-alpha color channel, with the
 * channel index as an input
 */
#define sc_color_process()  \
G_STMT_START                \
  {                         \
    sc_color_expr(0);       \
    sc_color_expr(1);       \
    sc_color_expr(2);       \
  }                         \
G_STMT_END

typedef struct {
  GeglBuffer    *bg;
  GeglRectangle  bg_rect;

  GeglBuffer    *fg;
  GeglRectangle  fg_rect;

  gint           xoff;
  gint           yoff;

  gboolean       render_bg;
} ScRenderInfo;

#endif
