kernel void
cl_gblur_selective(global const float4 *in,
                   global const float4 *delta,
                   global       float4 *out,
                          const float   radius,
                          const float   max_delta)
{
  const int gidx       = get_global_id(0);
  const int gidy       = get_global_id(1);
  const int iradius    = (int)radius;
  const int dst_width  = get_global_size(0);
  const int src_width  = dst_width + iradius * 2;

  const int center_gid1d = (gidy + iradius) * src_width + gidx + iradius;
  const float4 center_pix = in[center_gid1d];
  const float3 center_delta = delta[center_gid1d].xyz;

  float3 accumulated = 0.0f;
  float3 count       = 0.0f;

  for (int v = -iradius; v <= iradius; v++)
    {
      for (int u = -iradius; u <= iradius; u++)
        {
          const int i = gidx + iradius + u;
          const int j = gidy + iradius + v;
          const int gid1d = i + j * src_width;

          const float4 src_pix = in[gid1d];
          const float3 delta_pix = delta[gid1d].xyz;

          const float gaussian_weight = exp(-0.5f * (u * u + v * v) / radius);

          const float weight = gaussian_weight * src_pix.w;
          const float3 diff = center_delta - delta_pix;
          const float3 w = convert_float3 (fabs (diff) <= max_delta);
          accumulated += w * weight * src_pix.xyz;
          count += w * weight;
        }
    }

  const float3 out_v = select (center_pix.xyz,
                               accumulated / count,
                               count != 0.0f);
  out[gidx + gidy * dst_width] = (float4)(out_v, center_pix.w);
}
