#define MAX_RANK 3

/* Random feature counts following the Poisson distribution with
   lambda equal to 7. */

const __constant char poisson[256] = {
  7, 9, 12, 12, 8, 7, 5, 5, 6, 7, 8, 6, 10, 7, 6, 2, 8, 3, 9, 5, 13, 10, 9,
  8, 8, 9, 3, 8, 9, 6, 8, 7, 4, 9, 6, 3, 10, 7, 7, 7, 6, 7, 4, 14, 7, 6, 11,
  7, 7, 7, 12, 7, 10, 6, 8, 11, 3, 5, 7, 7, 8, 7, 9, 8, 5, 8, 11, 3, 4, 5, 8,
  8, 7, 8, 9, 2, 7, 8, 12, 4, 8, 2, 11, 8, 14, 7, 8, 2, 3, 10, 4, 6, 9, 5, 8,
  7, 10, 10, 10, 14, 5, 7, 6, 4, 5, 6, 11, 8, 7, 3, 11, 5, 5, 2, 9, 7, 7, 7,
  9, 2, 7, 6, 9, 7, 6, 5, 12, 5, 3, 11, 9, 12, 8, 6, 8, 6, 8, 5, 5, 7, 5, 2,
  9, 5, 5, 8, 11, 8, 8, 10, 6, 4, 7, 14, 7, 3, 10, 7, 7, 4, 9, 10, 10, 9, 8,
  8, 7, 6, 5, 10, 10, 5, 10, 7, 7, 10, 7, 4, 9, 9, 6, 8, 5, 10, 7, 3, 9, 9,
  7, 8, 9, 7, 5, 7, 6, 5, 5, 12, 4, 7, 5, 5, 4, 5, 7, 10, 8, 7, 9, 4, 6, 11,
  6, 3, 7, 8, 9, 5, 8, 6, 7, 8, 7, 7, 3, 7, 7, 9, 4, 5, 5, 6, 9, 7, 6, 12, 4,
  9, 10, 8, 8, 6, 4, 9, 9, 8, 11, 6, 8, 13, 8, 9, 12, 6, 9, 8
};

uint
philox (uint s,
        uint t,
        uint k)
{
  ulong p;
  int   i;

  for (i = 0 ; i < 3 ; i += 1)
    {
      p = s * 0xcd9e8d57ul;

      s = ((uint)(p >> 32)) ^ t ^ k;
      t = (uint)p;

      k += 0x9e3779b9u;
    }

  return s;
}

float
lcg (uint *hash)
{
  return (*hash = *hash * 1664525u + 1013904223u) / 4294967296.0f;
}

void
search_box (float              *closest,
            uint               *feature,
            int                 s,
            int                 t,
            float               x,
            float               y,
            float               shape,
            uint                rank,
            uint                seed)
{
  uint hash;
  int  i, n;

  hash = philox ((uint)s, (uint)t, seed);
  n = poisson[hash >> 24];

  for (i = 0 ; i < n ; i += 1)
    {
      float delta_x, delta_y, d;
      int   j, k;

      /* Calculate the distance to each feature point. */

      delta_x = s + lcg (&hash) - x;
      delta_y = t + lcg (&hash) - y;

      if (shape == 2)
        d = delta_x * delta_x + delta_y * delta_y;
      else if (shape == 1)
        d = fabs (delta_x) + fabs (delta_y);
      else
        d = pow (fabs (delta_x), shape) +
          pow (fabs (delta_y), shape);

      /* Insert it into the list of n closest distances if needed. */

      for (j = 0 ; j < rank && d > closest[j] ; j += 1);

      if (j < rank)
        {
          for (k = rank - 1 ; k > j ; k -= 1)
            {
              closest[k] = closest[k - 1];
            }

          closest[j] = d;

          if (j == rank - 1)
            *feature = hash;
        }
    }
}

__kernel void kernel_noise (__global float *out,
                            const int       x_0,
                            const int       y_0,
                            const uint      iterations,
                            float           scale,
                            float           shape,
                            uint            rank,
                            uint            seed,
                            int             palettize)
{
  const int gidx = get_global_id(0);
  const int gidy = get_global_id(1);

  float c, d, n, closest[MAX_RANK], *d_0 = &closest[MAX_RANK - 1];
  uint  feature;
  int   i, j;

  for (j = 0, n = 0, c = 1, d = scale;
       j < iterations;
       c *= 2, d *= 2, j += 1)
    {
      float d_l, d_r, d_t, d_b;
      float x = (float)(gidx + x_0) * d;
      float y = (float)(gidy + y_0) * d;
      int   s = (int)floor(x);
      int   t = (int)floor(y);

      for (i = 0 ; i < rank ; closest[i] = 1.0f / 0.0f, i += 1);

      /* Search the box the point is in. */

      search_box (closest, &feature, s, t, x, y, shape, rank, seed);

      d_0 = &closest[rank - 1];
      d_l = x - s; d_l *= d_l;
      d_r = 1.0f - x + s; d_r *= d_r;
      d_b = y - t; d_b *= d_b;
      d_t = 1.0f - y + t; d_t *= d_t;

      /* Search adjacent boxes if it is possible for them to contain a
       * nearby feature point. */

      if (d_l < *d_0)
        {
          if (d_l + d_b < *d_0)
            search_box (closest, &feature, s - 1, t - 1, x, y,
                        shape, rank, seed);

          search_box (closest, &feature, s - 1, t, x, y,
                      shape, rank, seed);

          if (d_l + d_t < *d_0)
              search_box (closest, &feature, s - 1, t + 1, x, y,
                          shape, rank, seed);
        }

      if (d_b < *d_0)
          search_box (closest, &feature, s, t - 1, x, y,
                      shape, rank, seed);

      if (d_t < *d_0)
          search_box (closest, &feature, s, t + 1, x, y,
                      shape, rank, seed);

      if (d_r < *d_0)
        {
          if (d_r + d_b < *d_0)
              search_box (closest, &feature, s + 1, t - 1, x, y,
                          shape, rank, seed);

          search_box (closest, &feature, s + 1, t, x, y,
                      shape, rank, seed);

          if (d_r + d_t < *d_0)
              search_box (closest, &feature, s + 1, t + 1, x, y,
                          shape, rank, seed);
        }

      /* If palettized output is requested return the normalized hash of
       * the closest feature point, otherwise return the closest
       * distance. */

      if(palettize)
        {
          n += feature / 4294967295.0f / c;
        }
      else
        {
          n += pow(closest[rank - 1], 1 / shape) / c;
        }
    }

  out[gidy * get_global_size(0) + gidx] = n;
}
