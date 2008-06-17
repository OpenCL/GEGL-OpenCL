TEST ()
{
  GeglBuffer   *buffer;
  GeglRectangle extent = {0,0,40,20};
  GeglRectangle roi = {1,1,30,10};
  test_start();
  g_print ("foo!\n");
  buffer = gegl_buffer_linear_new (&extent, babl_format ("Y float"));
  fill_rect (buffer, &roi, 0.5);
  roi.y+=3;
  roi.x+=20;
  fill_rect (buffer, &roi, 0.2);
  print_buffer (buffer);
  test_end ();
  gegl_buffer_destroy (buffer);
}
