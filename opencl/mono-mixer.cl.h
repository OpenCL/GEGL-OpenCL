static const char* mono_mixer_cl_source =
"__kernel void gegl_mono_mixer (__global const float4 *src_buf,                \n"
"                               __global       float2 *dst_buf,                \n"
"                               float                  red,                    \n"
"                               float                  green,                  \n"
"                               float                  blue)                   \n"
"{                                                                             \n"
"  int gid = get_global_id(0);                                                 \n"
"  float4 in_v = src_buf[gid];                                                 \n"
"  dst_buf[gid].x = in_v.x * red + in_v.y * green + in_v.z * blue;             \n"
"  dst_buf[gid].y = in_v.w;                                                    \n"
"}                                                                             \n"
;
