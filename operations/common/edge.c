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
 * Author: Jef Poskanzer.
 *
 * GEGL Port: Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

enum_start (gegl_edge_algo)
   enum_value (GEGL_EDGE_SOBEL,    "sobel",    N_("Sobel"))
   enum_value (GEGL_EDGE_PREWITT,  "prewitt",  N_("Prewitt compass"))
   enum_value (GEGL_EDGE_GRADIENT, "gradient", N_("Gradient"))
   enum_value (GEGL_EDGE_ROBERTS,  "roberts",  N_("Roberts"))
   enum_value (GEGL_EDGE_DIFFERENTIAL, "differential", N_("Differential"))
   enum_value (GEGL_EDGE_LAPLACE,  "laplace",  N_("Laplace"))
enum_end (GeglEdgeAlgo)

property_enum (algorithm, _("Algorithm"),
               GeglEdgeAlgo, gegl_edge_algo,
               GEGL_EDGE_SOBEL)
  description (_("Edge detection algorithm"))

property_double (amount, _("Amount"), 2.0)
    description (_("Edge detection amount"))
    value_range (1.0, 10.0)
    ui_range    (1.0, 10.0)

property_enum (border_behavior, _("Border behavior"),
               GeglAbyssPolicy, gegl_abyss_policy,
               GEGL_ABYSS_CLAMP)
  description (_("Edge detection behavior"))

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME        edge
#define GEGL_OP_C_SOURCE    edge.c

#include <math.h>
#include "gegl-op.h"

static inline gfloat
edge_sobel (gfloat *pixels, gdouble amount)
{
  const gint v_kernel[9] = { -1,  0,  1,
                             -2,  0,  2,
                             -1,  0,  1 };
  const gint h_kernel[9] = { -1, -2, -1,
                              0,  0,  0,
                              1,  2,  1 };

  gint i;
  gfloat v_grad, h_grad;

  for (i = 0, v_grad = 0.0f, h_grad = 0.0f; i < 9; i++)
    {
      v_grad += v_kernel[i] * pixels[i];
      h_grad += h_kernel[i] * pixels[i];
    }

  return sqrt (v_grad * v_grad * amount +
               h_grad * h_grad * amount);
}

static inline gfloat
edge_prewitt (gfloat *pixels, gdouble amount)
{
  gint k;
  gfloat max;
  gfloat m[8];

  m[0] =   pixels[0] +   pixels[1] + pixels[2]
         + pixels[3] - 2*pixels[4] + pixels[5]
         - pixels[6] -   pixels[7] - pixels[8];
  m[1] =   pixels[0] +   pixels[1] + pixels[2]
         + pixels[3] - 2*pixels[4] - pixels[5]
         + pixels[6] -   pixels[7] - pixels[8];
  m[2] =   pixels[0] +   pixels[1] - pixels[2]
         + pixels[3] - 2*pixels[4] - pixels[5]
         + pixels[6] +   pixels[7] - pixels[8];
  m[3] =   pixels[0] -   pixels[1] - pixels[2]
         + pixels[3] - 2*pixels[4] - pixels[5]
         + pixels[6] +   pixels[7] + pixels[8];
  m[4] = - pixels[0] -   pixels[1] - pixels[2]
         + pixels[3] - 2*pixels[4] + pixels[5]
         + pixels[6] +   pixels[7] + pixels[8];
  m[5] = - pixels[0] -   pixels[1] + pixels[2]
         - pixels[3] - 2*pixels[4] + pixels[5]
         + pixels[6] +   pixels[7] + pixels[8];
  m[6] = - pixels[0] +   pixels[1] + pixels[2]
         - pixels[3] - 2*pixels[4] + pixels[5]
         - pixels[6] +   pixels[7] + pixels[8];
  m[7] =   pixels[0] +   pixels[1] + pixels[2]
         - pixels[3] - 2*pixels[4] + pixels[5]
         - pixels[6] -   pixels[7] + pixels[8];

  for (k = 0, max = 0.0f; k < 8; k++)
    if (max < m[k])
      max = m[k];

  return amount * max;
}

static inline gfloat
edge_gradient (gfloat *pixels, gdouble amount)
{
  const gint v_kernel[9] = { 0,  0,  0,
                             0,  4, -4,
                             0,  0,  0 };
  const gint h_kernel[9] = { 0,  0,  0,
                             0, -4,  0,
                             0,  4,  0 };

  gint i;
  gfloat v_grad, h_grad;

  for (i = 0, v_grad = 0.0f, h_grad = 0.0f; i < 9; i++)
    {
      v_grad += v_kernel[i] * pixels[i];
      h_grad += h_kernel[i] * pixels[i];
    }

  return  sqrt (v_grad * v_grad * amount +
                h_grad * h_grad * amount);
}

static inline gfloat
edge_roberts (gfloat *pixels, gdouble amount)
{
  const gint v_kernel[9] = { 0,  0,  0,
                             0,  4,  0,
                             0,  0, -4 };
  const gint h_kernel[9] = { 0,  0,  0,
                             0,  0,  4,
                             0, -4,  0 };
  gint i;
  gfloat v_grad, h_grad;

  for (i = 0, v_grad = 0.0f, h_grad = 0.0f; i < 9; i++)
    {
      v_grad += v_kernel[i] * pixels[i];
      h_grad += h_kernel[i] * pixels[i];
    }

  return sqrt (v_grad * v_grad * amount +
               h_grad * h_grad * amount);
}

static inline gfloat
edge_differential (gfloat *pixels, gdouble amount)
{
  const gint v_kernel[9] = { 0,  0,  0,
                             0,  2, -2,
                             0,  2, -2 };
  const gint h_kernel[9] = { 0,  0,  0,
                             0, -2, -2,
                             0,  2,  2 };
  gint i;
  gfloat v_grad, h_grad;

  for (i = 0, v_grad = 0.0f, h_grad = 0.0f; i < 9; i++)
    {
      v_grad += v_kernel[i] * pixels[i];
      h_grad += h_kernel[i] * pixels[i];
    }

  return sqrt (v_grad * v_grad * amount +
               h_grad * h_grad * amount);
}

static inline gfloat
edge_laplace (gfloat *pixels, gdouble amount)
{
  const gint kernel[9] = { 1,  1,  1,
                           1, -8,  1,
                           1,  1,  1 };
  gint i;
  gfloat grad;

  for (i = 0, grad = 0.0f; i < 9; i++)
    grad += kernel[i] * pixels[i];

  return grad * amount;
}

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);

  const Babl *input_f = gegl_operation_get_source_format (operation, "input");
  const Babl *format  = babl_format ("R'G'B' float");

  area->left   =
  area->right  =
  area->top    =
  area->bottom = 1;

  if (input_f)
    {
      if (babl_format_has_alpha (input_f))
        format = babl_format ("R'G'B'A float");
    }

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = { 0, 0, 0, 0 };
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");
  if (in_rect)
    {
      result = *in_rect;
    }

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  const Babl     *format = gegl_operation_get_format (operation, "output");
  gint            components = babl_format_get_n_components (format);
  gboolean        has_alpha  = babl_format_has_alpha (format);

  gfloat *src_buff;
  gfloat *dst_buff;
  GeglRectangle rect;
  gint x, y, ix, iy, b, idx;

  rect = gegl_operation_get_required_for_output (operation, "input", roi);

  src_buff = g_new (gfloat, rect.width * rect.height * components);
  dst_buff = g_new0 (gfloat, roi->width * roi->height * components);

  gegl_buffer_get (input, &rect, 1.0, format, src_buff,
                   GEGL_AUTO_ROWSTRIDE, o->border_behavior);

  for (y = 0; y < roi->height; y++)
    {
      iy = y + 1;
      for (x = 0; x < roi->width; x++)
        {
          ix = x + 1;
          for (b = 0; b < 3; b++)
            {

#define SRCPIX(X,Y,B) src_buff[((X) + (Y) * rect.width) * components + B]

              gfloat window[9];
              window[0] = SRCPIX(ix - 1, iy - 1, b);
              window[1] = SRCPIX(ix, iy - 1, b);
              window[2] = SRCPIX(ix + 1, iy - 1, b);
              window[3] = SRCPIX(ix - 1, iy, b);
              window[4] = SRCPIX(ix, iy, b);
              window[5] = SRCPIX(ix + 1, iy, b);
              window[6] = SRCPIX(ix - 1, iy + 1, b);
              window[7] = SRCPIX(ix, iy + 1, b);
              window[8] = SRCPIX(ix + 1, iy + 1, b);

              idx = (x + y * roi->width) * components + b;

              switch (o->algorithm)
                {
                  default:
                  case GEGL_EDGE_SOBEL:
                    dst_buff[idx] = edge_sobel (window, o->amount);
                    break;

                  case GEGL_EDGE_PREWITT:
                    dst_buff[idx] = edge_prewitt (window, o->amount);
                    break;

                  case GEGL_EDGE_GRADIENT:
                    dst_buff[idx] = edge_gradient (window, o->amount);
                    break;

                  case GEGL_EDGE_ROBERTS:
                    dst_buff[idx] = edge_roberts (window, o->amount);
                    break;

                  case GEGL_EDGE_DIFFERENTIAL:
                    dst_buff[idx] = edge_differential (window, o->amount);
                    break;

                  case GEGL_EDGE_LAPLACE:
                    dst_buff[idx] = edge_laplace (window, o->amount);
                    break;
                }
            }

          if (has_alpha)
            dst_buff[idx + 1] = SRCPIX(ix, iy, 3);
        }
#undef SRCPIX
    }

  gegl_buffer_set (output, roi, level, format, dst_buff, GEGL_AUTO_ROWSTRIDE);

  g_free (src_buff);
  g_free (dst_buff);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process             = process;
  operation_class->prepare          = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->opencl_support   = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:edge",
    "title",       _("Edge Detection"),
    "categories",  "edge-detect",
    "license",     "GPL3+",
    "reference-hash",  "dd9be3825edb58d7b331ec8844a16b5c",
    "reference-hashB", "d18fdd59dd60ad454cdd202bcc9d3035",
    "description", _("Several simple methods for detecting edges"),
    NULL);
}

#endif
