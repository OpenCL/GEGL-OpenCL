TEST ()
{
  GeglBuffer    *buffer;
  GeglRectangle  source = {2, 2, 2, 2};
  GeglRectangle  dest  = {15, 15, 1, 1};
  GeglRectangle  rect = {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));

  vgrad (buffer);
  gegl_buffer_copy (buffer, &source, GEGL_ABYSS_NONE, buffer, &dest); /* copying to self */
  print_buffer (buffer);
  g_object_unref (buffer);
  test_end ();
}
