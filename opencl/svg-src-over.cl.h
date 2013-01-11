static const char* svg_src_over_cl_source =
"__kernel void svg_src_over (__global const float4 *in,                        \n"
"                            __global const float4 *aux,                       \n"
"                            __global       float4 *out)                       \n"
"{                                                                             \n"
"  int gid = get_global_id(0);                                                 \n"
"  float4 in_v  = in [gid];                                                    \n"
"  float4 aux_v = aux[gid];                                                    \n"
"  float4 out_v;                                                               \n"
"  out_v.xyz = aux_v.xyz + in_v.xyz * (1.0f - aux_v.w);                        \n"
"  out_v.w   = aux_v.w + in_v.w - aux_v.w * in_v.w;                            \n"
"  out[gid]  = out_v;                                                          \n"
"}                                                                             \n"
;
