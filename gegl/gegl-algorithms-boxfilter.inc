void
BOXFILTER_FUNCNAME (guchar              *dest_buf,
                    const guchar        *source_buf,
                    const GeglRectangle *dst_rect,
                    const GeglRectangle *src_rect,
                    const gint           s_rowstride,
                    const gdouble        scale,
                    const gint           bpp,
                    const gint           d_rowstride)
{
  gfloat left_weight, center_weight, right_weight;
  gfloat top_weight, middle_weight, bottom_weight;
  const BOXFILTER_TYPE *src[9];
  gint     x, y;
  gint     components = bpp / sizeof(BOXFILTER_TYPE);

  for (y = 0; y < dst_rect->height; y++)
    {
      const gfloat sy = (dst_rect->y + y + .5) / scale - src_rect->y;
      const gint     ii = floorf (sy);
      BOXFILTER_TYPE             *dst = (BOXFILTER_TYPE*)(dest_buf + y * d_rowstride);
      const guchar  *src_base = source_buf + ii * s_rowstride;

      top_weight    = MAX (0., .5 - scale * (sy - ii));
      bottom_weight = MAX (0., .5 - scale * ((ii + 1 ) - sy));
      middle_weight = 1. - top_weight - bottom_weight;

      for (x = 0; x < dst_rect->width; x++)
        {
          const gfloat sx = (dst_rect->x + x + .5) / scale - src_rect->x;
          const gint   jj = floorf (sx);

          left_weight   = MAX (0., .5 - scale * (sx - jj));
          right_weight  = MAX (0., .5 - scale * ((jj + 1) - sx));
          center_weight = 1. - left_weight - right_weight;

          src[4] = (const BOXFILTER_TYPE*)src_base + jj * components;
          src[1] = (const BOXFILTER_TYPE*)(src_base - s_rowstride) + jj * components;
          src[7] = (const BOXFILTER_TYPE*)(src_base + s_rowstride) + jj * components;

          src[2] = src[1] + components;
          src[5] = src[4] + components;
          src[8] = src[7] + components;

          src[0] = src[1] - components;
          src[3] = src[4] - components;
          src[6] = src[7] - components;

          {
            const gdouble lt = left_weight * top_weight;
            const gdouble lm = left_weight * middle_weight;
            const gdouble lb = left_weight * bottom_weight;
            const gdouble ct = center_weight * top_weight;
            const gdouble cm = center_weight * middle_weight;
            const gdouble cb = center_weight * bottom_weight;
            const gdouble rt = right_weight * top_weight;
            const gdouble rm = right_weight * middle_weight;
            const gdouble rb = right_weight * bottom_weight;

            switch (components)
            {
              case 4:
                dst[3] = BOXFILTER_ROUND(
                  src[0][3] * lt + src[3][3] * lm + src[6][3] * lb +
                  src[1][3] * ct + src[4][3] * cm + src[7][3] * cb +
                  src[2][3] * rt + src[5][3] * rm + src[8][3] * rb);
              case 3:
                dst[2] = BOXFILTER_ROUND(
                  src[0][2] * lt + src[3][2] * lm + src[6][2] * lb +
                  src[1][2] * ct + src[4][2] * cm + src[7][2] * cb +
                  src[2][2] * rt + src[5][2] * rm + src[8][2] * rb);
              case 2:
                dst[1] = BOXFILTER_ROUND(
                  src[0][1] * lt + src[3][1] * lm + src[6][1] * lb +
                  src[1][1] * ct + src[4][1] * cm + src[7][1] * cb +
                  src[2][1] * rt + src[5][1] * rm + src[8][1] * rb);
              case 1:
                dst[0] = BOXFILTER_ROUND(
                  src[0][0] * lt + src[3][0] * lm + src[6][0] * lb +
                  src[1][0] * ct + src[4][0] * cm + src[7][0] * cb +
                  src[2][0] * rt + src[5][0] * rm + src[8][0] * rb);
                break;
              default:
              for (gint i = 0; i < components; ++i)
                {
                  dst[i] = BOXFILTER_ROUND(
                    src[0][i] * lt + src[3][i] * lm + src[6][i] * lb +
                    src[1][i] * ct + src[4][i] * cm + src[7][i] * cb +
                    src[2][i] * rt + src[5][i] * rm + src[8][i] * rb);
                }
            }
          }

          dst += components;
        }
    }
}
