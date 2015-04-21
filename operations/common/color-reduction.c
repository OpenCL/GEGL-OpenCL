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
 *           2012 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

enum_start (gegl_dither_strategy)
  enum_value (GEGL_DITHER_NONE,             "none",             N_("None"))
  enum_value (GEGL_DITHER_RANDOM,           "random",           N_("Random"))
  enum_value (GEGL_DITHER_RESILIENT,        "resilient",        N_("Resilient"))
  enum_value (GEGL_DITHER_RANDOM_COVARIANT, "random-covariant", N_("Random Covariant"))
  enum_value (GEGL_DITHER_BAYER,            "bayer",            N_("Bayer"))
  enum_value (GEGL_DITHER_FLOYD_STEINBERG,  "floyd-steinberg",  N_("Floyd-Steinberg"))
enum_end (GeglDitherStrategy)

property_int  (red_bits, _("Red bits"), 8)
    description(_("Number of bits for red channel"))
    value_range (1, 16)

property_int  (green_bits, _("Green bits"), 8)
    description(_("Number of bits for green channel"))
    value_range (1, 16)

property_int  (blue_bits, _("Blue bits"), 8)
    description(_("Number of bits for blue channel"))
    value_range (1, 16)

property_int  (alpha_bits, _("Alpha bits"), 8)
    description(_("Number of bits for alpha channel"))
    value_range (1, 16)

property_enum (dither_strategy, _("Dithering strategy"),
               GeglDitherStrategy, gegl_dither_strategy, GEGL_DITHER_RESILIENT)
    description (_("The dithering strategy to use"))

property_seed (seed, _("Random seed"), rand)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_C_SOURCE color-reduction.c

#include "gegl-op.h"

#define REDUCE_16B(value) (((value) & ((1 << 17) - 1)) - 65536)

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("R'G'B'A u16"));
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A u16"));
}

static void
generate_channel_masks (guint *channel_bits,
                        guint *channel_mask)
{
  gint i;

  for (i = 0; i < 4; i++)
    channel_mask [i] = ~((1 << (16 - channel_bits [i])) - 1);
}

static guint
quantize_value (guint value,
                guint n_bits,
                guint mask)
{
  gint i;

  value &= mask;

  for (i = n_bits; i < 16; i += n_bits)
    value |= value >> i;

  return value;
}

static void
process_floyd_steinberg (GeglBuffer          *input,
                         GeglBuffer          *output,
                         const GeglRectangle *result,
                         guint               *channel_bits)
{
  GeglRectangle  line_rect;
  guint16       *line_buf;
  gdouble       *error_buf [2];
  guint          channel_mask [4];
  gint           y;

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

      gegl_buffer_get (input, &line_rect, 1.0, babl_format ("R'G'B'A u16"), line_buf,
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

      gegl_buffer_set (output, &line_rect, 0, babl_format ("R'G'B'A u16"), line_buf, GEGL_AUTO_ROWSTRIDE);
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
process_row_bayer (GeglBufferIterator *gi,
                   guint               channel_mask [4],
                   guint               channel_bits [4],
                   gint                y)
{
  guint16 *data_in  = (guint16*) gi->data [0];
  guint16 *data_out = (guint16*) gi->data [1];
  guint x;
  for (x = 0; x < gi->roi->width; x++)
    {
      guint pixel = 4 * (gi->roi->width * y + x);
      guint ch;

      for (ch = 0; ch < 4; ch++)
        {
          gdouble bayer;
          gdouble value;
          gdouble value_clamped;
          gdouble quantized;

          bayer         = bayer_matrix_8x8 [((gi->roi->y + y) % 8) * 8 + ((gi->roi->x + x) % 8)];
          bayer         = ((bayer - 32) * 65536.0 / 65.0) / (1 << (channel_bits [ch] - 1));
          value         = data_in [pixel + ch] + bayer;
          value_clamped = CLAMP (value, 0.0, 65535.0);
          quantized     = quantize_value ((guint) (value_clamped + 0.5),
                                          channel_bits [ch],
                                          channel_mask [ch]);

          data_out [pixel + ch] = (guint16) quantized;
        }
    }
}

static void
process_row_random_covariant (GeglBufferIterator *gi,
                              guint               channel_mask [4],
                              guint               channel_bits [4],
                              gint                y,
                              GeglRandom         *rand)
{
  guint16 *data_in  = (guint16*) gi->data [0];
  guint16 *data_out = (guint16*) gi->data [1];
  guint x;
  for (x = 0; x < gi->roi->width; x++)
    {
      guint pixel = 4 * (gi->roi->width * y + x);
      guint ch;
      gint  r = REDUCE_16B (gegl_random_int (rand, gi->roi->x + x,
                                             gi->roi->y + y, 0, 0));
      for (ch = 0; ch < 4; ch++)
        {
          gdouble value;
          gdouble value_clamped;
          gdouble quantized;

          value         = data_in [pixel + ch] + (r / (1 << channel_bits [ch]));
          value_clamped = CLAMP (value, 0.0, 65535.0);
          quantized     = quantize_value ((guint) (value_clamped + 0.5),
                                          channel_bits [ch],
                                          channel_mask [ch]);

          data_out [pixel + ch] = (guint16) quantized;
        }
    }
}

static void
process_row_random (GeglBufferIterator *gi,
                    guint               channel_mask [4],
                    guint               channel_bits [4],
                    gint                y,
                    GeglRandom         *rand)
{
  guint16 *data_in  = (guint16*) gi->data [0];
  guint16 *data_out = (guint16*) gi->data [1];
  guint x;
  for (x = 0; x < gi->roi->width; x++)
    {
      guint pixel = 4 * (gi->roi->width * y + x);
      guint ch;
      for (ch = 0; ch < 4; ch++)
        {
          gdouble value;
          gdouble value_clamped;
          gdouble quantized;
          gint    r = REDUCE_16B (gegl_random_int (rand, gi->roi->x + x,
                                                   gi->roi->y + y, 0, ch));

          value         = data_in [pixel + ch] + (r / (1 << channel_bits [ch]));
          value_clamped = CLAMP (value, 0.0, 65535.0);
          quantized     = quantize_value ((guint) (value_clamped + 0.5),
                                          channel_bits [ch],
                                          channel_mask [ch]);

          data_out [pixel + ch] = (guint16) quantized;
        }
    }
}

static void
process_row_resilient (GeglBufferIterator *gi,
                       guint               channel_mask [4],
                       guint               channel_bits [4],
                       gint                y,
                       GeglRandom         *rand)
{
  guint16 *data_in  = (guint16*) gi->data [0];
  guint16 *data_out = (guint16*) gi->data [1];
  guint    x;
  for (x = 0; x < gi->roi->width; x++)
    {
      guint pixel = 4 * (gi->roi->width * y + x);
      guint ch;
      for (ch = 0; ch < 4; ch++)
        {
          gdouble value;
          gdouble value_clamped;
          gdouble quantized;
          gint    r = REDUCE_16B (gegl_random_int (rand, gi->roi->x + x,
                                                   gi->roi->y + y, 0, ch));
          value         = data_in [pixel + ch];
          value         = value + ((65535.0 / (8 * value + 48 * 65535)) + 1.2) *
                                  (r / (1 << channel_bits [ch]));
          value_clamped = CLAMP (value, 0.0, 65535.0);
          quantized     = quantize_value ((guint) (value_clamped + 0.5),
                                          channel_bits [ch],
                                          channel_mask [ch]);

          data_out [pixel + ch] = (guint16) quantized;
        }
    }
}

static void
process_row_no_dither (GeglBufferIterator *gi,
                       guint               channel_mask [4],
                       guint               channel_bits [4],
                       guint               y)
{
  guint16 *data_in  = (guint16*) gi->data [0];
  guint16 *data_out = (guint16*) gi->data [1];
  guint x;
  for (x = 0; x < gi->roi->width; x++)
    {
      guint pixel = 4 * (gi->roi->width * y + x);
      guint ch;
      for (ch = 0; ch < 4; ch++)
        {
          data_out [pixel + ch] = (guint16) quantize_value (data_in [pixel + ch],
                                                            channel_bits [ch],
                                                            channel_mask [ch]);
        }
    }
}

static void
process_standard (GeglBuffer          *input,
                  GeglBuffer          *output,
                  const GeglRectangle *result,
                  guint               *channel_bits,
                  GeglRandom          *rand,
                  GeglDitherStrategy   dither_strategy)
{
  GeglBufferIterator *gi;
  guint               channel_mask [4];

  generate_channel_masks (channel_bits, channel_mask);

  gi = gegl_buffer_iterator_new (input, result, 0, babl_format ("R'G'B'A u16"),
                                 GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (gi, output, result, 0, babl_format ("R'G'B'A u16"),
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (gi))
    {
      guint    y;
      for (y = 0; y < gi->roi->height; y++)
        {
          switch (dither_strategy)
            {
            case GEGL_DITHER_NONE:
              process_row_no_dither (gi, channel_mask, channel_bits, y);
              break;
            case GEGL_DITHER_RANDOM:
              process_row_random (gi, channel_mask, channel_bits, y, rand);
              break;
            case GEGL_DITHER_RESILIENT:
              process_row_resilient (gi, channel_mask, channel_bits, y, rand);
              break;
            case GEGL_DITHER_RANDOM_COVARIANT:
              process_row_random_covariant (gi, channel_mask, channel_bits, y, rand);
              break;
            case GEGL_DITHER_BAYER:
              process_row_bayer (gi, channel_mask, channel_bits, y);
              break;
            case GEGL_DITHER_FLOYD_STEINBERG:
              /* Done separately */
              break;
            default:
              process_row_no_dither (gi, channel_mask, channel_bits, y);
            }
        }
    }
}

static GeglRectangle
get_required_for_output (GeglOperation       *self,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglProperties *o = GEGL_PROPERTIES (self);

  if (o->dither_strategy == GEGL_DITHER_FLOYD_STEINBERG)
    return *gegl_operation_source_get_bounding_box (self, "input");
  else
    return *roi;
}

static GeglRectangle
get_cached_region (GeglOperation       *self,
                   const GeglRectangle *roi)
{
  GeglProperties *o = GEGL_PROPERTIES (self);

  if (o->dither_strategy == GEGL_DITHER_FLOYD_STEINBERG)
    return *gegl_operation_source_get_bounding_box (self, "input");
  else
    return *roi;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  guint       channel_bits [4];

  channel_bits [0] = o->red_bits;
  channel_bits [1] = o->green_bits;
  channel_bits [2] = o->blue_bits;
  channel_bits [3] = o->alpha_bits;

  if (o->dither_strategy != GEGL_DITHER_FLOYD_STEINBERG)
    process_standard (input, output, result, channel_bits,
                      o->rand, o->dither_strategy);
  else
    process_floyd_steinberg (input, output, result, channel_bits);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;
  gchar                    *composition = "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "<node operation='gegl:color-reduction'>"
    "  <params>"
    "    <param name='red-bits'>2</param>"
    "    <param name='green-bits'>2</param>"
    "    <param name='blue-bits'>2</param>"
    "    <param name='alpha-bits'>2</param>"
    "    <param name='dither-strategy'>floyd-steinberg</param>"
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

  operation_class->prepare = prepare;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region = get_cached_region;
  filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:color-reduction",
    "title",       _("Color Reduction"),
    "categories",  "dither",
    "description", _("Reduce the number of colors in the image, by reducing "
                     "the bits per channel (colors and alpha). Different dithering methods "
                     "can be specified to counteract quantization induced banding."),
    "reference-composition", composition,
    NULL);
}

#endif
