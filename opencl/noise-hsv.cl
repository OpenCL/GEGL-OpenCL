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
 */

float
randomize_value (__global const int        *random_data,
                          const GeglRandom  rand,
                                float       now,
                                float       min_val,
                                float       max_val,
                                bool        wraps_around,
                                float       rand_max,
                                int         holdness,
                                int         x,
                                int         y,
                                int         n)
{
  const float steps = max_val - min_val;
  float rand_val = gegl_cl_random_float (random_data, rand, x, y, 0, n++);

  for (int i = 1; i < holdness; i++)
    {
      float tmp = gegl_cl_random_float (random_data, rand, x, y, 0, n++);
      rand_val = min (rand_val, tmp);
    }

  const int flag = (gegl_cl_random_float (random_data, rand, x, y, 0, n) < 0.5)
                   ? -1 : 1;
  float new_val = now + flag * fmod (rand_max * rand_val, steps);

  if (new_val < min_val)
    {
      if (wraps_around)
        new_val += steps;
      else
        new_val = min_val;
    }

  if (max_val < new_val)
    {
      if (wraps_around)
        new_val -= steps;
      else
        new_val = max_val;
    }

  return new_val;
}

__kernel void cl_noise_hsv(__global const float4    *in,
                           __global       float4    *out,
                           __global const int       *random_data,
                                    const GeglRandom rand,
                                          int        x_offset,
                                          int        y_offset,
                                          int        roi_width,
                                          int        whole_region_width,
                                          int        holdness,
                                          float      hue_distance,
                                          float      saturation_distance,
                                          float      value_distance)
{
  const int gid  = get_global_id(0);
  const int gidy = gid / roi_width;
  const int gidx = gid - gidy * roi_width;

  const int x = gidx + x_offset;
  const int y = gidy + y_offset;

  int n = (3 * holdness + 4) * (x + whole_region_width * y);

  const float4 in_v = in[gid];

  float hue        = in_v.s0;
  float saturation = in_v.s1;
  float value      = in_v.s2;
  float alpha      = in_v.s3;

  /* there is no need for scattering hue of desaturated pixels here */
  if ((hue_distance > 0) && (saturation > 0))
    hue = randomize_value (random_data, rand,
                           hue, 0.0, 1.0, true,
                           hue_distance, holdness,
                           x, y, n);

  n += holdness + 1;
  if (saturation_distance > 0)
    {
      /* desaturated pixels get random hue before increasing saturation */
      if (saturation == 0)
        hue = gegl_cl_random_float_range (random_data, rand,
                                          x, y, 0, n, 0.0, 1.0);
      saturation = randomize_value (random_data, rand,
                                    saturation, 0.0, 1.0, false,
                                    saturation_distance, holdness,
                                    x, y, n + 1);
    }

  n += holdness + 2;
  if (value_distance > 0)
    value = randomize_value (random_data, rand,
                             value, 0.0, 1.0, false,
                             value_distance, holdness,
                             x, y, n);

  out[gid] = (float4)(hue, saturation, value, alpha);
}
