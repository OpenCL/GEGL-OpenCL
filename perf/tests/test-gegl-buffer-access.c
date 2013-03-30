#include "test-common.h"

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer    *buffer;
  GeglRectangle  bound = {0, 0, 1024, 1024};
  gchar *buf;
  gint i;

  gegl_init (NULL, NULL);
  buffer = gegl_buffer_new (&bound, babl_format ("RGBA float"));
  buf = g_malloc0 (bound.width * bound.height * 16);

#define ITERATIONS 8

  test_start ();
  for (i=0;i<ITERATIONS;i++)
    {
      gegl_buffer_set (buffer, &bound, NULL, buf, GEGL_AUTO_ROWSTRIDE);
     }
  test_end ("gegl_buffer_set", bound.width * bound.height * ITERATIONS * 16);


  test_start ();
  for (i=0;i<ITERATIONS;i++)
    {
      gegl_buffer_get (buffer, 1.0, &bound, NULL, buf, GEGL_AUTO_ROWSTRIDE);
     }
  test_end ("gegl_buffer_get", bound.width * bound.height * ITERATIONS * 16);

  g_free (buf);
  gegl_buffer_destroy (buffer);

  return 0;
}
