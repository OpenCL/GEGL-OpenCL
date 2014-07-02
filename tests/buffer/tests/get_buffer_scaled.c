TEST ()
{
  GeglBuffer    *buffer;
  GeglRectangle  rect = {0, 0, 50, 50};
  GeglRectangle  getrect = {0, 0, 12, 8};
  guchar        *buf;

  test_start ();

  buffer = gegl_buffer_new (&rect, babl_format ("Y u8"));
  checkerboard (buffer, 2, 0.0, 1.0);
  buf = g_malloc (getrect.width*getrect.height*sizeof(gfloat));

    {
      gint i;

      for (i=0; i<10; i++)
        {
          getrect.x=i;
          /*getrect.y=i;*/
          gegl_buffer_get (buffer, &getrect, 1.2, babl_format ("Y u8"), buf, 0,
                           GEGL_ABYSS_NONE);
          print_linear_buffer_u8 (getrect.width, getrect.height, buf);
        }
    }

  g_object_unref (buffer);

  g_free (buf);
  test_end ();
}
