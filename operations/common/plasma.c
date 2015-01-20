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
 * Copyright (C) 1996 Stephen Norris  (srn@flibble.cs.su.oz.au)
 * Copyright (C) 1996 Eiichi Takamori (taka@ma1.seikyou.ne.jp)
 * Copyright (C) 2000 Tim Copperfield (timecop@japan.co.jp)
 * Copyright (C) 2011 Robert Sasu     (sasu.robert@gmail.com)
 * Copyright (C) 2013 Téo Mazars      (teo.mazars@ensimag.fr)
 */

/*
 * This plug-in produces plasma fractal images. The algorithm is losely
 * based on a description of the fractint algorithm, but completely
 * re-implemented because the fractint code was too ugly to read :). It
 * was written by Stephen Norris for GIMP, and was ported to GEGL in
 * 2011 by Robert Sasu.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (turbulence, _("Turbulence"), 1.0)
    description (_("High values give more variation in details"))
    value_range (0.0, 7.0)

property_int   (x, _("X"), 0)
    description(_("X start of the generated buffer"))
    ui_range   (-4096, 4096)
    ui_meta    ("unit", "pixel-coordinate")
    ui_meta    ("axis", "x")
    ui_meta    ("role", "output-extent")

property_int   (y, _("Y"), 0)
    description(_("Y start of the generated buffer"))
    ui_range   (-4096, 4096)
    ui_meta    ("unit", "pixel-coordinate")
    ui_meta    ("axis", "y")
    ui_meta    ("role", "output-extent")

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

property_seed (seed, _("Random seed"), rand)

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_C_SOURCE plasma.c

#include "gegl-op.h"

#define TILE_SIZE 512

typedef struct
{
  GeglBuffer     *output;
  GRand          *gr;
  GeglProperties *o;
  float          *buffer;
  gboolean        using_buffer;
  gint            buffer_x;
  gint            buffer_y;
  gint            buffer_width;
} PlasmaContext;

static void
average_pixel (gfloat *dst_buf,
               gfloat *src_buf1,
               gfloat *src_buf2)
{
  gint i;

  for (i = 0; i < 3; i++)
    *dst_buf++ = (*src_buf1++ + *src_buf2++) / 2.0;
}

static void
random_rgba (GRand  *gr,
             gfloat *dest)
{
  gint i;

  for (i = 0; i < 3; i++)
    dest[i] = (gfloat) g_rand_double_range (gr, 0.0, 1.0);
}

static void
add_random (GRand  *gr,
            gfloat *dest,
            gfloat  amount)
{
  gint    i;
  gfloat  tmp;

  amount /= 2.0;

  if (amount > 0)
    for (i = 0; i < 3; i++)
      {
        tmp = dest[i] + (gfloat) g_rand_double_range (gr, -amount, amount);
        dest[i] = CLAMP (tmp, 0.0, 1.0);
      }
}

static void
put_pixel (PlasmaContext *context,
           gfloat        *pixel,
           gint           x,
           gint           y)
{
  if (G_UNLIKELY (!context->using_buffer))
    {
      GeglRectangle rect;

      rect.x = x;
      rect.y = y;
      rect.width = 1;
      rect.height = 1;

      gegl_buffer_set (context->output, &rect, 0, babl_format ("R'G'B' float"),
                       pixel, GEGL_AUTO_ROWSTRIDE);
      return;
    }
  else
    {
      float *ptr;
      gint index;

      index = ((y - context->buffer_y) * context->buffer_width +
               x - context->buffer_x);

      ptr = context->buffer + index * 3;

      *ptr++ = *pixel++;
      *ptr++ = *pixel++;
      *ptr++ = *pixel++;
    }
}

static gboolean
do_plasma (PlasmaContext *context,
           gint           x1,
           gint           y1,
           gint           x2,
           gint           y2,
           gint           plasma_depth,
           gint           recursion_depth,
           gint           level)
{
  gfloat tl[3], ml[3], bl[3], mt[3], mm[3], mb[3], tr[3], mr[3], br[3];
  gfloat tmp[3];
  gint    xm, ym;
  gfloat  ran;

  if (G_UNLIKELY ((!context->using_buffer) &&
                  ((x2 - x1 + 1) <= TILE_SIZE) &&
                  ((y2 - y1 + 1) <= TILE_SIZE)))
    {
      gboolean ret;
      GeglRectangle rect;

      rect.x = x1;
      rect.y = y1;
      rect.width = x2 - x1 + 1;
      rect.height = y2 - y1 + 1;

      gegl_buffer_get (context->output, &rect, 1.0, babl_format ("R'G'B' float"),
                       context->buffer, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      context->using_buffer = TRUE;
      context->buffer_x = x1;
      context->buffer_y = y1;
      context->buffer_width = x2 - x1 + 1;

      ret = do_plasma (context, x1, y1, x2, y2, plasma_depth, recursion_depth, level);

      context->using_buffer = FALSE;

      gegl_buffer_set (context->output, &rect, 0, babl_format ("R'G'B' float"),
                       context->buffer, GEGL_AUTO_ROWSTRIDE);

      return ret;
    }

  xm = (x1 + x2) / 2;
  ym = (y1 + y2) / 2;

  if (plasma_depth == -1)
    {
      random_rgba (context->gr, tl);
      put_pixel (context, tl, x1, y1);

      random_rgba (context->gr, tr);
      put_pixel (context, tr, x2, y1);

      random_rgba (context->gr, bl);
      put_pixel (context, bl, x1, y2);

      random_rgba (context->gr, br);
      put_pixel (context, br, x2, y2);

      random_rgba (context->gr, mm);
      put_pixel (context, mm, xm, ym);

      random_rgba (context->gr, ml);
      put_pixel (context, ml, x1, ym);

      random_rgba (context->gr, mr);
      put_pixel (context, mr, x2, ym);

      random_rgba (context->gr, mt);
      put_pixel (context, mt, xm, y1);

      random_rgba (context->gr, mb);
      put_pixel (context, mb, xm, y2);

      return FALSE;
    }

  if (!plasma_depth)
    {
      if (x1 == x2 && y1 == y2)
        return FALSE;

      gegl_buffer_sample_at_level (context->output, x1, y1, NULL, tl,
                          babl_format ("R'G'B' float"), level,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
      gegl_buffer_sample_at_level (context->output, x1, y2, NULL, bl,
                          babl_format ("R'G'B' float"), level,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
      gegl_buffer_sample_at_level (context->output, x2, y1, NULL, tr,
                          babl_format ("R'G'B' float"), level,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
      gegl_buffer_sample_at_level (context->output, x2, y2, NULL, br,
                          babl_format ("R'G'B' float"), level,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

      ran = context->o->turbulence / (2.0 * recursion_depth);

      if (xm != x1 || xm != x2)
        {
          /* Left. */
          average_pixel (ml, tl, bl);
          add_random (context->gr, ml, ran);
          put_pixel (context, ml, x1, ym);

          /* Right. */
          if (x1 != x2)
            {
              average_pixel (mr, tr, br);
              add_random (context->gr, mr, ran);
              put_pixel (context, mr, x2, ym);
            }
        }


      if (ym != y1 || ym != x2)
        {
          /* Bottom. */
          if (x1 != xm || ym != y2)
            {
              average_pixel (mb, bl, br);
              add_random (context->gr, mb, ran);
              put_pixel (context, mb, xm, y2);
            }

          if (y1 != y2)
            {
              /* Top. */
              average_pixel (mt, tl, tr);
              add_random (context->gr, mt, ran);
              put_pixel (context, mt, xm, y1);
            }
        }

      if (y1 != y2 || x1 != x2)
        {
          /* Middle pixel. */
          average_pixel (mm, tl, br);
          average_pixel (tmp, bl, tr);
          average_pixel (mm, mm, tmp);

          add_random (context->gr, mm, ran);
          put_pixel (context, mm, xm, ym);
        }

      return x2 - x1 < 3 && y2 - y1 < 3;
    }

  if (x1 < x2 || y1 < y2)
    {
      /* Top left. */
      do_plasma (context, x1, y1, xm, ym, plasma_depth - 1, recursion_depth + 1, level);
      /* Bottom left. */
      do_plasma (context, x1, ym, xm, y2, plasma_depth - 1, recursion_depth + 1, level);
      /* Top right. */
      do_plasma (context, xm, y1, x2, ym, plasma_depth - 1, recursion_depth + 1, level);
      /* Bottom right. */
      return do_plasma (context, xm, ym, x2, y2,
                        plasma_depth - 1, recursion_depth + 1, level);
    }

  return TRUE;
}

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B' float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  PlasmaContext *context;
  gint           depth;
  gint           x, y;

  context = g_new (PlasmaContext, 1);
  context->o = GEGL_PROPERTIES (operation);
  context->output = output;
  context->buffer = g_malloc (TILE_SIZE * TILE_SIZE * 3 * sizeof (gfloat));
  context->using_buffer = FALSE;

  /*
   * The first time only puts seed pixels (corners, center of edges,
   * center of image)
   */
  x = result->x + result->width;
  y = result->y + result->height;

  context->gr = g_rand_new_with_seed (context->o->seed);

  do_plasma (context, result->x, result->y, x-1, y-1, -1, 0, level);

  /*
   * Now we recurse through the images, going deeper each time
   */
  depth = 1;
  while (!do_plasma (context, result->x, result->y, x-1, y-1, depth, 0, level))
    depth++;

  gegl_buffer_sample_cleanup (context->output);
  g_rand_free (context->gr);
  g_free (context->buffer);
  g_free (context);

  return TRUE;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  return *GEGL_RECTANGLE (o->x, o->y, o->width, o->height);
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process                    = process;
  operation_class->prepare                 = prepare;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_bounding_box        = get_bounding_box;
  operation_class->get_cached_region       = get_cached_region;

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:plasma",
    "title",              _("Plasma"),
    "categories",         "render",
    "position-dependent", "true",
    "license",            "GPL3+",
    "description", _("Creates an image filled with a plasma effect."),
    NULL);
}

#endif
