TEST ()
{
  GeglBuffer    *buffer, *sub, *subsub;
  GeglRectangle  subrect =    {5, 5, 10, 10};
  GeglRectangle  subsubrect = {3, 3, 4, 4};
  GeglRectangle  rect =       {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));
  sub = gegl_buffer_create_sub_buffer (buffer, &subrect);

  fill (sub, 0.5);

  subsub = gegl_buffer_create_sub_buffer (sub, &subsubrect);
  fill (subsub, 1.0);
  print_buffer (buffer);
  g_object_unref (sub);
  g_object_unref (subsub);
  g_object_unref (buffer);
  test_end ();
}
