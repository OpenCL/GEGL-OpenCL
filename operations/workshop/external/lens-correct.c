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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 * Copyright 2008 Bradley Broom <bmbroom@gmail.com>
 * Copyright 2011 Robert Sasu <sasu.robert@gmail.com>
 */

#include "config.h"
#include <lensfun.h>
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_string (maker, _("Maker:"),"none",
                   _("Write lens maker correctly"))
gegl_chant_string (Camera, _("Camera:"),"none",
                   _("Write camera name correctly"))
gegl_chant_string (Lens, _("Lens:"),"none",
                   _("Write your lens model with majuscules"))
gegl_chant_double (focal, _("Focal of the camera"), 0.0, 300.0, 20.0,
                   _("Calculate b value from focal"))

gegl_chant_boolean (center, _("Center"), TRUE,
                    _("If you want center"))
gegl_chant_int (cx, _("Lens center x"), -G_MAXINT, G_MAXINT, 0,
                _("Coordinates of lens center"))
gegl_chant_int (cy, _("Lens center y"), -G_MAXINT, G_MAXINT, 0,
                _("Coordinates of lens center"))
gegl_chant_double (rscale, _("Scale"), 0.001, 10.0, 0.5,
                   _("Scale of the image"))
gegl_chant_boolean (correct, _("Autocorrect d values"), TRUE,
                    _("Autocorrect D values for lens correction models."))

gegl_chant_double (red_a, _("Model red a:"), -1.0, 1.0, 0.0,
                   _("Correction parameters for each color channel"))
gegl_chant_double (red_b, _("Model red b:"), -1.0, 1.0, 0.0,
                   _("Correction parameters for each color channel"))
gegl_chant_double (red_c, _("Model red c:"), -1.0, 1.0, 0.0,
                   _("Correction parameters for each color channel"))
gegl_chant_double (red_d, _("Model red d:"), 0.0, 2.0, 1.0,
                   _("Correction parameters for each color channel"))

gegl_chant_double (green_a, _("Model green a:"), -1.0, 1.0, 0.0,
                   _("Correction parameters for each color channel"))
gegl_chant_double (green_b, _("Model green b:"), -1.0, 1.0, 0.0,
                   _("Correction parameters for each color channel"))
gegl_chant_double (green_c, _("Model green c:"), -1.0, 1.0, 0.0,
                   _("Correction parameters for each color channel"))
gegl_chant_double (green_d, _("Model green d:"), 0.0, 2.0, 1.00,
                   _("Correction parameters for each color channel"))

gegl_chant_double (blue_a, _("Model blue a:"), -1.0, 1.0, 0.0,
                   _("Correction parameters for each color channel"))
gegl_chant_double (blue_b, _("Model blue b:"), -1.0, 1.0, 0.0,
                   _("Correction parameters for each color channel"))
gegl_chant_double (blue_c, _("Model blue c:"), -1.0, 1.0, 0.0,
                   _("Correction parameters for each color channel"))
gegl_chant_double (blue_d, _("Model blue d:"), 0.0, 2.0, 1.0,
                   _("Correction parameters for each color channel"))

gegl_chant_double (alpha_a, _("Model alpha a:"), -1.0, 1.0, 0.0,
                   _("Correction parameters for alpha channel"))
gegl_chant_double (alpha_b, _("Model alpha b:"), -1.0, 1.0, 0.0,
                   _("Correction parameters for alpha channel"))
gegl_chant_double (alpha_c, _("Model alpha c:"), -1.0, 1.0, 0.0,
                   _("Correction parameters for alpha channel"))
gegl_chant_double (alpha_d, _("Model alpha d:"), 0.0, 2.0, 1.0,
                   _("Correction parameters for alpha channel"))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "lens-correct.c"

#include "gegl-chant.h"
#include <math.h>
#include <stdio.h>

/* Struct containing the correction parameters a,b,c,d for a lens color
 * channel.  These parameters as the same as used in the Panotools
 * system.  For a detailed explanation, please consult
 * http://wiki.panotools.org/Lens_correction_model
 *
 * Normally a, b, and c are close to zero, and d is close to one.  Note
 * that d is the parameter that's approximately equal to 1 - a - b - c,
 * NOT one of the image shift parameters as used in some GUIs.
 */
typedef struct {
  gfloat a, b, c, d;
} ChannelCorrectionModel;

/* Struct containing all the information required for lens correction.
 * It includes the total size of the image plus the correction
 * parameters for each color channel.
 */
typedef struct {
  GeglRectangle BB;                               /* Bounding box of the imaged area. */
  gfloat cx, cy;                                  /* Coordinates of lens center within the imaged area.  */
  gfloat rscale;                                  /* Scale of the image (1/2 of the shortest side). */
  ChannelCorrectionModel red, green, blue, alpha; /* Correction parameters for each color channel. */
} LensCorrectionModel;

static void
make_lens (LensCorrectionModel *lens,
           GeglChantO          *o,
           GeglRectangle        boundary)
{
  lens->BB.x = boundary.x;
  lens->BB.y = boundary.y;
  lens->BB.width = boundary.width;
  lens->BB.height = boundary.height;

  if (o->center)
    {
      o->cx = (boundary.x + boundary.width) / 2;
      o->cy = (boundary.y + boundary.height) / 2;
    }

  lens->cx = o->cx;
  lens->cy = o->cy;

  lens->rscale = o->rscale;

  lens->red.a = o->red_a;
  lens->red.b = o->red_b;
  lens->red.c = o->red_c;

  if (o->correct)
    lens->red.d = 1 - o->red_a - o->red_b - o->red_c;
  else
    lens->red.d = o->red_d;

  lens->green.a = o->green_a;
  lens->green.b = o->green_b;
  lens->green.c = o->green_c;

  if (o->correct)
    lens->green.d = 1 - o->green_a - o->green_b - o->green_c;
  else
    lens->green.d = o->green_d;

  lens->blue.a = o->blue_a;
  lens->blue.b = o->blue_b;
  lens->blue.c = o->blue_c;

  if (o->correct)
    lens->blue.d = 1 - o->blue_a - o->blue_b - o->blue_c;
  else
    lens->blue.d = o->blue_d;

  lens->alpha.a = o->alpha_a;
  lens->alpha.b = o->alpha_b;
  lens->alpha.c = o->alpha_c;

  if (o->correct)
    lens->alpha.d = 1 - o->alpha_a - o->alpha_b - o->alpha_c;
  else
    lens->alpha.d = o->red_d;

  if (o->focal!=0.0)
    {
      gdouble f = o->focal;
      lens->red.b = lens->green.b = lens->blue.b = lens->alpha.b
        = 0.000005142 * f*f*f - 0.000380839 * f*f + 0.009606325 * f - 0.075316854;
    }
}

static gboolean
find_make_lens(LensCorrectionModel *lens,
               GeglChantO          *o,
               GeglRectangle        boundary)
{
  struct lfDatabase *ldb;
  const lfCamera **cameras;
  const lfLens   **lenses;
  const lfCamera  *camera;
  const lfLens    *onelen;

  struct lfLensCalibDistortion **dist;

  gint             i, j=0;
  gfloat           aux = G_MAXINT;

  lens->BB.x = boundary.x;
  lens->BB.y = boundary.y;
  lens->BB.width = boundary.width;
  lens->BB.height = boundary.height;

  if (o->center)
    {
      o->cx = (boundary.x + boundary.width) / 2;
      o->cy = (boundary.y + boundary.height) / 2;
    }

  lens->cx = o->cx;
  lens->cy = o->cy;

  lens->rscale = o->rscale;

  ldb = lf_db_new ();
  if (!ldb)
    return FALSE;

  lf_db_load (ldb);

  cameras =  lf_db_find_cameras (ldb, o->maker, o->Camera);
  if (!cameras)
    return FALSE;
  camera = cameras[0];

  lf_free (cameras);

  lenses = lf_db_find_lenses_hd (ldb, camera, o->maker, o->Lens, 0);
  if (!lenses)
    return FALSE;
  onelen = lenses[0];

  dist = onelen->CalibDistortion;

  for (i=0; lenses[i]; i++)
    if (lenses[i]->MinFocal < o->focal && o->focal < lenses[i]->MaxFocal)
      break;

  dist = lenses[i]->CalibDistortion;

  if (!dist)
    return FALSE;

  for (i=0; dist[i]; i++)
    {
      if (dist[i]->Focal == o->focal)
        {
          lens->red.a = lens->green.a = lens->blue.a = lens->alpha.a
            = dist[i]->Terms[0];
          lens->red.b = lens->green.b = lens->blue.b = lens->alpha.b
            = dist[i]->Terms[1];
          lens->red.c = lens->green.c = lens->blue.c = lens->alpha.c
            = dist[i]->Terms[2];

          lens->red.d = 1 - lens->red.a - lens->red.b - lens->red.c;
          lens->green.d = 1 - lens->green.a - lens->green.b - lens->green.c;
          lens->blue.d = 1 - lens->blue.a - lens->blue.b - lens->blue.c;
          lens->alpha.d = 1 - lens->alpha.a - lens->alpha.b - lens->alpha.c;

          aux = -G_MAXINT;
          break;
        }
      else if (i > 0)
        {
          if (aux > fabs (dist[i]->Focal - o->focal
                          + dist[i-1]->Focal - o->focal))
            {
              aux = fabs (dist[i]->Focal + dist[i-1]->Focal - 2 * o->focal);
              j = i;
            }
        }
    }
  lf_free (lenses);

  if (aux != -G_MAXINT)
    {
      gfloat aux[3];
      for (i=0; i<3; i++)
        aux[i] = (dist[j]->Terms[i] - dist[j-1]->Terms[i]) / 2.0;

      lens->red.a = lens->green.a = lens->blue.a = lens->alpha.a
        = aux[0];
      lens->red.b = lens->green.b = lens->blue.b = lens->alpha.b
        = aux[1];
      lens->red.c = lens->green.c = lens->blue.c = lens->alpha.c
        = aux[2];

      lens->red.d = 1 - lens->red.a - lens->red.b - lens->red.c;
      lens->green.d = 1 - lens->green.a - lens->green.b - lens->green.c;
      lens->blue.d = 1 - lens->blue.a - lens->blue.b - lens->blue.c;
      lens->alpha.d = 1 - lens->alpha.a - lens->alpha.b - lens->alpha.c;

    }

  return TRUE;
}


static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");
  if (!in_rect)
    return result;

  return *in_rect;
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static void
find_src_pixel (LensCorrectionModel *lcip, ChannelCorrectionModel *pp,
                gfloat x, gfloat y, gfloat *srcx, gfloat *srcy)
{
  gdouble r, radj;

  r = hypot (x - lcip->cx, y - lcip->cy) / lcip->rscale;
  radj = (((pp->a*r+pp->b)*r+pp->c)*r+pp->d);
  *srcx = (x - lcip->cx) * radj + lcip->cx;
  *srcy = (y - lcip->cy) * radj + lcip->cy;

}

static void
lens_distort_newl (gfloat              *src_buf,
                   gfloat              *dst_buf,
                   const GeglRectangle *extended,
                   const GeglRectangle *result,
                   const GeglRectangle *boundary,
                   LensCorrectionModel *lens,
                   gint                 xx,
                   gint                 yy,
                   GeglBuffer          *input)
{
  gfloat temp[4];
  gint   tmpx, tmpy, x, y, rgb;
  gint   offset;

  ChannelCorrectionModel ccm[4];

  /* Compute each dst pixel in turn and store into dst buffer. */
  ccm[0] = lens->red;
  ccm[1] = lens->green;
  ccm[2] = lens->blue;
  ccm[3] = lens->alpha;

  for (rgb = 0; rgb < 4; rgb++)
    {
      gfloat gx, gy;
      gfloat val = 0.0;
      gfloat wx[2], wy[2], wt = 0.0;

      find_src_pixel (lens, &ccm[rgb], (gfloat)xx, (gfloat)yy, &gx, &gy);
      tmpx = (gint) gx;
      tmpy = (gint) gy;

      wx[1] = gx - tmpx;
      wx[0] = 1.0 - wx[1];
      wy[1] = gy - tmpy;
      wy[0] = 1.0 - wy[1];

      for (x = 0; x < 2; x++)
        {
          for (y = 0; y < 2; y++)
            {
              if (tmpx+x >= extended->x && tmpx+x < extended->x + extended->width
                  && tmpy+y >= extended->y && tmpy+y < extended->y + extended->height)
                {
                  offset = (tmpy + y - extended->y) * extended->width * 4 +
                    (tmpx + x - extended->x) * 4 + rgb;
                  val += src_buf[offset] * wx[x] * wy[y];
                  wt += wx[x] * wy[y];
                }
              else if (tmpx+x >= boundary->x &&
                       tmpx+x < boundary->x + boundary->width &&
                       tmpy+y >= boundary->y &&
                       tmpy+y < boundary->y + boundary->height)
                {
                  gfloat color[4];
                  gegl_buffer_sample (input, tmpx+x, tmpy+y, NULL, color,
                                      babl_format ("RGBA float"),
                                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
                  val += color[rgb] * wx[x] * wy[y];
                  wt += wx[x] * wy[y];
                }
            }
        }
      if (wt <= 0)
        {
          temp [rgb] = 0.0;
        }
      else
        {
          temp [rgb] =  val / wt;
        }
    }

  offset = (yy - result->y) * result->width * 4 + (xx - result->x) * 4;
  for (x=0; x<4; x++)
    dst_buf[offset++] = temp[x];
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO          *o = GEGL_CHANT_PROPERTIES (operation);
  LensCorrectionModel  lens = { { 0, }, };
  GeglRectangle        boundary = *gegl_operation_source_get_bounding_box
    (operation, "input");

  gint     x, y, found = FALSE;
  gfloat *src_buf, *dst_buf;
  src_buf    = g_new0 (gfloat, result->width * result->height * 4);
  dst_buf    = g_new0 (gfloat, result->width * result->height * 4);

  found = find_make_lens (&lens, o, boundary);
  if (!found) make_lens (&lens, o, boundary);

  gegl_buffer_get (input, result, 1.0, babl_format ("RGBA float"),
                   src_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  for (y = result->y; y < result->y + result->height; y++)
    for (x = result->x; x < result->x + result->width; x++)
      {
        lens_distort_newl (src_buf, dst_buf, result,
                           result, &boundary, &lens, x, y, input);
      }

  gegl_buffer_set (output, result, 0, babl_format ("RGBA float"),
                   dst_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (dst_buf);
  g_free (src_buf);

  return TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;

  gegl_operation_class_set_keys (operation_class,
   "name"       , "gegl:lens-correct",
   "categories" , "blur",
   "description",
    _("Copies image performing lens distortion correction."),
    NULL);
}

#endif
