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

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl.h"
#include "gegl-operation-area-filter.h"
#include "gegl-operation-context.h"
#include "gegl-types-internal.h"


static void          prepare                  (GeglOperation       *operation);
static GeglRectangle get_bounding_box          (GeglOperation       *operation);
static GeglRectangle get_required_for_output   (GeglOperation       *operation,
                                                 const gchar         *input_pad,
                                                 const GeglRectangle *region);
static GeglRectangle get_invalidated_by_change (GeglOperation       *operation,
                                                 const gchar         *input_pad,
                                                 const GeglRectangle *input_region);

G_DEFINE_TYPE (GeglOperationAreaFilter, gegl_operation_area_filter,
               GEGL_TYPE_OPERATION_FILTER)

static void
gegl_operation_area_filter_class_init (GeglOperationAreaFilterClass *klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->prepare = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_invalidated_by_change = get_invalidated_by_change;
  operation_class->get_required_for_output = get_required_for_output;
}

static void
gegl_operation_area_filter_init (GeglOperationAreaFilter *self)
{
  self->left=0;
  self->right=0;
  self->bottom=0;
  self->top=0;
}

static void prepare (GeglOperation *operation)
{
  const Babl *format = gegl_babl_rgba_linear_float ();
  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglRectangle            result = { 0, };
  GeglRectangle           *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (operation,"input");

  if (!in_rect)
    return result;

  if (gegl_rectangle_is_infinite_plane (in_rect))
    return *in_rect;

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

static GeglRectangle
get_required_for_output (GeglOperation        *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *region)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglRectangle            rect;
  GeglRectangle            defined;

  defined = get_bounding_box (operation);
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
get_invalidated_by_change (GeglOperation        *operation,
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
