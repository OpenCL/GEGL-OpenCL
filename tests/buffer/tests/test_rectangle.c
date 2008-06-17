TEST ()
{
  GeglBuffer    *buffer;
  GeglRectangle  rect = {0, 0, 20, 20};

  test_start ();

  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));
  fill (buffer, 0.5);
  rectangle (buffer, 5,5, 10, 10, 0.0);
  print_buffer (buffer);
  gegl_buffer_destroy (buffer);

  test_end ();
}
