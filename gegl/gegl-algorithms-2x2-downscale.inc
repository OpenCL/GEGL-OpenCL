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

              ((DOWNSCALE_TYPE *)dst)[0] = (aa[0] + ab[0] + ba[0] + bb[0]) / DOWNSCALE_DIVISOR;

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

              ((DOWNSCALE_TYPE *)dst)[0] = (aa[0] + ab[0] + ba[0] + bb[0]) / DOWNSCALE_DIVISOR;
              ((DOWNSCALE_TYPE *)dst)[1] = (aa[1] + ab[1] + ba[1] + bb[1]) / DOWNSCALE_DIVISOR;

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

              ((DOWNSCALE_TYPE *)dst)[0] = (aa[0] + ab[0] + ba[0] + bb[0]) / DOWNSCALE_DIVISOR;
              ((DOWNSCALE_TYPE *)dst)[1] = (aa[1] + ab[1] + ba[1] + bb[1]) / DOWNSCALE_DIVISOR;
              ((DOWNSCALE_TYPE *)dst)[2] = (aa[2] + ab[2] + ba[2] + bb[2]) / DOWNSCALE_DIVISOR;

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

              ((DOWNSCALE_TYPE *)dst)[0] = (aa[0] + ab[0] + ba[0] + bb[0]) / DOWNSCALE_DIVISOR;
              ((DOWNSCALE_TYPE *)dst)[1] = (aa[1] + ab[1] + ba[1] + bb[1]) / DOWNSCALE_DIVISOR;
              ((DOWNSCALE_TYPE *)dst)[2] = (aa[2] + ab[2] + ba[2] + bb[2]) / DOWNSCALE_DIVISOR;
              ((DOWNSCALE_TYPE *)dst)[3] = (aa[3] + ab[3] + ba[3] + bb[3]) / DOWNSCALE_DIVISOR;

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
                ((DOWNSCALE_TYPE *)dst)[i] = (aa[i] + ab[i] + ba[i] + bb[i]) / DOWNSCALE_DIVISOR;

              dst += bpp;
              src += bpp * 2;
            }
        }
    break;
  }
}
