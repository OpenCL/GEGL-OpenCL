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
 * Copyright Nigel Wetten
 * Copyright 2000 Tim Copperfield <timecop@japan.co.jp>
 * Copyright 2011 Hans Lo <hansshulo@gmail.com>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_wind_style)
  enum_value (GEGL_WIND_STYLE_WIND, "wind", N_("Wind"))
  enum_value (GEGL_WIND_STYLE_BLAST, "blast", N_("Blast"))
enum_end (GeglWindStyle)

enum_start (gegl_wind_direction)
  enum_value (GEGL_WIND_DIRECTION_LEFT, "left", N_("Left"))
  enum_value (GEGL_WIND_DIRECTION_RIGHT, "right", N_("Right"))
  enum_value (GEGL_WIND_DIRECTION_TOP, "top", N_("Top"))
  enum_value (GEGL_WIND_DIRECTION_BOTTOM, "bottom", N_("Bottom"))
enum_end (GeglWindDirection)

enum_start (gegl_wind_edge)
  enum_value (GEGL_WIND_EDGE_BOTH, "both", N_("Both"))
  enum_value (GEGL_WIND_EDGE_LEADING, "leading", N_("Leading"))
  enum_value (GEGL_WIND_EDGE_TRAILING, "trailing", N_("Trailing"))
enum_end (GeglWindEdge)

property_enum (style, _("Style"),
               GeglWindStyle, gegl_wind_style,
               GEGL_WIND_STYLE_WIND)
  description (_("Style of effect"))

property_enum (direction, _("Direction"),
               GeglWindDirection, gegl_wind_direction,
               GEGL_WIND_DIRECTION_LEFT)
  description (_("Direction of the effect"))

property_enum (edge, _("Edge Affected"),
               GeglWindEdge, gegl_wind_edge,
               GEGL_WIND_EDGE_LEADING)
  description (_("Edge behavior"))

property_int (threshold, _("Threshold"), 10)
 description (_("Higher values restrict the effect to fewer areas of the image"))
 value_range (0, 50)

property_int (strength, _("Strength"), 10)
 description (_("Higher values increase the magnitude of the effect"))
 value_range (1, 100)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_C_SOURCE wind.c

#include "gegl-op.h"
#include "gegl-config.h"
#include <math.h>

#define COMPARE_WIDTH    3

typedef struct ThreadData
{
  GeglOperationFilterClass *klass;
  GeglOperation            *operation;
  GeglBuffer               *input;
  GeglBuffer               *output;
  gint                     *pending;
  gint                      level;
  gboolean                  success;
  GeglRectangle             roi;
} ThreadData;

static void
thread_process (gpointer thread_data, gpointer unused)
{
  ThreadData *data = thread_data;
  if (!data->klass->process (data->operation,
                       data->input, data->output, &data->roi, data->level))
    data->success = FALSE;
  g_atomic_int_add (data->pending, -1);
}

static GThreadPool *
thread_pool (void)
{
  static GThreadPool *pool = NULL;
  if (!pool)
    {
      pool =  g_thread_pool_new (thread_process, NULL, gegl_config_threads (),
                                 FALSE, NULL);
    }
  return pool;
}

static void
get_derivative (gfloat       *pixel1,
                gfloat       *pixel2,
                gboolean      has_alpha,
                GeglWindEdge  edge,
                gfloat       *derivative)
{
  gint i;

  for (i = 0; i < 3; i++)
    derivative[i] = pixel2[i] - pixel1[i];

  if (has_alpha)
    derivative[3] = pixel2[3] - pixel1[3];
  else
    derivative[3] = 0.0;

  if (edge == GEGL_WIND_EDGE_BOTH)
    {
      for (i = 0; i < 4; i++)
        derivative[i] = fabs (derivative[i]);
    }
  else if (edge == GEGL_WIND_EDGE_LEADING)
    {
      for (i = 0; i < 4; i++)
        derivative[i] = - derivative[i];
    }
}

static gboolean
threshold_exceeded (gfloat         *pixel1,
                    gfloat         *pixel2,
                    gboolean        has_alpha,
                    GeglWindEdge    edge,
                    gint            threshold)
{
  gfloat derivative[4];
  gint i;
  gfloat sum = 0.0;

  get_derivative (pixel1, pixel2, has_alpha, edge, derivative);

  for (i = 0; i < 4; i++)
    sum += derivative[i];

  return ((sum / 4.0f) > (threshold / 200.0));
}

static void
reverse_buffer (gfloat *buffer,
                gint    length,
                gint    bytes)
{
  gint b, i, si;
  gfloat temp;
  gint midpoint;

  midpoint = length / 2;
  for (i = 0; i < midpoint; i += bytes)
    {
      si = length - bytes - i;

      for (b = 0; b < bytes; b++)
        {
          temp = buffer[i + b];
          buffer[i + b] = buffer[si + b];
          buffer[si + b] = temp;
        }
    }
}

static void
render_wind_row (gfloat         *buffer,
                 gint            n_components,
                 gint            lpi,
                 GeglProperties *o,
                 GRand          *gr)
{
  gfloat *blend_color;
  gfloat *target_color;
  gfloat *blend_amt;
  gboolean has_alpha;
  gint i, j, b;
  gint bleed_length;
  gint n;
  gint sbi;  /* starting bleed index */
  gint lbi;      /* last bleed index */
  gdouble denominator;
  gint comp_stride = n_components * COMPARE_WIDTH;

  target_color = g_new0 (gfloat, n_components);
  blend_color  = g_new0 (gfloat, n_components);
  blend_amt    = g_new0 (gfloat, n_components);

  has_alpha = n_components > 3 ? TRUE : FALSE;

  for (j = 0; j < lpi; j += n_components)
    {
      gint pxi = j;

      if (threshold_exceeded (buffer + pxi,
                              buffer + pxi + comp_stride,
                              has_alpha,
                              o->edge,
                              o->threshold))
        {
          gdouble bleed_length_max;
          sbi = pxi + comp_stride;

          for (b = 0; b < n_components; b++)
            {
              blend_color[b]  = buffer[pxi + b];
              target_color[b] = buffer[sbi + b];
            }

          if (g_rand_int_range (gr, 0, 3))
            {
              bleed_length_max = o->strength;
            }
          else
            {
              bleed_length_max = 4 * o->strength;
            }

          bleed_length = 1 + (gint) (bleed_length_max * g_rand_double (gr));

          lbi = sbi + bleed_length * n_components;
          if (lbi > lpi)
            {
              lbi = lpi;
            }

          for (b = 0; b < n_components; b++)
            blend_amt[b] = target_color[b] - blend_color[b];

          denominator = 2.0 / (bleed_length * bleed_length + bleed_length);
          n = bleed_length;

          for (i = sbi; i < lbi; i += n_components)
            {
              if (!threshold_exceeded (buffer + pxi,
                                       buffer + i,
                                       has_alpha,
                                       o->edge,
                                       o->threshold)
                  && g_rand_boolean (gr))
                {
                  break;
                }

              for (b = 0; b < n_components; b++)
                {
                  blend_color[b] += blend_amt[b] * n * denominator;
                  blend_color[b] = CLAMP (blend_color[b], 0.0, 1.0);
                  buffer[i + b] = (blend_color[b] * 2 + buffer[i + b]) / 3;
                }

              if (threshold_exceeded (buffer + i,
                                      buffer + i + comp_stride,
                                      has_alpha,
                                      GEGL_WIND_EDGE_BOTH,
                                      o->threshold))
                {
                  for (b = 0; b < n_components; b++)
                    {
                      target_color[b] = buffer[i + comp_stride + b];
                      blend_amt[b] = target_color[b] - blend_color[b];
                    }

                  denominator = 2.0 / (n * n + n);
                }
              n--;
            }
        }
    }

  g_free (target_color);
  g_free (blend_color);
  g_free (blend_amt);
}

static gboolean
render_blast_row (gfloat         *buffer,
                  gint            n_components,
                  gint            lpi,
                  GeglProperties *o,
                  GRand          *gr)
{
  gint sbi, lbi;
  gint bleed_length;
  gint i, j, b;
  gint weight, random_factor;
  gboolean skip = FALSE;

  for (j = 0; j < lpi; j += n_components)
    {
      gfloat *pbuf = buffer + j;

      if (threshold_exceeded (pbuf,
                              pbuf + n_components,
                              n_components > 3,
                              o->edge,
                              o->threshold))
        {
          sbi = j;
          weight = g_rand_int_range (gr, 0, 10);

          if (weight > 5)
            {
              random_factor = 2;
            }
          else if (weight > 3)
            {
              random_factor = 3;
            }
          else
            {
              random_factor = 4;
            }

          bleed_length = 0;

          switch (g_rand_int_range (gr, 0, random_factor))
            {
            case 3:
              bleed_length += o->strength;
            case 2:
              bleed_length += o->strength;
            case 1:
              bleed_length += o->strength;
            case 0:
              bleed_length += o->strength;
            }

          lbi = sbi + n_components * bleed_length;
          if (lbi > lpi)
            {
              lbi = lpi;
            }

          for (i = sbi; i < lbi; i += n_components)
            for (b = 0; b < n_components; b++)
                buffer[i+b] = *(pbuf + b);

          j = lbi - n_components;

          if (g_rand_int_range (gr, 0, 10) > 7)
            {
              skip = TRUE;
            }
        }
    }
  return skip;
}

static void
prepare (GeglOperation *operation)
{
  const Babl *in_format = gegl_operation_get_source_format (operation, "input");
  const Babl *format = babl_format ("RGB float");

  if (in_format)
  {
    if (babl_format_has_alpha (in_format))
      format = babl_format ("RGBA float");
  }

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle  *boundary;
  GeglRectangle   result;

  boundary = gegl_operation_source_get_bounding_box (operation, "input");
  result   = *roi;

  if (o->direction == GEGL_WIND_DIRECTION_LEFT ||
      o->direction == GEGL_WIND_DIRECTION_RIGHT)
    {
      result.x     = boundary->x;
      result.width = boundary->width;
    }
  else
    {
      result.y      = boundary->y;
      result.height = boundary->height;
    }

  return result;
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle  *boundary;
  GeglRectangle   result;

  boundary = gegl_operation_source_get_bounding_box (operation, "input");
  result   = *roi;

  if (o->direction == GEGL_WIND_DIRECTION_TOP)
    {
      result.height = boundary->height - roi->y;
    }
  else if (o->direction == GEGL_WIND_DIRECTION_BOTTOM)
    {
      result.y      = boundary->y;
      result.height = boundary->height - roi->y + roi->height;
    }
  else if (o->direction == GEGL_WIND_DIRECTION_RIGHT)
    {
      result.x     = boundary->x;
      result.width = boundary->width - roi->x + roi->width;
    }
  else
    {
      result.width = boundary->width - roi->x;
    }

  return result;
}

static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglProperties           *o = GEGL_PROPERTIES (operation);
  GeglOperationFilterClass *klass;
  GeglBuffer               *input;
  GeglBuffer               *output;
  gboolean                  success = FALSE;

  klass = GEGL_OPERATION_FILTER_GET_CLASS (operation);

  g_assert (klass->process);

  if (strcmp (output_prop, "output"))
    {
      g_warning ("requested processing of %s pad on a filter", output_prop);
      return FALSE;
    }

  input  = gegl_operation_context_get_source (context, "input");
  output = gegl_operation_context_get_target (context, "output");

  if (gegl_operation_use_threading (operation, result))
  {
    gint threads = gegl_config_threads ();
    GThreadPool *pool = thread_pool ();
    ThreadData thread_data[GEGL_MAX_THREADS];
    gint pending = threads;

    if (o->direction == GEGL_WIND_DIRECTION_LEFT ||
        o->direction == GEGL_WIND_DIRECTION_RIGHT)
    {
      gint bit = result->height / threads;
      for (gint j = 0; j < threads; j++)
      {
        thread_data[j].roi.x = result->x;
        thread_data[j].roi.width = result->width;
        thread_data[j].roi.y = result->y + bit * j;
        thread_data[j].roi.height = bit;
      }
      thread_data[threads-1].roi.height = result->height - (bit * (threads-1));
    }
    else
    {
      gint bit = result->width / threads;
      for (gint j = 0; j < threads; j++)
      {
        thread_data[j].roi.y = result->y;
        thread_data[j].roi.height = result->height;
        thread_data[j].roi.x = result->x + bit * j;
        thread_data[j].roi.width = bit;
      }
      thread_data[threads-1].roi.width = result->width - (bit * (threads-1));
    }
    for (gint i = 0; i < threads; i++)
    {
      thread_data[i].klass = klass;
      thread_data[i].operation = operation;
      thread_data[i].input = input;
      thread_data[i].output = output;
      thread_data[i].pending = &pending;
      thread_data[i].level = level;
      thread_data[i].success = TRUE;
    }

    for (gint i = 1; i < threads; i++)
      g_thread_pool_push (pool, &thread_data[i], NULL);
    thread_process (&thread_data[0], NULL);

    while (g_atomic_int_get (&pending)) {};

    success = thread_data[0].success;
  }
  else
  {
    success = klass->process (operation, input, output, result, level);
  }

  if (input != NULL)
    g_object_unref (input);

  return success;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *format = gegl_operation_get_format (operation, "output");
  gint n_components = babl_format_get_n_components (format);

  gint           y, row_size;
  gint           row_start, row_end;
  GeglRectangle  row_rect;
  gfloat        *row_buf;
  GRand         *gr;
  gboolean       skip_rows;
  gboolean       need_reverse;
  gboolean       horizontal_effect;
  gint           last_pix;

  gr = g_rand_new ();

  horizontal_effect = (o->direction == GEGL_WIND_DIRECTION_LEFT ||
                       o->direction == GEGL_WIND_DIRECTION_RIGHT);

  need_reverse = (o->direction == GEGL_WIND_DIRECTION_RIGHT ||
                  o->direction == GEGL_WIND_DIRECTION_TOP);

  if (horizontal_effect)
    {
      row_size   = result->width * n_components;
      row_start  = result->y;
      row_end    = result->y + result->height;
      row_rect.x = result->x;
      row_rect.width  = result->width;
      row_rect.height = 1;
    }
  else
    {
      row_size  = result->height * n_components;
      row_start = result->x;
      row_end   = result->x + result->width;
      row_rect.y = result->y;
      row_rect.width  = 1;
      row_rect.height = result->height;
    }

  row_buf = g_new (gfloat, row_size);

  for (y = row_start; y < row_end; y++)
    {
      if (horizontal_effect)
        row_rect.y = y;
      else
        row_rect.x = y;

      gegl_buffer_get (input, &row_rect, 1.0, format, row_buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      if (need_reverse)
        reverse_buffer (row_buf, row_size, n_components);

      if (o->style == GEGL_WIND_STYLE_WIND)
        {
          last_pix  = row_size - (n_components * COMPARE_WIDTH);
          skip_rows = FALSE;
          render_wind_row (row_buf, n_components, last_pix, o, gr);
        }
      else
        {
          last_pix = row_size - n_components;
          skip_rows = render_blast_row (row_buf, n_components, last_pix, o, gr);
        }

      if (need_reverse)
        reverse_buffer (row_buf, row_size, n_components);

      gegl_buffer_set (output, &row_rect, level, format, row_buf,
                       GEGL_AUTO_ROWSTRIDE);

      if (skip_rows)
        {
          GeglRectangle rect   = row_rect;
          gint          n_rows = g_rand_int_range (gr, 1, 3);

          if (horizontal_effect)
            {
              rect.y      = y + 1;
              rect.height = n_rows;
            }
          else
            {
              rect.x     = y + 1;
              rect.width = n_rows;
            }

          gegl_buffer_copy (input, &rect, GEGL_ABYSS_CLAMP, output, &rect);
          y += n_rows;
        }
    }

  g_rand_free (gr);
  g_free (row_buf);

  return TRUE;
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process                    = process;
  operation_class->process                 = operation_process;
  operation_class->prepare                 = prepare;
  operation_class->get_cached_region       = get_cached_region;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->opencl_support          = FALSE;

  gegl_operation_class_set_keys (operation_class,
     "name",       "gegl:wind",
     "title",      _("Wind"),
     "categories", "distort",
     "license",    "GPL3+",
     "description", _("Wind-like bleed effect"),
     NULL);
}

#endif
