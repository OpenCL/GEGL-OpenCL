static const char* brightnesscontrast_cl_source =
"__kernel void gegl_brightness_contrast(__global const float4     *in,         \n"
"                                       __global       float4     *out,        \n"
"                                       float contrast,                        \n"
"                                       float brightness)                      \n"
"{                                                                             \n"
"  int gid = get_global_id(0);                                                 \n"
"  float4 in_v  = in[gid];                                                     \n"
"  float4 out_v;                                                               \n"
"  out_v.xyz = (in_v.xyz - 0.5f) * contrast + brightness + 0.5f;               \n"
"  out_v.w   =  in_v.w;                                                        \n"
"  out[gid]  =  out_v;                                                         \n"
"}                                                                             \n"
;
