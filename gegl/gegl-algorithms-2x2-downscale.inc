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
  const gint components = bpp / sizeof(DOWNSCALE_TYPE);

  if (!src_data || !dst_data)
    return;

  for (y = 0; y < src_height / 2; y++)
    {
      gint    x;
      guchar *src = src_data;
      guchar *dst = dst_data;

      for (x = 0; x < src_width / 2; x++)
        {
          gint i;

          for (i = 0; i < components; i++)
            {
              DOWNSCALE_SUM aa = ((DOWNSCALE_TYPE *)(src))[i];
              DOWNSCALE_SUM ab = ((DOWNSCALE_TYPE *)(src + bpp))[i];
              DOWNSCALE_SUM ba = ((DOWNSCALE_TYPE *)(src + src_rowstride))[i];
              DOWNSCALE_SUM bb = ((DOWNSCALE_TYPE *)(src + src_rowstride + bpp))[i];

              ((DOWNSCALE_TYPE *)dst)[i] = (aa + ab + ba + bb) / DOWNSCALE_DIVISOR;
            }

          dst += bpp;
          src += bpp * 2;
        }

      dst_data += dst_rowstride;
      src_data += 2 * src_rowstride;
    }
}