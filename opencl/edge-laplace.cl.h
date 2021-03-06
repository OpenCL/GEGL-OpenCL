static const char* edge_laplace_cl_source =
"#define LAPLACE_RADIUS 2                                                      \n"
"#define EPSILON        1e-5f                                                  \n"
"                                                                              \n"
"void minmax(float4 x1, float4 x2, float4 x3,                                  \n"
"            float4 x4, float4 x5,                                             \n"
"            float4 *min_result,                                               \n"
"            float4 *max_result)                                               \n"
"{                                                                             \n"
"    // Step 0                                                                 \n"
"    float16 first = (float16)(x1, x2, x3, x4);                                \n"
"                                                                              \n"
"    // Step 1                                                                 \n"
"    float8 min1 = fmin(first.hi, first.lo);                                   \n"
"    float8 max1 = fmax(first.hi, first.lo);                                   \n"
"                                                                              \n"
"    // Step 2                                                                 \n"
"    float4 min2 = fmin(min1.hi, min1.lo);                                     \n"
"    float4 max2 = fmax(max1.hi, max1.lo);                                     \n"
"                                                                              \n"
"    // Step 3                                                                 \n"
"    *min_result = fmin(min2, x5);                                             \n"
"    *max_result = fmax(max2, x5);                                             \n"
"}                                                                             \n"
"                                                                              \n"
"kernel void pre_edgelaplace (const global float4 *in,                         \n"
"                             global float4 *out)                              \n"
"{                                                                             \n"
"    int gidx = get_global_id(0);                                              \n"
"    int gidy = get_global_id(1);                                              \n"
"    int src_width  = get_global_size(0) + LAPLACE_RADIUS;                     \n"
"    int src_height = get_global_size(1) + LAPLACE_RADIUS;                     \n"
"                                                                              \n"
"    int i = gidx + 1, j = gidy + 1;                                           \n"
"                                                                              \n"
"    float4 pix_fl = in[(i - 1) + (j - 1)*src_width];                          \n"
"    float4 pix_fm = in[(i - 0) + (j - 1)*src_width];                          \n"
"    float4 pix_fr = in[(i + 1) + (j - 1)*src_width];                          \n"
"    float4 pix_ml = in[(i - 1) + (j - 0)*src_width];                          \n"
"    float4 pix_mm = in[(i - 0) + (j - 0)*src_width];                          \n"
"    float4 pix_mr = in[(i + 1) + (j - 0)*src_width];                          \n"
"    float4 pix_bl = in[(i - 1) + (j + 1)*src_width];                          \n"
"    float4 pix_bm = in[(i - 0) + (j + 1)*src_width];                          \n"
"    float4 pix_br = in[(i + 1) + (j + 1)*src_width];                          \n"
"                                                                              \n"
"    float4 minval, maxval;                                                    \n"
"    minmax(pix_fm, pix_bm, pix_ml, pix_mr,                                    \n"
"        pix_mm, &minval, &maxval);                                            \n"
"    float4 gradient = fmax((maxval - pix_mm), (pix_mm - minval))              \n"
"        * select((float4)0.5f, (float4)-0.5f,                                 \n"
"        (pix_fl + pix_fm + pix_fr +                                           \n"
"         pix_bm - 8.0f * pix_mm + pix_br +                                    \n"
"         pix_ml + pix_mr + pix_bl) < EPSILON);                                \n"
"    gradient.w = pix_mm.w;                                                    \n"
"                                                                              \n"
"    out[gidx + gidy * get_global_size(0)] =  gradient;                        \n"
"}                                                                             \n"
"                                                                              \n"
"kernel void knl_edgelaplace (const global float4 *in,                         \n"
"                             global float4 *out)                              \n"
"{                                                                             \n"
"    int gidx = get_global_id(0);                                              \n"
"    int gidy = get_global_id(1);                                              \n"
"                                                                              \n"
"    int src_width  = get_global_size(0) + LAPLACE_RADIUS;                     \n"
"    int src_height = get_global_size(1) + LAPLACE_RADIUS;                     \n"
"                                                                              \n"
"    int i = gidx + 1, j = gidy + 1;                                           \n"
"                                                                              \n"
"    float4 pix_fl = in[(i - 1) + (j - 1)*src_width];                          \n"
"    float4 pix_fm = in[(i - 0) + (j - 1)*src_width];                          \n"
"    float4 pix_fr = in[(i + 1) + (j - 1)*src_width];                          \n"
"    float4 pix_ml = in[(i - 1) + (j - 0)*src_width];                          \n"
"    float4 pix_mm = in[(i - 0) + (j - 0)*src_width];                          \n"
"    float4 pix_mr = in[(i + 1) + (j - 0)*src_width];                          \n"
"    float4 pix_bl = in[(i - 1) + (j + 1)*src_width];                          \n"
"    float4 pix_bm = in[(i - 0) + (j + 1)*src_width];                          \n"
"    float4 pix_br = in[(i + 1) + (j + 1)*src_width];                          \n"
"                                                                              \n"
"    float4 value = select(0.0f, pix_mm, (pix_mm > 0.0f) &&                    \n"
"         (pix_fl < 0.0f || pix_fm < 0.0f ||                                   \n"
"          pix_fr < 0.0f || pix_ml < 0.0f ||                                   \n"
"          pix_mr < 0.0f || pix_bl < 0.0f ||                                   \n"
"          pix_bm < 0.0f || pix_br < 0.0f ));                                  \n"
"    value.w = pix_mm.w;                                                       \n"
"                                                                              \n"
"    out[gidx + gidy * get_global_size(0)] =  value;                           \n"
"}                                                                             \n"
;
