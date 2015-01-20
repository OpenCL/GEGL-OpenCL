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
 * Author: Eiichi Takamori <taka@ma1.seikyou.ne.jp>
 *
 * GEGL port: Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

property_double (center_x, _("Center X"), 0.5)
  description (_("X coordinates of the center of supernova"))
  ui_range (0.0, 1.0)
  ui_meta  ("unit", "relative-coordinate")
  ui_meta  ("axis", "x")

property_double (center_y, _("Center Y"), 0.5)
  description (_("Y coordinates of the center of supernova"))
  ui_range (0.0, 1.0)
  ui_meta  ("unit", "relative-coordinate")
  ui_meta  ("axis", "y")

property_int (radius, _("Radius"), 20)
  description (_("Radius of supernova"))
  ui_range (1.0, 100.0)
  value_range (1, 3000)

property_int (spokes_count, _("Number of spokes"), 100)
  description (_("Number of spokes"))
  ui_range (1, 1024)
  value_range (1, 1024)

property_int (random_hue, _("Random hue"), 0)
  description (_("Random hue"))
  ui_range (0, 360)
  value_range (0, 360)

property_color (color, _("Color"), "blue")
  description(_("The color of supernova."))

property_seed (seed, _("Random seed"), rand)
  description (_("The random seed for spokes and random hue"))

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_C_SOURCE supernova.c

#include "gegl-op.h"

typedef struct
{
  gdouble rand;
  gdouble color[4];
} SpokeType;

typedef struct
{
  gint       spokes_count;
  gint       seed;
  gint       random_hue;
  gdouble    color[4];
  SpokeType *spokes;
} SnParamsType;

static gdouble
gauss (GRand *gr)
{
  gdouble sum = 0.0;
  gint    i;

  for (i = 0; i < 6; i++)
    sum += g_rand_double (gr);

  return sum / 6.0;
}


static void
preprocess_spokes (GeglOperation *operation)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  const Babl     *format = babl_format ("R'G'B'A double");
  SnParamsType *params;
  GRand      *gr;
  GeglColor  *tmp;
  gdouble     color[4];
  gint        i;

  params = (SnParamsType *) o->user_data;

  gr = g_rand_new_with_seed (o->seed);

  gegl_color_get_pixel (o->color, babl_format ("HSVA double"), &color);

  for (i = 0; i < o->spokes_count; i++)
    {
      params->spokes[i].rand = gauss (gr);

      color[0] += ((gdouble) o->random_hue / 360.0) *
                    g_rand_double_range (gr, -0.5, 0.5);

      if (color[0] < 0)
        color[0] += 1.0;
      else if (color[0] >= 1.0)
        color[0] -= 1.0;

      tmp = gegl_color_duplicate (o->color);

      gegl_color_set_pixel (tmp, babl_format ("HSVA double"), &color);

      gegl_color_get_pixel (tmp, format, &(params->spokes[i].color));
    }

  /* store parameters used for this spokes calculation */
  params->spokes_count = o->spokes_count;
  params->seed = o->seed;
  params->random_hue = o->random_hue;
  gegl_color_get_pixel (o->color, format, &(params->color));

  g_rand_free (gr);
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  const Babl     *format = babl_format ("R'G'B'A double");
  SnParamsType   *params;
  gdouble        c[4];
  gboolean need_preprocess_spokes = FALSE;

  if (o->user_data == NULL)
    {
      o->user_data = g_slice_new0 (SnParamsType);
      params = (SnParamsType *) o->user_data;
      params->spokes = g_new0 (SpokeType, o->spokes_count);
      need_preprocess_spokes = TRUE;
    }
  else
    {
      /* Preprocessing is done only if spokes_count, seed, random_hue
       * or color parameters are changed.
       * Reallocation is done only when spokes_count is changed.
       *
       * This to avoid the systematic reallocation and preproccessing of
       * spokes at each prepare() call.
       */

      params = (SnParamsType *) o->user_data;

      if (params->spokes_count != o->spokes_count)
        {
          params->spokes = g_renew (SpokeType, params->spokes,
                                    o->spokes_count);

          need_preprocess_spokes = TRUE;
        }
      else
        {
          gegl_color_get_pixel (o->color, format, &c);

          if ((params->seed != o->seed) ||
              (params->random_hue != o->random_hue) ||
              (c[0] != params->color[0]) ||
              (c[1] != params->color[1]) ||
              (c[2] != params->color[2]) ||
              (c[3] != params->color[3]))
            {
              need_preprocess_spokes = TRUE;
            }
         }
    }

  if (need_preprocess_spokes)
    preprocess_spokes (operation);

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static void
finalize (GObject *object)
{
  GeglOperation *op = (void*) object;
  GeglProperties *o = GEGL_PROPERTIES (op);

  if (o->user_data)
    {
      SnParamsType *params;
      params = (SnParamsType *) o->user_data;
      g_free (params->spokes);
      g_slice_free (SnParamsType, o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle  *whole_region;
  gdouble *input  = in_buf;
  gdouble *output = out_buf;
  SnParamsType *params;
  gdouble color[3];
  gint x, y;
  gint real_x, real_y;
  gint idx;
  gdouble       u, v, l, w, w1, c, t;
  gdouble       nova_alpha, src_alpha, new_alpha = 0.0;
  gdouble       spokecol;
  gint          i, b;
  gdouble       compl_ratio, ratio;
  SpokeType *spokes;
  gdouble cx, cy;

  params = (SnParamsType *) o->user_data;
  g_assert (params != NULL);

  whole_region = gegl_operation_source_get_bounding_box (operation, "input");

  cx = o->center_x * whole_region->width;
  cy = o->center_y * whole_region->height;

  spokes = params->spokes;
  g_assert (spokes != NULL);

  for (y = 0 ; y < roi->height ; y++)
    {
      real_y = roi->y + y;
      for (x = 0 ; x < roi->width ; x++)
        {
          real_x = roi->x + x;
          idx = (x + y * roi->width) * 4;

          u = (gdouble) (real_x - cx) / o->radius;
          v = (gdouble) (real_y - cy) / o->radius;
          l = sqrt(u*u + v*v);

          t = (atan2 (u, v) / (2 * G_PI) + .51) * o->spokes_count;
          i = (gint) floor (t);
          t -= i;
          i %= o->spokes_count;

          w1 = spokes[i].rand * (1 - t) +
                 spokes[(i + 1) % o->spokes_count].rand * t;

          w1 = w1 * w1;

          w = 1.0 / (l + 0.001) * 0.9;

          nova_alpha = CLAMP (w, 0.0, 1.0);
          src_alpha = input[idx + 3];
          new_alpha = src_alpha + (1.0 - src_alpha) * nova_alpha;

          if (new_alpha != 0.0)
            ratio = nova_alpha / new_alpha;
          else
            ratio = 0.0;

          compl_ratio = 1.0 - ratio;

          for (b = 0 ; b < 3 ; b++)
            {
              spokecol = spokes[i].color[b] * (1.0-t) +
                         spokes[(i+1) % o->spokes_count].color[b] * t;

              if (w > 1.0)
                {
                  color[b] = CLAMP (spokecol * w, 0.0, 1.0);
                }
              else
                {
                  color[b] = input[idx + b] * compl_ratio +
                             spokecol * ratio;
                }

              c = CLAMP (w1 * w, 0.0, 1.0);
              color[b] += c;
              output[idx + b] = CLAMP (color[b], 0.0, 1.0);
            }

          output[idx + 3] = new_alpha;
        }
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass                  *object_class;
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *filter_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->finalize = finalize;

  operation_class->prepare = prepare;
  operation_class->opencl_support = FALSE;

  filter_class->process    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:supernova",
    "title",       _("Supernova"),
    "categories",  "light",
    "license",     "GPL3+",
    "description", _("This plug-in produces an effect like a supernova "
                     "burst. The amount of the light effect is "
                     "approximately in proportion to 1/r, where r is "
                     "the distance from the center of the star."),
    NULL);
}

#endif
