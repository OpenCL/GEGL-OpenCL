TEST ()
{
  GeglBuffer   *buffer;
  GeglRectangle extent = {0,0, 10, 10};
  gfloat       *buf;
  gint          i;
  test_start();

  buf = g_malloc (sizeof (float) * 10 * 10);
  for (i=0;i<100;i++)
    buf[i]=i/100.0;

  buffer = gegl_buffer_linear_new_from_data (buf, babl_format ("Y float"),
                                             &extent,
                                             10 * 4,
                                             G_CALLBACK(g_free), /* destroy_notify */
                                             NULL   /* destroy_notify_data */);
  print_buffer (buffer);
  test_end ();
  gegl_buffer_destroy (buffer);
}
