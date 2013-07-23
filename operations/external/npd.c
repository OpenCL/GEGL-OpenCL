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
#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "npd.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <math.h>
#include <npd/npd.h>
#include <npd/npd_gegl.h>

struct _NPDImage
{
  gint     width;
  gint     height;
  NPDPoint position;
  guchar  *buffer;
};

struct _NPDDisplay
{
  NPDImage image;
};

typedef struct
{
  gboolean first_run;
  NPDModel model;
} NPDProperties;

void npd_create_image (NPDImage   *image,
                       GeglBuffer *gegl_buffer,
                       const Babl *format);

void
npd_set_pixel_color (NPDImage *image,
                     gint      x,
                     gint      y,
                     NPDColor *color)
{
  gint position = 4 * (y * image->width + x);
  gint max = 4 * image->width * image->height;
  
  if (position > 0 && position < max)
    {
      image->buffer[position + 0] = color->r;
      image->buffer[position + 1] = color->g;
      image->buffer[position + 2] = color->b;
      image->buffer[position + 3] = color->a;
    }
}

void
npd_get_pixel_color (NPDImage *image,
                     gint      x,
                     gint      y,
                     NPDColor *color)
{
  gint position = 4 * (y * image->width + x);
  gint max = 4 * image->width * image->height;
  
  if (position > 0 && position < max)
    {
      color->r = image->buffer[position + 0];
      color->g = image->buffer[position + 1];
      color->b = image->buffer[position + 2];
      color->a = image->buffer[position + 3];
    }
  else
    {
      color->r = color->g = color->b = color->a = 0;
    }
}

void
npd_draw_model (NPDModel *model,
                NPDDisplay *display)
{
  NPDHiddenModel *hidden_model = model->hidden_model;
  NPDImage *image = model->reference_image;
  gint i;

  /* draw texture */
  if (model->texture_visible)
    {
      for (i = 0; i < hidden_model->num_of_bones; i++)
        {
          npd_texture_quadrilateral(&hidden_model->reference_bones[i],
                                    &hidden_model->current_bones[i],
                                     image,
                                    &display->image);
        }
    }
}

void
npd_create_model_from_image (NPDModel *model,
                             NPDImage *image,
                             gint      square_size)
{
  npd_init_model(model);
  model->reference_image = image;
  model->mesh_square_size = square_size;
    
  npd_create_mesh_from_image(model, image->width, image->height, 0, 0);
}

void
npd_create_image (NPDImage   *image,
                  GeglBuffer *gegl_buffer,
                  const Babl *format)
{
  guchar *buffer;
  buffer = g_new0 (guchar, gegl_buffer_get_pixel_count (gegl_buffer) * 4);
  gegl_buffer_get (gegl_buffer, NULL, 1.0, format, buffer, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  image->buffer = buffer;
  image->width = gegl_buffer_get_width (gegl_buffer);
  image->height = gegl_buffer_get_height (gegl_buffer);
}

void
npd_destroy_image (NPDImage *image)
{
  g_free(image->buffer);
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
      o->chant_data = props;
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
  NPDControlPoint *cp;
  NPDPoint coord, new_coord;
  guchar *output_buffer;
  
  if (props->first_run)
    {
      gint width, height;
      NPDImage input_image;
      NPDDisplay display;

      npd_create_image (&input_image, input, format);
      width = input_image.width;
      height = input_image.height;

      output_buffer = g_new0 (guchar, gegl_buffer_get_pixel_count (input) * 4);
      display.image.width = width;
      display.image.height = height;
      display.image.buffer = output_buffer;

      npd_create_model_from_image(model, &input_image, 20);
      npd_create_list_of_overlapping_points (model->hidden_model);

      model->display = &display;
    }
  else
    {
      output_buffer = model->display->image.buffer;
    }

  coord.x = 20; coord.y = 200;
  new_coord.x = 100; new_coord.y = 250;

  cp = npd_add_control_point (model, &coord);
  npd_set_point_coordinates (&cp->point, &new_coord);
  
  npd_deform_model (model, 1000);
  npd_draw_model (model, model->display);
  
  gegl_buffer_set (output, NULL, 0, format, output_buffer, GEGL_AUTO_ROWSTRIDE);

  return  TRUE;
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
/*    "description" , _("Performs n-point image deformation"),*/
    "description" , "Performs n-point image deformation",
    NULL);
}

#endif