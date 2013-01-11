static const char* color_temperature_cl_source =
"__kernel void gegl_color_temperature(__global const float4     *in,           \n"
"                                     __global       float4     *out,          \n"
"                                     float coeff1,                            \n"
"                                     float coeff2,                            \n"
"                                     float coeff3)                            \n"
"{                                                                             \n"
"  int gid = get_global_id(0);                                                 \n"
"  float4 in_v  = in[gid];                                                     \n"
"  float4 out_v;                                                               \n"
"  out_v = in_v * (float4) (coeff1, coeff2, coeff3, 1.0f);                     \n"
"  out[gid]  =  out_v;                                                         \n"
"}                                                                             \n"
;
