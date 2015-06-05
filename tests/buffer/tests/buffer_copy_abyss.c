TEST ()
{
  GeglBuffer    *buffer, *buffer2;
  GeglRectangle  bound = {0, 0, 20, 20};
  GeglRectangle  source = {15, 15, 10, 10};
  GeglRectangle  dest1 = {10, 10, 10, 10};
  GeglRectangle  dest2 = {0, 0, 10, 10};
  test_start ();
  buffer = gegl_buffer_new (&bound, babl_format ("Y float"));
  buffer2 = gegl_buffer_new (&bound, babl_format ("Y float"));

  vgrad (buffer);
  gegl_buffer_copy (buffer, &source, GEGL_ABYSS_NONE, buffer2, &dest1);
  gegl_buffer_copy (buffer, &source, GEGL_ABYSS_CLAMP, buffer2, &dest2);
  print_buffer (buffer2);
  g_object_unref (buffer);
  g_object_unref (buffer2);
  test_end ();
}
