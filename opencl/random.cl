/* This file is part of GEGL
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
 * Copyright 2013 Carlos Zubieta (czubieta.dev@gmail.com)
 */

/* XXX: this file should be kept in sync with gegl-random. */

typedef ushort4 GeglRandom;

unsigned int _gegl_cl_random_int (__global const int *cl_random_data,
                                  const GeglRandom  rand,
                                  int x,
                                  int y,
                                  int z,
                                  int n);

unsigned int gegl_cl_random_int  (__global const int *cl_random_data,
                                  const GeglRandom  rand,
                                  int x,
                                  int y,
                                  int z,
                                  int n);

int gegl_cl_random_int_range     (__global const int  *cl_random_data,
                                  const GeglRandom  rand,
                                  int x,
                                  int y,
                                  int z,
                                  int n,
                                  int min,
                                  int max);

float gegl_cl_random_float       (__global const int  *cl_random_data,
                                  const GeglRandom  rand,
                                  int x,
                                  int y,
                                  int z,
                                  int n);

float gegl_cl_random_float_range (__global const int  *cl_random_data,
                                  const GeglRandom  rand,
                                  int x,
                                  int y,
                                  int z,
                                  int n,
                                  float min,
                                  float max);

unsigned int
_gegl_cl_random_int (__global const int  *cl_random_data,
                     const GeglRandom    rand,
                     int                 x,
                     int                 y,
                     int                 z,
                     int                 n)
{
  const long XPRIME = 103423;
  const long YPRIME = 101359;
  const long NPRIME = 101111;

  unsigned long idx = x * XPRIME +
                      y * YPRIME * XPRIME +
                      n * NPRIME * YPRIME * XPRIME;

  int r0 = cl_random_data[idx % rand.x],
      r1 = cl_random_data[rand.x + (idx % rand.y)],
      r2 = cl_random_data[rand.x + rand.y + (idx % rand.z)];
  return r0 ^ r1 ^ r2;
}

unsigned int
gegl_cl_random_int (__global const int *cl_random_data,
                    const GeglRandom    rand,
                    int                 x,
                    int                 y,
                    int                 z,
                    int                 n)
{
  return _gegl_cl_random_int (cl_random_data, rand, x, y, z, n);
}

int
gegl_cl_random_int_range (__global const int  *cl_random_data,
                          const GeglRandom    rand,
                          int                 x,
                          int                 y,
                          int                 z,
                          int                 n,
                          int                 min,
                          int                 max)
{
  int ret = _gegl_cl_random_int (cl_random_data, rand, x, y, z, n);
  return (ret % (max-min)) + min;
}


#define G_RAND_FLOAT_TRANSFORM  0.00001525902189669642175f

float
gegl_cl_random_float (__global const int *cl_random_data,
                      const GeglRandom    rand,
                      int                 x,
                      int                 y,
                      int                 z,
                      int                 n)
{
  int u = _gegl_cl_random_int (cl_random_data, rand, x, y, z, n);
  return (u & 0xffff) * G_RAND_FLOAT_TRANSFORM;
}

float
gegl_cl_random_float_range (__global const int *cl_random_data,
                            const GeglRandom    rand,
                            int                 x,
                            int                 y,
                            int                 z,
                            int                 n,
                            float               min,
                            float               max)
{
  float f = gegl_cl_random_float (cl_random_data, rand, x, y, z, n);
  return f * (max - min) + min;
}
