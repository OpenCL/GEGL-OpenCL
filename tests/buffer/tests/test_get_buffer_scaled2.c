TEST ()
{
  GeglBuffer    *buffer;
  GeglRectangle  rect = {0, 0, 10, 10};
  GeglRectangle  getrect = {-2, -2, 10, 10};
  guchar        *buf;

  test_start ();

  buffer = gegl_buffer_new (&rect, babl_format ("Y u8"));
  checkerboard (buffer, 2, 0.0, 1.0);
  buf = g_malloc (getrect.width*getrect.height*sizeof(gfloat));

  gegl_buffer_get (buffer, 0.66, &getrect, babl_format ("Y u8"), buf, 0);


  print_linear_buffer_u8 (getrect.width, getrect.height, buf);
  gegl_buffer_destroy (buffer);

  g_free (buf);
  test_end ();
}
