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
 *
 * The porting to GEGL was done by Barak Itkin
 *
 *     Copyright (C) 2013 Barak Itkin <lightningismyname@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

property_boolean (preserve_luminosity, _("Preserve luminosity"), FALSE)

/* Red channel */
property_double (rr_gain, _("Red in Red channel"), 1.0)
  description(_("Set the red amount for the red channel"))
  value_range (-2.0, 2.0)

property_double (rg_gain, _("Green in Red channel"), 0.0)
  description(_("Set the green amount for the red channel"))
  value_range (-2.0, 2.0)

property_double (rb_gain, _("Blue in Red channel"), 0.0)
  description(_("Set the blue amount for the red channel"))
  value_range (-2.0, 2.0)

/* Green channel */
property_double (gr_gain, _("Red in Green channel"), 0.0)
  description(_("Set the red amount for the green channel"))
  value_range (-2.0, 2.0)

property_double (gg_gain, _("Green for Green channel"), 1.0)
  description(_("Set the green amount for the green channel"))
  value_range (-2.0, 2.0)

property_double (gb_gain, _("Blue in Green channel"), 0.0)
  description(_("Set the blue amount for the green channel"))
  value_range (-2.0, 2.0)

/* Blue channel */
property_double (br_gain, _("Red in Blue channel"), 0.0)
  description(_("Set the red amount for the blue channel"))
  value_range (-2.0, 2.0)

property_double (bg_gain, _("Green in Blue channel"), 0.0)
  description(_("Set the green amount for the blue channel"))
  value_range (-2.0, 2.0)

property_double (bb_gain, _("Blue in Blue channel"), 1.0)
  description(_("Set the blue amount for the blue channel"))
  value_range (-2.0, 2.0)


#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_C_SOURCE channel-mixer.c

#include "gegl-op.h"

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

  gboolean       preserve_luminosity;
  gboolean       has_alpha;
} CmParamsType;

static void prepare (GeglOperation *operation)
{
  const Babl *input_format = gegl_operation_get_source_format (operation, "input");
  GeglProperties *o = GEGL_PROPERTIES (operation);
  CmParamsType *mix;
  const Babl *format;

  if (o->user_data == NULL)
    o->user_data = g_slice_new0 (CmParamsType);

  mix = (CmParamsType*) o->user_data;

  mix->preserve_luminosity = o->preserve_luminosity;

  mix->red.red_gain     = o->rr_gain;
  mix->red.green_gain   = o->rg_gain;
  mix->red.blue_gain    = o->rb_gain;

  mix->green.red_gain   = o->gr_gain;
  mix->green.green_gain = o->gg_gain;
  mix->green.blue_gain  = o->gb_gain;

  mix->blue.red_gain    = o->br_gain;
  mix->blue.green_gain  = o->bg_gain;
  mix->blue.blue_gain   = o->bb_gain;

  if (input_format == NULL || babl_format_has_alpha (input_format))
    {
      mix->has_alpha = TRUE;
      format = babl_format ("RGBA float");
    }
  else
    {
      mix->has_alpha = FALSE;
      format = babl_format ("RGB float");
    }

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static void finalize (GObject *object)
{
  GeglOperation *op = (void*) object;
  GeglProperties *o = GEGL_PROPERTIES (op);

  if (o->user_data)
    {
      g_slice_free (CmParamsType, o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
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

  return (gfloat) c;
}

static inline void
cm_process_pixel (CmParamsType  *mix,
                  const gfloat  *s,
                  gfloat        *d,
                  const gdouble  red_norm,
                  const gdouble  green_norm,
                  const gdouble  blue_norm)
{

  d[0] = cm_mix_pixel (&mix->red,   s[0], s[1], s[2], red_norm);
  d[1] = cm_mix_pixel (&mix->green, s[0], s[1], s[2], green_norm);
  d[2] = cm_mix_pixel (&mix->blue,  s[0], s[1], s[2], blue_norm);
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties   *o = GEGL_PROPERTIES (op);
  CmParamsType *mix = (CmParamsType*) o->user_data;

  gdouble       red_norm, green_norm, blue_norm;
  gfloat       *in, *out;

  g_assert (mix != NULL);

  red_norm   = cm_calculate_norm (mix, &mix->red);
  green_norm = cm_calculate_norm (mix, &mix->green);
  blue_norm  = cm_calculate_norm (mix, &mix->blue);

  in = in_buf;
  out = out_buf;

  if (mix->has_alpha)
    {
      while (samples--)
        {
          cm_process_pixel (mix, in, out,
                            red_norm, green_norm, blue_norm);
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
                            red_norm, green_norm, blue_norm);
          in += 3;
          out += 3;
        }
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
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
      "name",       "gegl:channel-mixer",
      "categories", "color",
      "title",      _("Channel Mixer"),
      "license",    "GPL3+",
      "description", _("Remix colors; by defining relative contributions from source components."),
      NULL);
}

#endif

