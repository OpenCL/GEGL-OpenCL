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
 * Copyright 2013 Téo Mazars   <teo.mazars@ensimag.fr>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_pixelize_norm)
  enum_value (GEGL_PIXELIZE_NORM_MANHATTAN, "diamond", N_("Diamond"))
  enum_value (GEGL_PIXELIZE_NORM_EUCLIDEAN, "round",   N_("Round"))
  enum_value (GEGL_PIXELIZE_NORM_INFINITY,  "square",  N_("Square"))
enum_end (GeglPixelizeNorm)

property_enum   (norm, _("Shape"),
    GeglPixelizeNorm, gegl_pixelize_norm, GEGL_PIXELIZE_NORM_INFINITY)
    description (_("The shape of pixels"))

property_int    (size_x, _("Block width"), 16)
    description (_("Width of blocks in pixels"))
    value_range (1, G_MAXINT)
    ui_range    (1, 2048)
    ui_gamma    (1.5)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "x")

property_int    (size_y, _("Block height"), 16)
    description (_("Height of blocks in pixels"))
    value_range (1, G_MAXINT)
    ui_range    (1, 2048)
    ui_gamma    (1.5)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "y")

property_double (ratio_x, _("Size ratio X"), 1.0)
    description (_("Horizontal size ratio of a pixel inside each block"))
    value_range (0.0, 1.0)
    ui_meta     ("axis", "x")

property_double (ratio_y, _("Size ratio Y"), 1.0)
    description (_("Vertical size ratio of a pixel inside each block"))
    value_range (0.0, 1.0)
    ui_meta     ("axis", "y")

property_color  (background, _("Background color"), "white")
    description (_("Color used to fill the background"))
    ui_meta     ("role", "color-secondary")

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     pixelize
#define GEGL_OP_C_SOURCE pixelize.c

#include "gegl-op.h"

#define CHUNK_SIZE           (1024)
#define ALLOC_THRESHOLD_SIZE (64)
#define SQR(x)               ((x)*(x))

static void
prepare (GeglOperation *operation)
{
  GeglProperties              *o;
  GeglOperationAreaFilter *op_area;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);
  o       = GEGL_PROPERTIES (operation);

  op_area->left   =
  op_area->right  = o->size_x;
  op_area->top    =
  op_area->bottom = o->size_y;

  gegl_operation_set_format (operation, "input",  babl_format ("RaGaBaA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RaGaBaA float"));
}

static GeglRectangle
get_bounding_box (GeglOperation *self)
{
  GeglRectangle  result = { 0, 0, 0, 0 };
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (self, "input");
  if (in_rect)
    {
      result = *in_rect;
    }

  return result;
}

static void
mean_rectangle_noalloc (GeglBuffer    *input,
                        GeglRectangle *rect,
                        GeglColor     *color)
{
  GeglBufferIterator *gi;
  gfloat              col[] = {0.0, 0.0, 0.0, 0.0};
  gint                c;

  gi = gegl_buffer_iterator_new (input, rect, 0, babl_format ("RaGaBaA float"),
                                 GEGL_ACCESS_READ, GEGL_ABYSS_CLAMP);

  while (gegl_buffer_iterator_next (gi))
    {
      gint    k;
      gfloat *data = (gfloat*) gi->data[0];

      for (k = 0; k < gi->length; k++)
        {
          for (c = 0; c < 4; c++)
            col[c] += data[c];

          data += 4;
        }
    }

  for (c = 0; c < 4; c++)
    col[c] /= rect->width * rect->height;

  gegl_color_set_pixel (color, babl_format ("RaGaBaA float"), col);
}

static void
mean_rectangle (gfloat        *input,
                GeglRectangle *rect,
                gint           rowstride,
                gfloat        *color)
{
  gint c, x, y;

  for (c = 0; c < 4; c++)
    color[c] = 0;

  for (y = rect->y; y < rect->y + rect->height; y++)
    for (x = rect->x; x < rect->x + rect->width; x++)
      for (c = 0; c < 4; c++)
        color[c] += input [4 * (y * rowstride + x) + c];

  for (c = 0; c < 4; c++)
    color[c] /= rect->width * rect->height;
}

static void
set_rectangle (gfloat          *output,
               GeglRectangle   *rect,
               GeglRectangle   *rect_shape,
               gint             rowstride,
               gfloat          *color,
               GeglPixelizeNorm norm)
{
  gint c, x, y;
  gfloat center_x, center_y;
  GeglRectangle rect2;

  gfloat shape_area = rect_shape->width * rect_shape->height;

  center_x = rect_shape->x + rect_shape->width / 2.0f;
  center_y = rect_shape->y + rect_shape->height / 2.0f;

  gegl_rectangle_intersect (&rect2, rect, rect_shape);

  switch (norm)
    {
    case (GEGL_PIXELIZE_NORM_INFINITY):

      for (y = rect2.y; y < rect2.y + rect2.height; y++)
        for (x = rect2.x; x < rect2.x + rect2.width; x++)
          for (c = 0; c < 4; c++)
            output [4 * (y * rowstride + x) + c] = color[c];
      break;

    case (GEGL_PIXELIZE_NORM_EUCLIDEAN):

      for (y = rect->y; y < rect->y + rect->height; y++)
        for (x = rect->x; x < rect->x + rect->width; x++)
          if (SQR ((x - center_x) / (gfloat) rect_shape->width) +
              SQR ((y - center_y) / (gfloat) rect_shape->height) <= 1.0f)
            for (c = 0; c < 4; c++)
              output [4 * (y * rowstride + x) + c] = color[c];
      break;

    case (GEGL_PIXELIZE_NORM_MANHATTAN):

      for (y = rect->y; y < rect->y + rect->height; y++)
        for (x = rect->x; x < rect->x + rect->width; x++)
          if (fabsf (center_x - x) * rect_shape->height +
              fabsf (center_y - y) * rect_shape->width < shape_area)
            for (c = 0; c < 4; c++)
              output [4 * (y * rowstride + x) + c] = color[c];

      break;
    }
}

static void
set_rectangle_noalloc (GeglBuffer      *output,
                       GeglRectangle   *rect,
                       GeglRectangle   *rect_shape,
                       GeglColor       *color,
                       GeglPixelizeNorm norm)
{
  if (norm == GEGL_PIXELIZE_NORM_INFINITY)
    {
      GeglRectangle rect2;
      gegl_rectangle_intersect (&rect2, rect, rect_shape);
      gegl_buffer_set_color (output, &rect2, color);
    }
  else
    {
      GeglBufferIterator *gi;
      gint                c, x, y;
      gfloat              col[4];
      gfloat              center_x, center_y;
      gfloat              shape_area = rect_shape->width * rect_shape->height;

      center_x = rect_shape->x + rect_shape->width / 2.0f;
      center_y = rect_shape->y + rect_shape->height / 2.0f;

      gegl_color_get_pixel (color, babl_format ("RaGaBaA float"), col);

      gi = gegl_buffer_iterator_new (output, rect, 0, babl_format ("RaGaBaA float"),
                                     GEGL_ACCESS_WRITE, GEGL_ABYSS_CLAMP);

      while (gegl_buffer_iterator_next (gi))
        {
          gfloat       *data = (gfloat*) gi->data[0];
          GeglRectangle roi = gi->roi[0];

          switch (norm)
            {
            case (GEGL_PIXELIZE_NORM_EUCLIDEAN):

              for (y = 0; y < roi.height; y++)
                for (x = 0; x < roi.width; x++)
                  if (SQR ((x + roi.x - center_x) / (gfloat) rect_shape->width) +
                      SQR ((y + roi.y - center_y) / (gfloat) rect_shape->height) <= 1.0f)
                    for (c = 0; c < 4; c++)
                      data [4 * (y * roi.width + x) + c] = col[c];
              break;

            case (GEGL_PIXELIZE_NORM_MANHATTAN):

              for (y = 0; y < roi.height; y++)
                for (x = 0; x < roi.width; x++)
                  if (fabsf (x + roi.x - center_x) * rect_shape->height +
                      fabsf (y + roi.y - center_y) * rect_shape->width
                      < shape_area)
                    for (c = 0; c < 4; c++)
                      data [4 * (y * roi.width + x) + c] = col[c];
              break;

            case (GEGL_PIXELIZE_NORM_INFINITY):
              break;
            }
        }
    }
}

static int
block_index (int pos,
             int size)
{
  return pos < 0 ? ((pos + 1) / size - 1) : (pos / size);
}

static void
pixelize_noalloc (GeglBuffer          *input,
                  GeglBuffer          *output,
                  const GeglRectangle *roi,
                  const GeglRectangle *whole_region,
                  GeglProperties          *o)
{
  gint start_x = block_index (roi->x, o->size_x) * o->size_x;
  gint start_y = block_index (roi->y, o->size_y) * o->size_y;
  gint x, y;
  gint off_shape_x, off_shape_y;

  GeglColor *color = gegl_color_new ("white");

  GeglRectangle rect_shape;

  rect_shape.width  = ceilf (o->size_x * (gfloat)o->ratio_x);
  rect_shape.height = ceilf (o->size_y * (gfloat)o->ratio_y);

  off_shape_x = floorf ((o->size_x - (gfloat)o->ratio_x * o->size_x) / 2.0f);
  off_shape_y = floorf ((o->size_y - (gfloat)o->ratio_y * o->size_y) / 2.0f);

  for (y = start_y; y < roi->y + roi->height; y += o->size_y)
    for (x = start_x; x < roi->x + roi->width; x += o->size_x)
      {
        GeglRectangle rect = {x, y, o->size_x, o->size_y};

        gegl_rectangle_intersect (&rect, whole_region, &rect);

        if (rect.width < 1 || rect.height < 1)
          continue;

        mean_rectangle_noalloc (input, &rect, color);

        gegl_rectangle_intersect (&rect, roi, &rect);

        rect_shape.x = x + off_shape_x;
        rect_shape.y = y + off_shape_y;

        set_rectangle_noalloc (output, &rect, &rect_shape, color, o->norm);
      }

  g_object_unref (color);
}

static void
pixelize (gfloat              *input,
          gfloat              *output,
          const GeglRectangle *roi,
          const GeglRectangle *extended_roi,
          const GeglRectangle *whole_region,
          GeglProperties          *o)
{
  gint          start_x = block_index (roi->x, o->size_x) * o->size_x;
  gint          start_y = block_index (roi->y, o->size_y) * o->size_y;
  gint          x, y;
  gint          off_shape_x, off_shape_y;
  gfloat        color[4];
  GeglRectangle rect_shape;

  rect_shape.width  = ceilf (o->size_x * (gfloat)o->ratio_x);
  rect_shape.height = ceilf (o->size_y * (gfloat)o->ratio_y);

  off_shape_x = floorf ((o->size_x - (gfloat)o->ratio_x * o->size_x) / 2.0f);
  off_shape_y = floorf ((o->size_y - (gfloat)o->ratio_y * o->size_y) / 2.0f);

  for (y = start_y; y < roi->y + roi->height; y += o->size_y)
    for (x = start_x; x < roi->x + roi->width; x += o->size_x)
      {
        GeglRectangle rect = {x, y, o->size_x, o->size_y};
        GeglRectangle rect2;

        rect_shape.x = x + off_shape_x;
        rect_shape.y = y + off_shape_y;

        gegl_rectangle_intersect (&rect, whole_region, &rect);

        if (rect.width < 1 || rect.height < 1)
          continue;

        rect2.x = rect.x - extended_roi->x;
        rect2.y = rect.y - extended_roi->y;
        rect2.width  = rect.width;
        rect2.height = rect.height;

        mean_rectangle (input, &rect2, extended_roi->width, color);

        gegl_rectangle_intersect (&rect, roi, &rect);

        rect2.x = rect.x - roi->x;
        rect2.y = rect.y - roi->y;
        rect2.width  = rect.width;
        rect2.height = rect.height;

        rect_shape.x -= roi->x;
        rect_shape.y -= roi->y;

        set_rectangle (output, &rect2, &rect_shape,
                       roi->width, color, o->norm);
      }
}

#include "opencl/gegl-cl.h"
#include "gegl-buffer-cl-iterator.h"

#include "opencl/pixelize.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_pixelize (cl_mem               in_tex,
             cl_mem               aux_tex,
             cl_mem               out_tex,
             const GeglRectangle *src_rect,
             const GeglRectangle *roi,
             gint                 xsize,
             gint                 ysize,
             gfloat               xratio,
             gfloat               yratio,
             gfloat               bg_color[4],
             gint                 norm,
             GeglRectangle       *image_extent)
{
  cl_int cl_err = 0;
  const size_t gbl_size[2]= {roi->width, roi->height};

  gint cx0 = block_index (roi->x, xsize);
  gint cy0 = block_index (roi->y, ysize);
  gint block_count_x = block_index (roi->x + roi->width  + xsize - 1, xsize) - cx0;
  gint block_count_y = block_index (roi->y + roi->height + ysize - 1, ysize) - cy0;
  cl_int4 bbox = {{ image_extent->x, image_extent->y,
                    image_extent->x + image_extent->width,
                    image_extent->y + image_extent->height }};

  cl_int line_width = roi->width + 2 * xsize;

  size_t gbl_size_tmp[2] = {block_count_x, block_count_y};

  if (!cl_data)
  {
    const char *kernel_name[] = {"calc_block_color", "kernel_pixelize", NULL};
    cl_data = gegl_cl_compile_and_build (pixelize_cl_source, kernel_name);
  }

  if (!cl_data) return 1;

  cl_err = gegl_cl_set_kernel_args (cl_data->kernel[0],
                                    sizeof(cl_mem), (void*)&in_tex,
                                    sizeof(cl_mem), (void*)&aux_tex,
                                    sizeof(cl_int), (void*)&xsize,
                                    sizeof(cl_int), (void*)&ysize,
                                    sizeof(cl_int), (void*)&roi->x,
                                    sizeof(cl_int), (void*)&roi->y,
                                    sizeof(cl_int4), &bbox,
                                    sizeof(cl_int), (void*)&line_width,
                                    sizeof(cl_int), (void*)&block_count_x,
                                    NULL);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                        cl_data->kernel[0], 2,
                                        NULL, gbl_size_tmp, NULL,
                                        0, NULL, NULL);
  CL_CHECK;

  cl_err = gegl_cl_set_kernel_args (cl_data->kernel[1],
                                    sizeof(cl_mem),   (void*)&aux_tex,
                                    sizeof(cl_mem),   (void*)&out_tex,
                                    sizeof(cl_int),   (void*)&xsize,
                                    sizeof(cl_int),   (void*)&ysize,
                                    sizeof(cl_float), (void*)&xratio,
                                    sizeof(cl_float), (void*)&yratio,
                                    sizeof(cl_int),   (void*)&roi->x,
                                    sizeof(cl_int),   (void*)&roi->y,
                                    sizeof(cl_float4),(void*)bg_color,
                                    sizeof(cl_int),   (void*)&norm,
                                    sizeof(cl_int),   (void*)&block_count_x,
                                    NULL);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                        cl_data->kernel[1], 2,
                                        NULL, gbl_size, NULL,
                                        0, NULL, NULL);
  CL_CHECK;

  return FALSE;

error:
  return TRUE;
}

static gboolean
cl_process (GeglOperation       *operation,
            GeglBuffer          *input,
            GeglBuffer          *output,
            const GeglRectangle *roi)
{
  const Babl *in_format  = babl_format ("RaGaBaA float");
  const Babl *out_format = babl_format ("RaGaBaA float");
  gint   err;
  gfloat bg_color[4];
  gint   norm;

  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle *image_extent;

  GeglBufferClIterator *i = gegl_buffer_cl_iterator_new   (output,
                                                           roi,
                                                           out_format,
                                                           GEGL_CL_BUFFER_WRITE);

  gint read = gegl_buffer_cl_iterator_add_2 (i,
                                             input,
                                             roi,
                                             in_format,
                                             GEGL_CL_BUFFER_READ,
                                             op_area->left,
                                             op_area->right,
                                             op_area->top,
                                             op_area->bottom,
                                             GEGL_ABYSS_CLAMP);

  gint aux = gegl_buffer_cl_iterator_add_aux (i,
                                              roi,
                                              in_format,
                                              op_area->left,
                                              op_area->right,
                                              op_area->top,
                                              op_area->bottom);


  gegl_color_get_pixel (o->background, babl_format ("RaGaBaA float"), bg_color);

  norm = 0;
  norm = o->norm == GEGL_PIXELIZE_NORM_EUCLIDEAN ? 1 : norm;
  norm = o->norm == GEGL_PIXELIZE_NORM_INFINITY  ? 2 : norm;

  image_extent = gegl_operation_source_get_bounding_box (operation, "input");

  while (gegl_buffer_cl_iterator_next (i, &err) && !err)
    {
      err = cl_pixelize(i->tex[read],
                        i->tex[aux],
                        i->tex[0],
                        &i->roi[read],
                        &i->roi[0],
                        o->size_x,
                        o->size_y,
                        o->ratio_x,
                        o->ratio_y,
                        bg_color,
                        norm,
                        image_extent);

      if (err)
        {
          gegl_buffer_cl_iterator_stop (i);
          break;
        }
    }

  return !err;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglRectangle            src_rect;
  GeglRectangle           *whole_region;
  GeglProperties          *o = GEGL_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);

  whole_region = gegl_operation_source_get_bounding_box (operation, "input");

  if (gegl_operation_use_opencl (operation))
    if (cl_process (operation, input, output, roi))
      return TRUE;

  if (o->size_x * o->size_y < SQR (ALLOC_THRESHOLD_SIZE))
    {
      gfloat  background_color[4];
      gfloat *input_buf  = g_new (gfloat,
                                  (CHUNK_SIZE + o->size_x * 2) *
                                  (CHUNK_SIZE + o->size_y * 2) * 4);
      gfloat *output_buf = g_new (gfloat, SQR (CHUNK_SIZE) * 4);
      gint    i, j;

      gegl_color_get_pixel (o->background, babl_format("RaGaBaA float"),
                            background_color);

      for (j = 0; (j-1) * CHUNK_SIZE < roi->height; j++)
        for (i = 0; (i-1) * CHUNK_SIZE < roi->width; i++)
          {
            GeglRectangle chunked_result;
            GeglRectangle chunked_sizes;

            chunked_result = *GEGL_RECTANGLE (roi->x + i * CHUNK_SIZE,
                                              roi->y + j * CHUNK_SIZE,
                                              CHUNK_SIZE, CHUNK_SIZE);

            gegl_rectangle_intersect (&chunked_result, &chunked_result, roi);

            if (chunked_result.width < 1  || chunked_result.height < 1)
              continue;

            src_rect = chunked_result;
            src_rect.x -= op_area->left;
            src_rect.y -= op_area->top;
            src_rect.width += op_area->left + op_area->right;
            src_rect.height += op_area->top + op_area->bottom;

            gegl_buffer_get (input, &src_rect, 1.0, babl_format ("RaGaBaA float"),
                             input_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

            gegl_rectangle_copy (&chunked_sizes, &chunked_result);
            chunked_sizes.x = 0;
            chunked_sizes.y = 0;

            set_rectangle (output_buf, &chunked_sizes, &chunked_sizes,
                           chunked_result.width, background_color,
                           GEGL_PIXELIZE_NORM_INFINITY);

            pixelize (input_buf, output_buf, &chunked_result, &src_rect,
                      whole_region, o);

            gegl_buffer_set (output, &chunked_result, 0,
                             babl_format ("RaGaBaA float"),
                             output_buf, GEGL_AUTO_ROWSTRIDE);
          }

      g_free (input_buf);
      g_free (output_buf);
    }
  else
    {
      gegl_buffer_set_color (output, roi, o->background);
      pixelize_noalloc (input, output, roi, whole_region, o);
    }

  return  TRUE;
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare          = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->opencl_support   = TRUE;

  filter_class->process           = process;

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:pixelize",
    "categories",         "blur:scramble",
    "position-dependent", "true",
    "title",              _("Pixelize"),
    "reference-hash",     "0bad844f03b9950e5d64b66317e97bd9",
    "description", _("Simplify image into an array of solid-colored rectangles"),
    NULL);
}

#endif
