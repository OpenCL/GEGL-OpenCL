__kernel void cl_copy_weigthed_blend(__global const float4 *in,
                                     __global       float4 *out)
{
  int gid = get_global_id(0);
  float4 in_v = in[gid];
  out[gid] = in_v;
}

__kernel void cl_weighted_blend(__global const float4 *in,
                                __global const float4 *aux,
                                __global       float4 *out)
{
  int gid = get_global_id(0);
  float4 in_v = in[gid];
  float4 aux_v = aux[gid];
  float4 out_v;
  float in_weight;
  float aux_weight;
  float total_alpha = in_v.w + aux_v.w;

  total_alpha = total_alpha == 0 ? 1 : total_alpha;

  in_weight = in_v.w / total_alpha;
  aux_weight = 1.0f - in_weight;

  out_v.xyz = in_weight * in_v.xyz + aux_weight * aux_v.xyz;
  out_v.w = total_alpha;
  out[gid] = out_v;
}
