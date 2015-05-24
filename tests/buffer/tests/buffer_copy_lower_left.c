TEST ()
{
  GeglBuffer    *buffer, *buffer2;
  GeglRectangle  bound = {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&bound, babl_format ("Y float"));
  vgrad (buffer);
  {
    GeglRectangle rect = *gegl_buffer_get_extent(buffer);

    rect.x=0;
    rect.y=10;
    rect.width-=10;
    rect.height-=10;
  buffer2 = gegl_buffer_new (gegl_buffer_get_extent (buffer), gegl_buffer_get_format (buffer));
  gegl_buffer_copy (buffer, &rect, GEGL_ABYSS_NONE, buffer2, &rect);
  }
  print_buffer (buffer2);
  g_object_unref (buffer);
  g_object_unref (buffer2);
  test_end ();
}
