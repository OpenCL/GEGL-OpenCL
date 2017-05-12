/* This file is an image processing operation for GEGL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:   1995 Spencer Kimball and Peter Mattis
 *            1999 Thom van Os <thom@vanos.com>
 *            2006 Loren Merritt
 *
 * GEGL port: Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (blur_radius, _("Blur radius"), 5.0)
  description(_("Radius of square pixel region, (width and height will be radius*2+1)."))
  value_range   (1.0, 1000.0)
  ui_range      (1.0, 100.0)

property_double (max_delta, _("Max. delta"), 0.2)
  description   (_("Maximum delta"))
  value_range   (0.0, 1.0)

#else

#define GEGL_OP_COMPOSER
#define GEGL_OP_NAME     gaussian_blur_selective
#define GEGL_OP_C_SOURCE gaussian-blur-selective.c

#include "gegl-op.h"
#include <math.h>

static void
prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("R'G'B'A float");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "aux",    format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_enlarged_input (GeglOperation       *operation,
                    const GeglRectangle *input_region)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle   rect;

  gint radius  = o->blur_radius;
  rect.x       = input_region->x - radius;
  rect.y       = input_region->y - radius;
  rect.width   = input_region->width  + 2 * radius;
  rect.height  = input_region->height + 2 * radius;

  return rect;
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *region)
{
  GeglRectangle   rect;
  GeglRectangle   defined;

  defined = gegl_operation_get_bounding_box (operation);
  gegl_rectangle_intersect (&rect, region, &defined);

  if (rect.width  != 0 && rect.height != 0)
    {
      rect = get_enlarged_input (operation, &rect);
    }

  return rect;
}

static GeglRectangle
get_invalidated_by_change (GeglOperation       *operation,
                           const gchar         *input_pad,
                           const GeglRectangle *input_region)
{
  return get_enlarged_input (operation, input_region);
}

static gboolean
gblur_selective (GeglBuffer          *input,
                 const GeglRectangle *src_rect,
                 GeglBuffer          *aux,
                 GeglBuffer          *output,
                 const GeglRectangle *dst_rect,
                 gdouble              radius,
                 gdouble              max_delta)
{
  const Babl *format = babl_format ("R'G'B'A float");
  gfloat *gauss;
  gfloat *src_buf;
  gfloat *delta_buf;
  gfloat *dst_buf;

  gint    x, y, dst_offset;
  gint    width = (int) radius * 2 + 1;
  gint    iradius = radius;
  gint    src_width = src_rect->width;
  gint    src_height = src_rect->height;

  gauss   = g_newa (gfloat, width * width);
  src_buf = g_new (gfloat, src_rect->width * src_rect->height * 4);
  dst_buf = g_new (gfloat, dst_rect->width * dst_rect->height * 4);

  if (!aux)
    delta_buf = src_buf;
  else
    {
      delta_buf = g_new (gfloat, src_rect->width * src_rect->height * 4);
      gegl_buffer_get (aux, src_rect, 1.0, format, delta_buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
    }

  gegl_buffer_get (input, src_rect, 1.0, format, src_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  dst_offset = 0;

#define POW2(a) ((a)*(a))

  for (y = -iradius; y <= iradius; y++)
    for (x = -iradius; x <= iradius; x++)
      {
        gauss[x + iradius + (y + iradius) * width] =
            exp(- 0.5 * (POW2(x) + POW2(y)) / radius);
      }

  for (y = 0; y < dst_rect->height; y++)
    for (x = 0; x < dst_rect->width; x++)
      {
        gint u, v, b, src_offset;
        gfloat *center_src;
        gfloat *center_delta;

        gfloat  accumulated[3] = {0.0, };
        gfloat  count[3] = {0.0, };

        src_offset = (x + iradius + (y + iradius) * src_width) * 4;
        center_src   = src_buf + src_offset;
        center_delta = delta_buf + src_offset;

        for (v = -iradius; v <= iradius; v++)
          for (u = -iradius; u <= iradius; u++)
            {
              gint i,j;
              i = x + radius + u;
              j = y + radius + v;
              if (i >= 0 && i < src_width &&
                  j >= 0 && j < src_height)
                {
                  gfloat *src_pix;
                  gfloat *delta_pix;
                  gfloat  diff[3];
                  gfloat  weight;
                  gint    offset;

                  offset = (i + j * src_width) * 4;
                  src_pix   = src_buf + offset;
                  delta_pix = delta_buf + offset;

                  weight = gauss[u + iradius + (v + iradius) * width];
                  weight *= src_pix[3];

                  for (b = 0; b < 3; b++)
                    {
                      diff[b] = center_delta[b] - delta_pix[b];

                      if (diff[b] > max_delta || diff[b] < -max_delta)
                        continue;

                      accumulated[b] += weight * src_pix[b];
                      count[b] += weight;
                    }
                }
            }

        for (b = 0; b < 3; b++)
          {
            if (count[b] != 0.0)
              dst_buf[dst_offset * 4 + b] = accumulated[b] / count[b];
            else
              dst_buf[dst_offset * 4 + b] = center_src[b];
          }

        dst_buf[dst_offset * 4 + 3] = center_src[3];
        dst_offset++;
      }

  gegl_buffer_set (output, dst_rect, 0, format, dst_buf,
                   GEGL_AUTO_ROWSTRIDE);

  g_free (src_buf);
  g_free (dst_buf);

  if (aux)
    g_free (delta_buf);

  return TRUE;
}

#include "opencl/gegl-cl.h"
#include "gegl-buffer-cl-iterator.h"

#include "opencl/gaussian-blur-selective.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_gblur_selective (cl_mem                in,
                    cl_mem                delta,
                    cl_mem                out,
                    size_t                global_worksize,
                    const GeglRectangle  *roi,
                    gfloat                radius,
                    gfloat                max_delta)
{
  cl_int cl_err = 0;
  size_t global_ws[2];

  if (!cl_data)
    {
      const char *kernel_name[] = { "cl_gblur_selective", NULL };
      cl_data = gegl_cl_compile_and_build (gaussian_blur_selective_cl_source,
                                           kernel_name);
    }

  if (!cl_data)
    return TRUE;

  global_ws[0] = roi->width;
  global_ws[1] = roi->height;

  gegl_cl_set_kernel_args (cl_data->kernel[0],
                           sizeof(cl_mem),     &in,
                           sizeof(cl_mem),     &delta,
                           sizeof(cl_mem),     &out,
                           sizeof(cl_float),   &radius,
                           sizeof(cl_float),   &max_delta,
                           NULL);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                        cl_data->kernel[0], 2,
                                        NULL, global_ws, NULL,
                                        0, NULL, NULL);
  CL_CHECK;

  return FALSE;

error:
  return TRUE;
}

static gboolean
cl_process (GeglOperation       *operation,
            GeglBuffer          *input,
            GeglBuffer          *aux,
            GeglBuffer          *output,
            const GeglRectangle *result)
{
  const Babl *in_format  = gegl_operation_get_format (operation, "input");
  const Babl *aux_format = gegl_operation_get_format (operation, "aux");
  const Babl *out_format = gegl_operation_get_format (operation, "output");
  gint err;

  GeglProperties *o = GEGL_PROPERTIES (operation);

  GeglBufferClIterator *i = gegl_buffer_cl_iterator_new (output,
                                                         result,
                                                         out_format,
                                                         GEGL_CL_BUFFER_WRITE);

  gint radius  = o->blur_radius;

  gint read = gegl_buffer_cl_iterator_add_2 (i,
                                             input,
                                             result,
                                             in_format,
                                             GEGL_CL_BUFFER_READ,
                                             radius, radius, radius, radius,
                                             GEGL_ABYSS_CLAMP);

  gint delta = !aux ?
               read :
               gegl_buffer_cl_iterator_add_2 (i,
                                              aux,
                                              result,
                                              aux_format,
                                              GEGL_CL_BUFFER_READ,
                                              radius, radius, radius, radius,
                                              GEGL_ABYSS_CLAMP);

  while (gegl_buffer_cl_iterator_next (i, &err))
    {
      if (err)
        return FALSE;

      err = cl_gblur_selective(i->tex[read],
                               i->tex[delta],
                               i->tex[0],
                               i->size[0],
                               &i->roi[0],
                               o->blur_radius,
                               o->max_delta);

      if (err)
        return FALSE;
    }

  return TRUE;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gboolean        success;
  GeglRectangle   compute;

  compute = get_required_for_output (operation, "input", result);

  if (gegl_operation_use_opencl (operation))
    if (cl_process (operation, input, aux, output, result))
      return TRUE;

  success = gblur_selective (input, &compute,
                             aux,
                             output, result,
                             o->blur_radius, o->max_delta);
  return success;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass         *operation_class;
  GeglOperationComposerClass *composer_class;

  operation_class  = GEGL_OPERATION_CLASS (klass);
  composer_class   = GEGL_OPERATION_COMPOSER_CLASS (klass);

  operation_class->prepare                   = prepare;
  operation_class->get_required_for_output   = get_required_for_output;
  operation_class->get_invalidated_by_change = get_invalidated_by_change;
  operation_class->opencl_support            = TRUE;

  composer_class->process = process;

  gegl_operation_class_set_keys (operation_class,
   "name",        "gegl:gaussian-blur-selective",
   "title",       _("Selective Gaussian Blur"),
   "categories",  "enhance:noise-reduction",
   "reference-hash", "5af7851bcc4c6b9276c105fb6a6b6742",
   "license",     "GPL3+",
   "description", _("Blur neighboring pixels, but only in low-contrast areas"),
   NULL);
}

#endif
