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
 * Copyright 2015 Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_boolean (clip_low, _("Clip low pixel values"), TRUE)
     description (_("Clip low pixel values"))

property_double (low_limit, _("Low limit"), 0.0)
    value_range (-G_MAXDOUBLE, 0.0)
    ui_range    (-2.0, 0.0)
    description (_("Pixels values lower than this limit will be set to it"))
    ui_meta     ("sensitive", "clip-low")

property_boolean (clip_high, _("Clip high pixel values"), TRUE)
     description (_("Clip high pixel values"))

property_double (high_limit, _("High limit"), 1.0)
    value_range (1.0, G_MAXDOUBLE)
    ui_range    (1.0, 3.0)
    description (_("Pixels values higher than this limit will be set to it"))
    ui_meta     ("sensitive", "clip-high")

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     rgb_clip
#define GEGL_OP_C_SOURCE rgb-clip.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  const Babl *src_format = gegl_operation_get_source_format (operation, "input");
  const char *format     = "RGB float";

  if (src_format)
    {
      const Babl *model = babl_format_get_model (src_format);

      if (model == babl_model ("RGB"))
        format = "RGB float";
      else if (model == babl_model ("RGBA"))
        format = "RGBA float";
      else if (model == babl_model ("R'G'B'"))
        format = "R'G'B' float";
      else if (model == babl_model ("R'G'B'A"))
        format = "R'G'B'A float";
      else if (babl_format_has_alpha (src_format))
        format = "RGBA float";
    }

  gegl_operation_set_format (operation, "input",  babl_format (format));
  gegl_operation_set_format (operation, "output", babl_format (format));
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o         = GEGL_PROPERTIES (operation);
  const Babl     *format    = gegl_operation_get_format (operation, "input");
  gboolean        has_alpha = babl_format_has_alpha (format);
  gint            n_components = has_alpha ? 4 : 3;
  gfloat *input  = in_buf;
  gfloat *output = out_buf;

  if (o->clip_low && o->clip_high)
    {
      while (n_pixels--)
        {
          output[0] = CLAMP (input[0], o->low_limit, o->high_limit);
          output[1] = CLAMP (input[1], o->low_limit, o->high_limit);
          output[2] = CLAMP (input[2], o->low_limit, o->high_limit);

          if (has_alpha)
            output[3] = input[3];

          input  += n_components;
          output += n_components;
        }
    }

  else if (o->clip_high)
    {
      while (n_pixels--)
          {
            output[0] = input[0] > o->high_limit ? o->high_limit : input[0];
            output[1] = input[1] > o->high_limit ? o->high_limit : input[1];
            output[2] = input[2] > o->high_limit ? o->high_limit : input[2];

            if (has_alpha)
              output[3] = input[3];

            input  += n_components;
            output += n_components;
          }
    }

  else if (o->clip_low)
    {
      while (n_pixels--)
        {
          output[0] = input[0] < o->low_limit ? o->low_limit : input[0];
          output[1] = input[1] < o->low_limit ? o->low_limit : input[1];
          output[2] = input[2] < o->low_limit ? o->low_limit : input[2];

          if (has_alpha)
            output[3] = input[3];

          input  += n_components;
          output += n_components;
        }
    }

  return TRUE;
}

static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglOperationClass  *operation_class;
  GeglProperties      *o = GEGL_PROPERTIES (operation);

  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

  if (!o->clip_high && !o->clip_low)
    {
      gpointer in = gegl_operation_context_get_object (context, "input");
      gegl_operation_context_take_object (context, "output",
                                              g_object_ref (G_OBJECT (in)));
      return TRUE;
    }

  /* chain up, which will create the needed buffers for our actual
   * process function
   */
  return operation_class->process (operation, context, output_prop, result,
                                   gegl_operation_context_get_level (context));
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  operation_class->process = operation_process;
  operation_class->opencl_support = FALSE;

  filter_class->process    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:rgb-clip",
    "title",       _("Clip RGB"),
    "categories",  "color",
    "description", _("Keep RGB pixels values inside a specific range"),
    NULL);
}

#endif
