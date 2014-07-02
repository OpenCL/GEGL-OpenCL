TEST ()
{
  GeglBuffer    *buffer, *buffer2;
  gchar         *buf2;
  GeglRectangle  bound = { 0, 0, 128, 128 };
  GeglBufferIterator *iter;
  gint x, y;

  test_start ();

  buffer = gegl_buffer_new (&bound, babl_format ("Y float"));
  fill (buffer, 0.5);

  for (y = 0; y < bound.height; y += 64)
    {
      for (x = 0; x < bound.width; x += 64)
        {
          GeglRectangle source = {x, y, 30, 30};

          buf2 = g_malloc0 (source.width * source.height * sizeof (float));

          buffer2 = gegl_buffer_linear_new_from_data (buf2,
                                                      babl_format ("Y float"),
                                                      &source,
                                                      GEGL_AUTO_ROWSTRIDE,
                                                      NULL, NULL);
          fill (buffer2, 1.0);

          iter = gegl_buffer_iterator_new (buffer2, &source, 0, NULL,
                                           GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

          gegl_buffer_iterator_add (iter, buffer, &source, 0, NULL,
                                    GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

          while (gegl_buffer_iterator_next (iter))
            {
              gfloat *s = iter->data[0];
              gfloat *d = iter->data[1];
              gint length = iter->length;

              while (length--)
                *d++ = *s++;
            }

          g_object_unref (buffer2);
        }
    }

  print_buffer (buffer);
  g_object_unref (buffer);
  test_end ();
}
