/* this functionality is not accesible through the normal (non gobject
 * constructor properties based) API
 */

TEST ()
{
  GeglBuffer    *buffer, *sub, *subsub;
  GeglRectangle  subrect =    {5, 5, 10, 10};
  GeglRectangle  subsubrect = {3, 3, 4, 4};
  GeglRectangle  rect =       {0, 0, 20, 20};
  test_start ();
  buffer = gegl_buffer_new (&rect, babl_format ("Y float"));

  sub = g_object_new (GEGL_TYPE_BUFFER,
                         "source", buffer,
                         "x", subrect.x,
                         "y", subrect.y,
                         "width", subrect.width,
                         "height", subrect.height,
                         "abyss-width", -1,
                         "abyss-height", -1,
                         NULL);
  fill (sub, 0.5);
  subsub = g_object_new (GEGL_TYPE_BUFFER,
                         "source", sub,
                         "x", subsubrect.x,
                         "y", subsubrect.y,
                         "width", subsubrect.width,
                         "height", subsubrect.height,
                         "abyss-width", -1,
                         "abyss-height", -1,
                         NULL);

  fill (subsub, 1.0);
  print_buffer (buffer);
  g_object_unref (sub);
  g_object_unref (subsub);
  g_object_unref (buffer);
  test_end ();
}
