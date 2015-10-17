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

  gfloat left_weight[dst_rect->width];
  gfloat center_weight[dst_rect->width];
  gfloat right_weight[dst_rect->width];

  gint   jj[dst_rect->width];

  for (gint x = 0; x < dst_rect->width; x++)
  {
    gfloat sx  = (dst_rect->x + x + .5) / scale - src_rect->x;
    jj[x]  = int_floorf (sx);

    left_weight[x]   = .5 - scale * (sx - jj[x]);
    left_weight[x]   = MAX (0.0, left_weight[x]);
    right_weight[x]  = .5 - scale * ((jj[x] + 1) - sx);
    right_weight[x]  = MAX (0.0, right_weight[x]);
    center_weight[x] = 1. - left_weight[x] - right_weight[x];

    jj[x] *= components;
  }

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

      switch (components)
      {
        case 4:
          for (gint x = 0; x < dst_rect->width; x++)
            {
            src[4] = (const BOXFILTER_TYPE*)src_base + jj[x];
            src[1] = (const BOXFILTER_TYPE*)(src_base - s_rowstride) + jj[x];
            src[7] = (const BOXFILTER_TYPE*)(src_base + s_rowstride) + jj[x];
            src[2] = src[1] + 4;
            src[5] = src[4] + 4;
            src[8] = src[7] + 4;
            src[0] = src[1] - 4;
            src[3] = src[4] - 4;
            src[6] = src[7] - 4;

            if (src[0][3] == 0 &&  /* XXX: it would be even better to not call this at all for the abyss...  */
                src[1][3] == 0 &&
                src[2][3] == 0 &&
                src[3][3] == 0 &&
                src[4][3] == 0 &&
                src[5][3] == 0 &&
                src[6][3] == 0 &&
                src[7][3] == 0)
            {
              dst[0] = dst[1] = dst[2] = dst[3] = 0;
            }
            else
            {
              const gfloat lt = left_weight[x] * top_weight;
              const gfloat lm = left_weight[x] * middle_weight;
              const gfloat lb = left_weight[x] * bottom_weight;
              const gfloat ct = center_weight[x] * top_weight;
              const gfloat cm = center_weight[x] * middle_weight;
              const gfloat cb = center_weight[x] * bottom_weight;
              const gfloat rt = right_weight[x] * top_weight;
              const gfloat rm = right_weight[x] * middle_weight;
              const gfloat rb = right_weight[x] * bottom_weight;

                dst[0] = BOXFILTER_ROUND(
                  src[0][0] * lt + src[3][0] * lm + src[6][0] * lb +
                  src[1][0] * ct + src[4][0] * cm + src[7][0] * cb +
                  src[2][0] * rt + src[5][0] * rm + src[8][0] * rb);
                dst[1] = BOXFILTER_ROUND(
                  src[0][1] * lt + src[3][1] * lm + src[6][1] * lb +
                  src[1][1] * ct + src[4][1] * cm + src[7][1] * cb +
                  src[2][1] * rt + src[5][1] * rm + src[8][1] * rb);
                dst[2] = BOXFILTER_ROUND(
                  src[0][2] * lt + src[3][2] * lm + src[6][2] * lb +
                  src[1][2] * ct + src[4][2] * cm + src[7][2] * cb +
                  src[2][2] * rt + src[5][2] * rm + src[8][2] * rb);
                dst[3] = BOXFILTER_ROUND(
                  src[0][3] * lt + src[3][3] * lm + src[6][3] * lb +
                  src[1][3] * ct + src[4][3] * cm + src[7][3] * cb +
                  src[2][3] * rt + src[5][3] * rm + src[8][3] * rb);
              }
            dst += 4;
            }
          break;
        case 3:
          for (gint x = 0; x < dst_rect->width; x++)
            {
            src[4] = (const BOXFILTER_TYPE*)src_base + jj[x];
            src[1] = (const BOXFILTER_TYPE*)(src_base - s_rowstride) + jj[x];
            src[7] = (const BOXFILTER_TYPE*)(src_base + s_rowstride) + jj[x];
            src[2] = src[1] + 3;
            src[5] = src[4] + 3;
            src[8] = src[7] + 3;
            src[0] = src[1] - 3;
            src[3] = src[4] - 3;
            src[6] = src[7] - 3;
            {
              const gfloat lt = left_weight[x] * top_weight;
              const gfloat lm = left_weight[x] * middle_weight;
              const gfloat lb = left_weight[x] * bottom_weight;
              const gfloat ct = center_weight[x] * top_weight;
              const gfloat cm = center_weight[x] * middle_weight;
              const gfloat cb = center_weight[x] * bottom_weight;
              const gfloat rt = right_weight[x] * top_weight;
              const gfloat rm = right_weight[x] * middle_weight;
              const gfloat rb = right_weight[x] * bottom_weight;
              dst[0] = BOXFILTER_ROUND(
                src[0][0] * lt + src[3][0] * lm + src[6][0] * lb +
                src[1][0] * ct + src[4][0] * cm + src[7][0] * cb +
                src[2][0] * rt + src[5][0] * rm + src[8][0] * rb);
              dst[1] = BOXFILTER_ROUND(
                src[0][1] * lt + src[3][1] * lm + src[6][1] * lb +
                src[1][1] * ct + src[4][1] * cm + src[7][1] * cb +
                src[2][1] * rt + src[5][1] * rm + src[8][1] * rb);
              dst[2] = BOXFILTER_ROUND(
                src[0][2] * lt + src[3][2] * lm + src[6][2] * lb +
                src[1][2] * ct + src[4][2] * cm + src[7][2] * cb +
                src[2][2] * rt + src[5][2] * rm + src[8][2] * rb);
            }
            dst += 3;
            }
          break;
        case 2:
          for (gint x = 0; x < dst_rect->width; x++)
            {
            src[4] = (const BOXFILTER_TYPE*)src_base + jj[x];
            src[1] = (const BOXFILTER_TYPE*)(src_base - s_rowstride) + jj[x];
            src[7] = (const BOXFILTER_TYPE*)(src_base + s_rowstride) + jj[x];
            src[2] = src[1] + 2;
            src[5] = src[4] + 2;
            src[8] = src[7] + 2;
            src[0] = src[1] - 2;
            src[3] = src[4] - 2;
            src[6] = src[7] - 2;
            {
              const gfloat lt = left_weight[x] * top_weight;
              const gfloat lm = left_weight[x] * middle_weight;
              const gfloat lb = left_weight[x] * bottom_weight;
              const gfloat ct = center_weight[x] * top_weight;
              const gfloat cm = center_weight[x] * middle_weight;
              const gfloat cb = center_weight[x] * bottom_weight;
              const gfloat rt = right_weight[x] * top_weight;
              const gfloat rm = right_weight[x] * middle_weight;
              const gfloat rb = right_weight[x] * bottom_weight;
              dst[0] = BOXFILTER_ROUND(
                src[0][0] * lt + src[3][0] * lm + src[6][0] * lb +
                src[1][0] * ct + src[4][0] * cm + src[7][0] * cb +
                src[2][0] * rt + src[5][0] * rm + src[8][0] * rb);
              dst[1] = BOXFILTER_ROUND(
                src[0][1] * lt + src[3][1] * lm + src[6][1] * lb +
                src[1][1] * ct + src[4][1] * cm + src[7][1] * cb +
                src[2][1] * rt + src[5][1] * rm + src[8][1] * rb);
            }
            dst += 2;
            }
          break;
        case 1:
          for (gint x = 0; x < dst_rect->width; x++)
            {
            src[4] = (const BOXFILTER_TYPE*)src_base + jj[x];
            src[1] = (const BOXFILTER_TYPE*)(src_base - s_rowstride) + jj[x];
            src[7] = (const BOXFILTER_TYPE*)(src_base + s_rowstride) + jj[x];
            src[2] = src[1] + 1;
            src[5] = src[4] + 1;
            src[8] = src[7] + 1;
            src[0] = src[1] - 1;
            src[3] = src[4] - 1;
            src[6] = src[7] - 1;
            {
              const gfloat lt = left_weight[x] * top_weight;
              const gfloat lm = left_weight[x] * middle_weight;
              const gfloat lb = left_weight[x] * bottom_weight;
              const gfloat ct = center_weight[x] * top_weight;
              const gfloat cm = center_weight[x] * middle_weight;
              const gfloat cb = center_weight[x] * bottom_weight;
              const gfloat rt = right_weight[x] * top_weight;
              const gfloat rm = right_weight[x] * middle_weight;
              const gfloat rb = right_weight[x] * bottom_weight;
              dst[0] = BOXFILTER_ROUND(
                src[0][0] * lt + src[3][0] * lm + src[6][0] * lb +
                src[1][0] * ct + src[4][0] * cm + src[7][0] * cb +
                src[2][0] * rt + src[5][0] * rm + src[8][0] * rb);
            }
            dst += 1;
            }
          break;
        default:
          for (gint x = 0; x < dst_rect->width; x++)
          {
            src[4] = (const BOXFILTER_TYPE*)src_base + jj[x];
            src[1] = (const BOXFILTER_TYPE*)(src_base - s_rowstride) + jj[x];
            src[7] = (const BOXFILTER_TYPE*)(src_base + s_rowstride) + jj[x];
            src[2] = src[1] + components;
            src[5] = src[4] + components;
            src[8] = src[7] + components;
            src[0] = src[1] - components;
            src[3] = src[4] - components;
            src[6] = src[7] - components;
            {
              const gfloat lt = left_weight[x] * top_weight;
              const gfloat lm = left_weight[x] * middle_weight;
              const gfloat lb = left_weight[x] * bottom_weight;
              const gfloat ct = center_weight[x] * top_weight;
              const gfloat cm = center_weight[x] * middle_weight;
              const gfloat cb = center_weight[x] * bottom_weight;
              const gfloat rt = right_weight[x] * top_weight;
              const gfloat rm = right_weight[x] * middle_weight;
              const gfloat rb = right_weight[x] * bottom_weight;
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
        break;
      }
    }
}
