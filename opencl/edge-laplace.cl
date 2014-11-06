#define LAPLACE_RADIUS 2
#define EPSILON        1e-5f

void minmax(float4 x1, float4 x2, float4 x3,
            float4 x4, float4 x5,
            float4 *min_result,
            float4 *max_result)
{
    // Step 0
    float16 first = (float16)(x1, x2, x3, x4);

    // Step 1
    float8 min1 = fmin(first.hi, first.lo);
    float8 max1 = fmax(first.hi, first.lo);

    // Step 2
    float4 min2 = fmin(min1.hi, min1.lo);
    float4 max2 = fmax(max1.hi, max1.lo);

    // Step 3
    *min_result = fmin(min2, x5);
    *max_result = fmax(max2, x5);
}

kernel void pre_edgelaplace (const global float4 *in,
                             global float4 *out)
{
    int gidx = get_global_id(0);
    int gidy = get_global_id(1);
    int src_width  = get_global_size(0) + LAPLACE_RADIUS;
    int src_height = get_global_size(1) + LAPLACE_RADIUS;

    int i = gidx + 1, j = gidy + 1;

    float4 pix_fl = in[(i - 1) + (j - 1)*src_width];
    float4 pix_fm = in[(i - 0) + (j - 1)*src_width];
    float4 pix_fr = in[(i + 1) + (j - 1)*src_width];
    float4 pix_ml = in[(i - 1) + (j - 0)*src_width];
    float4 pix_mm = in[(i - 0) + (j - 0)*src_width];
    float4 pix_mr = in[(i + 1) + (j - 0)*src_width];
    float4 pix_bl = in[(i - 1) + (j + 1)*src_width];
    float4 pix_bm = in[(i - 0) + (j + 1)*src_width];
    float4 pix_br = in[(i + 1) + (j + 1)*src_width];

    float4 minval, maxval;
    minmax(pix_fm, pix_bm, pix_ml, pix_mr,
        pix_mm, &minval, &maxval);
    float4 gradient = fmax((maxval - pix_mm), (pix_mm - minval))
        * select((float4)0.5f, (float4)-0.5f,
        (pix_fl + pix_fm + pix_fr +
         pix_bm - 8.0f * pix_mm + pix_br +
         pix_ml + pix_mr + pix_bl) < EPSILON);
    gradient.w = pix_mm.w;

    out[gidx + gidy * get_global_size(0)] =  gradient;
}

kernel void knl_edgelaplace (const global float4 *in,
                             global float4 *out)
{
    int gidx = get_global_id(0);
    int gidy = get_global_id(1);

    int src_width  = get_global_size(0) + LAPLACE_RADIUS;
    int src_height = get_global_size(1) + LAPLACE_RADIUS;

    int i = gidx + 1, j = gidy + 1;

    float4 pix_fl = in[(i - 1) + (j - 1)*src_width];
    float4 pix_fm = in[(i - 0) + (j - 1)*src_width];
    float4 pix_fr = in[(i + 1) + (j - 1)*src_width];
    float4 pix_ml = in[(i - 1) + (j - 0)*src_width];
    float4 pix_mm = in[(i - 0) + (j - 0)*src_width];
    float4 pix_mr = in[(i + 1) + (j - 0)*src_width];
    float4 pix_bl = in[(i - 1) + (j + 1)*src_width];
    float4 pix_bm = in[(i - 0) + (j + 1)*src_width];
    float4 pix_br = in[(i + 1) + (j + 1)*src_width];

    float4 value = select(0.0f, pix_mm, (pix_mm > 0.0f) &&
         (pix_fl < 0.0f || pix_fm < 0.0f ||
          pix_fr < 0.0f || pix_ml < 0.0f ||
          pix_mr < 0.0f || pix_bl < 0.0f ||
          pix_bm < 0.0f || pix_br < 0.0f ));
    value.w = pix_mm.w;

    out[gidx + gidy * get_global_size(0)] =  value;
}
