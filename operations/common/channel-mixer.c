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
 * This operation is a port of the GIMP channel-mixer plugin:
 *
 *     Copyright (C) 2002 Martin Guldahl <mguldahl@xmission.com>
 *         Based on GTK code from:
 *         homomorphic (Copyright (C) 2001 Valter Marcus Hilden)
 *         rand-noted  (Copyright (C) 1998 Miles O'Neal)
 *         nlfilt      (Copyright (C) 1997 Eric L. Hernes)
 *         pagecurl    (Copyright (C) 1996 Federico Mena Quintero)
 *
 * The porting to GEGL was done by Barak Itkin
 *
 *     Copyright (C) 2013 Barak Itkin <lightningismyname@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_boolean (monochrome, _("Monochrome"),
                    FALSE,
                    _("Monochrome"))

gegl_chant_boolean (preserve_luminosity, _("Preserve Luminosity"),
                    FALSE,
                    _("Preserve Luminosity"))

gegl_chant_double (rr_gain, _("Red Red Gain"),
                   -2.0, +2.0, 1.0,
                   _("Set the red gain for the red channel"))

gegl_chant_double (rg_gain, _("Red Green Gain"),
                   -2.0, +2.0, 0.0,
                   _("Set the green gain for the red channel"))

gegl_chant_double (rb_gain, _("Red Blue Gain"),
                   -2.0, +2.0, 0.0,
                   _("Set the blue gain for the red channel"))

gegl_chant_double (gr_gain, _("Green Red Gain"),
                   -2.0, +2.0, 0.0,
                   _("Set the red gain for the green channel"))

gegl_chant_double (gg_gain, _("Green Green Gain"),
                   -2.0, +2.0, 1.0,
                   _("Set the green gain for the green channel"))

gegl_chant_double (gb_gain, _("Green Blue Gain"),
                   -2.0, +2.0, 0.0,
                   _("Set the blue gain for the green channel"))

gegl_chant_double (br_gain, _("Blue Red Gain"),
                   -2.0, +2.0, 0.0,
                   _("Set the red gain for the blue channel"))

gegl_chant_double (bg_gain, _("Blue Green Gain"),
                   -2.0, +2.0, 0.0,
                   _("Set the green gain for the blue channel"))

gegl_chant_double (bb_gain, _("Blue Blue Gain"),
                   -2.0, +2.0, 1.0,
                   _("Set the blue gain for the blue channel"))

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE       "channel-mixer.c"

#include "gegl-chant.h"

typedef enum
{
  CM_RED_CHANNEL,
  CM_GREEN_CHANNEL,
  CM_BLUE_CHANNEL
} CmModeType;

typedef struct
{
  gdouble       red_gain;
  gdouble       green_gain;
  gdouble       blue_gain;
} CmChannelType;

typedef struct
{
  CmChannelType  red;
  CmChannelType  green;
  CmChannelType  blue;
  CmChannelType  black;

  gboolean       monochrome;
  gboolean       preserve_luminosity;
  gboolean       has_alpha;
} CmParamsType;

static void prepare (GeglOperation *operation)
{
  const Babl *input_format = gegl_operation_get_source_format (operation, "input");
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  CmParamsType *mix;
  const Babl *format;

  if (o->chant_data == NULL)
    o->chant_data = g_slice_new (CmParamsType);

  mix = (CmParamsType*) o->chant_data;

  mix->monochrome          = o->monochrome;
  mix->preserve_luminosity = o->preserve_luminosity;

  if (mix->monochrome)
    {
      mix->black.red_gain   = o->rr_gain;
      mix->black.green_gain = o->rg_gain;
      mix->black.blue_gain  = o->rb_gain;
    }
  else
    {
      mix->red.red_gain     = o->rr_gain;
      mix->red.green_gain   = o->rg_gain;
      mix->red.blue_gain    = o->rb_gain;

      mix->green.red_gain   = o->gr_gain;
      mix->green.green_gain = o->gg_gain;
      mix->green.blue_gain  = o->gb_gain;

      mix->blue.red_gain    = o->br_gain;
      mix->blue.green_gain  = o->bg_gain;
      mix->blue.blue_gain   = o->bb_gain;
    }

  if (input_format == NULL || babl_format_has_alpha (input_format))
    {
      mix->has_alpha = TRUE;
      format = babl_format ("R'G'B'A float");
    }
  else
    {
      mix->has_alpha = FALSE;
      format = babl_format ("R'G'B' float");
    }

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static void finalize (GObject *object)
{
  GeglOperation *op = (void*) object;
  GeglChantO *o = GEGL_CHANT_PROPERTIES (op);

  if (o->chant_data)
    {
      g_slice_free (CmParamsType, o->chant_data);
      o->chant_data = NULL;
    }

  G_OBJECT_CLASS (gegl_chant_parent_class)->finalize (object);
}

static gdouble
cm_calculate_norm (CmParamsType  *mix,
                   CmChannelType *ch)
{
  gdouble sum = ch->red_gain + ch->green_gain + ch->blue_gain;

  if (sum == 0.0 || ! mix->preserve_luminosity)
    return 1.0;

  return fabs (1 / sum);
}

static inline gfloat
cm_mix_pixel (CmChannelType *ch,
              gfloat         r,
              gfloat         g,
              gfloat         b,
              gdouble        norm)
{
  gdouble c = ch->red_gain * r + ch->green_gain * g + ch->blue_gain * b;

  c *= norm;

  return (gfloat) MIN(1, MAX(0, c));
}

static inline void
cm_process_pixel (CmParamsType  *mix,
                  const gfloat  *s,
                  gfloat        *d,
                  const gdouble  red_norm,
                  const gdouble  green_norm,
                  const gdouble  blue_norm,
                  const gdouble  black_norm)
{
  if (mix->monochrome)
    {
      d[0] = d[1] = d[2] =
        cm_mix_pixel (&mix->black, s[0], s[1], s[2], black_norm);
    }
  else
    {
      d[0] = cm_mix_pixel (&mix->red,   s[0], s[1], s[2], red_norm);
      d[1] = cm_mix_pixel (&mix->green, s[0], s[1], s[2], green_norm);
      d[2] = cm_mix_pixel (&mix->blue,  s[0], s[1], s[2], blue_norm);
    }
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (op);
  CmParamsType *mix = (CmParamsType*) o->chant_data;

  gdouble       red_norm, green_norm, blue_norm, black_norm;
  gfloat       *in, *out;

  g_assert (mix != NULL);

  red_norm   = cm_calculate_norm (mix, &mix->red);
  green_norm = cm_calculate_norm (mix, &mix->green);
  blue_norm  = cm_calculate_norm (mix, &mix->blue);
  black_norm = cm_calculate_norm (mix, &mix->black);

  in = in_buf;
  out = out_buf;

  if (mix->has_alpha)
    {
      while (samples--)
        {
          cm_process_pixel (mix, in, out,
                            red_norm, green_norm, blue_norm,
                            black_norm);
          out[3] = in[3];

          in += 4;
          out += 4;
        }
    }
  else
    {
      while (samples--)
        {
          cm_process_pixel (mix, in, out,
                            red_norm, green_norm, blue_norm,
                            black_norm);
          in += 3;
          out += 3;
        }
    }

  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  point_filter_class->process = process;
  operation_class->prepare = prepare;
  G_OBJECT_CLASS (klass)->finalize = finalize;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
      "name"       , "gegl:channel-mixer",
      "categories" , "color",
      "description", _("Alter colors by mixing RGB Channels"),
      NULL);
}

#endif

