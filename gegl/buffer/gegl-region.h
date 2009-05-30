/* GEGL - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __GEGL_REGION_H__
#define __GEGL_REGION_H__

#include <gegl/gegl-types-internal.h>
#include "gegl-buffer-types.h"

G_BEGIN_DECLS

struct _GeglSegment
{
  gint x1;
  gint y1;
  gint x2;
  gint y2;
};

struct _GeglSpan
{
  gint x;
  gint y;
  gint width;
};

/* Type definitions for the basic structures.
 */
typedef struct _GeglSegment   GeglSegment;
typedef struct _GeglSpan      GeglSpan;

/* GC fill rule for polygons
 *  EvenOddRule
 *  WindingRule
 */
typedef enum
{
  GEGL_EVEN_ODD_RULE,
  GEGL_WINDING_RULE
} GeglFillRule;

/* Types of overlapping between a rectangle and a region
 * GEGL_OVERLAP_RECTANGLE_IN: rectangle is in region
 * GEGL_OVERLAP_RECTANGLE_OUT: rectangle in not in region
 * GEGL_OVERLAP_RECTANGLE_PART: rectangle in partially in region
 */
typedef enum
{
  GEGL_OVERLAP_RECTANGLE_IN,
  GEGL_OVERLAP_RECTANGLE_OUT,
  GEGL_OVERLAP_RECTANGLE_PART
} GeglOverlapType;

typedef void (* GeglSpanFunc) (GeglSpan *span,
                               gpointer data);

GeglRegion    * gegl_region_new             (void);
GeglRegion    * gegl_region_polygon         (GeglPoint           *points,
                                             gint                 n_points,
                                             GeglFillRule         fill_rule);
GeglRegion    * gegl_region_copy            (const GeglRegion    *region);
GeglRegion    * gegl_region_rectangle       (const GeglRectangle *rectangle);
void            gegl_region_destroy         (GeglRegion          *region);

void	        gegl_region_get_clipbox     (GeglRegion          *region,
                                             GeglRectangle       *rectangle);
void            gegl_region_get_rectangles  (GeglRegion          *region,
                                             GeglRectangle      **rectangles,
                                             gint                *n_rectangles);

gboolean        gegl_region_empty           (const GeglRegion    *region);
gboolean        gegl_region_equal           (const GeglRegion    *region1,
                                             const GeglRegion    *region2);
gboolean        gegl_region_point_in        (const GeglRegion    *region,
                                             gint                 x,
                                             gint                 y);
GeglOverlapType gegl_region_rect_in         (const GeglRegion    *region,
                                             const GeglRectangle *rectangle);

void            gegl_region_offset          (GeglRegion          *region,
                                             gint                 dx,
                                             gint                 dy);
void            gegl_region_shrink          (GeglRegion          *region,
                                             gint                 dx,
                                             gint                 dy);
void            gegl_region_union_with_rect (GeglRegion          *region,
                                             const GeglRectangle *rect);
void            gegl_region_intersect       (GeglRegion          *source1,
                                             const GeglRegion    *source2);
void            gegl_region_union           (GeglRegion          *source1,
                                             const GeglRegion    *source2);
void            gegl_region_subtract        (GeglRegion          *source1,
                                             const GeglRegion    *source2);
void            gegl_region_xor             (GeglRegion          *source1,
                                             const GeglRegion    *source2);
void            gegl_region_dump            (GeglRegion          *region);

void    gegl_region_spans_intersect_foreach (GeglRegion          *region,
                                             GeglSpan            *spans,
                                             int                  n_spans,
                                             gboolean             sorted,
                                             GeglSpanFunc         function,
                                             gpointer             data);

G_END_DECLS

#endif /* __GEGL_REGION_H__ */

