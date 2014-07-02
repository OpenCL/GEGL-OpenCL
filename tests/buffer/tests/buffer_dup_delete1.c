TEST ()
{
  GeglBuffer    *buffer, *buffer2;
  GeglRectangle  bound = {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&bound, babl_format ("Y float"));
  vgrad (buffer);
  buffer2 = gegl_buffer_dup (buffer);
  g_object_unref (buffer);
  print_buffer (buffer2);
  g_object_unref (buffer2);
  test_end ();
}
