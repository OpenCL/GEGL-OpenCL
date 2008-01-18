/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2007 Øyvind Kolås
 */
#define GEGL_INTERNAL

#include "config.h"

#include <glib-object.h>
#include <string.h>
#include "gegl-types.h"
#include "gegl-operation-area-filter.h"
#include "gegl-utils.h"
#include "graph/gegl-node.h"
#include "graph/gegl-connection.h"
#include "graph/gegl-pad.h"
#include "buffer/gegl-region.h"
#include "buffer/gegl-buffer.h"

#include <string.h>

G_DEFINE_TYPE (GeglOperationAreaFilter, gegl_operation_area_filter, GEGL_TYPE_OPERATION_FILTER)

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static GeglRectangle get_defined_region      (GeglOperation       *operation);
static GeglRectangle compute_input_request   (GeglOperation       *operation,
                                              const gchar         *input_pad,
                                              const GeglRectangle *region);
static GeglRectangle compute_affected_region (GeglOperation       *operation,
                                              const gchar         *input_pad,
                                              const GeglRectangle *input_region);

static void
gegl_operation_area_filter_class_init (GeglOperationAreaFilterClass *klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->prepare = prepare;
  operation_class->get_defined_region  = get_defined_region;
  operation_class->compute_affected_region = compute_affected_region;
  operation_class->compute_input_request = compute_input_request;
}

static void
gegl_operation_area_filter_init (GeglOperationAreaFilter *self)
{
  self->left=0;
  self->right=0;
  self->bottom=0;
  self->top=0;
}

#include <math.h>
static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_defined_region (operation,
                                                                     "input");
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);

  if (!in_rect)
    return result;

  result = *in_rect;
  if (result.width != 0 &&
      result.height != 0)
    {
      result.x-= area->left;
      result.y-= area->top;
      result.width += area->left + area->right;
      result.height += area->top + area->bottom;
    }

  return result;
}

#include "gegl-utils.h"

static GeglRectangle
compute_input_request (GeglOperation       *operation,
                       const gchar         *input_pad,
                       const GeglRectangle *region)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglRectangle       rect;
  GeglRectangle       defined;
  defined = get_defined_region (operation);
  gegl_rectangle_intersect (&rect, region, &defined);

  if (rect.width  != 0 &&
      rect.height != 0)
    {
      rect.x -= area->left;
      rect.y -= area->top;
      rect.width  += area->left + area->right;
      rect.height  += area->top + area->bottom;
    }

  return rect;
}

static GeglRectangle
compute_affected_region (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *input_region)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglRectangle            retval;

  retval.x      = input_region->x - area->left;
  retval.y      = input_region->y - area->top;
  retval.width  = input_region->width  + area->left + area->right;
  retval.height = input_region->height + area->top  + area->bottom;

  return retval;
}
