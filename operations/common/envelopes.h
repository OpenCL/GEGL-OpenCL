/* GEGL is free software; you can redistribute it and/or
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
 * Copyright 2007, 2009 Øyvind Kolås     <pippin@gimp.org>
 */

#define ANGLE_PRIME  95273  /* the lookuptables are sized as primes to avoid  */
#define RADIUS_PRIME 29537  /* repetitions when they are used cyclcly simulatnously */

static gfloat   lut_cos[ANGLE_PRIME];
static gfloat   lut_sin[ANGLE_PRIME];
static gfloat   radiuses[RADIUS_PRIME];
static gdouble  luts_computed = 0.0; 
static gint     angle_no=0;
static gint     radius_no=0;

static void compute_luts(gdouble rgamma)
{
  gint i;
  GRand *rand;

  if (luts_computed==rgamma)
    return;
  luts_computed = rgamma;
  rand = g_rand_new();

  for (i=0;i<ANGLE_PRIME;i++)
    {
      gfloat angle = g_rand_double_range (rand, 0.0, G_PI*2);
      lut_cos[i] = cos(angle);
      lut_sin[i] = sin(angle);
    }
  for (i=0;i<RADIUS_PRIME;i++)
    {
      radiuses[i] = pow(g_rand_double_range (rand, 0.0, 1.0), rgamma);
    }

  g_rand_free(rand);

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

        if (pixel[3]>0.0) /* ignore fully transparent pixels */
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

static inline void compute_envelopes (gfloat  *buf,
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
  gfloat  range_sum[4]               = {0,0,0,0};
  gfloat  relative_brightness_sum[4] = {0,0,0,0};
  gfloat *pixel = buf + (width*y+x)*4;

  /* compute lookuptables for the gamma, currently not used/exposed
   * as a tweakable property */
  compute_luts(rgamma);

  if (same_spray)
    {
      angle_no = 0;
      radius_no = 0;
    }

  for (i=0;i<iterations;i++)
    {
      gfloat min[3], max[3];

      sample_min_max (buf,
                      width,
                      height,
                      x, y,
                      radius, samples,
                      min, max);

      for (c=0;c<3;c++)
        {
          gfloat range, relative_brightness;

          range = max[c] - min[c];

          if (range>0.0)
            {
              relative_brightness = (pixel[c] - min[c]) / range;
            }
          else
            {
              relative_brightness = 0.5;
            }

          relative_brightness_sum[c] += relative_brightness;
          range_sum[c] += range;
        }
    }

    for (c=0;c<3;c++)
      {
        gfloat relative_brightness = relative_brightness_sum[c] / iterations;
        gfloat range               = range_sum[c] / iterations;
        
        if (max_envelope)
          max_envelope[c] = pixel[c] + (1.0 - relative_brightness) * range;
        if (min_envelope)
          min_envelope[c] = pixel[c] - relative_brightness * range;
      }
}
