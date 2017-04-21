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

#ifdef GEGL_PROPERTIES
property_pointer (model, _("Model"), NULL)
description (_("Model - basic element we operate on"))

property_int  (square_size, _("Square Size"), 20)
value_range (5, 1000)
description(_("Size of an edge of square the mesh consists of"))

property_int (rigidity,    _("Rigidity"), 100)
value_range (0, 10000)
description(_("The number of deformation iterations"))

property_boolean (asap_deformation, _("ASAP Deformation"), FALSE)
description(_("ASAP deformation is performed when TRUE, ARAP deformation otherwise"))

property_boolean (mls_weights, _("MLS Weights"), FALSE)
description(_("Use MLS weights"))

property_double  (mls_weights_alpha, _("MLS Weights Alpha"), 1.0)
value_range (0.1, 2.0)
description(_("Alpha parameter of MLS weights"))

property_boolean (preserve_model, _("Preserve Model"), FALSE)
description(_("When TRUE the model will not be freed"))

property_enum (sampler_type, _("Sampler"),
                    GeglSamplerType, gegl_sampler_type,
                    GEGL_SAMPLER_CUBIC)
description(_("Sampler used internally"))
#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     npd
#define GEGL_OP_C_SOURCE npd.c

#include "gegl-op.h"
#include <stdio.h>
#include <math.h>
#include <npd/npd.h>
#include <npd/npd_gegl.h>

struct _NPDDisplay
{
  NPDImage  image;
};

typedef struct
{
  gboolean  first_run;
  NPDModel *model;
} NPDProperties;

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  NPDProperties *props;
  if (o->user_data == NULL)
    {
      props = g_new (NPDProperties, 1);
      props->first_run      = TRUE;
      props->model          = o->model;
      o->user_data         = props;
    }

  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

#define NPD_BLEND_BAND(src,dst,src_alpha,dst_alpha,out_alpha_recip) \
(src * src_alpha + dst * dst_alpha * (1 - src_alpha)) * out_alpha_recip;

static void
npd_gegl_process_pixel (NPDImage *input_image,
                        gfloat    ix,
                        gfloat    iy,
                        NPDImage *output_image,
                        gfloat    ox,
                        gfloat    oy,
                        NPDSettings settings)
{
  if (ox > -1 && ox < output_image->width &&
      oy > -1 && oy < output_image->height)
    {
      gint    position = 4 * (((gint) oy) * output_image->width + ((gint) ox));
      gfloat  sample_color[4];
      gfloat *dst_color = &output_image->buffer_f[position];
      gfloat  src_A, dst_A, out_alpha, out_alpha_recip;
      gegl_buffer_sample (input_image->gegl_buffer, ix, iy, NULL,
                          sample_color, output_image->format,
                          output_image->sampler_type, GEGL_ABYSS_NONE);

      /* suppose that output_image has RGBA float pixel format - and what about endianness? */
      src_A = sample_color[3];
      dst_A = dst_color[3];
      out_alpha = src_A + dst_A * (1 - src_A);
      if (out_alpha > 0)
        {
          out_alpha_recip = 1 / out_alpha;
          dst_color[0] = NPD_BLEND_BAND (sample_color[0], dst_color[0], src_A, dst_A, out_alpha_recip);
          dst_color[1] = NPD_BLEND_BAND (sample_color[1], dst_color[1], src_A, dst_A, out_alpha_recip);
          dst_color[2] = NPD_BLEND_BAND (sample_color[2], dst_color[2], src_A, dst_A, out_alpha_recip);
        }
      dst_color[3] = out_alpha;
    }
}

#undef NPD_BLEND_BAND

static void
npd_gegl_get_pixel_color_f (NPDImage *image,
                            gint      x,
                            gint      y,
                            NPDColor *color)
{
  gegl_buffer_sample (image->gegl_buffer, x, y, NULL,
                      &color->color, babl_format ("RGBA u8"),
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties*o          = GEGL_PROPERTIES (operation);
  const Babl    *format_f   = babl_format ("RGBA float");
  NPDProperties *props      = o->user_data;
  NPDModel      *model      = props->model;
  gboolean       have_model = model != NULL;
  NPDDisplay    *display    = NULL;

  if (props->first_run)
    {
      NPDImage *input_image = g_new (NPDImage, 1);
      display = g_new (NPDDisplay, 1);

      npd_init (NULL,
                npd_gegl_get_pixel_color_f,
                npd_gegl_process_pixel, NULL);

      npd_gegl_init_image (&display->image, output, format_f);
      display->image.sampler_type = o->sampler_type;
      npd_gegl_init_image (input_image, input, gegl_buffer_get_format (output));

      if (!have_model)
        {
          model = props->model = o->model = g_new (NPDModel, 1);
          gegl_buffer_copy (input, NULL, GEGL_ABYSS_NONE,
                            output, NULL);
          display->image.buffer_f = (gfloat*) gegl_buffer_linear_open (display->image.gegl_buffer,
                                                                       NULL,
                                                                      &display->image.rowstride,
                                                                       format_f);
          npd_create_model_from_image (model, input_image,
                                       input_image->width, input_image->height,
                                       0, 0, o->square_size);
        }

      model->reference_image = input_image;
      model->display = display;
      props->first_run = FALSE;
    }

  if (have_model)
    {
      display = model->display;

      npd_set_deformation_type (model, o->asap_deformation, o->mls_weights);

      if (o->mls_weights &&
          model->hidden_model->MLS_weights_alpha != o->mls_weights_alpha)
        {
          model->hidden_model->MLS_weights_alpha = o->mls_weights_alpha;
          npd_compute_MLS_weights (model);
        }

      gegl_buffer_clear (display->image.gegl_buffer, NULL);
      display->image.buffer_f = (gfloat*) gegl_buffer_linear_open (display->image.gegl_buffer,
                                                                   NULL,
                                                                  &display->image.rowstride,
                                                                   format_f);

      npd_deform_model (model, o->rigidity);
      npd_draw_model_into_image (model, &display->image);
    }

  gegl_buffer_linear_close (display->image.gegl_buffer, display->image.buffer_f);

  return TRUE;
}

static void
finalize (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data)
    {
      NPDProperties *props   = o->user_data;
      NPDModel      *model   = props->model;
      NPDDisplay    *display = model->display;

      g_free (display);
      g_free (model->reference_image);
      if (!o->preserve_model)
        {
          npd_destroy_model (model);
          g_free (model);
        }

      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process    = process;
  operation_class->prepare = prepare;
  G_OBJECT_CLASS (klass)->finalize = finalize;

  gegl_operation_class_set_keys (operation_class,
    "categories"  , "transform",
    "name"        , "gegl:npd",
    "description" , _("Performs n-point image deformation"),
    NULL);
}

#endif
