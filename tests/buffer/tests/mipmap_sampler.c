TEST ()
{
  GeglBuffer    *buffer2, *buffer;
  GeglRectangle  bound = {0, 0, 20, 20};
  GeglRectangle  dest = {0, 0, 16, 16};
  GeglSampler   *sampler;
  gchar *temp = g_malloc0 (100000);
  test_start ();

  buffer2 = gegl_buffer_new (&bound, babl_format ("Y float"));
  buffer = gegl_buffer_new (&bound, babl_format ("Y float"));

// vgrad (buffer2);

  {
    GeglBufferIterator *iterator = gegl_buffer_iterator_new (buffer2,
        &dest, 1, babl_format ("Y float"), GEGL_BUFFER_READWRITE, GEGL_ABYSS_NONE);
    while (gegl_buffer_iterator_next (iterator))
    {
      int i;
      gfloat *d = iterator->data[0];
      for (i = 0; i < iterator->length; i++)
        d[i] = 1.0 * i / iterator->length / 2;
    }
  }
  {
    GeglBufferIterator *iterator = gegl_buffer_iterator_new (buffer2,
        &dest, 1, babl_format ("Y float"), GEGL_BUFFER_READWRITE, GEGL_ABYSS_NONE);
    while (gegl_buffer_iterator_next (iterator))
    {
      int i;
      gfloat *d = iterator->data[0];
      for (i = 0; i < iterator->length; i++)
        d[i] += (1.0 * i / iterator->length/2);
    }
  }
  print_buffer (buffer2);

  sampler = gegl_buffer_sampler_new_at_level (buffer2, babl_format("Y float"),
                                              GEGL_SAMPLER_LINEAR, 1);

  {
    float coords[][2] = {{4,5}, {6,4},{8,10}};
    int i;
    float val;
    for (i = 0; i < sizeof(coords)/sizeof(coords[0]); i++)
    {
      gegl_sampler_get (sampler, coords[i][0], coords[i][1], NULL,
                        &val, GEGL_ABYSS_NONE);

      g_string_append_printf(gstring, 
          "%f,%f : %f\n", coords[i][0], coords[i][1], val);

    }
  }

  g_object_unref (sampler);

  gegl_buffer_get (buffer2, &bound, 0.5, babl_format ("Y float"), temp, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  gegl_buffer_set (buffer, &bound, 0, babl_format ("Y float"), temp, GEGL_AUTO_ROWSTRIDE);

  print_buffer (buffer);

  g_object_unref (buffer2);
  g_object_unref (buffer);

  test_end ();

}
