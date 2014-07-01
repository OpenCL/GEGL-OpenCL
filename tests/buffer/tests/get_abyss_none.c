TEST ()
{
  GeglAbyssPolicy abyss_type = GEGL_ABYSS_NONE;
  gint            i, j;
  GeglBuffer     *buffer, *buffer2;
  const Babl     *format = babl_format ("Y float");
  GeglRectangle   query_rect = {0, 0, 13, 13};
  GeglRectangle   buffer_rect = {0, 0, 12, 12};
  gfloat          buf[query_rect.width * query_rect.height * sizeof(gfloat)];

  gint x_offsets[] = {-query_rect.width - 6, -query_rect.width - 0,
                      -query_rect.width + 1, -query_rect.width + 6,
                      -1,
                      buffer_rect.width - 6, buffer_rect.width - 1,
                      buffer_rect.width - 0, buffer_rect.width + 6};

  gint y_offsets[] = {-query_rect.height - 6, -query_rect.height - 0,
                      -query_rect.height + 1, -query_rect.height + 6,
                      -1,
                      buffer_rect.height - 6, buffer_rect.height - 1,
                      buffer_rect.height - 0, buffer_rect.height + 6};

  test_start ();
  buffer = gegl_buffer_new (&buffer_rect, format);
  buffer2 = gegl_buffer_new (&query_rect, format);
  vgrad (buffer);
  // checkerboard (buffer, 2, 0.0f, 1.0f);

  for (i = 0; i < G_N_ELEMENTS(x_offsets); ++i)
    for (j = 0; j < G_N_ELEMENTS(y_offsets); ++j)
    {
      GeglRectangle cur_query_rect = query_rect;
      cur_query_rect.x = cur_query_rect.x + x_offsets[i];
      cur_query_rect.y = cur_query_rect.y + y_offsets[j];

      print (("%d,%d %dx%d\n", cur_query_rect.x, cur_query_rect.y, cur_query_rect.width, cur_query_rect.height));

      gegl_buffer_get (buffer, &cur_query_rect, 1.0, format, buf, GEGL_AUTO_ROWSTRIDE, abyss_type);
      gegl_buffer_set (buffer2, &query_rect, 0, format, buf, GEGL_AUTO_ROWSTRIDE);

      print_buffer (buffer2);
    }

  g_object_unref (buffer);
  g_object_unref (buffer2);

  test_end ();
}
