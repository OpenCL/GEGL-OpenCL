TEST ()
{
  GeglBuffer    *buffer, *sub, *subsub, *subsubsub;
  GeglRectangle  subrect       = {5, 5, 10, 10};
  GeglRectangle  subsubrect    = {3, 3, 14, 14};
  GeglRectangle  subsubsubrect = {5, 3, 2, 2};
  GeglRectangle  rect =       {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));

  vgrad (buffer);

  sub = gegl_buffer_create_sub_buffer (buffer, &subrect);

  fill (sub, 0.5);

  subsub = gegl_buffer_create_sub_buffer (sub, &subsubrect);
  fill (subsub, 1.0);
  subsubsub = gegl_buffer_create_sub_buffer (buffer, &subsubsubrect);
  fill (subsubsub, 1.0);

  print_buffer (buffer);
  g_object_unref (sub);
  g_object_unref (subsub);
  g_object_unref (subsubsub);
  g_object_unref (buffer);
  test_end ();
}
