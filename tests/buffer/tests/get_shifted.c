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
  gegl_buffer_destroy (sub);
  gegl_buffer_destroy (subsub);
  gegl_buffer_destroy (buffer);
  gegl_buffer_destroy (foo);
  test_end ();
}
