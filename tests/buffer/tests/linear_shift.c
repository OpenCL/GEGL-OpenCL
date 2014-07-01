TEST ()
{
  GeglBuffer         *out;
  gint                i;
  GeglBuffer         *linear_a;
  GeglBuffer         *linear_b;
  GeglBuffer         *linear_c;
  GeglBuffer         *linear_d;
  gfloat             *linear_data;
  GeglBufferIterator *iter;
  GeglRectangle       out_extent = {-1, -1, 5, 5};

  test_start();

  linear_data = malloc (sizeof(float) * 3 * 3);

  for (i = 0; i < 3 * 3; ++i)
    linear_data[i] = 0.25;

  linear_a = gegl_buffer_linear_new_from_data (linear_data,
                                               babl_format ("Y float"),
                                               GEGL_RECTANGLE(-1, -1, 3, 3),
                                               GEGL_AUTO_ROWSTRIDE,
                                               NULL,
                                               NULL);

  linear_b = gegl_buffer_linear_new_from_data (linear_data,
                                               babl_format ("Y float"),
                                               GEGL_RECTANGLE(1, -1, 3, 3),
                                               GEGL_AUTO_ROWSTRIDE,
                                               NULL,
                                               NULL);

  linear_c = gegl_buffer_linear_new_from_data (linear_data,
                                               babl_format ("Y float"),
                                               GEGL_RECTANGLE(1, 1, 3, 3),
                                               GEGL_AUTO_ROWSTRIDE,
                                               NULL,
                                               NULL);

  linear_d = gegl_buffer_linear_new_from_data (linear_data,
                                               babl_format ("Y float"),
                                               GEGL_RECTANGLE(-1, 1, 3, 3),
                                               GEGL_AUTO_ROWSTRIDE,
                                               NULL,
                                               NULL);

  out = gegl_buffer_new (&out_extent, babl_format ("Y float"));

  iter = gegl_buffer_iterator_new (out, &out_extent, 0, NULL,
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, linear_a, &out_extent, 0, NULL,
                            GEGL_ACCESS_READ, GEGL_ABYSS_BLACK);

  gegl_buffer_iterator_add (iter, linear_b, &out_extent, 0, NULL,
                            GEGL_ACCESS_READ, GEGL_ABYSS_BLACK);

  gegl_buffer_iterator_add (iter, linear_c, &out_extent, 0, NULL,
                            GEGL_ACCESS_READ, GEGL_ABYSS_BLACK);

  gegl_buffer_iterator_add (iter, linear_d, &out_extent, 0, NULL,
                            GEGL_ACCESS_READ, GEGL_ABYSS_BLACK);

  while (gegl_buffer_iterator_next (iter))
    {
      gint ix, iy, pos;

      pos = 0;
      for (iy = iter->roi[0].y; iy < iter->roi[0].y + iter->roi[0].height; ++iy)
        for (ix = iter->roi[0].x; ix < iter->roi[0].x + iter->roi[0].width; ++ix)
          {
            gfloat **fdata = (gfloat **)iter->data;

            fdata[0][pos] = fdata[1][pos] + fdata[2][pos] + fdata[3][pos] + fdata[4][pos];
            if (fdata[0][pos] > 1.0f)
              fdata[0][pos] = 1.0f;

            ++pos;
          }
    }

  print_buffer (out);
  g_object_unref (out);
  g_object_unref (linear_a);
  g_object_unref (linear_b);
  g_object_unref (linear_c);
  g_object_unref (linear_d);
  free (linear_data);
  test_end ();
}
