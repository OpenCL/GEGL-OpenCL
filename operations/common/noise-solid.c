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
 * This operation generates solid noise textures based on the
 * `Noise' and `Turbulence' functions described in the paper
 *
 *    Perlin, K, and Hoffert, E. M., "Hypertexture",
 *    Computer Graphics 23, 3 (August 1989)
 *
 * The algorithm implemented here also makes possible the
 * creation of seamless tiles.
 *
 * Copyright (C) 1997, 1998 Marcelo de Gomensoro Malheiros
 *
 * GEGL port: Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

property_double (x_size, _("X Size"), 4.0)
    description (_("Horizontal texture size"))
    value_range (0.1, 16.0)
    ui_range    (0.1, 16.0)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "x")

property_double (y_size, _("Y Size"), 4.0)
    description (_("Vertical texture size"))
    value_range (0.1, 16.0)
    ui_range    (0.1, 16.0)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "y")

property_int    (detail, _("Detail"), 1)
    description (_("Detail level"))
    ui_range    (0, 15)
    value_range (0, 15)

property_boolean (tilable, _("Tilable"), FALSE)
    description  (_("Create a tilable output"))

property_boolean (turbulent, _("Turbulent"), FALSE)
    description  (_("Make a turbulent noise"))

property_seed (seed, _("Random seed"), rand)

property_int    (width, _("Width"), 1024)
    description (_("Width of the generated buffer"))
    value_range (0, G_MAXINT)
    ui_range    (0, 4096)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "x")
    ui_meta     ("role", "output-extent")

property_int (height, _("Height"), 768)
    description(_("Height of the generated buffer"))
    value_range (0, G_MAXINT)
    ui_range    (0, 4096)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "y")
    ui_meta     ("role", "output-extent")

#else

#define GEGL_OP_POINT_RENDER
#define GEGL_OP_C_SOURCE noise-solid.c

#include "gegl-op.h"

#define TABLE_SIZE  64
#define WEIGHT(T)   ((2.0 * fabs (T) - 3.0) * (T) * (T) + 1.0)

typedef struct
{
  gdouble x;
  gdouble y;
} Vector2;

typedef struct
{
  gint         xclip;
  gint         yclip;
  gdouble      offset;
  gdouble      factor;
  gdouble      xsize;
  gdouble      ysize;
  gint         perm_tab[TABLE_SIZE];
  Vector2      grad_tab[TABLE_SIZE];
} NsParamsType;

static void
solid_noise_init (GeglProperties *o)
{
  gint          i, j, k, t;
  gdouble       m;
  GRand        *gr;
  NsParamsType *params;

  params = (NsParamsType *) o->user_data;

  g_assert (params != NULL);

  gr = g_rand_new_with_seed (o->seed);

  /*  Set scaling factors  */
  if (o->tilable)
    {
      params->xsize = ceil (o->x_size);
      params->ysize = ceil (o->y_size);
      params->xclip = (gint) params->xsize;
      params->yclip = (gint) params->ysize;
    }
  else
    {
      params->xsize = o->x_size;
      params->ysize = o->y_size;
    }

  /*  Set totally empiric normalization values  */
  if (o->turbulent)
    {
      params->offset = 0.0;
      params->factor = 1.0;
    }
  else
    {
      params->offset = 0.94;
      params->factor = 0.526;
    }

  /*  Initialize the permutation table  */
  for (i = 0; i < TABLE_SIZE; i++)
    params->perm_tab[i] = i;

  for (i = 0; i < (TABLE_SIZE >> 1); i++)
    {
      j = g_rand_int_range (gr, 0, TABLE_SIZE);
      k = g_rand_int_range (gr, 0, TABLE_SIZE);
      t = params->perm_tab[j];
      params->perm_tab[j] = params->perm_tab[k];
      params->perm_tab[k] = t;
    }

  /*  Initialize the gradient table  */
  for (i = 0; i < TABLE_SIZE; i++)
    {
      do
        {
          params->grad_tab[i].x = g_rand_double_range (gr, -1, 1);
          params->grad_tab[i].y = g_rand_double_range (gr, -1, 1);
          m = params->grad_tab[i].x * params->grad_tab[i].x +
                   params->grad_tab[i].y * params->grad_tab[i].y;
        }
      while (m == 0.0 || m > 1.0);

      m = 1.0 / sqrt(m);
      params->grad_tab[i].x *= m;
      params->grad_tab[i].y *= m;
    }

  g_rand_free (gr);
}

static gdouble
plain_noise (gdouble         x,
             gdouble         y,
             guint           s,
             GeglProperties *o)
{
  Vector2       v;
  gint          a, b, i, j, n;
  gdouble       sum;
  NsParamsType *p;

  p = (NsParamsType *) o->user_data;

  sum = 0.0;
  x *= s;
  y *= s;
  a = (gint) floor (x);
  b = (gint) floor (y);

  for (i = 0; i < 2; i++)
    {
      for (j = 0; j < 2; j++)
        {
          if (o->tilable)
            n = p->perm_tab[(((a + i) % (p->xclip * s)) + p->perm_tab[((b + j) % (p->yclip * s)) % TABLE_SIZE]) % TABLE_SIZE];
          else
            n = p->perm_tab[(a + i + p->perm_tab[(b + j) % TABLE_SIZE]) % TABLE_SIZE];
          v.x = x - a - i;
          v.y = y - b - j;
          sum += WEIGHT(v.x) * WEIGHT(v.y) * (p->grad_tab[n].x * v.x + p->grad_tab[n].y * v.y);
        }
    }

  return sum / s;
}

static gdouble
noise (gdouble         x,
       gdouble         y,
       GeglProperties *o)
{
  gint          i;
  guint         s;
  gdouble       sum;
  NsParamsType *p;

  p = (NsParamsType *) o->user_data;

  s = 1;
  sum = 0.0;
  x *= p->xsize;
  y *= p->ysize;

  for (i = 0; i <= o->detail; i++)
    {
      if (o->turbulent)
        sum += fabs (plain_noise (x, y, s, o));
      else
        sum += plain_noise (x, y, s, o);
      s <<= 1;
    }

  return (sum + p->offset) * p->factor;
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl     *format = babl_format ("Y' float");

  if (o->user_data == NULL)
    o->user_data = g_slice_new0 (NsParamsType);

  solid_noise_init (o);

  gegl_operation_set_format (operation, "output", format);
}

static gboolean
c_process (GeglOperation       *operation,
           void                *out_buf,
           glong                n_pixels,
           const GeglRectangle *roi,
           gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gint      x, y;
  gfloat    val;
  gfloat   *output = out_buf;

  for (y = roi->y; y < (roi->y + roi->height); y++)
    {
      for (x = roi->x; x < (roi->x + roi->width); x++)
        {
          val = noise (((gdouble) x / o->width), ((gdouble) y / o->height), o);
          *output = val;
          output++;
        }
    }

  return TRUE;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *out_buf,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglBufferIterator *iter;
  const Babl         *format = gegl_operation_get_format (operation,
                                                          "output");

  iter = gegl_buffer_iterator_new (out_buf, roi, level, format,
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    c_process (operation, iter->data[0], iter->length, &iter->roi[0], level);

  return  TRUE;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  return gegl_rectangle_infinite_plane ();
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;
  operation_class->opencl_support = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:noise-solid",
    "title",              _("Solid Noise"),
    "categories",         "render",
    "position-dependent", "true",
    "license",            "GPL3+",
    "description", _("Create a random cloud-like texture"),
    NULL);
}

#endif
