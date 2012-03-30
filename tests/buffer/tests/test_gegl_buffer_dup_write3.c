TEST ()
{
  GeglBuffer    *buffer, *buffer2;
  GeglRectangle  bound = {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&bound, babl_format ("Y float"));
  vgrad (buffer);
  buffer2 = gegl_buffer_dup (buffer);
  checkerboard (buffer2, 3, 0.0, 1.0);
  print_buffer (buffer);
  g_object_unref (buffer);
  g_object_unref (buffer2);
  test_end ();
}
