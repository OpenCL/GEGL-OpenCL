/* GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2007 Øyvind Kolås     <pippin@gimp.org>
 */

#define ANGLE_PRIME  95273
#define RADIUS_PRIME 29537
#define SCALE_PRIME 85643

static gfloat   lut_cos[ANGLE_PRIME];
static gfloat   lut_sin[ANGLE_PRIME];
static gfloat   radiuses[RADIUS_PRIME];
static gdouble  luts_computed = 0.0; 
static gint     angle_no=0;
static gint     radius_no=0;

static void compute_luts(gdouble rgamma)
{
  gint i;
  if (luts_computed==rgamma)
    return;
  luts_computed = rgamma;

  for (i=0;i<ANGLE_PRIME;i++)
    {
      gfloat angle = (random() / (RAND_MAX*1.0)) * 3.141592653589793*2;
      lut_cos[i] = cos(angle);
      lut_sin[i] = sin(angle);
    }
  for (i=0;i<RADIUS_PRIME;i++)
    {
      radiuses[i] = pow(random() / (RAND_MAX*1.0), rgamma);
    }
}

static inline void
sample (gfloat *buf,
        gint    width,
        gint    height,
        gint    x,
        gint    y,
        gfloat *dst)
{
  gfloat *pixel = (buf + ((width * y) + x) * 4);
  gint c;

  for (c=0;c<4;c++)
    {
      dst[c] = pixel[c];
    }
}

static inline void
sample_min_max (gfloat *buf,
                gint    width,
                gint    height,
                gint    x,
                gint    y,
                gint    radius,
                gint    samples,
                gfloat *min,
                gfloat *max)
{
  gfloat best_min[3];
  gfloat best_max[3];
  gfloat *center_pix = (buf + (width * y + x) * 4);

  gint i, c;

  for (c=0;c<3;c++)
    {
      best_min[c]=center_pix[c];
      best_max[c]=center_pix[c];
    }

  for (i=0; i<samples; i++)
    {
      gint u, v;
      gint angle;
      gfloat rmag;
retry:                      /* if we've sampled outside the valid image
                               area, we grab another sample instead, this
                               should potentially work better than mirroring
                               or extending the image
                             */
      angle = angle_no++;
      rmag = radiuses[radius_no++] * radius;

      if (angle_no>=ANGLE_PRIME)
        angle_no=0;
      if (radius_no>=RADIUS_PRIME)
        radius_no=0;

      u = x + rmag * lut_cos[angle];
      v = y + rmag * lut_sin[angle];

      if (u>=width ||
          u<0 ||
          v>=height ||
          v<0)
        goto retry;

      {
        gfloat pixel[4];

        sample (buf, width, height, u, v, pixel);

        if (pixel[3]>0.0)
          {
            for (c=0;c<3;c++)
              {
                if (pixel[c]<best_min[c])
                  best_min[c]=pixel[c];

                if (pixel[c]>best_max[c])
                  best_max[c]=pixel[c];
              }
          }
        else
          {
            goto retry;
          }
      }
    }
  for (c=0;c<3;c++)
    {
      min[c]=best_min[c];
      max[c]=best_max[c];
    }
}

static void compute_envelopes (gfloat  *buf,
                               gint     width,
                               gint     height,
                               gint     x,
                               gint     y,
                               gint     radius,
                               gint     samples,
                               gint     iterations,
                               gboolean same_spray,
                               gdouble  rgamma,
                               gfloat  *min_envelope,
                               gfloat  *max_envelope)
{
  gint    i;
  gint    c;
  /*gfloat  range_avg[4]  = {0,0,0,0};*/
  gfloat  dark_avg[4]   = {0,0,0,0};
  gfloat  bright_avg[4] = {0,0,0,0};
  /*gfloat *pixel = buf + (width*y+x)*4;*/

  compute_luts(rgamma);

  if (same_spray)
    {
      angle_no = 0;
      radius_no = 0;
    }

  for (i=0;i<iterations;i++)
    {
      gfloat min[3], max[3];      /* sampled min/max */

      sample_min_max (buf,
                      width,
                      height,
                      x, y,
                      radius, samples,
                      min, max);

      for (c=0;c<3;c++)
        {
          /*gfloat range, bright, dark;

          range = max[c] - min[c];

          if (range>0.0)
            {
              bright = (max[c] - pixel[c]) / range;
              dark = (pixel[c] - min[c]) / range;
            }
          else
            {
              bright = 0.5;
              dark = 0.5;
            }*/

          dark_avg[c] += min[c];
          bright_avg[c] += max[c];
        }
    }

    for (c=0;c<3;c++)
      {
        dark_avg[c]   /= iterations;
        bright_avg[c] /= iterations;
      }

    if (max_envelope)
      for (c=0;c<3;c++)
        max_envelope[c] = bright_avg[c];/*pixel[c] + bright_avg[c] * range_avg[c];*/

    if (min_envelope)
      for (c=0;c<3;c++)
        min_envelope[c] = dark_avg[c];/*pixel[c] - dark_avg[c] * range_avg[c];*/
}
