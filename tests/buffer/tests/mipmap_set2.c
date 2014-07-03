TEST ()
{
  GeglBuffer    *buffer2, *buffer;
  GeglRectangle  bound = {0, 0, 20, 20};
  //GeglRectangle  source = {2, 2, 5, 5};
  GeglRectangle  dest = {4, 4, 5, 5};
  float *blank = g_malloc0 (100000);
  gchar *temp = g_malloc0 (100000);
  test_start ();

  buffer2 = gegl_buffer_new (&bound, babl_format ("Y float"));
  buffer = gegl_buffer_new (&bound, babl_format ("Y float"));

  vgrad (buffer2);
  blank[0] = 0.5;
  blank[1] = 0.25;
  blank[2] = 1.0;
  blank[3] = 1.0;
  blank[4] = 1.0;
  blank[5] = 0.2;

  /* we need to expand the width/height to compensate for the level */
  dest.width  *= 2;
  dest.height *= 2;
  gegl_buffer_set (buffer2, &dest, 1, babl_format ("Y float"), blank, GEGL_AUTO_ROWSTRIDE);
  print_buffer (buffer2);

  gegl_buffer_get (buffer2, &bound, 0.5, babl_format ("Y float"), temp, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  gegl_buffer_set (buffer, &bound, 0, babl_format ("Y float"), temp, GEGL_AUTO_ROWSTRIDE);

  print_buffer (buffer);

  gegl_buffer_get (buffer2, &bound, 0.25, babl_format ("Y float"), temp, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  gegl_buffer_set (buffer, &bound, 0, babl_format ("Y float"), temp, GEGL_AUTO_ROWSTRIDE);

  print_buffer (buffer);

  test_end ();

  g_object_unref (buffer);
  g_object_unref (buffer2);

}
