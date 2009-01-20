TEST ()
{
  GeglBuffer    *buffer;
  GeglRectangle  source = {2, 2, 2, 2};
  GeglRectangle  dest  = {15, 15, 1, 1};
  GeglRectangle  rect = {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));

  vgrad (buffer);
  gegl_buffer_copy (buffer, &source, buffer, &dest); /* copying to self */
  print_buffer (buffer);
  gegl_buffer_destroy (buffer);
  test_end ();
}
