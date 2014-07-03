TEST ()
{
  GeglBuffer    *buffer, *buffer2;
  GeglRectangle  bound = {0, 0, 20, 20};
  GeglRectangle  dest = {10, 10, 5, 5};
  const gchar *blank = g_malloc0 (100000);
  test_start ();

  buffer = gegl_buffer_new (&bound, babl_format ("Y float"));
  buffer2 = gegl_buffer_new (&bound, babl_format ("Y float"));

  vgrad (buffer2);

  gegl_buffer_set (buffer2, &dest, 0, babl_format ("Y float"), blank, GEGL_AUTO_ROWSTRIDE);

  print_buffer (buffer2);
  vgrad (buffer2);

  gegl_buffer_set (buffer2, &dest, 1, babl_format ("Y float"), blank, GEGL_AUTO_ROWSTRIDE);

  print_buffer (buffer2);

  g_object_unref (buffer);
  g_object_unref (buffer2);
  test_end ();
}
