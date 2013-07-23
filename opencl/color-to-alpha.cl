__kernel void cl_color_to_alpha(__global const float4 *in,
                                __global       float4 *out,
                                float4                color)
{
  int gid = get_global_id(0);
  float4 in_v = in[gid];
  float4 out_v = in_v;
  float4 alpha;

  alpha.w = in_v.w;

  /*First component*/
  if ( color.x < 0.00001f )
    alpha.x = in_v.x;
  else if ( in_v.x > color.x + 0.00001f )
    alpha.x = (in_v.x - color.x) / (1.0f - color.x);
  else if ( in_v.x < color.x - 0.00001f )
    alpha.x = (color.x - in_v.x) / color.x;
  else
    alpha.x = 0.0f;
  /*Second component*/
  if ( color.y < 0.00001f )
    alpha.y = in_v.y;
  else if ( in_v.y > color.y + 0.00001f )
    alpha.y = (in_v.y - color.y) / (1.0f - color.y);
  else if ( in_v.y < color.y - 0.00001f )
    alpha.y = (color.y - in_v.y) / color.y;
  else
    alpha.y = 0.0f;
  /*Third component*/
  if ( color.z < 0.00001f )
    alpha.z = in_v.z;
  else if ( in_v.z > color.z + 0.00001f )
    alpha.z = (in_v.z - color.z) / (1.0f - color.z);
  else if ( in_v.z < color.z - 0.00001f )
    alpha.z = (color.z - in_v.z) / color.z;
  else
    alpha.z = 0.0f;

  if (alpha.x > alpha.y)
    {
      if (alpha.x > alpha.z)
        out_v.w = alpha.x;
      else
        out_v.w = alpha.z;
    }
  else if (alpha.y > alpha.z)
    {
      out_v.w = alpha.y;
    }
  else
    {
      out_v.w = alpha.z;
    }

  if (out_v.w >= 0.00001f)
    {
      out_v.xyz = (out_v.xyz - color.xyz) / out_v.www + color.xyz;
      out_v.w *= alpha.w;
    }

    out[gid] = out_v;
}
