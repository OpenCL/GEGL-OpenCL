#define S(a)   ((DOWNSCALE_SUM)(a))

void
DOWNSCALE_FUNCNAME (gint    bpp,
                    gint    src_width,
                    gint    src_height,
                    guchar *src_data,
                    gint    src_rowstride,
                    guchar *dst_data,
                    gint    dst_rowstride)
{
  gint y;
  gint diag = src_rowstride + bpp;
  const gint components = bpp / sizeof(DOWNSCALE_TYPE);

  if (!src_data || !dst_data)
    return;

  switch (components)
  {
    case 1:
      for (y = 0; y < src_height / 2; y++)
        {
          gint    x;
          guchar *src = src_data + src_rowstride * y * 2;
          guchar *dst = dst_data + dst_rowstride * y;

          for (x = 0; x < src_width / 2; x++)
            {
              DOWNSCALE_TYPE * aa = ((DOWNSCALE_TYPE *)(src));
              DOWNSCALE_TYPE * ab = ((DOWNSCALE_TYPE *)(src + bpp));
              DOWNSCALE_TYPE * ba = ((DOWNSCALE_TYPE *)(src + src_rowstride));
              DOWNSCALE_TYPE * bb = ((DOWNSCALE_TYPE *)(src + diag));

              ((DOWNSCALE_TYPE *)dst)[0] = (S(aa[0]) + S(ab[0]) + S(ba[0]) + S(bb[0])) / DOWNSCALE_DIVISOR;

              dst += bpp;
              src += bpp * 2;
            }
        }
    break;
    case 2:
      for (y = 0; y < src_height / 2; y++)
        {
          gint    x;
          guchar *src = src_data + src_rowstride * y * 2;
          guchar *dst = dst_data + dst_rowstride * y;

          for (x = 0; x < src_width / 2; x++)
            {
              DOWNSCALE_TYPE * aa = ((DOWNSCALE_TYPE *)(src));
              DOWNSCALE_TYPE * ab = ((DOWNSCALE_TYPE *)(src + bpp));
              DOWNSCALE_TYPE * ba = ((DOWNSCALE_TYPE *)(src + src_rowstride));
              DOWNSCALE_TYPE * bb = ((DOWNSCALE_TYPE *)(src + diag));

              ((DOWNSCALE_TYPE *)dst)[0] = (S(aa[0]) + S(ab[0]) + S(ba[0]) +
                S(bb[0])) / DOWNSCALE_DIVISOR;
              ((DOWNSCALE_TYPE *)dst)[1] = (S(aa[1]) + S(ab[1]) + S(ba[1]) +
                  S(bb[1])) / DOWNSCALE_DIVISOR;

              dst += bpp;
              src += bpp * 2;
            }
        }
    break;
    case 3:
      for (y = 0; y < src_height / 2; y++)
        {
          gint    x;
          guchar *src = src_data + src_rowstride * y * 2;
          guchar *dst = dst_data + dst_rowstride * y;

          for (x = 0; x < src_width / 2; x++)
            {
              DOWNSCALE_TYPE * aa = ((DOWNSCALE_TYPE *)(src));
              DOWNSCALE_TYPE * ab = ((DOWNSCALE_TYPE *)(src + bpp));
              DOWNSCALE_TYPE * ba = ((DOWNSCALE_TYPE *)(src + src_rowstride));
              DOWNSCALE_TYPE * bb = ((DOWNSCALE_TYPE *)(src + diag));

              ((DOWNSCALE_TYPE *)dst)[0] =
               (S(aa[0]) + S(ab[0]) + S(ba[0]) + S(bb[0])) / DOWNSCALE_DIVISOR;
              ((DOWNSCALE_TYPE *)dst)[1] =
               (S(aa[1]) + S(ab[1]) + S(ba[1]) + S(bb[1])) / DOWNSCALE_DIVISOR;
              ((DOWNSCALE_TYPE *)dst)[2] =
               (S(aa[2]) + S(ab[2]) + S(ba[2]) + S(bb[2])) / DOWNSCALE_DIVISOR;

              dst += bpp;
              src += bpp * 2;
            }
        }
    break;
    case 4:
      for (y = 0; y < src_height / 2; y++)
        {
          gint    x;
          guchar *src = src_data + src_rowstride * y * 2;
          guchar *dst = dst_data + dst_rowstride * y;

          for (x = 0; x < src_width / 2; x++)
            {
              DOWNSCALE_TYPE * aa = ((DOWNSCALE_TYPE *)(src));
              DOWNSCALE_TYPE * ab = ((DOWNSCALE_TYPE *)(src + bpp));
              DOWNSCALE_TYPE * ba = ((DOWNSCALE_TYPE *)(src + src_rowstride));
              DOWNSCALE_TYPE * bb = ((DOWNSCALE_TYPE *)(src + diag));

              ((DOWNSCALE_TYPE *)dst)[0] =
               (S(aa[0]) + S(ab[0]) + S(ba[0]) + S(bb[0])) / DOWNSCALE_DIVISOR;
              ((DOWNSCALE_TYPE *)dst)[1] =
               (S(aa[1]) + S(ab[1]) + S(ba[1]) + S(bb[1])) / DOWNSCALE_DIVISOR;
              ((DOWNSCALE_TYPE *)dst)[2] =
               (S(aa[2]) + S(ab[2]) + S(ba[2]) + S(bb[2])) / DOWNSCALE_DIVISOR;
              ((DOWNSCALE_TYPE *)dst)[3] =
               (S(aa[3]) + S(ab[3]) + S(ba[3]) + S(bb[3])) / DOWNSCALE_DIVISOR;

              dst += bpp;
              src += bpp * 2;
            }
        }
    break;
    default:
      for (y = 0; y < src_height / 2; y++)
        {
          gint    x;
          guchar *src = src_data + src_rowstride * y * 2;
          guchar *dst = dst_data + dst_rowstride * y;

          for (x = 0; x < src_width / 2; x++)
            {
              gint i;
              DOWNSCALE_TYPE * aa = ((DOWNSCALE_TYPE *)(src));
              DOWNSCALE_TYPE * ab = ((DOWNSCALE_TYPE *)(src + bpp));
              DOWNSCALE_TYPE * ba = ((DOWNSCALE_TYPE *)(src + src_rowstride));
              DOWNSCALE_TYPE * bb = ((DOWNSCALE_TYPE *)(src + diag));

              for (i = 0; i < components; i++)
                ((DOWNSCALE_TYPE *)dst)[i] =
                  (S(aa[i]) + S(ab[i]) + S(ba[i]) + S(bb[i]))/DOWNSCALE_DIVISOR;

              dst += bpp;
              src += bpp * 2;
            }
        }
    break;
  }
}

#undef S
