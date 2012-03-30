TEST ()
{
  GeglBuffer   *buffer;
  GeglBuffer   *sub;
  GeglRectangle extent = {0,0,20,20};
  GeglRectangle sextent = {2,2,20,20};
  GeglRectangle roi = {0,0,20,20};
  test_start();
  buffer = gegl_buffer_new (&extent, babl_format ("Y float"));
  sub = gegl_buffer_create_sub_buffer (buffer, &sextent);

  fill_rect (sub, &roi, 0.5);
  print_buffer (buffer);
  g_object_unref (sub);
  g_object_unref (buffer);
  test_end ();
}
