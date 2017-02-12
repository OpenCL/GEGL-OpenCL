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
#include <stdlib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_median_blur_neighborhood)
  enum_value (GEGL_MEDIAN_BLUR_NEIGHBORHOOD_SQUARE,  "square",  N_("Square"))
  enum_value (GEGL_MEDIAN_BLUR_NEIGHBORHOOD_CIRCLE,  "circle",  N_("Circle"))
  enum_value (GEGL_MEDIAN_BLUR_NEIGHBORHOOD_DIAMOND, "diamond", N_("Diamond"))
enum_end (GeglMedianBlurNeighborhood)

property_enum (neighborhood, _("Neighborhood"),
               GeglMedianBlurNeighborhood, gegl_median_blur_neighborhood,
               GEGL_MEDIAN_BLUR_NEIGHBORHOOD_CIRCLE)
  description (_("Neighborhood type"))

property_int  (radius, _("Radius"), 3)
  value_range (0, 100)
  ui_meta     ("unit", "pixel-distance")
  description (_("Neighborhood radius"))

property_double  (percentile, _("Percentile"), 50)
  value_range (0, 100)
  description (_("Neighborhood color percentile"))

property_double  (alpha_percentile, _("Alpha percentile"), 50)
  value_range (0, 100)
  description (_("Neighborhood alpha percentile"))

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME         median_blur
#define GEGL_OP_C_SOURCE     median-blur.c

#include "gegl-op.h"

#define N_BINS  1024

typedef struct
{
  gint bins[N_BINS];
  gint last_median;
  gint last_median_sum;
} HistogramComponent;

typedef struct
{
  HistogramComponent components[4];
  gint               count;
  gint               size;
} Histogram;

typedef enum
{
  LEFT_TO_RIGHT,
  RIGHT_TO_LEFT,
  TOP_TO_BOTTOM
} Direction;

static inline gfloat
histogram_get_median (Histogram *hist,
                      gint       component,
                      gdouble    percentile)
{
  gint                count = hist->count;
  HistogramComponent *comp  = &hist->components[component];
  gint                i     = comp->last_median;
  gint                sum   = comp->last_median_sum;

  if (component == 3)
    count = hist->size;

  if (count == 0)
    return 0.0f;

  count = (gint) ceil (count * percentile);
  count = MAX (count, 1);

  if (sum < count)
    {
      while ((sum += comp->bins[++i]) < count);
    }
  else
    {
      while ((sum -= comp->bins[i--]) >= count);
      sum += comp->bins[++i];
    }

  comp->last_median     = i;
  comp->last_median_sum = sum;

  return ((gfloat) i + .5f) / (gfloat) N_BINS;
}

static inline void
histogram_modify_val (Histogram    *hist,
                      const gint32 *src,
                      gboolean      has_alpha,
                      gint          diff)
{
  gint alpha = diff;
  gint c;

  if (has_alpha)
    alpha *= src[3];

  for (c = 0; c < 3; c++)
    {
      HistogramComponent *comp = &hist->components[c];
      gint                bin  = src[c];

      comp->bins[bin] += alpha;

      /* this is shorthand for:
       *
       *   if (bin <= comp->last_median)
       *     comp->last_median_sum += alpha;
       *
       * but with a notable speed boost.
       */
      comp->last_median_sum += (bin <= comp->last_median) * alpha;
    }

  if (has_alpha)
    {
      HistogramComponent *comp = &hist->components[3];
      gint                bin  = src[3];

      comp->bins[bin] += diff;

      comp->last_median_sum += (bin <= comp->last_median) * diff;
    }

  hist->count += alpha;
}

static inline void
histogram_modify_vals (Histogram    *hist,
                       const gint32 *src,
                       gint          bpp,
                       gint          stride,
                       gboolean      has_alpha,
                       gint          xmin,
                       gint          ymin,
                       gint          xmax,
                       gint          ymax,
                       gint          diff)
{
  gint x;
  gint y;

  if (xmin > xmax || ymin > ymax)
    return;

  src += ymin * stride + xmin * bpp;

  for (y = ymin; y <= ymax; y++, src += stride)
    {
      const gint32 *pixel = src;

      for (x = xmin; x <= xmax; x++, pixel += bpp)
        {
          histogram_modify_val (hist, pixel, has_alpha, diff);
        }
    }
}

static inline void
histogram_update (Histogram                  *hist,
                  const gint32               *src,
                  gint                        bpp,
                  gint                        stride,
                  gboolean                    has_alpha,
                  GeglMedianBlurNeighborhood  neighborhood,
                  gint                        radius,
                  const gint                 *neighborhood_outline,
                  Direction                   dir)
{
  gint i;

  switch (neighborhood)
    {
    case GEGL_MEDIAN_BLUR_NEIGHBORHOOD_SQUARE:
      switch (dir)
        {
          case LEFT_TO_RIGHT:
            histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                   -radius - 1, -radius,
                                   -radius - 1, +radius,
                                   -1);

            histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                   +radius, -radius,
                                   +radius, +radius,
                                   +1);
            break;

          case RIGHT_TO_LEFT:
            histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                   +radius + 1, -radius,
                                   +radius + 1, +radius,
                                   -1);

            histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                   -radius, -radius,
                                   -radius, +radius,
                                   +1);
            break;

          case TOP_TO_BOTTOM:
            histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                   -radius, -radius - 1,
                                   +radius, -radius - 1,
                                   -1);

            histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                   -radius, +radius,
                                   +radius, +radius,
                                   +1);
            break;
        }
      break;

    default:
      switch (dir)
        {
          case LEFT_TO_RIGHT:
            for (i = 0; i < radius; i++)
              {
                histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                       -i - 1, -neighborhood_outline[i],
                                       -i - 1, -neighborhood_outline[i + 1] - 1,
                                       -1);
                histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                       -i - 1, +neighborhood_outline[i + 1] + 1,
                                       -i - 1, +neighborhood_outline[i],
                                       -1);

                histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                       +i, -neighborhood_outline[i],
                                       +i, -neighborhood_outline[i + 1] - 1,
                                       +1);
                histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                       +i, +neighborhood_outline[i + 1] + 1,
                                       +i, +neighborhood_outline[i],
                                       +1);
              }

            histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                   -i - 1, -neighborhood_outline[i],
                                   -i - 1, +neighborhood_outline[i],
                                   -1);

            histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                   +i, -neighborhood_outline[i],
                                   +i, +neighborhood_outline[i],
                                   +1);

            break;

          case RIGHT_TO_LEFT:
            for (i = 0; i < radius; i++)
              {
                histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                       +i + 1, -neighborhood_outline[i],
                                       +i + 1, -neighborhood_outline[i + 1] - 1,
                                       -1);
                histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                       +i + 1, +neighborhood_outline[i + 1] + 1,
                                       +i + 1, +neighborhood_outline[i],
                                       -1);

                histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                       -i, -neighborhood_outline[i],
                                       -i, -neighborhood_outline[i + 1] - 1,
                                       +1);
                histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                       -i, +neighborhood_outline[i + 1] + 1,
                                       -i, +neighborhood_outline[i],
                                       +1);
              }

            histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                   +i + 1, -neighborhood_outline[i],
                                   +i + 1, +neighborhood_outline[i],
                                   -1);

            histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                   -i, -neighborhood_outline[i],
                                   -i, +neighborhood_outline[i],
                                   +1);

            break;

          case TOP_TO_BOTTOM:
            for (i = 0; i < radius; i++)
              {
                histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                       -neighborhood_outline[i],         -i - 1,
                                       -neighborhood_outline[i + 1] - 1, -i - 1,
                                       -1);
                histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                       +neighborhood_outline[i + 1] + 1, -i - 1,
                                       +neighborhood_outline[i],         -i - 1,
                                       -1);

                histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                       -neighborhood_outline[i],         +i,
                                       -neighborhood_outline[i + 1] - 1, +i,
                                       +1);
                histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                       +neighborhood_outline[i + 1] + 1, +i,
                                       +neighborhood_outline[i],         +i,
                                       +1);
              }

            histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                   -neighborhood_outline[i], -i - 1,
                                   +neighborhood_outline[i], -i - 1,
                                   -1);

            histogram_modify_vals (hist, src, bpp, stride, has_alpha,
                                   -neighborhood_outline[i], +i,
                                   +neighborhood_outline[i], +i,
                                   +1);

            break;
        }
      break;
    }
}

static void
init_neighborhood_outline (GeglMedianBlurNeighborhood  neighborhood,
                           gint                        radius,
                           gint                       *neighborhood_outline)
{
  gint i;

  for (i = 0; i <= radius; i++)
    {
      switch (neighborhood)
        {
        case GEGL_MEDIAN_BLUR_NEIGHBORHOOD_SQUARE:
          neighborhood_outline[i] = radius;
          break;

        case GEGL_MEDIAN_BLUR_NEIGHBORHOOD_CIRCLE:
          neighborhood_outline[i] =
            (gint) sqrt ((radius + .5) * (radius + .5) - i * i);
          break;

        case GEGL_MEDIAN_BLUR_NEIGHBORHOOD_DIAMOND:
          neighborhood_outline[i] = radius - i;
          break;
        }
    }
}

static void
convert_values_to_bins (gint32   *src,
                        gint      bpp,
                        gboolean  has_alpha,
                        gint      count)
{
  gint components = 3;
  gint c;

  if (has_alpha)
    components++;

  while (count--)
    {
      for (c = 0; c < components; c++)
        {
          gfloat value = ((gfloat *) src)[c];
          gint   bin;

          bin = (gint) (CLAMP (value, 0.0f, 1.0f) * N_BINS);
          bin = MIN (bin, N_BINS - 1);

          src[c] = bin;
        }

      src += bpp;
    }
}

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area      = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties          *o         = GEGL_PROPERTIES (operation);
  const Babl              *in_format = gegl_operation_get_source_format (operation, "input");
  const Babl              *format    = babl_format ("RGB float");

  area->left   =
  area->right  =
  area->top    =
  area->bottom = o->radius;

  o->user_data = g_renew (gint, o->user_data, o->radius + 1);
  init_neighborhood_outline (o->neighborhood, o->radius, o->user_data);

  if (in_format)
    {
      if (babl_format_has_alpha (in_format))
        format = babl_format ("RGBA float");
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
  GeglProperties *o = GEGL_PROPERTIES (operation);

  gdouble         percentile           = o->percentile       / 100.0;
  gdouble         alpha_percentile     = o->alpha_percentile / 100.0;
  const gint     *neighborhood_outline = o->user_data;

  const Babl     *format               = gegl_operation_get_format (operation, "input");
  gint            n_components         = babl_format_get_n_components (format);
  gboolean        has_alpha            = babl_format_has_alpha (format);

  G_STATIC_ASSERT (sizeof (gint32) == sizeof (gfloat));
  gint32         *src_buf;
  gfloat         *dst_buf;
  GeglRectangle   src_rect;
  gint            src_stride;
  gint            dst_stride;
  gint            n_pixels;

  Histogram      *hist;

  const gint32   *src;
  gfloat         *dst;
  gint            dst_x, dst_y;
  Direction       dir;

  gint            i;
  gint            c;

  src_rect   = gegl_operation_get_required_for_output (operation, "input", roi);
  src_stride = src_rect.width * n_components;
  dst_stride = roi->width * n_components;
  n_pixels   = roi->width * roi->height;
  dst_buf = g_new0 (gfloat, n_pixels                         * n_components);
  src_buf = g_new0 (gint32, src_rect.width * src_rect.height * n_components);

  gegl_buffer_get (input, &src_rect, 1.0, format, src_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
  convert_values_to_bins (src_buf, n_components, has_alpha,
                          src_rect.width * src_rect.height);

  hist = g_slice_new0 (Histogram);

  src = src_buf + o->radius * (src_rect.width + 1) * n_components;
  dst = dst_buf;

  /* compute the first window */

  for (i = -o->radius; i <= o->radius; i++)
    {
      histogram_modify_vals (hist, src, n_components, src_stride, has_alpha,
                             i, -neighborhood_outline[abs (i)],
                             i, +neighborhood_outline[abs (i)],
                             +1);

      hist->size += 2 * neighborhood_outline[abs (i)] + 1;
    }

  for (c = 0; c < 3; c++)
    dst[c] = histogram_get_median (hist, c, percentile);

  if (has_alpha)
    dst[3] = histogram_get_median (hist, 3, alpha_percentile);

  dst_x = 0;
  dst_y = 0;

  n_pixels--;
  dir = LEFT_TO_RIGHT;

  while (n_pixels--)
    {
      /* move the src coords based on current direction and positions */
      if (dir == LEFT_TO_RIGHT)
        {
          if (dst_x != roi->width - 1)
            {
              dst_x++;
              src += n_components;
              dst += n_components;
            }
          else
            {
              dst_y++;
              src += src_stride;
              dst += dst_stride;
              dir = TOP_TO_BOTTOM;
            }
        }
      else if (dir == TOP_TO_BOTTOM)
        {
          if (dst_x == 0)
            {
              dst_x++;
              src += n_components;
              dst += n_components;
              dir = LEFT_TO_RIGHT;
            }
          else
            {
              dst_x--;
              src -= n_components;
              dst -= n_components;
              dir = RIGHT_TO_LEFT;
            }
        }
      else if (dir == RIGHT_TO_LEFT)
        {
          if (dst_x != 0)
            {
              dst_x--;
              src -= n_components;
              dst -= n_components;
            }
          else
            {
              dst_y++;
              src += src_stride;
              dst += dst_stride;
              dir = TOP_TO_BOTTOM;
            }
        }

      histogram_update (hist, src, n_components, src_stride, has_alpha,
                        o->neighborhood, o->radius, neighborhood_outline,
                        dir);

      for (c = 0; c < 3; c++)
        dst[c] = histogram_get_median (hist, c, percentile);

      if (has_alpha)
        dst[3] = histogram_get_median (hist, 3, alpha_percentile);
    }

  gegl_buffer_set (output, roi, 0, format, dst_buf, GEGL_AUTO_ROWSTRIDE);

  g_slice_free (Histogram, hist);
  g_free (dst_buf);
  g_free (src_buf);

  return TRUE;
}

static void
finalize (GObject *object)
{
  GeglOperation  *op = (void*) object;
  GeglProperties *o  = GEGL_PROPERTIES (op);

  if (o->user_data)
    {
      g_free (o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass             *object_class;
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  object_class->finalize            = finalize;
  filter_class->process             = process;
  operation_class->prepare          = prepare;
  operation_class->get_bounding_box = get_bounding_box;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:median-blur",
    "title",       _("Median Blur"),
    "categories",  "blur",
    "description", _("Blur resulting from computing the median "
                     "color in the neighborhood of each pixel."),
    NULL);
}

#endif
