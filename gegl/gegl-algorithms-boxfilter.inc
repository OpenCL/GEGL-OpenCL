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
  const BOXFILTER_TYPE *src[9];
  gint  components = bpp / sizeof(BOXFILTER_TYPE);

  for (gint y = 0; y < dst_rect->height; y++)
    {
      gfloat top_weight, middle_weight, bottom_weight;
      const gfloat sy = (dst_rect->y + y + .5) / scale - src_rect->y;
      const gint     ii = int_floorf (sy);
      BOXFILTER_TYPE             *dst = (BOXFILTER_TYPE*)(dest_buf + y * d_rowstride);
      const guchar  *src_base = source_buf + ii * s_rowstride;

      top_weight    = .5 - scale * (sy - ii);
      top_weight    = MAX (0., top_weight);
      bottom_weight = .5 - scale * ((ii + 1 ) - sy);
      bottom_weight = MAX (0., bottom_weight);
      middle_weight = 1. - top_weight - bottom_weight;

      for (gint x = 0; x < dst_rect->width; x++)
        {
          /* XXX: do the following computations once for first row only */
          gfloat left_weight, center_weight, right_weight;
          const gfloat sx = (dst_rect->x + x + .5) / scale - src_rect->x;
          const gint   jj = int_floorf (sx);

          left_weight   = .5 - scale * (sx - jj);
          left_weight   = MAX (0.0, left_weight);
          right_weight  = .5 - scale * ((jj + 1) - sx);
          right_weight  = MAX (0.0, right_weight);
          center_weight = 1. - left_weight - right_weight;

          src[3] = src[4] = src[5] = (const BOXFILTER_TYPE*)src_base + jj * components;
          src[0] = src[1] = src[2] = src[3] - s_rowstride;
          src[6] = src[7] = src[8] = src[3] + s_rowstride;

          src[2] += components;
          src[5] += components;
          src[8] += components;

          src[0] -= components;
          src[3] -= components;
          src[6] -= components;

          {
            const gfloat lt = left_weight * top_weight;
            const gfloat lm = left_weight * middle_weight;
            const gfloat lb = left_weight * bottom_weight;
            const gfloat ct = center_weight * top_weight;
            const gfloat cm = center_weight * middle_weight;
            const gfloat cb = center_weight * bottom_weight;
            const gfloat rt = right_weight * top_weight;
            const gfloat rm = right_weight * middle_weight;
            const gfloat rb = right_weight * bottom_weight;

            /* XXX: move switch outside scanline loop */
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
