TEST ()
{
  GeglBuffer    *buffer, *buffer2;
  GeglRectangle  bound = {0, 0, 10, 10};
  GeglRectangle  bound2 = {0, 0, 20, 20};
  GeglRectangle  source = {-5, -5, 20, 20};
  GeglRectangle  dest = {0, 0, 20, 20};
  GeglRectangle  fill1 = {0, 0, 3, 3};
  GeglRectangle  fill2 = {3, 3, 7, 7};
  const Babl    *format = babl_format ("Y float");
  gfloat         buf[20 * 20 * sizeof(gfloat)];

  test_start ();
  buffer = gegl_buffer_new (&bound, format);
  buffer2 = gegl_buffer_new (&bound2, format);

  fill_rect (buffer, &fill1, 0.2);
  fill_rect (buffer, &fill2, 0.5);

  gegl_buffer_get (buffer, &source, 1.0, format, buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_LOOP);
  gegl_buffer_set (buffer2, &dest, 1.0, format, buf, GEGL_AUTO_ROWSTRIDE);
  print_buffer (buffer2);
  g_object_unref (buffer);
  g_object_unref (buffer2);
  test_end ();
}
