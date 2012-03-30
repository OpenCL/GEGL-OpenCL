TEST ()
{
  GeglBuffer   *buffer;
  GeglRectangle extent = {0,0,20,20};
  GeglRectangle roi = {0,0,10,10};
  test_start();
  buffer = gegl_buffer_new (&extent, babl_format ("Y float"));
  fill_rect (buffer, &roi, 0.5);
  print_buffer (buffer);
  g_object_unref (buffer);
  test_end ();
}
