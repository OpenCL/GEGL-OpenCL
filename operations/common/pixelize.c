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
#include <math.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int_ui (size_x, _("Block Width"),
                   1, 123456, 16, 1, 2048, 1.5,
                   _("Width of blocks in pixels"))

gegl_chant_int_ui (size_y, _("Block Height"),
                   1, 123456, 16, 1, 2048, 1.5,
                   _("Height of blocks in pixels"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE "pixelize.c"

#include "gegl-chant.h"

#define CELL_X(px, cell_width)  ((px) / (cell_width))
#define CELL_Y(py, cell_height) ((py) / (cell_height))

#define CHUNK_SIZE              (1024)
#define ALLOC_THRESHOLD_SIZE    (128)
#define SQR(x)                  ((x)*(x))

static void
prepare (GeglOperation *operation)
{
  GeglChantO              *o;
  GeglOperationAreaFilter *op_area;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);
  o       = GEGL_CHANT_PROPERTIES (operation);

  op_area->left   =
  op_area->right  = o->size_x;
  op_area->top    =
  op_area->bottom = o->size_y;

  gegl_operation_set_format (operation, "input",  babl_format ("RaGaBaA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RaGaBaA float"));
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
                                 GEGL_BUFFER_READ, GEGL_ABYSS_CLAMP);

  while (gegl_buffer_iterator_next (gi))
    {
      gint  k;
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
set_rectangle (gfloat        *output,
               GeglRectangle *rect,
               gint           rowstride,
               gfloat        *color)
{
  gint c, x, y;

  for (y = rect->y; y < rect->y + rect->height; y++)
    for (x = rect->x; x < rect->x + rect->width; x++)
      for (c = 0; c < 4; c++)
        output [4 * (y * rowstride + x) + c] = color[c];
}


static void
pixelize_noalloc (GeglBuffer          *input,
                  GeglBuffer          *output,
                  const GeglRectangle *roi,
                  const GeglRectangle *whole_region,
                  gint                 size_x,
                  gint                 size_y)
{
  gint start_x = (roi->x / size_x) * size_x;
  gint start_y = (roi->y / size_y) * size_y;
  gint x, y;

  GeglColor *color = gegl_color_new ("white");

  for (y = start_y; y < roi->y + roi->height; y += size_y)
    for (x = start_x; x < roi->x + roi->width; x += size_x)
      {
        GeglRectangle rect = {x, y, size_x, size_y};

        gegl_rectangle_intersect (&rect, whole_region, &rect);

        if (rect.width < 1 || rect.height < 1)
          continue;

        mean_rectangle_noalloc (input, &rect, color);

        gegl_rectangle_intersect (&rect, roi, &rect);

        gegl_buffer_set_color (output, &rect, color);
      }

  g_object_unref (color);
}

static void
pixelize (gfloat              *input,
          gfloat              *output,
          const GeglRectangle *roi,
          const GeglRectangle *extended_roi,
          const GeglRectangle *whole_region,
          gint                 size_x,
          gint                 size_y)
{
  gint start_x = (roi->x / size_x) * size_x;
  gint start_y = (roi->y / size_y) * size_y;
  gint x, y;
  gfloat color[4];

  for (y = start_y; y < roi->y + roi->height; y += size_y)
    for (x = start_x; x < roi->x + roi->width; x += size_x)
      {
        GeglRectangle rect = {x, y, size_x, size_y};
        GeglRectangle rect2;

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

        set_rectangle (output, &rect2, roi->width, color);
      }
}

#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"

#include "opencl/pixelize.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_pixelise (cl_mem                in_tex,
             cl_mem                aux_tex,
             cl_mem                out_tex,
             const GeglRectangle  *src_rect,
             const GeglRectangle  *roi,
             gint                  xsize,
             gint                  ysize)
{
  cl_int cl_err = 0;
  const size_t gbl_size[2]= {roi->width, roi->height};

  gint cx0 = CELL_X(roi->x ,xsize);
  gint cy0 = CELL_Y(roi->y ,ysize);
  gint block_count_x = CELL_X(roi->x+roi->width - 1, xsize)-cx0 + 1;
  gint block_count_y = CELL_Y(roi->y+roi->height - 1, ysize)-cy0 + 1;
  cl_int line_width = roi->width + 2 * xsize;

  size_t gbl_size_tmp[2] = {block_count_x,block_count_y};

  if (!cl_data)
  {
    const char *kernel_name[] = {"calc_block_color", "kernel_pixelise", NULL};
    cl_data = gegl_cl_compile_and_build (pixelize_cl_source, kernel_name);
  }

  if (!cl_data) return 1;

  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem),   (void*)&in_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_mem),   (void*)&aux_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_int),   (void*)&xsize);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 3, sizeof(cl_int),   (void*)&ysize);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 4, sizeof(cl_int),   (void*)&roi->x);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 5, sizeof(cl_int),   (void*)&roi->y);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 6, sizeof(cl_int),   (void*)&line_width);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 7, sizeof(cl_int),   (void*)&block_count_x);
  CL_CHECK;
  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                        cl_data->kernel[0], 2,
                                        NULL, gbl_size_tmp, NULL,
                                        0, NULL, NULL);
  CL_CHECK;

  cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 0, sizeof(cl_mem),   (void*)&aux_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 1, sizeof(cl_mem),   (void*)&out_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 2, sizeof(cl_int),   (void*)&xsize);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 3, sizeof(cl_int),   (void*)&ysize);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 4, sizeof(cl_int),   (void*)&roi->x);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 5, sizeof(cl_int),   (void*)&roi->y);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 6, sizeof(cl_int),   (void*)&block_count_x);
  CL_CHECK;
  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
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
  gint err;

  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

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

  gint aux  = gegl_buffer_cl_iterator_add_2 (i,
                                             NULL,
                                             roi,
                                             in_format,
                                             GEGL_CL_BUFFER_AUX,
                                             op_area->left,
                                             op_area->right,
                                             op_area->top,
                                             op_area->bottom,
                                             GEGL_ABYSS_NONE);

  while (gegl_buffer_cl_iterator_next (i, &err))
    {
      if (err) return FALSE;

      err = cl_pixelise(i->tex[read],
                        i->tex[aux],
                        i->tex[0],
                        &i->roi[read],
                        &i->roi[0],
                        o->size_x,
                        o->size_y);

      if (err) return FALSE;
    }

  return TRUE;
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
  GeglChantO              *o = GEGL_CHANT_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);

  whole_region = gegl_operation_source_get_bounding_box (operation, "input");

  if (gegl_cl_is_accelerated ())
    if (cl_process (operation, input, output, roi))
      return TRUE;

  if (o->size_x * o->size_y < SQR (ALLOC_THRESHOLD_SIZE))
    {
      gfloat *input_buf  = g_new (gfloat,
                                  (CHUNK_SIZE + o->size_x * 2) *
                                  (CHUNK_SIZE + o->size_y * 2) * 4);
      gfloat *output_buf = g_new (gfloat, SQR (CHUNK_SIZE) * 4);
      gint    i, j;

      for (j = 0; (j-1) * CHUNK_SIZE < roi->height; j++)
        for (i = 0; (i-1) * CHUNK_SIZE < roi->width; i++)
          {
            GeglRectangle chunked_result;

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

            pixelize (input_buf, output_buf, &chunked_result, &src_rect, whole_region,
                      o->size_x, o->size_y);

            gegl_buffer_set (output, &chunked_result, 1.0, babl_format ("RaGaBaA float"),
                             output_buf, GEGL_AUTO_ROWSTRIDE);
          }

      g_free (input_buf);
      g_free (output_buf);
    }
  else
    {
      pixelize_noalloc (input, output, roi, whole_region,
                        o->size_x, o->size_y);
    }

  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare        = prepare;
  operation_class->opencl_support = TRUE;

  filter_class->process           = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:pixelize",
    "categories",  "blur",
    "description", _("Simplify image into an array of solid-colored rectangles"),
    NULL);
}

#endif
