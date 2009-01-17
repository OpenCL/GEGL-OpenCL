TEST ()
{
  GeglBuffer   *buffer;
  GeglRectangle extent = {0,0, 10, 10};
  gfloat       *buf;
  test_start();

  buf = g_malloc (sizeof (float) * 12 * 10);
  gint i;
  for (i=0;i<120;i++)
    buf[i]=i%12==5?0.5:0.0;

  buffer = gegl_buffer_linear_new_from_data (buf, babl_format_from_name ("Y float"),
                                             &extent,
                                             12 * 4,
                                             G_CALLBACK(g_free), /* destroy_notify */
                                             NULL   /* destroy_notify_data */);
  print_buffer (buffer);
  test_end ();
  gegl_buffer_destroy (buffer);
}
