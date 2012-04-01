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
 * Copyright (C) 1996 Stephen Norris
 * Copyright (C) 2011 Robert Sasu (sasu.robert@gmail.com)
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

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (seed, _("Seed"), -1, G_MAXINT, -1,
                _("Random seed. Passing -1 implies that the seed is randomly chosen."))
gegl_chant_double (turbulence, _("Turbulence"), 0.1, 7.0, 1.0,
                   _("The value of the turbulence"))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE        "plasma.c"

#include "gegl-chant.h"
#include <math.h>

#define TILE_SIZE 128

typedef struct
{
  GeglBuffer *output;
  GRand      *gr;
  GeglChantO *o;
  float      *buffer;
  gboolean    using_buffer;
  gint        buffer_x;
  gint        buffer_y;
  gint        buffer_width;
} PlasmaContext;

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static void
average_pixel (gfloat *dst_buf,
               gfloat *src_buf1,
               gfloat *src_buf2)
{
  gint i;

  for (i = 0; i < 4; i++)
    *dst_buf++ = (*src_buf1++ + *src_buf2++) / 2;
}

static void
random_rgba (GRand  *gr,
             gfloat *dest)
{
  gint i;

  for (i = 0; i < 4; i++)
    dest[i] = (gfloat) g_rand_double_range (gr, 0, 1);
}

static void
add_random (GRand  *gr,
            gfloat *dest,
            gfloat  amount)
{
  gint    i;
  gfloat  tmp;

  amount /= 2;

  if (amount > 0)
    for (i = 0; i < 4; i++)
      {
        tmp = dest[i] + (gfloat) g_rand_double_range(gr, -amount, amount);
        dest[i] = CLAMP (tmp, 0, 1);
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

      gegl_buffer_set (context->output, &rect, 0, babl_format ("RGBA float"), pixel,
                       GEGL_AUTO_ROWSTRIDE);
      return;
    }
  else
    {
      float *ptr;

      ptr = context->buffer + ((y - context->buffer_y) * 4 * context->buffer_width) + ((x - context->buffer_x) * 4);

      *ptr++ = *pixel++;
      *ptr++ = *pixel++;
      *ptr++ = *pixel++;
      *ptr++ = *pixel++;
    }
}

static gboolean
do_plasma_big (PlasmaContext *context,
               gint           x1,
               gint           y1,
               gint           x2,
               gint           y2,
               gint           depth,
               gint           scale_depth)
{
  gfloat tl[4], ml[4], bl[4], mt[4], mm[4], mb[4], tr[4], mr[4], br[4];
  gfloat tmp[4];
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

      gegl_buffer_get (context->output, &rect, 1.0, babl_format ("RGBA float"),
                       context->buffer, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      context->using_buffer = TRUE;
      context->buffer_x = x1;
      context->buffer_y = y1;
      context->buffer_width = x2 - x1 + 1;

      ret = do_plasma_big (context, x1, y1, x2, y2, depth, scale_depth);

      context->using_buffer = FALSE;

      gegl_buffer_set (context->output, &rect, 0, babl_format ("RGBA float"),
                       context->buffer, GEGL_AUTO_ROWSTRIDE);

      return ret;
    }

  xm = (x1 + x2) / 2;
  ym = (y1 + y2) / 2;

  if (depth == -1)
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

  if (!depth)
    {
      if (x1 == x2 && y1 == y2)
        return FALSE;

      gegl_buffer_sample (context->output, x1, y1, NULL, tl, babl_format ("RGBA float"),
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
      gegl_buffer_sample (context->output, x1, y2, NULL, bl, babl_format ("RGBA float"),
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
      gegl_buffer_sample (context->output, x2, y1, NULL, tr, babl_format ("RGBA float"),
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
      gegl_buffer_sample (context->output, x2, y2, NULL, br, babl_format ("RGBA float"),
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

      ran = context->o->turbulence / (2.0 * scale_depth);

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
      do_plasma_big (context, x1, y1, xm, ym, depth - 1, scale_depth + 1);
      /* Bottom left. */
      do_plasma_big (context, x1, ym, xm, y2, depth - 1, scale_depth + 1);
      /* Top right. */
      do_plasma_big (context, xm, y1, x2, ym, depth - 1, scale_depth + 1);
      /* Bottom right. */
      return do_plasma_big (context, xm, ym, x2, y2, depth - 1, scale_depth + 1);
    }

  return TRUE;
}

static GeglRectangle
plasma_get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  gegl_rectangle_copy (&result, in_rect);

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  PlasmaContext *context;
  GeglRectangle boundary;
  gint   depth;
  gint   x, y;

  context = g_new (PlasmaContext, 1);
  context->o = GEGL_CHANT_PROPERTIES (operation);
  context->output = output;
  context->buffer = g_malloc (TILE_SIZE * TILE_SIZE * 4 * sizeof (float));
  context->using_buffer = FALSE;

  boundary = plasma_get_bounding_box (operation);

  /*
   * The first time only puts seed pixels (corners, center of edges,
   * center of image)
   */
  x = boundary.x + boundary.width;
  y = boundary.y + boundary.height;

  if (context->o->seed == -1)
    context->gr = g_rand_new ();
  else
    context->gr = g_rand_new_with_seed (context->o->seed);

  do_plasma_big (context, boundary.x, boundary.y, x-1, y-1, -1, 0);

  /*
   * Now we recurse through the images, going deeper each time
   */
  depth = 1;
  while (!do_plasma_big (context, boundary.x, boundary.y, x-1, y-1, depth, 0))
    depth++;

  gegl_buffer_sample_cleanup (context->output);
  g_free (context->buffer);
  g_free (context);

  return TRUE;
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (operation, "input");
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (operation, "input");
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process                    = process;
  operation_class->prepare                 = prepare;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region       = get_cached_region;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:plasma",
    "categories" , "render",
    "description", _("Performs plasma effect on the image"),
    NULL);
}

#endif
