TEST ()
{
  GeglBuffer   *buffer;
  GeglRectangle extent = {0,0,40,20};
  GeglRectangle roi = {1,1,30,10};
  test_start();
  buffer = gegl_buffer_new (&extent, babl_format ("Y float"));
  fill_rect (buffer, &roi, 0.5);
  roi.y+=3;
  roi.x+=20;

  {
    gint    rowstride;
    gfloat *buf;
    gint    x, y, i;

    buf = gegl_buffer_linear_open (buffer, &extent, &rowstride, NULL);
    g_assert (buf);

    i=0;
    for (y=0;y<extent.height;y++)
      for (x=0;x<extent.width;x++)
        {
          buf[i++]= ((x+y)*1.0) / extent.width;
        }
    gegl_buffer_linear_close (buffer, buf);
  }
  fill_rect (buffer, &roi, 0.2);

  print_buffer (buffer);
  g_object_unref (buffer);
  test_end ();
}
