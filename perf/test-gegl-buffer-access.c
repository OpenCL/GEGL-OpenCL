#include "test-common.h"

#define BPP 16

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer    *buffer;
  GeglRectangle  bound = {0, 0, 4024, 4024};
  const Babl *format;
  gchar *buf;
  gint i;

  gegl_init (NULL, NULL);
  format = babl_format ("RGBA float");
  buffer = gegl_buffer_new (&bound, format);
  buf = g_malloc0 (bound.width * bound.height * BPP);

#define ITERATIONS 4

  /* pre-initialize */
  gegl_buffer_set (buffer, &bound, 0, NULL, buf, GEGL_AUTO_ROWSTRIDE);

  test_start ();
  for (i=0;i<ITERATIONS;i++)
    {
      gegl_buffer_get (buffer, &bound, 1.0, NULL, buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
     }
  test_end ("gegl_buffer_get", bound.width * bound.height * ITERATIONS * BPP);


  test_start ();
  for (i=0;i<ITERATIONS;i++)
    {
      gegl_buffer_set (buffer, &bound, 0, NULL, buf, GEGL_AUTO_ROWSTRIDE);
     }
  test_end ("gegl_buffer_set", bound.width * bound.height * ITERATIONS * BPP);


  format = babl_format ("RGBA float");

  {
#define SAMPLES 150000
    gint rands[SAMPLES*2];

  for (i = 0; i < SAMPLES; i ++)
  {
    rands[i*2]   = rand()%1000;
    rands[i*2+1] = rand()%1000;
    rands[i*2]   = i / 1000;
    rands[i*2+1] = i % 1000;
  }

  test_start ();
  for (i = 0; i < ITERATIONS; i++)
  {
    int j;
    for (j = 0; j < SAMPLES; j ++)
    {
      int x = rands[j*2];
      int y = rands[j*2+1];
      float px[4] = {0.2, 0.4, 0.1, 0.5};
#if 1
      GeglRectangle rect = {x, y, 1, 1};
      gegl_buffer_set (buffer, &rect, 0, format, (void*)&px[0], GEGL_AUTO_ROWSTRIDE);
#else
      gegl_buffer_set_pixel (buffer, x, y, format, (void*)&px[0], 3);
#endif
    }
  }
  test_end ("gegl_buffer_set 1x1", SAMPLES * ITERATIONS * BPP);

  test_start ();
  for (i = 0; i < ITERATIONS; i++)
  {
    int j;
    float px[4] = {0.2, 0.4, 0.1, 0.5};
    for (j = 0; j < SAMPLES; j ++)
    {
      int x = rands[j*2];
      int y = rands[j*2+1];
      GeglRectangle rect = {x, y, 1, 1};
      gegl_buffer_get (buffer, &rect, 1.0, format, (void*)&px[0],
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
    }
  }
  test_end ("gegl_buffer_get 1x1", SAMPLES * ITERATIONS * BPP);

  test_start ();
  for (i = 0; i < ITERATIONS; i++)
  {
    int j;
    float px[4] = {0.2, 0.4, 0.1, 0.5};
    for (j = 0; j < SAMPLES; j ++)
    {
      int x = rands[j*2];
      int y = rands[j*2+1];
      gegl_buffer_sample (buffer, x, y, NULL, (void*)&px[0], format,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE); 
    }
  }
  test_end ("gegl_buffer_sample nearest", SAMPLES * ITERATIONS * BPP);

  }

  g_free (buf);
  g_object_unref (buffer);

  return 0;
}
