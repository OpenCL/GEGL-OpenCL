TEST ()
{
  /* Makes sure it is possible to save a small roi in a buffer with a
   * bigger extent
   */
  GeglBuffer    *buffer = NULL;
  GeglRectangle  extent = {-40, -40, 80, 80};
  GeglRectangle  roi = {0, 0, 20, 20};
  gchar         *path = NULL;

  test_start ();

  /* Create */
  buffer = gegl_buffer_new (&extent, babl_format ("Y float"));
  fill (buffer, 0.5);
  path = g_build_filename (g_get_tmp_dir (), "gegl-buffer-tmp.gegl", NULL);

  /* Save */
  gegl_buffer_save (buffer, path, &roi);
  g_object_unref (buffer);
  buffer = NULL;

  /* Load */
  buffer = gegl_buffer_load (path);
  print_buffer (buffer);
  g_object_unref (buffer);
  buffer = NULL;

  g_unlink (path);
  g_free (path);
  test_end ();
}
