TEST ()
{
  GeglBuffer    *buffer, *sub, *subsub, *foo;
  GeglRectangle  subrect =    {5, 5, 10, 10};
  GeglRectangle  foor =  {0, 0, 10, 10};
  GeglRectangle  rect =  {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));

  sub = gegl_buffer_create_sub_buffer (buffer, &subrect);
  vgrad (buffer);
  vgrad (sub);
  subsub = g_object_new (GEGL_TYPE_BUFFER,
                         "source", sub,
                         "x", 0,
                         "y", 0,
                         "width", 40,
                         "height", 40,
                         "shift-x", 0,
                         "shift-y", 0,
                         NULL);
  foo = gegl_buffer_create_sub_buffer (subsub, &foor);

  /*fill (subsub, 0.2);*/
  print_buffer (buffer);
  print_buffer (foo);
  g_object_unref (sub);
  g_object_unref (subsub);
  g_object_unref (buffer);
  g_object_unref (foo);
  test_end ();
}
