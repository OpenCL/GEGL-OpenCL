#define MAX_RANK 3

float2
philox (uint2 st,
        uint k)
{
  ulong p;
  int   i;

  for (i = 0 ; i < 3 ; i += 1)
    {
      p = st.x * 0xcd9e8d57ul;

      st.x = ((uint)(p >> 32)) ^ st.y ^ k;
      st.y = (uint)p;

      k += 0x9e3779b9u;
    }

  return convert_float2(st) / 2147483648.0f - 1.0f;
}

__kernel void kernel_noise (__global float *out,
                            const int x_0,
                            const int y_0,
                            const uint iterations,
                            const float scale,
                            const uint seed)
{
  const int gidx = get_global_id(0);
  const int gidy = get_global_id(1);

  float  c, d, m;
  float2 p;
  int    j;

  for (j = 0, m = 0, c = 1, d = scale;
       j < iterations;
       c *= 2, d *= 2, j += 1)
    {
      float s, t, n;
      float2 g[3], u[3], i, di;
      int k;

      p = (float2)(gidx + x_0, gidy + y_0) * d;

      /* Skew the input point and find the lowest corner of the containing
         simplex. */

      s = (p.x + p.y) * (sqrt(3.0f) - 1) / 2;
      i = floor(p + s);

      /* Calculate the (unskewed) distance between the input point and all
         simplex corners. */

      s = (i.x + i.y) * (3 - sqrt(3.0f)) / 6;
      u[0] = p - i + s;

      di = u[0].x >= u[0].y ? (float2)(1, 0) : (float2)(0, 1);

      u[1] = u[0] - di + (3 - sqrt(3.0f)) / 6;
      u[2] = u[0] - 1 + (3 - sqrt(3.0f)) / 3;

      /* Calculate gradients for each corner vertex. We convert to
       * signed int first to avoid implementation-defined behavior for
       * out-of-range values.  See section 6.2.3.3 of the OpenCL
       * specification. */

      g[0] = philox(convert_uint2(convert_int2(i)), seed);
      g[1] = philox(convert_uint2(convert_int2(i + di)), seed);
      g[2] = philox(convert_uint2(convert_int2(i + 1)), seed);

      for (k = 0, n = 0 ; k < 3 ; k += 1)
        {
          t = 0.5f - dot(u[k], u[k]);

          if (t > 0)
            {
              t *= t;
              n += t * t * dot(g[k], u[k]);
            }
        }

      m += 70 * n / c;
    }

  out[gidy * get_global_size(0) + gidx] = m;
}
