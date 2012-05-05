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
 * Copyright 2008 Hans Petter Jansson <hpj@copyleft.no>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (red_bits,   _("Red bits"),   1, 16, 16, _("Number of bits for red channel"))
gegl_chant_int (green_bits, _("Green bits"), 1, 16, 16, _("Number of bits for green channel"))
gegl_chant_int (blue_bits,  _("Blue bits"),  1, 16, 16, _("Number of bits for blue channel"))
gegl_chant_int (alpha_bits, _("Alpha bits"), 1, 16, 16, _("Number of bits for alpha channel"))
gegl_chant_string (dither_type, _("Dither"), "none",
              _("Dithering strategy (none, random, random-covariant, bayer, floyd-steinberg)"))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "color-reduction.c"

#include "gegl-chant.h"

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA u16"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA u16"));
}

static void
generate_channel_masks (guint *channel_bits, guint *channel_mask)
{
  gint i;

  for (i = 0; i < 4; i++)
    channel_mask [i] = ~((1 << (16 - channel_bits [i])) - 1);
}

static guint
quantize_value (guint value, guint n_bits, guint mask)
{
  gint i;

  value &= mask;

  for (i = n_bits; i < 16; i += n_bits)
    value |= value >> i;

  return value;
}

static void
process_floyd_steinberg (GeglBuffer *input,
                         GeglBuffer *output,
                         const GeglRectangle *result,
                         guint *channel_bits)
{
  GeglRectangle        line_rect;
  guint16             *line_buf;
  gdouble             *error_buf [2];
  guint                channel_mask [4];
  gint                 y;

  line_rect.x      = result->x;
  line_rect.y      = result->y;
  line_rect.width  = result->width;
  line_rect.height = 1;

  line_buf      = g_new  (guint16, line_rect.width * 4);
  error_buf [0] = g_new0 (gdouble, line_rect.width * 4);
  error_buf [1] = g_new0 (gdouble, line_rect.width * 4);

  generate_channel_masks (channel_bits, channel_mask);

  for (y = 0; y < result->height; y++)
  {
    gdouble  *error_buf_swap;
    gint      step;
    gint      start_x;
    gint      end_x;
    gint      x;

    /* Serpentine scanning; reverse direction every row */

    if (y & 1)
    {
      start_x = result->width - 1;
      end_x   = -1;
      step    = -1;
    }
    else
    {
      start_x = 0;
      end_x   = result->width;
      step    = 1;
    }

    /* Pull input row */

    gegl_buffer_get (input, &line_rect, 1.0, babl_format ("RGBA u16"), line_buf,
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

    /* Process the row */

    for (x = start_x; x != end_x; x += step)
    {
      guint16  *pixel = &line_buf [x * 4];
      guint     ch;

      for (ch = 0; ch < 4; ch++)
      {
        gdouble value;
        gdouble value_clamped;
        gdouble quantized;
        gdouble qerror;

        value         = pixel [ch] + error_buf [0] [x * 4 + ch];
        value_clamped = CLAMP (value, 0.0, 65535.0);
        quantized     = quantize_value ((guint) (value_clamped + 0.5), channel_bits [ch], channel_mask [ch]);
        qerror        = value - quantized;

        pixel [ch] = (guint16) quantized;

        /* Distribute the error */

        error_buf [1] [x * 4 + ch] += qerror * 5.0 / 16.0;  /* Down */

        if (x + step >= 0 && x + step < result->width)
        {
          error_buf [0] [(x + step) * 4 + ch] += qerror * 6.0 / 16.0;  /* Ahead */
          error_buf [1] [(x + step) * 4 + ch] += qerror * 1.0 / 16.0;  /* Down, ahead */
        }

        if (x - step >= 0 && x - step < result->width)
        {
          error_buf [1] [(x - step) * 4 + ch] += qerror * 3.0 / 16.0;  /* Down, behind */
        }
      }
    }

    /* Swap error accumulation rows */

    error_buf_swap = error_buf [0];
    error_buf [0]  = error_buf [1];
    error_buf [1]  = error_buf_swap;

    /* Clear error buffer for next-plus-one line */

    memset (error_buf [1], 0, line_rect.width * 4 * sizeof (gdouble));

    /* Push output row */

    gegl_buffer_set (output, &line_rect, 0, babl_format ("RGBA u16"), line_buf, GEGL_AUTO_ROWSTRIDE);
    line_rect.y++;
  }

  g_free (line_buf);
  g_free (error_buf [0]);
  g_free (error_buf [1]);
}

static const gdouble bayer_matrix_8x8 [] =
{
   1, 49, 13, 61,  4, 52, 16, 64,
  33, 17, 45, 29, 36, 20, 48, 32,
   9, 57,  5, 53, 12, 60,  8, 56,
  41, 25, 37, 21, 44, 28, 40, 24,
   3, 51, 15, 63,  2, 50, 14, 62,
  35, 19, 47, 31, 34, 18, 46, 30,
  11, 59,  7, 55, 10, 58,  6, 54,
  43, 27, 39, 23, 42, 26, 38, 22
};

static void
process_bayer (GeglBuffer *input,
               GeglBuffer *output,
               const GeglRectangle *result,
               guint *channel_bits)
{
  GeglRectangle        line_rect;
  guint16             *line_buf;
  guint                channel_mask [4];
  guint                y;

  line_rect.x = result->x;
  line_rect.y = result->y;
  line_rect.width = result->width;
  line_rect.height = 1;

  line_buf = g_new (guint16, line_rect.width * 4);

  generate_channel_masks (channel_bits, channel_mask);

  for (y = 0; y < result->height; y++)
  {
    guint x;

    gegl_buffer_get (input, &line_rect, 1.0, babl_format ("RGBA u16"), line_buf,
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

    for (x = 0; x < result->width; x++)
    {
      guint16 *pixel = &line_buf [x * 4];
      guint    ch;

      for (ch = 0; ch < 4; ch++)
      {
        gdouble value;
        gdouble value_clamped;
        gdouble quantized;

        value         = pixel [ch] + ((bayer_matrix_8x8 [(y % 8) * 8 + (x % 8)] - 32) * 65536.0 / 65.0) / (1 << (channel_bits [ch] - 1));
        value_clamped = CLAMP (value, 0.0, 65535.0);
        quantized     = quantize_value ((guint) (value_clamped + 0.5), channel_bits [ch], channel_mask [ch]);

        pixel [ch] = (guint16) quantized;
      }
    }

    gegl_buffer_set (output, &line_rect, 0, babl_format ("RGBA u16"), line_buf, GEGL_AUTO_ROWSTRIDE);
    line_rect.y++;
  }

  g_free (line_buf);
}

static void
process_random_covariant (GeglBuffer *input,
                          GeglBuffer *output,
                          const GeglRectangle *result,
                          guint *channel_bits)
{
  GeglRectangle        line_rect;
  guint16             *line_buf;
  guint                channel_mask [4];
  guint                y;

  line_rect.x = result->x;
  line_rect.y = result->y;
  line_rect.width = result->width;
  line_rect.height = 1;

  line_buf = g_new (guint16, line_rect.width * 4);

  generate_channel_masks (channel_bits, channel_mask);

  for (y = 0; y < result->height; y++)
  {
    guint x;

    gegl_buffer_get (input, &line_rect, 1.0, babl_format ("RGBA u16"), line_buf,
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

    for (x = 0; x < result->width; x++)
    {
      guint16 *pixel = &line_buf [x * 4];
      guint    ch;
      gint     r = g_random_int_range (-65536, 65536);

      for (ch = 0; ch < 4; ch++)
      {
        gdouble value;
        gdouble value_clamped;
        gdouble quantized;

        value         = pixel [ch] + (r / (1 << channel_bits [ch]));
        value_clamped = CLAMP (value, 0.0, 65535.0);
        quantized     = quantize_value ((guint) (value_clamped + 0.5), channel_bits [ch], channel_mask [ch]);

        pixel [ch] = (guint16) quantized;
      }
    }

    gegl_buffer_set (output, &line_rect, 0, babl_format ("RGBA u16"), line_buf, GEGL_AUTO_ROWSTRIDE);
    line_rect.y++;
  }

  g_free (line_buf);
}

static void
process_random (GeglBuffer *input,
                GeglBuffer *output,
                const GeglRectangle *result,
                guint *channel_bits)
{
  GeglRectangle        line_rect;
  guint16             *line_buf;
  guint                channel_mask [4];
  guint                y;

  line_rect.x = result->x;
  line_rect.y = result->y;
  line_rect.width = result->width;
  line_rect.height = 1;

  line_buf = g_new (guint16, line_rect.width * 4);

  generate_channel_masks (channel_bits, channel_mask);

  for (y = 0; y < result->height; y++)
  {
    guint x;

    gegl_buffer_get (input, &line_rect, 1.0, babl_format ("RGBA u16"), line_buf,
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

    for (x = 0; x < result->width; x++)
    {
      guint16 *pixel = &line_buf [x * 4];
      guint    ch;

      for (ch = 0; ch < 4; ch++)
      {
        gdouble value;
        gdouble value_clamped;
        gdouble quantized;

        value         = pixel [ch] + (g_random_int_range (-65536, 65536) / (1 << channel_bits [ch]));
        value_clamped = CLAMP (value, 0.0, 65535.0);
        quantized     = quantize_value ((guint) (value_clamped + 0.5), channel_bits [ch], channel_mask [ch]);

        pixel [ch] = (guint16) quantized;
      }
    }

    gegl_buffer_set (output, &line_rect, 0, babl_format ("RGBA u16"), line_buf, GEGL_AUTO_ROWSTRIDE);
    line_rect.y++;
  }

  g_free (line_buf);
}

static void
process_no_dither (GeglBuffer *input,
                   GeglBuffer *output,
                   const GeglRectangle *result,
                   guint *channel_bits)
{
  GeglRectangle        line_rect;
  guint16             *line_buf;
  guint                channel_mask [4];
  guint                y;

  line_rect.x = result->x;
  line_rect.y = result->y;
  line_rect.width = result->width;
  line_rect.height = 1;

  line_buf = g_new (guint16, line_rect.width * 4);

  generate_channel_masks (channel_bits, channel_mask);

  for (y = 0; y < result->height; y++)
  {
    guint x;

    gegl_buffer_get (input, &line_rect, 1.0, babl_format ("RGBA u16"), line_buf,
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

    for (x = 0; x < result->width; x++)
    {
      guint16 *pixel = &line_buf [x * 4];
      guint    ch;

      for (ch = 0; ch < 4; ch++)
      {
        pixel [ch] = quantize_value (pixel [ch], channel_bits [ch], channel_mask [ch]);
      }
    }

    gegl_buffer_set (output, &line_rect, 0, babl_format ("RGBA u16"), line_buf, GEGL_AUTO_ROWSTRIDE);
    line_rect.y++;
  }

  g_free (line_buf);
}

static GeglRectangle
get_required_for_output (GeglOperation        *self,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (self, "input");
}

static GeglRectangle
get_cached_region (GeglOperation       *self,
                   const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (self, "input");
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  guint       channel_bits [4];

  channel_bits [0] = o->red_bits;
  channel_bits [1] = o->green_bits;
  channel_bits [2] = o->blue_bits;
  channel_bits [3] = o->alpha_bits;

  if (!o->dither_type)
    process_no_dither (input, output, result, channel_bits);
  else if (!strcasecmp (o->dither_type, "random"))
    process_random (input, output, result, channel_bits);
  else if (!strcasecmp (o->dither_type, "random-covariant"))
    process_random_covariant (input, output, result, channel_bits);
  else if (!strcasecmp (o->dither_type, "bayer"))
    process_bayer (input, output, result, channel_bits);
  else if (!strcasecmp (o->dither_type, "floyd-steinberg"))
    process_floyd_steinberg (input, output, result, channel_bits);
  else
    process_no_dither (input, output, result, channel_bits);

  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region = get_cached_region;
  filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name"        , "gegl:color-reduction",
    "categories"  , "misc",
    "description" ,
            _("Reduces the number of bits per channel (colors and alpha), with optional dithering"),
            NULL);
}

#endif
