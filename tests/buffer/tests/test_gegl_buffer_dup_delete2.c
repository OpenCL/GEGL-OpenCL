TEST ()
{
  GeglBuffer    *buffer, *buffer2;
  GeglRectangle  bound = {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&bound, babl_format ("Y float"));
  vgrad (buffer);
  buffer2 = gegl_buffer_dup (buffer);
  gegl_buffer_destroy (buffer2);
  print_buffer (buffer);
  gegl_buffer_destroy (buffer);
  test_end ();
}
