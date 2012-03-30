TEST ()
{
  GeglBuffer    *buffer, *sub, *subsub;
  GeglRectangle  subrect =    {5, 5, 10, 10};
  GeglRectangle  rect =       {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));

  sub = gegl_buffer_create_sub_buffer (buffer, &subrect);
  fill (sub, 0.5);
  subsub = g_object_new (GEGL_TYPE_BUFFER,
                         "source", sub,
                         "x", 5,
                         "y", 5,
                         "width", 4,
                         "height", 4,
                         "shift-y", 5,
                         "shift-x", 0,
                         NULL);

  fill (subsub, 1.0);
  print_buffer (buffer);
  g_object_unref (sub);
  g_object_unref (subsub);
  g_object_unref (buffer);
  test_end ();
}
