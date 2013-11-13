void
BOXFILTER_FUNCNAME (guchar              *dest_buf,
                    const guchar        *source_buf,
                    const GeglRectangle *dst_rect,
                    const GeglRectangle *src_rect,
                    const gint           s_rowstride,
                    const gdouble        scale,
                    const gint           components,
                    const gint           d_rowstride)
{
  gdouble  left_weight, center_weight, right_weight;
  gdouble  top_weight, middle_weight, bottom_weight;
  const BOXFILTER_TYPE *src[9];
  gint     x, y;

  for (y = 0; y < dst_rect->height; y++)
    {
      const gdouble  sy = (dst_rect->y + y + .5) / scale - src_rect->y;
      const gint     ii = floor (sy);
      BOXFILTER_TYPE             *dst = (BOXFILTER_TYPE*)(dest_buf + y * d_rowstride);
      const guchar  *src_base = source_buf + ii * s_rowstride;

      top_weight    = MAX (0., .5 - scale * (sy - ii));
      bottom_weight = MAX (0., .5 - scale * ((ii + 1 ) - sy));
      middle_weight = 1. - top_weight - bottom_weight;

      for (x = 0; x < dst_rect->width; x++)
        {
          const gdouble  sx = (dst_rect->x + x + .5) / scale - src_rect->x;
          const gint     jj = floor (sx);

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

            for (gint i = 0; i < components; ++i)
              {
                dst[i] = BOXFILTER_ROUND(
                  src[0][i] * lt + src[3][i] * lm + src[6][i] * lb +
                  src[1][i] * ct + src[4][i] * cm + src[7][i] * cb +
                  src[2][i] * rt + src[5][i] * rm + src[8][i] * rb);
              }
          }

          dst += components;
        }
    }
}
