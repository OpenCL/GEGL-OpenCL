TEST ()
{
  GeglBuffer    *buffer, *sub1, *sub2, *sub3;
  GeglRectangle  subrect1 = {5, 5, 10, 10};
  GeglRectangle  subrect2 = {8, 8, 30, 30};
  GeglRectangle  subrect3 = {-2, -2, 24, 24};
  GeglRectangle  rect = {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));
  sub1 = gegl_buffer_create_sub_buffer (buffer, &subrect1);
  sub2 = gegl_buffer_create_sub_buffer (buffer, &subrect2);
  sub3 = gegl_buffer_create_sub_buffer (buffer, &subrect3);

  fill (sub1, 0.5);
  print (("root with sub1 filled in:\n"));
  print_buffer (buffer);

  print (("sub2 before fill:\n"));
  print_buffer (sub2);
  fill (sub2, 1.0);
  print (("final root:\n"));
  print_buffer (buffer);
  print (("final sub1:\n"));
  print_buffer (sub1);
  print (("final sub3:\n"));
  print_buffer (sub3);

  g_object_unref (sub1);
  g_object_unref (sub2);
  g_object_unref (sub3);
  g_object_unref (buffer);
  test_end ();
}
