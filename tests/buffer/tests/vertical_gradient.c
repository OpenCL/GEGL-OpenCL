TEST ()
{
  GeglBuffer    *buffer;
  GeglRectangle  rect = {0, 0, 20, 20};

  test_start();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));
  vgrad (buffer);
  print_buffer (buffer);
  g_object_unref (buffer);
  test_end();
}
