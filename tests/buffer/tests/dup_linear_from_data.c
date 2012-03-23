TEST ()
{
  GeglBuffer   *buffer, *buffer2;
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
                                             (GDestroyNotify) g_free, /* destroy_notify */
                                             NULL   /* destroy_notify_data */);
  buffer2 = gegl_buffer_dup (buffer);
  gegl_buffer_destroy (buffer2);
  buffer2 = gegl_buffer_dup (buffer);
  vgrad (buffer);
  print_buffer (buffer);
  gegl_buffer_destroy (buffer);
  gegl_buffer_destroy (buffer2);
  test_end ();
}
