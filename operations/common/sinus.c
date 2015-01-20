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
 *
 * Exchange one color with the other (settable threshold to convert from
 * one color-shade to another...might do wonders on certain images, or be
 * totally useless on others).
 *
 * Copyright (C) 1997 Xavier Bouchoux
 *
 * GEGL port: Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

property_double (x_scale, _("X Scale"), 15.0000)
    description (_("Scale value for x axis"))
    value_range (0.0001, G_MAXDOUBLE)
    ui_range    (0.0001, 100.0000)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "x")

property_double (y_scale, _("Y Scale"), 15.0)
    description (_("Scale value for y axis"))
    value_range (0.0001, G_MAXDOUBLE)
    ui_range    (0.0001, 100.0000)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "y")

property_double (complexity, _("Complexity"), 1.0)
    description (_("Complexity factor"))
    value_range (0.0, 15.0)

property_seed (seed, _("Random seed"), rand)

property_boolean (tiling, _("Force tiling"), TRUE)
    description (_("If set, the pattern generated will tile"))

property_boolean (perturbation, _("Distorted"), TRUE)
    description (_("If set, the pattern will be a little more distorted"))

property_color  (color1, _("Color 1"), "yellow")

property_color  (color2, _("Color 2"), "blue")

enum_start (gegl_sinus_blend)
  enum_value (GEGL_SINUS_BLEND_LINEAR , "linear",   N_("Linear"))
  enum_value (GEGL_SINUS_BLEND_BILINEAR , "bilinear", N_("Bilinear"))
  enum_value (GEGL_SINUS_BLEND_SINUSOIDAL, "sinusoidal", N_("Sinusoidal"))
enum_end (GeglSinusBlend)

property_enum (blend_mode, _("Blend Mode"),
    GeglSinusBlend, gegl_sinus_blend,
    GEGL_SINUS_BLEND_SINUSOIDAL)

property_double (blend_power, _("Exponent"), 0.0)
    description (_("Power used to strech the blend"))
    value_range (-7.5, 7.5)

property_int    (width, _("Width"), 1024)
    description (_("Width of the generated buffer"))
    value_range (0, G_MAXINT)
    ui_range    (0, 4096)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "x")
    ui_meta     ("role", "output-extent")

property_int    (height, _("Height"), 768)
    description (_("Height of the generated buffer"))
    value_range (0, G_MAXINT)
    ui_range    (0, 4096)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "y")
    ui_meta     ("role", "output-extent")

#else

#define GEGL_OP_POINT_RENDER
#define GEGL_OP_C_SOURCE sinus.c

#include "gegl-op.h"

#define ROUND(x) ((int) ((x) + 0.5))

typedef struct
{
  gdouble c11, c12, c13, c21, c22, c23, c31, c32, c33;
  gdouble (*blend) (gdouble);
  gfloat  color[4];
  gfloat  dcolor[4];
} SParamsType;

static gdouble
linear (gdouble v)
{
  gdouble a = v - (gint) v;

  return (a < 0 ? 1.0 + a : a);
}

static gdouble
bilinear (gdouble v)
{
  double a = v - (int) v;

  a = (a < 0 ? 1.0 + a : a);
  return (a > 0.5 ? 2 - 2 * a : 2 * a);
}

static gdouble
cosinus (gdouble v)
{
  return 0.5 - 0.5 * sin ((v + 0.25) * G_PI * 2);
}

static void
prepare_coef (GeglProperties *o)
{
  SParamsType *params;
  GRand       *gr;
  gfloat       color[4];
  gdouble      scalex = o->x_scale;
  gdouble      scaley = o->y_scale;

  params = (SParamsType *) o->user_data;

  gr = g_rand_new_with_seed (o->seed);

  switch (o->blend_mode)
    {
    case GEGL_SINUS_BLEND_BILINEAR:
      params->blend = bilinear;
      break;
    case GEGL_SINUS_BLEND_SINUSOIDAL:
      params->blend = cosinus;
      break;
    case GEGL_SINUS_BLEND_LINEAR:
    default:
      params->blend = linear;
    }

  if (! o->perturbation)
    {
      /* Presumably the 0 * g_rand_int ()s are to pop random
       * values off the prng, I don't see why though. */
      params->c11= 0 * g_rand_int (gr);
      params->c12= g_rand_double_range (gr, -1, 1) * scaley;
      params->c13= g_rand_double_range (gr, 0, 2 * G_PI);
      params->c21= 0 * g_rand_int (gr);
      params->c22= g_rand_double_range (gr, -1, 1)  * scaley;
      params->c23= g_rand_double_range (gr, 0, 2 * G_PI);
      params->c31= g_rand_double_range (gr, -1, 1) * scalex;
      params->c32= 0 * g_rand_int (gr);
      params->c33= g_rand_double_range (gr, 0, 2 * G_PI);
    }
  else
    {
      params->c11= g_rand_double_range (gr, -1, 1) * scalex;
      params->c12= g_rand_double_range (gr, -1, 1) * scaley;
      params->c13= g_rand_double_range (gr, 0, 2 * G_PI);
      params->c21= g_rand_double_range (gr, -1, 1) * scalex;
      params->c22= g_rand_double_range (gr, -1, 1) * scaley;
      params->c23= g_rand_double_range (gr, 0, 2 * G_PI);
      params->c31= g_rand_double_range (gr, -1, 1) * scalex;
      params->c32= g_rand_double_range (gr, -1, 1) * scaley;
      params->c33= g_rand_double_range (gr, 0, 2 * G_PI);
    }

  if (o->tiling)
    {
      params->c11 = ROUND (params->c11 / (2 * G_PI)) * 2 * G_PI;
      params->c12 = ROUND (params->c12 / (2 * G_PI)) * 2 * G_PI;
      params->c21 = ROUND (params->c21 / (2 * G_PI)) * 2 * G_PI;
      params->c22 = ROUND (params->c22 / (2 * G_PI)) * 2 * G_PI;
      params->c31 = ROUND (params->c31 / (2 * G_PI)) * 2 * G_PI;
      params->c32 = ROUND (params->c32 / (2 * G_PI)) * 2 * G_PI;
    }

  gegl_color_get_pixel (o->color1, babl_format ("R'G'B'A float"),
                        &(params->color));

  gegl_color_get_pixel (o->color2, babl_format ("R'G'B'A float"),
                        &color);

  params->dcolor[0] = color[0] - params->color[0];
  params->dcolor[1] = color[1] - params->color[1];
  params->dcolor[2] = color[2] - params->color[2];
  params->dcolor[3] = color[3] - params->color[3];

  g_rand_free (gr);
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  if (o->user_data == NULL)
    o->user_data = g_slice_new0 (SParamsType);

  prepare_coef (o);

  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A float"));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  return gegl_rectangle_infinite_plane ();
}

static gboolean
process (GeglOperation       *operation,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  SParamsType    *p;
  gint     i, j;
  gdouble  x, y, grey;
  gdouble  pow_exp;
  gfloat   *dest = out_buf;

  p = (SParamsType *) o->user_data;

  pow_exp = exp (o->blend_power);

  for (j = roi->y; j < roi->y + roi->height; j++)
    {
      y = ((gdouble) j) / o->height;
      for (i = roi->x; i < roi->x + roi->width; i++)
        {
          gdouble c;

          x = ((gdouble) i) / o->width;

          c = 0.5 * sin(p->c31 * x + p->c32 * y + p->c33);

          grey = sin (p->c11 * x + p->c12 * y + p->c13) *
                   (0.5 + 0.5 * c) +
                   sin (p->c21 * x + p->c22 * y + p->c23) *
                   (0.5 - 0.5 * c);

          grey = pow (p->blend (o->complexity * (0.5 + 0.5 * grey)), pow_exp);

          dest[0] = p->color[0] + grey * p->dcolor[0];
          dest[1] = p->color[1] + grey * p->dcolor[1];
          dest[2] = p->color[2] + grey * p->dcolor[2];
          dest[3] = p->color[3] + grey * p->dcolor[3];

          dest += 4;
        }
    }

  return TRUE;
}

static void
finalize (GObject *object)
{
  GeglOperation  *op = (void*) object;
  GeglProperties *o  = GEGL_PROPERTIES (op);

  if (o->user_data)
    {
      g_slice_free (SParamsType, o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass                  *object_class;
  GeglOperationClass            *operation_class;
  GeglOperationPointRenderClass *point_render_class;

  object_class       = G_OBJECT_CLASS (klass);
  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_render_class = GEGL_OPERATION_POINT_RENDER_CLASS (klass);

  object_class->finalize = finalize;

  point_render_class->process = process;

  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;
  operation_class->opencl_support = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:sinus",
    "title",              _("Sinus"),
    "categories",         "render",
    "position-dependent", "true",
    "license",            "GPL3+",
    "description",        _("Generate complex sinusoidal textures"),
    NULL);
}

#endif
