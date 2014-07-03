TEST ()
{
  GeglBuffer    *buffer2, *buffer;
  GeglRectangle  bound = {0, 0, 20, 20};
  GeglRectangle  dest = {4, 4, 4, 6};
  float *blank = g_malloc0 (100000);
  gchar *temp = g_malloc0 (100000);
  test_start ();

  buffer2 = gegl_buffer_new (&bound, babl_format ("Y float"));
  buffer = gegl_buffer_new (&bound, babl_format ("Y float"));

  vgrad (buffer2);

  gegl_buffer_set (buffer2, &dest, 1, babl_format ("Y float"), blank, GEGL_AUTO_ROWSTRIDE);
  print_buffer (buffer2);

  gegl_buffer_get (buffer2, &bound, 0.5, babl_format ("Y float"), temp, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  gegl_buffer_set (buffer, &bound, 0, babl_format ("Y float"), temp, GEGL_AUTO_ROWSTRIDE);

  print_buffer (buffer);

  vgrad (buffer2);

  {
    GeglBufferIterator *iterator = gegl_buffer_iterator_new (buffer2,
        &dest, 1, babl_format ("Y float"), GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);
    while (gegl_buffer_iterator_next (iterator))
    {
      int i;
      gfloat *d = iterator->data[0];
      for (i = 0; i < iterator->length; i++)
        d[i] = 0;
    }
  }
  print_buffer (buffer2);

  gegl_buffer_get (buffer2, &bound, 0.5, babl_format ("Y float"), temp, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  gegl_buffer_set (buffer, &bound, 0, babl_format ("Y float"), temp, GEGL_AUTO_ROWSTRIDE);

  print_buffer (buffer);

  g_object_unref (buffer);
  g_object_unref (buffer2);

  test_end ();

}
