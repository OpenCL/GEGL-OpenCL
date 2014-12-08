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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <stdlib.h>

#ifdef GEGL_PROPERTIES

property_int (x, _("Width"), 16)
    description (_("Horizontal width of cells pixels"))
    value_range (1, G_MAXINT)
    ui_range    (1, 256)
    ui_gamma    (1.5)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "x")

property_int (y, _("Height"), 16)
    description (_("Vertical width of cells pixels"))
    value_range (1, G_MAXINT)
    ui_range    (1, 256)
    ui_gamma    (1.5)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "y")

property_int (x_offset, _("Offset X"), 0.0)
    description (_("Horizontal offset (from origin) for start of grid"))
    ui_range    (-128, 128)
    ui_meta     ("unit", "pixel-coordinate")
    ui_meta     ("axis", "x")

property_int (y_offset, _("Offset Y"), 0)
    description (_("Vertical offset (from origin) for start of grid"))
    ui_range    (-128, 128)
    ui_meta     ("unit", "pixel-coordinate")
    ui_meta     ("axis", "y")

property_color  (color1, _("Color 1"), "black")
    description (_("The first cell color"))
    ui_meta     ("role", "color-primary")

property_color  (color2, _("Color 2"), "white")
    description (_("The second cell color"))
    ui_meta     ("role", "color-secondary")

property_format (format, _("Babl Format"), NULL)
    description ( _("The babl format of the output"))

#else

#define GEGL_OP_POINT_RENDER
#define GEGL_OP_C_SOURCE checkerboard.c

#include "gegl-op.h"
#include <gegl-buffer-cl-iterator.h>
#include <gegl-debug.h>

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  if (o->format)
    gegl_operation_set_format (operation, "output", o->format);
  else
    gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  return gegl_rectangle_infinite_plane ();
}

#define TILE_INDEX(coordinate,stride) \
  (((coordinate) >= 0)?\
      (coordinate) / (stride):\
      ((((coordinate) + 1) /(stride)) - 1))


#include "opencl/checkerboard.cl.h"
static GeglClRunData *cl_data = NULL;

static gboolean
checkerboard_cl_process (GeglOperation       *operation,
                         cl_mem               out_tex,
                         size_t               global_worksize,
                         const GeglRectangle *roi,
                         gint                 level)
{
  GeglProperties *o         = GEGL_PROPERTIES (operation);
  const Babl   *out_format  = gegl_operation_get_format (operation, "output");
  const size_t  gbl_size[2] = {roi->width, roi->height};
  const size_t  gbl_offs[2] = {roi->x, roi->y};
  cl_int        cl_err      = 0;
  float         color1[4];
  float         color2[4];

  if (!cl_data)
  {
    const char *kernel_name[] = {"kernel_checkerboard", NULL};
    cl_data = gegl_cl_compile_and_build (checkerboard_cl_source, kernel_name);

    if (!cl_data)
      return TRUE;
  }

  gegl_color_get_pixel (o->color1, out_format, color1);
  gegl_color_get_pixel (o->color2, out_format, color2);

  cl_err = gegl_cl_set_kernel_args (cl_data->kernel[0],
                                    sizeof(cl_mem), &out_tex,
                                    sizeof(color1), &color1,
                                    sizeof(color2), &color2,
                                    sizeof(cl_int), &o->x,
                                    sizeof(cl_int), &o->y,
                                    sizeof(cl_int), &o->x_offset,
                                    sizeof(cl_int), &o->y_offset,
                                    NULL);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                        cl_data->kernel[0], 2,
                                        gbl_offs, gbl_size, NULL,
                                        0, NULL, NULL);
  CL_CHECK;

  return FALSE;
error:
  return TRUE;
}


static gboolean
checkerboard_process_simple (GeglOperation       *operation,
                             void                *out_buf,
                             glong                n_pixels,
                             const GeglRectangle *roi,
                             gint                 level)
{
  gint        factor = 1 << level;
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *out_format = gegl_operation_get_format (operation, "output");

  gint        pixel_size = babl_format_get_bytes_per_pixel (out_format);
  guchar     *out_pixel = out_buf;
  void       *color1 = alloca(pixel_size);
  void       *color2 = alloca(pixel_size);

  gint        x = roi->x; /* initial x                   */
  gint        y = roi->y; /*           and y coordinates */

  gegl_color_get_pixel (o->color1, out_format, color1);
  gegl_color_get_pixel (o->color2, out_format, color2);

  while (n_pixels--)
    {
      gint nx,ny;

      if ((x - o->x_offset) < 0)
        {
          nx = div (x - o->x_offset + 1, o->x / factor).quot;
        }
      else
        {
          nx = div (x - o->x_offset, o->x / factor).quot;
        }

      if ((y - o->y_offset) < 0)
        {
          ny = div (y - o->y_offset + 1, o->y / factor).quot;
        }
      else
        {
          ny = div (y - o->y_offset, o->y / factor).quot;
        }

      /* shift negative cell indices */
      nx -= (x - o->x_offset) < 0 ? 1 : 0;
      ny -= (y - o->y_offset) < 0 ? 1 : 0;

      if ( (nx+ny) % 2 == 0)
        memcpy (out_pixel, color1, pixel_size);
      else
        memcpy (out_pixel, color2, pixel_size);

      out_pixel += pixel_size;

      /* update x and y coordinates */
      x++;
      if (x>=roi->x + roi->width)
        {
          x=roi->x;
          y++;
        }
    }

  return  TRUE;
}

static gboolean
checkerboard_process (GeglOperation       *operation,
                      void                *out_buf,
                      glong                n_pixels,
                      const GeglRectangle *roi,
                      gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *out_format = gegl_operation_get_format (operation, "output");
  gint        pixel_size = babl_format_get_bytes_per_pixel (out_format);
  guchar     *out_pixel = out_buf;
  void       *color1 = alloca(pixel_size);
  void       *color2 = alloca(pixel_size);
  gint        y;
  const gint  x_min = roi->x - o->x_offset;
  const gint  y_min = roi->y - o->y_offset;
  const gint  x_max = roi->x + roi->width - o->x_offset;
  const gint  y_max = roi->y + roi->height - o->y_offset;

  const gint  square_width  = o->x;
  const gint  square_height = o->y;

  if (level)
    return checkerboard_process_simple (operation, out_buf, n_pixels, roi, level);

  gegl_color_get_pixel (o->color1, out_format, color1);
  gegl_color_get_pixel (o->color2, out_format, color2);

  for (y = y_min; y < y_max; y++)
    {
      gint  x = x_min;
      void *cur_color;

      /* Figure out which box we're in */
      gint tilex = TILE_INDEX (x, square_width);
      gint tiley = TILE_INDEX (y, square_height);
      if ((tilex + tiley) % 2 == 0)
        cur_color = color1;
      else
        cur_color = color2;

      while (x < x_max)
        {
          /* Figure out how long this stripe is */
          gint count;
          gint stripe_end = (TILE_INDEX (x, square_width) + 1) * square_width;
               stripe_end = stripe_end > x_max ? x_max : stripe_end;

          count = stripe_end - x;

          gegl_memset_pattern (out_pixel, cur_color, pixel_size, count);
          out_pixel += count * pixel_size;
          x = stripe_end;

          if (cur_color == color1)
            cur_color = color2;
          else
            cur_color = color1;
        }
    }

  return TRUE;
}


static gboolean
operation_source_process (GeglOperation       *operation,
                          GeglBuffer          *output,
                          const GeglRectangle *result,
                          gint                 level)
{
  const Babl *out_format = gegl_operation_get_format (operation, "output");

  if ((result->width > 0) && (result->height > 0))
    {
      GeglBufferIterator *iter;
      if (gegl_operation_use_opencl (operation) &&
          babl_format_get_n_components (out_format) == 4 &&
          babl_format_get_type (out_format, 0) == babl_type ("float"))
        {
          GeglBufferClIterator *cl_iter;
          gboolean err;

          GEGL_NOTE (GEGL_DEBUG_OPENCL, "GEGL_OPERATION_POINT_RENDER: %s", GEGL_OPERATION_GET_CLASS (operation)->name);

          cl_iter = gegl_buffer_cl_iterator_new (output, result, out_format, GEGL_CL_BUFFER_WRITE);

          while (gegl_buffer_cl_iterator_next (cl_iter, &err) && !err)
            {
              err = checkerboard_cl_process (operation, cl_iter->tex[0], cl_iter->size[0], &cl_iter->roi[0], level);

              if (err)
                {
                  gegl_buffer_cl_iterator_stop (cl_iter);
                  break;
                }
            }

          if (err)
            GEGL_NOTE (GEGL_DEBUG_OPENCL, "Error: %s", GEGL_OPERATION_GET_CLASS (operation)->name);
          else
            return TRUE;
        }

      iter = gegl_buffer_iterator_new (output, result, level, out_format,
                                       GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (iter))
        checkerboard_process (operation, iter->data[0], iter->length, &iter->roi[0], level);
    }
  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationSourceClass      *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = operation_source_process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:checkerboard",
    "categories",         "render",
    "title",              _("Checkerboard"),
    "position-dependent", "true",
    "description",        _("Create a checkerboard pattern"),
    NULL);
}

#endif
