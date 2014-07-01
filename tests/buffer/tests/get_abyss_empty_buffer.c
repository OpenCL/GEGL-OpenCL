TEST ()
{
  GeglAbyssPolicy  abyss_types[] = { GEGL_ABYSS_NONE,
                                     GEGL_ABYSS_CLAMP,
                                     GEGL_ABYSS_LOOP,
                                     GEGL_ABYSS_BLACK,
                                     GEGL_ABYSS_WHITE };
  const gchar     *abyss_names[] = { "NONE",
                                     "CLAMP",
                                     "LOOP",
                                     "BLACK",
                                     "WHITE" };
  gint            i;
  GeglBuffer     *buffer, *buffer2;
  const Babl     *format = babl_format ("Y float");
  GeglRectangle   query_rect = {-10, -10, 20, 20};
  GeglRectangle   buffer_rect = {0, 0, 0, 0};
  gfloat          buf[query_rect.width * query_rect.height * sizeof(gfloat)];

  test_start ();
  buffer  = gegl_buffer_new (&buffer_rect, format);
  buffer2 = gegl_buffer_new (&query_rect, format);

  for (i = 0; i < G_N_ELEMENTS(abyss_types); ++i)
    {
      print (("%s\n", abyss_names[i]));

      gegl_buffer_get (buffer, &query_rect, 1.0, format, buf, GEGL_AUTO_ROWSTRIDE, abyss_types[i]);
      gegl_buffer_set (buffer2, &query_rect, 0, format, buf, GEGL_AUTO_ROWSTRIDE);

      print_buffer (buffer2);
    }

  g_object_unref (buffer);
  g_object_unref (buffer2);

  test_end ();
}
