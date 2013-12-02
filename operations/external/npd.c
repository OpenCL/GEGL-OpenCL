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
 * Copyright (C) 2013 Marek Dvoroznak <dvoromar@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES
gegl_chant_pointer (model,       _("model"),
                    _("Model - basic element we operate on"))

gegl_chant_int     (square_size, _("square size"),
                    5,  1000,  20,
                    _("Size of an edge of square the mesh consists of"))

gegl_chant_int     (rigidity,    _("rigidity"),
                    0, 10000, 100,
                    _("The number of deformation iterations"))

gegl_chant_boolean (ASAP_deformation, _("ASAP deformation"),
                    FALSE,
                    _("ASAP deformation is performed when TRUE, ARAP deformation otherwise"))

gegl_chant_boolean (MLS_weights, _("MLS weights"),
                    FALSE,
                    _("Use MLS weights"))

gegl_chant_double  (MLS_weights_alpha, _("MLS weights alpha"),
                    0.1, 2.0, 1.0,
                    _("Alpha parameter of MLS weights"))

gegl_chant_boolean (mesh_visible, _("mesh visible"),
                    TRUE,
                    _("Should the mesh be visible?"))
#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "npd.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <math.h>
#include <npd/npd.h>
#include <npd/npd_gegl.h>
#include <cairo.h>

struct _NPDDisplay
{
  NPDImage  image;
  cairo_t  *cr;
};

typedef struct
{
  gboolean  first_run;
  NPDModel  model;
} NPDProperties;

static void npd_draw_line_impl (NPDDisplay *display,
                                gfloat      x0,
                                gfloat      y0,
                                gfloat      x1,
                                gfloat      y1)
{
  cairo_move_to (display->cr, x0, y0);
  cairo_line_to (display->cr, x1, y1);
}

static void
npd_draw_model (NPDModel   *model,
                NPDDisplay *display)
{
  npd_draw_model_into_image (model, &display->image);

  /* draw mesh */
  if (model->mesh_visible)
    {
      cairo_surface_t *surface;

      surface = cairo_image_surface_create_for_data (display->image.buffer,
                                                     CAIRO_FORMAT_ARGB32,
                                                     display->image.width,
                                                     display->image.height,
                                                     display->image.width);
      display->cr = cairo_create (surface);
      cairo_set_line_width (display->cr, 1);
      cairo_set_source_rgba (display->cr, 0, 0, 0, 1);
      npd_draw_mesh (model, display);
      cairo_stroke (display->cr);
    }
}

static void
prepare (GeglOperation *operation)
{
  GeglChantO    *o      = GEGL_CHANT_PROPERTIES (operation);
  NPDProperties *props;

  if (o->chant_data == NULL)
    {
      props = g_new (NPDProperties, 1);
      props->first_run = TRUE;
      o->chant_data    = props;
    }

  gegl_operation_set_format (operation, "input",
                             babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o      = GEGL_CHANT_PROPERTIES (operation);
  const Babl *format = babl_format ("RGBA u8");
  NPDProperties *props = o->chant_data;
  NPDModel *model = &props->model;
  NPDHiddenModel *hm;
  guchar *output_buffer;
  gint length = gegl_buffer_get_pixel_count (input) * 4;

  if (props->first_run)
    {
      gint width, height;
      NPDImage *input_image = g_new (NPDImage, 1);
      NPDDisplay *display = g_new (NPDDisplay, 1);

      npd_init (npd_gegl_set_pixel_color,
                npd_gegl_get_pixel_color,
                npd_draw_line_impl);

      npd_gegl_create_image (input_image, input, format);
      width = input_image->width;
      height = input_image->height;

      output_buffer = g_new0 (guchar, length);
      display->image.width = width;
      display->image.height = height;
      display->image.buffer = output_buffer;
      model->display = display;

      npd_create_model_from_image (model, input_image, width, height, 0, 0, o->square_size);
      hm = model->hidden_model;

      o->model = model;

      memcpy (output_buffer, input_image->buffer, length);

      props->first_run = FALSE;
    }
  else
    {
      npd_set_deformation_type (model, o->ASAP_deformation, o->MLS_weights);

      if (o->MLS_weights &&
          model->hidden_model->MLS_weights_alpha != o->MLS_weights_alpha)
        {
          model->hidden_model->MLS_weights_alpha = o->MLS_weights_alpha;
          npd_compute_MLS_weights (model);
        }

      model->mesh_visible = o->mesh_visible;

      output_buffer = model->display->image.buffer;
      memset (output_buffer, 0, length);
      npd_deform_model (model, o->rigidity);
      npd_draw_model (model, model->display);
    }

  gegl_buffer_set (output, NULL, 0, format, output_buffer, GEGL_AUTO_ROWSTRIDE);

  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process    = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "categories"  , "transform",
    "name"        , "gegl:npd",
    "description" , _("Performs n-point image deformation"),
    NULL);
}

#endif
