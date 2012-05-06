TEST ()
{
  GeglBuffer    *buffer;
  GeglRectangle  orig_extent = {0, 0, 20, 20};
  GeglRectangle  new_extent = {0, 0, 50, 50};

  test_start ();

  buffer = gegl_buffer_new(&orig_extent, babl_format ("Y float"));
  gegl_buffer_set_extent(buffer, &new_extent);

  fill (buffer, 0.5);
  print_buffer (buffer);
  g_object_unref (buffer);

  test_end ();
}
