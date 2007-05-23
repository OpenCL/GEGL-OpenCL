/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2007 Øyvind Kolås
 */
#include "gegl-operation-area-filter.h"
#include <string.h>

G_DEFINE_TYPE (GeglOperationAreaFilter, gegl_operation_area_filter, GEGL_TYPE_OPERATION_FILTER)

static void prepare (GeglOperation *operation,
                     gpointer       context_id)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static GeglRectangle get_defined_region  (GeglOperation *operation);
static gboolean      calc_source_regions (GeglOperation *operation,
                                          gpointer       context_id);
static GeglRectangle get_affected_region (GeglOperation *operation,
                                          const gchar   *input_pad,
                                          GeglRectangle  region);
static void
gegl_operation_area_filter_class_init (GeglOperationAreaFilterClass *klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->prepare = prepare;
  operation_class->get_defined_region  = get_defined_region;
  operation_class->get_affected_region = get_affected_region;
  operation_class->calc_source_regions = calc_source_regions;
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


static gboolean
calc_source_regions (GeglOperation *operation,
                     gpointer       context_id)
{
  GeglRectangle            rect;
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
 
  rect  = *gegl_operation_get_requested_region (operation, context_id);
  if (rect.width  != 0 &&
      rect.height  != 0)
    {
      rect.x-= area->left;
      rect.y-= area->top;
      rect.width += area->left + area->right;
      rect.height += area->top + area->bottom;
    }

  gegl_operation_set_source_region (operation, context_id, "input", &rect);

  return TRUE;
}

static GeglRectangle
get_affected_region (GeglOperation *operation,
                     const gchar   *input_pad,
                     GeglRectangle  region)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);

  region.x -= area->left;
  region.y -= area->top;
  region.width  += area->left + area->right;
  region.height  += area->top + area->bottom;
  return region;
}
