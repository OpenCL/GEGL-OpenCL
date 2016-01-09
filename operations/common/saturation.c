/* This file is an image processing operation for GEGL
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
 * Copyright 2016 Red Hat, Inc.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (scale, _("Scale"), 1.0)
    description(_("Scale, strength of effect"))
    value_range (0.0, 10.0)
    ui_range (0.0, 2.0)

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_C_SOURCE saturation.c

#include "gegl-op.h"

typedef void (*ProcessFunc) (GeglOperation       *operation,
                             void                *in_buf,
                             void                *out_buf,
                             glong                n_pixels,
                             const GeglRectangle *roi,
                             gint                 level);

static void
process_lab (GeglOperation       *operation,
             void                *in_buf,
             void                *out_buf,
             glong                n_pixels,
             const GeglRectangle *roi,
             gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  glong i;

  for (i = 0; i < n_pixels; i++)
    {
      out[0] = in[0];
      out[1] = in[1] * o->scale;
      out[2] = in[2] * o->scale;

      in += 3;
      out += 3;
    }
}

static void
process_lab_alpha (GeglOperation       *operation,
                   void                *in_buf,
                   void                *out_buf,
                   glong                n_pixels,
                   const GeglRectangle *roi,
                   gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  glong i;

  for (i = 0; i < n_pixels; i++)
    {
      out[0] = in[0];
      out[1] = in[1] * o->scale;
      out[2] = in[2] * o->scale;
      out[3] = in[3];

      in += 4;
      out += 4;
    }
}

static void
process_lch (GeglOperation       *operation,
             void                *in_buf,
             void                *out_buf,
             glong                n_pixels,
             const GeglRectangle *roi,
             gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  glong i;

  for (i = 0; i < n_pixels; i++)
    {
      out[0] = in[0];
      out[1] = in[1] * o->scale;
      out[2] = in[2];

      in += 3;
      out += 3;
    }
}

static void
process_lch_alpha (GeglOperation       *operation,
                   void                *in_buf,
                   void                *out_buf,
                   glong                n_pixels,
                   const GeglRectangle *roi,
                   gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  glong i;

  for (i = 0; i < n_pixels; i++)
    {
      out[0] = in[0];
      out[1] = in[1] * o->scale;
      out[2] = in[2];
      out[3] = in[3];

      in += 4;
      out += 4;
    }
}

static void prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *format;
  const Babl *input_format;
  const Babl *input_model;
  const Babl *lch_model;

  input_format = gegl_operation_get_source_format (operation, "input");
  if (input_format == NULL)
    return;

  input_model = babl_format_get_model (input_format);

  if (babl_format_has_alpha (input_format))
    {
      lch_model = babl_model ("CIE LCH(ab) alpha");
      if (input_model == lch_model)
        {
          format = babl_format ("CIE LCH(ab) alpha float");
          o->user_data = process_lch_alpha;
        }
      else
        {
          format = babl_format ("CIE Lab alpha float");
          o->user_data = process_lab_alpha;
        }
    }
  else
    {
      lch_model = babl_model ("CIE LCH(ab)");
      if (input_model == lch_model)
        {
          format = babl_format ("CIE LCH(ab) float");
          o->user_data = process_lch;
        }
      else
        {
          format = babl_format ("CIE Lab float");
          o->user_data = process_lab;
        }
    }

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  ProcessFunc real_process = (ProcessFunc) o->user_data;

  real_process (operation, in_buf, out_buf, n_pixels, roi, level);
  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_filter_class =
    GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  operation_class->opencl_support = FALSE;

  point_filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:saturation",
    "title",       _("Saturation"),
    "categories" , "color",
    "description", _("Changes the saturation"),
    NULL);
}

#endif
