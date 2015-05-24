TEST ()
{
  GeglBuffer    *buffer, *buffer2;
  GeglRectangle  bound = {0, 0, 20, 20};
  GeglRectangle  source = {2, 2, 5, 5};
  GeglRectangle  dest = {10, 10, 5, 5};
  test_start ();
  buffer = gegl_buffer_new (&bound, babl_format ("Y float"));
  buffer2 = gegl_buffer_new (&bound, babl_format ("Y float"));

  vgrad (buffer);
  gegl_buffer_copy (buffer, &source, GEGL_ABYSS_NONE, buffer2, &dest); 
  print_buffer (buffer2);
  g_object_unref (buffer);
  g_object_unref (buffer2);
  test_end ();
}
