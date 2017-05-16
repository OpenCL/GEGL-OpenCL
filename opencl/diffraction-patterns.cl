float3
diff_intensity (const float x,
                const float y,
                const float3 lam,
                const float polarization,
                const float scattering,
                const int   iterations,
                const float weird_factor)
{
  float3 cxy = (float3)0.0f;
  float3 sxy = (float3)0.0f;

  for (int i = 0; i <= iterations; i++)
    {
      const float a = -M_PI_F + i * (2.0f * M_PI_F / iterations);
      float sina, cosa;
      sina = sincos (a, &cosa);

      const float v0 = cosa;
      const float v1 = 0.75f * sina;
      const float v2 = 0.5f * (4.0f * cosa * cosa + sina * sina);
      const float3 p = 4.0f * lam * (v0 * x + v1 * y - v2);

      float3 sinp, cosp;
      sinp = sincos (p, &cosp);
      cxy += cosp;
      sxy += sinp;
    }

  cxy *= weird_factor;
  sxy *= weird_factor;

  const float polpi2 = polarization * (M_PI_F / 2.0f);
  float sinpolpi2, cospolpi2;
  sinpolpi2 = sincos (polpi2, &cospolpi2);

  return scattering * ((cospolpi2 + sinpolpi2) * cxy * cxy +
                       (cospolpi2 - sinpolpi2) * sxy * sxy);
}

__kernel void
cl_diffraction_patterns (__global       float *out,
                                  const int    offset_x,
                                  const int    offset_y,
                                  const int    width,
                                  const int    height,
                                  const float3 sedges,
                                  const float3 contours,
                                  const float3 frequency,
                                  const float  brightness,
                                  const float  polarization,
                                  const float  scattering,
                                  const int    iterations,
                                  const float  weird_factor)
{
  const int gidx = get_global_id (0);
  const int gidy = get_global_id (1);

  const int x = gidx + offset_x;
  const int y = gidy + offset_y;

  const float dh = +10.0f / (width - 1);
  const float dv = -10.0f / (height - 1);

  const float px = dh * x - 5.0f;
  const float py = dv * y + 5.0f;

  const float3 di = diff_intensity (px, py, frequency, polarization, scattering,
                                    iterations, weird_factor);
  const float3 out_v = fabs (sedges * sin (contours * atan (brightness * di)));

  const int gid = gidy * get_global_size (0) + gidx;
  out[gid * 3 + 0] = out_v.x;
  out[gid * 3 + 1] = out_v.y;
  out[gid * 3 + 2] = out_v.z;
}
