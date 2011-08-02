TEST ()
{
  GeglBuffer    *buffer, *buffer2;
  GeglRectangle  bound = {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&bound, babl_format ("Y float"));
  vgrad (buffer);
  buffer2 = gegl_buffer_dup (buffer);
  checkerboard (buffer, 3, 0.0, 1.0);
  print_buffer (buffer2);
  gegl_buffer_destroy (buffer);
  gegl_buffer_destroy (buffer2);
  test_end ();
}
