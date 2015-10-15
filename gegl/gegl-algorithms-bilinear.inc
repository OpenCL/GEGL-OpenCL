void
BILINEAR_FUNCNAME (guchar              *dest_buf,
                   const guchar        *source_buf,
                   const GeglRectangle *dst_rect,
                   const GeglRectangle *src_rect,
                   const gint           s_rowstride,
                   const gdouble        scale,
                   const gint           bpp,
                   const gint           d_rowstride)
{
  const BILINEAR_TYPE *src[4];
  gint  components = bpp / sizeof(BILINEAR_TYPE);

  gfloat dx[dst_rect->width];
  gint   jj[dst_rect->width];

  for (gint x = 0; x < dst_rect->width; x++)
  {
    gfloat sx  = (dst_rect->x + x + .5) / scale - src_rect->x;
    jj[x]  = int_floorf (sx);
    dx[x]  = sx - jj[x];
    jj[x] *= components;
  }

  for (gint y = 0; y < dst_rect->height; y++)
    {
      const gfloat sy = (dst_rect->y + y + .5) / scale - src_rect->y;
      const gint   ii = int_floorf (sy);
      const gfloat dy = (sy - ii);
      const gfloat rdy = 1.0 - dy;
      BILINEAR_TYPE *dst = (BILINEAR_TYPE*)(dest_buf + y * d_rowstride);
      const guchar  *src_base = source_buf + ii * s_rowstride;

      switch (components)
      {
        case 4:
          for (gint x = 0; x < dst_rect->width; x++)
            {
            src[0] = src[1] = src[2] = src[3] = (const BILINEAR_TYPE*)src_base + jj[x];
            src[1] += 4;
            src[2] += s_rowstride;
            src[3] += s_rowstride + 4;

            if (src[0][3] == 0 &&  /* XXX: it would be even better to not call this at all for the abyss...  */
                src[1][3] == 0 &&
                src[2][3] == 0 &&
                src[3][3] == 0)
            {
              dst[0] = dst[1] = dst[2] = dst[3] = 0;
            }
            else
            {
              dst[0] = BILINEAR_ROUND(
                (src[0][0] * (1.0-dx[x]) + src[1][0] * (dx[x])) * (rdy) +
                (src[2][0] * (1.0-dx[x]) + src[3][0] * (dx[x])) * (dy));
              dst[1] = BILINEAR_ROUND(
                (src[0][1] * (1.0-dx[x]) + src[1][1] * (dx[x])) * (rdy) +
                (src[2][1] * (1.0-dx[x]) + src[3][1] * (dx[x])) * (dy));
              dst[2] = BILINEAR_ROUND(
                (src[0][2] * (1.0-dx[x]) + src[1][2] * (dx[x])) * (rdy) +
                (src[2][2] * (1.0-dx[x]) + src[3][2] * (dx[x])) * (dy));
              dst[3] = BILINEAR_ROUND(
                (src[0][3] * (1.0-dx[x]) + src[1][3] * (dx[x])) * (rdy) +
                (src[2][3] * (1.0-dx[x]) + src[3][3] * (dx[x])) * (dy));
            }
            dst += 4;
            }
          break;
        case 3:
          for (gint x = 0; x < dst_rect->width; x++)
            {
            src[0] = src[1] = src[2] = src[3] = (const BILINEAR_TYPE*)src_base + jj[x];
            src[1] += 3;
            src[2] += s_rowstride;
            src[3] += s_rowstride + 3;
            dst[0] = BILINEAR_ROUND(
              (src[0][0] * (1.0-dx[x]) + src[1][0] * (dx[x])) * (rdy) +
              (src[2][0] * (1.0-dx[x]) + src[3][0] * (dx[x])) * (dy));
            dst[1] = BILINEAR_ROUND(
              (src[0][1] * (1.0-dx[x]) + src[1][1] * (dx[x])) * (rdy) +
              (src[2][1] * (1.0-dx[x]) + src[3][1] * (dx[x])) * (dy));
            dst[2] = BILINEAR_ROUND(
              (src[0][2] * (1.0-dx[x]) + src[1][2] * (dx[x])) * (rdy) +
              (src[2][2] * (1.0-dx[x]) + src[3][2] * (dx[x])) * (dy));
            dst += 3;
            }
          break;
        case 2:
          for (gint x = 0; x < dst_rect->width; x++)
            {
            src[0] = src[1] = src[2] = src[3] = (const BILINEAR_TYPE*)src_base + jj[x];
            src[1] += 2;
            src[2] += s_rowstride;
            src[3] += s_rowstride + 2;
            dst[0] = BILINEAR_ROUND(
              (src[0][0] * (1.0-dx[x]) + src[1][0] * (dx[x])) * (rdy) +
              (src[2][0] * (1.0-dx[x]) + src[3][0] * (dx[x])) * (dy));
            dst[1] = BILINEAR_ROUND(
              (src[0][1] * (1.0-dx[x]) + src[1][1] * (dx[x])) * (rdy) +
              (src[2][1] * (1.0-dx[x]) + src[3][1] * (dx[x])) * (dy));
            dst += 2;
            }
          break;
        case 1:
          for (gint x = 0; x < dst_rect->width; x++)
            {
            src[0] = src[1] = src[2] = src[3] = (const BILINEAR_TYPE*)src_base + jj[x];
            src[1] += 1;
            src[2] += s_rowstride;
            src[3] += s_rowstride + 1;
            dst[0] = BILINEAR_ROUND(
              (src[0][0] * (1.0-dx[x]) + src[1][0] * (dx[x])) * (rdy) +
              (src[2][0] * (1.0-dx[x]) + src[3][0] * (dx[x])) * (dy));
            dst += 1;
            }
          break;
        default:
          for (gint x = 0; x < dst_rect->width; x++)
          {
            src[0] = src[1] = src[2] = src[3] = (const BILINEAR_TYPE*)src_base + jj[x];
            src[1] += components;
            src[2] += s_rowstride;
            src[3] += s_rowstride + components;
            {
              for (gint i = 0; i < components; ++i)
                {
                  dst[i] = BILINEAR_ROUND(
                   (src[0][i] * (1.0-dx[x]) + src[1][i] * (dx[x])) * (rdy) +
                   (src[2][i] * (1.0-dx[x]) + src[3][i] * (dx[x])) * (dy));
                }
              }
            dst += components;
        }
        break;
      }
    }
}
