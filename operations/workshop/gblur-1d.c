/* This file is an image processing operation for GEGL
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
 * Copyright 2006 Dominik Ernst <dernst@gmx.de>
 * Copyright 2013 Massimo Valentini <mvalentini@src.gnome.org>
 *
 * Recursive Gauss IIR Filter as described by Young / van Vliet
 * in "Signal Processing 44 (1995) 139 - 151"
 *
 * NOTE: The IIR filter should not be used for radius < 0.5, since it
 *       becomes very inaccurate.
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES
#if 0
gegl_chant_register_enum (gegl_gblur_1d_policy)
   enum_value (GEGL_BLUR_1D_ABYSS_NONE, "None")
   enum_value (GEGL_BLUR_1D_ABYSS_CLAMP, "Clamp")
//   enum_value (GEGL_BLUR_1D_ABYSS_LOOP,
   enum_value (GEGL_BLUR_1D_ABYSS_BLACK, "Black")
   enum_value (GEGL_BLUR_1D_ABYSS_WHITE, "White")
gegl_chant_register_enum_end (GeglGblur1dPolicy)
#endif
gegl_chant_register_enum (gegl_gblur_1d_orientation)
  enum_value (GEGL_BLUR_1D_HORIZONTAL, "Horizontal")
  enum_value (GEGL_BLUR_1D_VERTICAL,   "Vertical")
gegl_chant_register_enum_end (GeglGblur1dOrientation)

gegl_chant_double_ui (std_dev, _("Size"),
                      0.5, 1500.0, 1.5, 0.5, 100.0, 3.0,
                      _("Standard deviation "
                        "(multiply by ~2 to get radius)"))
gegl_chant_enum      (orientation, _("Orientation"),
                      GeglGblur1dOrientation, gegl_gblur_1d_orientation,
                      GEGL_BLUR_1D_HORIZONTAL,
                      _("The orientation of the blur - hor/ver"))
gegl_chant_enum      (abyss_policy, _("Abyss policy"), GeglAbyssPolicy,
                      gegl_abyss_policy, GEGL_ABYSS_CLAMP, _(""))
#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "gblur-1d.c"

#include "gegl-chant.h"
#include <math.h>
#include <stdio.h>

static void
iir_young_find_constants (gfloat   sigma,
                          gdouble *b,
                          gdouble (*m)[3])
{
  const gdouble q = sigma >= 2.5 ?
                    0.98711 * sigma - 0.96330 :
                    3.97156 - 4.14554 * sqrt (1 - 0.26891 * sigma);

  const gdouble b0 = 1.57825 + q*(2.44413 + q*(1.4281  + q*0.422205));
  const gdouble b1 =           q*(2.44413 + q*(2.85619 + q*1.26661));
  const gdouble b2 =         - q*           q*(1.4281  + q*1.26661);
  const gdouble b3 =           q*           q*           q*0.422205;

  const double a1 = b1 / b0;
  const double a2 = b2 / b0;
  const double a3 = b3 / b0;
  const gdouble c = 1. / ((1+a1-a2+a3) * (1+a2+(a1-a3)*a3));

  m[0][0] = c * (-a3*(a1+a3)-a2 + 1);
  m[0][1] = c * (a3+a1)*(a2+a3*a1);
  m[0][2] = c * a3*(a1+a3*a2);

  m[1][0] = c * (a1+a3*a2);
  m[1][1] = c * (1-a2)*(a2+a3*a1);
  m[1][2] = c * a3*(1-a3*a1-a3*a3-a2);

  m[2][0] = c * (a3*a1+a2+a1*a1-a2*a2);
  m[2][1] = c * (a1*a2+a3*a2*a2-a1*a3*a3-a3*a3*a3-a3*a2+a3);
  m[2][2] = c * a3*(a1+a3*a2);

  b[0] = 1. - (b1 + b2 + b3) / b0;
  b[1] = a1;
  b[2] = a2;
  b[3] = a3;
}

static void
fix_right_boundary (gdouble        *buf,
                    gdouble       (*m)[3],
                    const gdouble   uplus)
{
  gdouble u[3] = { buf[-1] - uplus, buf[-2] - uplus, buf[-3] - uplus };
  gint    i, k;

  for (i = 0; i < 3; i++)
    {
      gdouble tmp = 0.;

      for (k = 0; k < 3; k++)
        tmp += m[i][k] * u[k];

      buf[i] = tmp + uplus;
    }
}

static inline void
iir_young_blur_1D (gfloat        *buf,
                   gdouble       *tmp,
                   const gdouble *b,
                   gdouble      (*m)[3],
                   const gint     len,
                   const gint     nc,
                   GeglAbyssPolicy policy)
{
  gfloat  white[4] = { 1, 1, 1, 1 };
  gfloat  black[4] = { 0, 0, 0, 1 };
  gfloat  none[4]  = { 0, };
  gfloat *uplus, *iminus;
  gint    i, j, c;

  switch (policy)
    {
    case GEGL_ABYSS_CLAMP: default:
      iminus = &buf[nc * 3]; uplus = &buf[nc * (len + 2)]; break;

    case GEGL_ABYSS_NONE:
      iminus = uplus = &none[0]; break;

    case GEGL_ABYSS_WHITE:
      iminus = uplus = &white[0]; break;

    case GEGL_ABYSS_BLACK:
      iminus = uplus = &black[nc == 2 ? 2 : 0]; break;
    }

  for (c = 0; c < nc; ++c)
    {
      for (i = 0; i < 3; ++i)
        tmp[i] = iminus[c];

      for (i = 3; i < 3 + len; ++i)
        {
          tmp[i] = buf[nc * i + c] * b[0];

          for (j = 1; j < 4; ++j)
            tmp[i] += b[j] * tmp[i - j];
        }

      fix_right_boundary (&tmp[3 + len], m, uplus[c]);

      for (i = 3 + len - 1; 3 <= i; --i)
        {
          gdouble tt = 0.;

          for (j = 0; j < 4; ++j)
            tt += b[j] * tmp[i + j];

          buf[nc * i + c] = tmp[i] = tt;
        }
    }
}

static void
iir_young_hor_blur (GeglBuffer          *src,
                    const GeglRectangle *rect,
                    GeglBuffer          *dst,
                    const gdouble       *b,
                    gdouble            (*m)[3],
                    GeglAbyssPolicy      policy,
                    const Babl          *format)
{
  GeglRectangle  cur_row = *rect;
  const gint     nc = babl_format_get_n_components (format);
  gfloat        *row = g_new (gfloat, (3 + rect->width + 3) * nc);
  gdouble       *tmp = g_new (gdouble, (3 + rect->width + 3));
  gint           v;

  cur_row.height = 1;

  for (v = 0; v < rect->height; v++)
    {
      cur_row.y = rect->y + v;

      gegl_buffer_get (src, &cur_row, 1.0, format, &row[3 * nc],
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      iir_young_blur_1D (row, tmp, b, m, rect->width, nc, policy);

      gegl_buffer_set (dst, &cur_row, 0, format, &row[3 * nc],
                       GEGL_AUTO_ROWSTRIDE);
    }

  g_free (tmp);
  g_free (row);
}

static void
iir_young_ver_blur (GeglBuffer          *src,
                    const GeglRectangle *rect,
                    GeglBuffer          *dst,
                    const gdouble       *b,
                    gdouble            (*m)[3],
                    GeglAbyssPolicy      policy,
                    const Babl          *format)
{
  GeglRectangle  cur_col = *rect;
  const gint     nc = babl_format_get_n_components (format);
  gfloat        *col = g_new (gfloat, (3 + rect->height + 3) * nc);
  gdouble       *tmp = g_new (gdouble, (3 + rect->height + 3));
  gint           i;

  cur_col.width = 1;

  for (i = 0; i < rect->width; i++)
    {
      cur_col.x = rect->x + i;

      gegl_buffer_get (src, &cur_col, 1.0, format, &col[3 * nc],
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      iir_young_blur_1D (col, tmp, b, m, rect->height, nc, policy);

      gegl_buffer_set (dst, &cur_col, 0, format, &col[3 * nc],
                       GEGL_AUTO_ROWSTRIDE);
    }

  g_free (tmp);
  g_free (col);
}

static void
gegl_gblur_1d_prepare (GeglOperation *operation)
{
  const Babl *src_format = gegl_operation_get_source_format (operation, "input");
  const char *format     = "RaGaBaA float";

  if (src_format)
    {
      const Babl *model = babl_format_get_model (src_format);

      if (model == babl_model ("RGB") || model == babl_model ("R'G'B'"))
        format = "RGB float";
      else if (model == babl_model ("Y") || model == babl_model ("Y'"))
        format = "Y float";
      else if (model == babl_model ("YA") || model == babl_model ("Y'A") ||
               model == babl_model ("YaA") || model == babl_model ("Y'aA"))
        format = "YaA float";
    }

  gegl_operation_set_format (operation, "output", babl_format (format));
}

static GeglRectangle
gegl_gblur_1d_get_required_for_output (GeglOperation       *operation,
                                       const gchar         *input_pad,
                                       const GeglRectangle *output_roi)
{
  GeglRectangle        required_for_output = { 0, };
  const GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, input_pad);

  if (in_rect && ! gegl_rectangle_is_infinite_plane (in_rect))
    {
      GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

      required_for_output = *output_roi;

      if (o->orientation == GEGL_BLUR_1D_HORIZONTAL)
        {
          required_for_output.x     = in_rect->x;
          required_for_output.width = in_rect->width;
        }
      else
        {
          required_for_output.y      = in_rect->y;
          required_for_output.height = in_rect->height;
        }
    }

  return required_for_output;
}

static GeglRectangle
gegl_gblur_1d_get_cached_region (GeglOperation       *operation,
                                 const GeglRectangle *output_roi)
{
  GeglRectangle        cached_region = { 0, };
  const GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation,
                                                                         "input");

  if (in_rect && ! gegl_rectangle_is_infinite_plane (in_rect))
    {
      GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

      cached_region = *output_roi;

      if (o->orientation == GEGL_BLUR_1D_HORIZONTAL)
        {
          cached_region.x     = in_rect->x;
          cached_region.width = in_rect->width;
        }
      else
        {
          cached_region.y      = in_rect->y;
          cached_region.height = in_rect->height;
        }
    }

  return cached_region;
}

static gboolean
gegl_gblur_1d_process (GeglOperation       *operation,
                       GeglBuffer          *input,
                       GeglBuffer          *output,
                       const GeglRectangle *result,
                       gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  const Babl *format = gegl_operation_get_format (operation, "output");
  gdouble b[4], m[3][3];

  iir_young_find_constants (o->std_dev, b, m);

  if (o->orientation == GEGL_BLUR_1D_HORIZONTAL)
    iir_young_hor_blur (input, result, output, b, m, o->abyss_policy, format);
  else
    iir_young_ver_blur (input, result, output, b, m, o->abyss_policy, format);

  return  TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process                    = gegl_gblur_1d_process;
  operation_class->prepare                 = gegl_gblur_1d_prepare;
  operation_class->get_required_for_output = gegl_gblur_1d_get_required_for_output;
  operation_class->get_cached_region       = gegl_gblur_1d_get_cached_region;
  gegl_operation_class_set_keys (operation_class,
    "name",       "gegl:gblur-1d",
    "categories", "hidden:blur",
    "description",
        _("Performs an averaging of neighboring pixels with the "
          "normal distribution as weighting"),
        NULL);
}

#endif
