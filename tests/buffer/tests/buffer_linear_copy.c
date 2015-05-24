TEST ()
{
  GeglBuffer    *buffer, *buffer2;
  gchar         *buf1, *buf2;
  GeglRectangle  bound = {2, 2, 20, 20};
  GeglRectangle  source = {5, 3, 10, 10};

  test_start ();

  buf1 = g_malloc0 (bound.width * bound.height * sizeof (float));
  buf2 = g_malloc0 (source.width * source.height * sizeof (float));

  buffer = gegl_buffer_linear_new_from_data (buf1, babl_format ("Y float"),
                                             &bound, GEGL_AUTO_ROWSTRIDE,
                                             NULL, NULL);
  buffer2 = gegl_buffer_linear_new_from_data (buf2, babl_format ("Y float"),
                                              &source, GEGL_AUTO_ROWSTRIDE,
                                              NULL, NULL);

  vgrad (buffer);
  fill (buffer2, 1.0);

  gegl_buffer_copy (buffer2, &source, GEGL_ABYSS_NONE, buffer, &source);
  print_buffer (buffer);
  g_object_unref (buffer2);
  g_object_unref (buffer);
  test_end ();
}
