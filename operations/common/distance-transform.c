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
 * Copyright (C) 2014 Simon Budig <simon@gimp.org>
 *
 * implemented following this paper:
 *   A. Meijster, J.B.T.M. Roerdink, and W.H. Hesselink
 *   "A General Algorithm for Computing Distance Transforms in Linear Time",
 *   Mathematical Morphology and its Applications to Image and
 *   Signal Processing, Kluwer Acad. Publ., 2000, pp. 331-340.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_dt_metric)
  enum_value (GEGL_DT_METRIC_EUCLIDEAN,  "euclidean",  N_("Euclidean"))
  enum_value (GEGL_DT_METRIC_MANHATTAN,  "manhattan",  N_("Manhattan"))
  enum_value (GEGL_DT_METRIC_CHESSBOARD, "chessboard", N_("Chessboard"))
enum_end (GeglDTMetric)

property_enum (metric, _("Metric"),
    GeglDTMetric, gegl_dt_metric, GEGL_DT_METRIC_EUCLIDEAN)
    description (_("Metric to use for the distance calculation"))

property_double (threshold_lo, _("Threshold low"), 0.0001)
  value_range (0.0, 1.0)

property_double (threshold_hi, _("Threshold high"), 1.0)
  value_range (0.0, 1.0)

property_int    (averaging, _("Grayscale Averaging"), 0)
    description (_("Number of computations for grayscale averaging"))
    value_range (0, 1000)
    ui_range    (0, 256)
    ui_gamma    (1.5)

property_boolean (normalize, _("Normalize"), TRUE)
  description(_("Normalize output to range 0.0 to 1.0."))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     distance_transform
#define GEGL_OP_C_SOURCE distance-transform.c
#include "gegl-op.h"
#include <math.h>
#include <stdio.h>

#define EPSILON 0.000000000001

gfloat edt_f   (gfloat x, gfloat i, gfloat g_i);
gint   edt_sep (gint i, gint u, gfloat g_i, gfloat g_u);
gfloat mdt_f   (gfloat x, gfloat i, gfloat g_i);
gint   mdt_sep (gint i, gint u, gfloat g_i, gfloat g_u);
gfloat cdt_f   (gfloat x, gfloat i, gfloat g_i);
gint   cdt_sep (gint i, gint u, gfloat g_i, gfloat g_u);

/**
 * Prepare function of gegl filter.
 * @param operation given Gegl operation
 */
static void
prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("Y float");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

/**
 * Returns the cached region. This is an area filter, which acts on the whole image.
 * @param operation given Gegl operation
 * @param roi the rectangle of interest
 * @return result the new rectangle
 */
static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");
  return result;
}


/* Meijster helper functions for euclidean distance transform */
gfloat
edt_f (gfloat x, gfloat i, gfloat g_i)
{
  return sqrt ((x - i) * (x - i) + g_i * g_i);
}

gint
edt_sep (gint i, gint u, gfloat g_i, gfloat g_u)
{
  return (u * u - i * i + ((gint) (g_u * g_u - g_i * g_i))) / (2 * (u - i));
}

/* Meijster helper functions for manhattan distance transform */
gfloat
mdt_f (gfloat x, gfloat i, gfloat g_i)
{
  return fabs (x - i) + g_i;
}

gint
mdt_sep (gint i, gint u, gfloat g_i, gfloat g_u)
{
  if (g_u >= g_i + u - i + EPSILON)
    return INT32_MAX / 4;
  if (g_i > g_u + u - i + EPSILON)
    return INT32_MIN / 4;

  return (((gint) (g_u - g_i)) + u + i) / 2;
}

/* Meijster helper functions for chessboard distance transform */
gfloat
cdt_f (gfloat x, gfloat i, gfloat g_i)
{
  return MAX (fabs (x - i), g_i);
}

gint
cdt_sep (gint i, gint u, gfloat g_i, gfloat g_u)
{
  if (g_i <= g_u)
    return MAX (i + (gint) g_u, (i+u)/2);
  else
    return MIN (u - (gint) g_i, (i+u)/2);
}


static void
binary_dt_2nd_pass (GeglOperation *operation,
                    gint           width,
                    gint           height,
                    gfloat         thres_lo,
                    GeglDTMetric   metric,
                    gfloat        *src,
                    gfloat        *dest)
{
  gint u, y;
  gint q, w, *t, *s;
  gfloat *g, *row_copy;

  gfloat (*dt_f)   (gfloat, gfloat, gfloat);
  gint   (*dt_sep) (gint, gint, gfloat, gfloat);

  switch (metric)
    {
      case GEGL_DT_METRIC_CHESSBOARD:
        dt_f   = cdt_f;
        dt_sep = cdt_sep;
        break;
      case GEGL_DT_METRIC_MANHATTAN:
        dt_f   = mdt_f;
        dt_sep = mdt_sep;
        break;
      default: /* GEGL_DT_METRIC_EUCLIDEAN */
        dt_f   = edt_f;
        dt_sep = edt_sep;
        break;
    }

  /* sorry for the variable naming, they are taken from the paper */

  s = gegl_calloc (sizeof (gint), width);
  t = gegl_calloc (sizeof (gint), width);
  row_copy = gegl_calloc (sizeof (gfloat), width);

  /* this could be parallelized */
  for (y = 0; y < height; y++)
    {
      q = 0;
      s[0] = 0;
      t[0] = 0;
      g = dest + y * width;

      dest[0 + y * width] = MIN (dest[0 + y * width], 1.0);
      dest[width - 1 + y * width] = MIN (dest[width - 1 + y * width], 1.0);

      for (u = 1; u < width; u++)
        {
          while (q >= 0 &&
                 dt_f (t[q], s[q], g[s[q]]) >= dt_f (t[q], u, g[u]) + EPSILON)
            {
              q --;
            }

          if (q < 0)
            {
              q = 0;
              s[0] = u;
            }
          else
            {
              /* function Sep from paper */
              w = dt_sep (s[q], u, g[s[q]], g[u]);
              w += 1;

              if (w < width)
                {
                  q ++;
                  s[q] = u;
                  t[q] = w;
                }
            }
        }

      memcpy (row_copy, g, width * sizeof (gfloat));

      for (u = width - 1; u >= 0; u--)
        {
          if (u == s[q])
            g[u] = row_copy[u];
          else
            g[u] = dt_f (u, s[q], row_copy[s[q]]);

          if (q > 0 && u == t[q])
            {
              q--;
            }
        }

      gegl_operation_progress (operation,
                               (gdouble) y / (gdouble) height / 2.0 + 0.5, "");
    }

  gegl_free (t);
  gegl_free (s);
  gegl_free (row_copy);
}


static void
binary_dt_1st_pass (GeglOperation *operation,
                    gint           width,
                    gint           height,
                    gfloat         thres_lo,
                    gfloat        *src,
                    gfloat        *dest)
{
  int x, y;

  /* this loop could be parallelized */
  for (x = 0; x < width; x++)
    {
      /* consider out-of-range as 0, i.e. the outside is "empty" */
      dest[x + 0 * width] = src[x + 0 * width] > thres_lo ? 1.0 : 0.0;

      for (y = 1; y < height; y++)
        {
          if (src[x + y * width] > thres_lo)
            dest[x + y * width] = 1.0 + dest[x + (y - 1) * width];
          else
            dest[x + y * width] = 0.0;
        }

      dest[x + (height - 1) * width] = MIN (dest[x + (height - 1) * width], 1.0);
      for (y = height - 2; y >= 0; y--)
        {
          if (dest[x + (y + 1) * width] + 1.0 < dest[x + y * width])
            dest [x + y * width] = dest[x + (y + 1) * width] + 1.0;
        }

      gegl_operation_progress (operation,
                               (gdouble) x / (gdouble) width / 2.0, "");
    }
}


/**
 * Process the gegl filter
 * @param operation the given Gegl operation
 * @param input the input buffer.
 * @param output the output buffer.
 * @param result the region of interest.
 * @param level the level of detail
 * @return True, if the filter was successfull applied.
 */
static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties         *o = GEGL_PROPERTIES (operation);
  const Babl  *input_format = babl_format ("Y float");
  const int bytes_per_pixel = babl_format_get_bytes_per_pixel (input_format);

  GeglDTMetric metric;
  gint     width, height, averaging, i;
  gfloat   threshold_lo, threshold_hi, maxval, *src_buf, *dst_buf;
  gboolean normalize;

  width  = result->width;
  height = result->height;

  threshold_lo = o->threshold_lo;
  threshold_hi = o->threshold_hi;
  normalize    = o->normalize;
  metric       = o->metric;
  averaging    = o->averaging;

  src_buf = gegl_malloc (width * height * bytes_per_pixel);
  dst_buf = gegl_calloc (width * height, bytes_per_pixel);

  gegl_operation_progress (operation, 0.0, "");

  gegl_buffer_get (input, result, 1.0, input_format, src_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  if (!averaging)
    {
      binary_dt_1st_pass (operation, width, height, threshold_lo,
                          src_buf, dst_buf);
      binary_dt_2nd_pass (operation, width, height, threshold_lo, metric,
                          src_buf, dst_buf);
    }
  else
    {
      gfloat *tmp_buf;
      gint i, j;

      tmp_buf = gegl_malloc (width * height * bytes_per_pixel);

      for (i = 0; i < averaging; i++)
        {
          gfloat thres;

          thres = (i+1) * (threshold_hi - threshold_lo) / (averaging + 1);
          thres += threshold_lo;

          binary_dt_1st_pass (operation, width, height, thres,
                              src_buf, tmp_buf);
          binary_dt_2nd_pass (operation, width, height, thres, metric,
                              src_buf, tmp_buf);

          for (j = 0; j < width * height; j++)
            dst_buf[j] += tmp_buf[j];
        }

      gegl_free (tmp_buf);
    }

  if (normalize)
    {
      maxval = EPSILON;

      for (i = 0; i < width * height; i++)
        maxval = MAX (dst_buf[i], maxval);
    }
  else
    {
      maxval = averaging;
    }

  if (averaging > 0 || normalize)
    {
      for (i = 0; i < width * height; i++)
        dst_buf[i] = dst_buf[i] * threshold_hi / maxval;
    }

  gegl_buffer_set (output, result, 0, input_format, dst_buf,
                   GEGL_AUTO_ROWSTRIDE);

  gegl_operation_progress (operation, 1.0, "");

  gegl_free (dst_buf);
  gegl_free (src_buf);

  return TRUE;
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;
  gchar                    *composition =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "<node operation='gegl:distance-transform'>"
    "  <params>"
    "    <param name='metric'>euclidean</param>"
    "    <param name='threshold_lo'>0.0001</param>"
    "    <param name='threshold_hi'>1.0</param>"
    "    <param name='averaging'>0</param>"
    "    <param name='normalize'>true</param>"
    "  </params>"
    "</node>"
    "<node operation='gegl:load'>"
    "  <params>"
    "    <param name='path'>standard-input.png</param>"
    "  </params>"
    "</node>"
    "</gegl>";

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->threaded          = FALSE;
  operation_class->prepare           = prepare;
  operation_class->get_cached_region = get_cached_region;
  filter_class->process              = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:distance-transform",
    "title",       _("Distance Transform"),
    "reference-hash", "31dd3c9b78a79583db929b0f77a56191",
    "categories",  "map",
    "description", _("Calculate a distance transform"),
    "reference-composition", composition,
    NULL);
}

#endif
