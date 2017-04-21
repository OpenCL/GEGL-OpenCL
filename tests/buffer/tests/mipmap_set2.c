TEST ()
{
  GeglBuffer    *buffer2, *buffer;
  GeglRectangle  bound = {0, 0, 20, 20};
  GeglRectangle  dest = {2, 2, 8, 8};
  float *blank = g_malloc0 (100000);
  gchar *temp = g_malloc0 (100000);
  test_start ();


  buffer2 = gegl_buffer_new (&bound, babl_format ("Y float"));
  buffer = gegl_buffer_new (&bound, babl_format ("Y float"));

  vgrad (buffer2);
#if 1
  blank[0] = 0.1;
  blank[1] = 0.3;
  blank[2] = 0.7;
  blank[3] = 1.0;
  blank[4] = 0.1;
  blank[5] = 0.3;
  blank[6] = 0.8;
  blank[7] = 1.0;
  blank[8] = 0.1;
  blank[9] = 0.3;
  blank[10] = 0.7;
  blank[11] = 1.0;
  blank[12] = 0.1;
  blank[13] = 0.3;
  blank[14] = 0.7;
  blank[15] = 1.0;
#endif

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
