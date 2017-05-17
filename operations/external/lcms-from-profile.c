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
 * Copyright 2013 Michael Henning <drawoc@darkrefraction.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_pointer (src_profile, _("Source Profile"),
                    _("The cmsHPROFILE corresponding to the icc profile for "
                      "the input data."))

/* These are positioned so their values match up with the LCMS enum */
enum_start (gegl_rendering_intent)
  enum_value (GEGL_RENDERING_INTENT_PERCEPTUAL,
              "perceptual",            N_("Perceptual"))
  enum_value (GEGL_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
              "relative-colorimetric", N_("Relative Colorimetric"))
  enum_value (GEGL_RENDERING_INTENT_SATURATION,
              "saturation",            N_("Saturation"))
  enum_value (GEGL_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC,
              "absolute-colorimetric", N_("Absolute Colorimetric"))
/* TODO: Add the K_ONLY and K_PLANE intents */
enum_end (GeglRenderingIntent)

property_enum (intent, _("Rendering intent"),
                 GeglRenderingIntent, gegl_rendering_intent,
                 GEGL_RENDERING_INTENT_PERCEPTUAL)
  description(_("The rendering intent to use in the conversion."))

property_boolean (black_point_compensation, _("Black point compensation"),
                  FALSE)
  description (_("Convert using black point compensation."))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME lcms_from_profile
#define GEGL_OP_C_SOURCE lcms-from-profile.c

#include "gegl-op.h"
#include <lcms2.h>

static void prepare (GeglOperation *operation)
{
  /* TODO: move input format detection code here */
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

/* This profile corresponds to babl_model ("RGB") */
static cmsHPROFILE
create_lcms_linear_rgb_profile (void)
{
  cmsHPROFILE ret;

  /* white point is D65 from the sRGB specs */
  cmsCIExyY whitepoint = {0.3127, 0.3290, 1.0};

  /* primaries are ITU‐R BT.709‐5 (xYY),
   * which are also the primaries from the sRGB specs,
   * modified to properly account for hexadecimal quantization
   * during the profile making process.
   */
  cmsCIExyYTRIPLE primaries = {
    /*R {0.6400, 0.3300, 1.0},*/
    /*G {0.3000, 0.6000, 1.0},*/
    /*B {0.1500, 0.0600, 1.0} */
    /*R*/ {0.639998686, 0.330010138, 1.0},
    /*G*/ {0.300003784, 0.600003357, 1.0},
    /*B*/ {0.150002046, 0.059997204, 1.0}
  };

  /* linear light */
  cmsToneCurve* linear[3];
  linear[0] = linear[1] = linear[2] = cmsBuildGamma (NULL, 1.0); 

  /* create the profile, cleanup, and return */
  ret = cmsCreateRGBProfile (&whitepoint, &primaries, linear);
  cmsFreeToneCurve (linear[0]);
  return ret;
}

static cmsUInt32Number
determine_lcms_format (const Babl *babl, cmsHPROFILE profile)
{
  cmsUInt32Number format = COLORSPACE_SH (PT_ANY);
  gint channels, bpc, alpha;
  const Babl *type;

  channels = cmsChannelsOf (cmsGetColorSpace (profile));
  alpha = babl_format_get_n_components (babl) - channels;
  bpc = babl_format_get_bytes_per_pixel (babl)
          / babl_format_get_n_components (babl);

  type = babl_format_get_type (babl, 0);
  if (type == babl_type ("half") ||
      type == babl_type ("float") ||
      type == babl_type ("double"))
    format |= FLOAT_SH (1);

  /* bpc == 8 overflows the bitfield otherwise */
  bpc &= 0x07;

  /*
   * This is needed so the alpha component lines up with RGBA float
   * for our memcpy hack later on.
   */
  if (alpha > 1 || (alpha && channels != 3))
    return 0;

  format |= EXTRA_SH (alpha) | CHANNELS_SH (channels) | BYTES_SH (bpc);
  return format;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  cmsHTRANSFORM transform;
  const Babl *in_format, *out_format;

  gboolean alpha;
  gint bpp;

  in_format = babl_format_n (babl_type ("float"),
                 babl_format_get_n_components (gegl_buffer_get_format (input)));

  bpp = babl_format_get_bytes_per_pixel (in_format);

  /* create the transformation */
  {
    cmsHPROFILE in_profile, out_profile;
    cmsUInt32Number format;

    in_profile = o->src_profile;

    format = determine_lcms_format (in_format, in_profile);

    if (format == 0)
      return FALSE;

    if (format & EXTRA_SH (1))
      alpha = TRUE;
    else
      alpha = FALSE;

    out_profile = create_lcms_linear_rgb_profile ();

    transform = cmsCreateTransform (in_profile, format,
                                    out_profile, alpha ? TYPE_RGBA_FLT : TYPE_RGB_FLT,
                                    o->intent, o->black_point_compensation ? cmsFLAGS_BLACKPOINTCOMPENSATION : 0);

    cmsCloseProfile (out_profile);
  }

  out_format = alpha ? babl_format ("RGBA float") : babl_format ("RGB float");

  /* iterate over the pixels */
  {
    GeglBufferIterator *gi;

    gi = gegl_buffer_iterator_new (input, result, 0, in_format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

    gegl_buffer_iterator_add (gi, output, result, 0, out_format,
                              GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

    while (gegl_buffer_iterator_next (gi))
      {
        if (alpha)
          memcpy (gi->data[1], gi->data[0], bpp * gi->length);

        cmsDoTransform (transform, gi->data[0], gi->data[1], gi->length);
      }
  }

  cmsDeleteTransform (transform);

  return TRUE;
}

static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglOperationClass  *operation_class;

  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

  /* If the profile is NULL, simply become a nop */
  if (!o->src_profile)
    {
      gpointer input = gegl_operation_context_get_object (context, "input");
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (input));
      return TRUE;
    }

  return operation_class->process (operation, context,
                                   output_prop, result, level);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  operation_class->process = operation_process;

  filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:lcms-from-profile",
    "title",       _("LCMS From Profile"),
    "categories",  "color",
    "description",
       _("Converts the input from an ICC color profile to "
         "a well defined babl format. The buffer's data will then be correctly managed by GEGL for further processing."),
    NULL);
}
#endif
