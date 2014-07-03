#include "test-common.h"

#define BPP 16
#define ITERATIONS 12

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer    *buffer;
  GeglRectangle  bound = {0, 0, 4024, 4024};
  const Babl *format, *format2;
  gchar *buf;
  gint i;

  gegl_init (NULL, NULL);
  format = babl_format ("RGBA float");
  format2 = babl_format ("R'G'B'A float");
  buffer = gegl_buffer_new (&bound, format);
  buf = g_malloc0 (bound.width * bound.height * BPP);

  /* pre-initialize */
  gegl_buffer_set (buffer, &bound, 0, NULL, buf, GEGL_AUTO_ROWSTRIDE);

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
    float px[4] = {0.2, 0.4, 0.1, 0.5};
    for (j = 0; j < SAMPLES; j ++)
    {
      int x = rands[j*2];
      int y = rands[j*2+1];
      GeglRectangle rect = {x, y, 1, 1};
      gegl_buffer_get (buffer, &rect, 1.0, format2, (void*)&px[0],
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
    }
  }
  test_end ("gegl_buffer_get 1x1 + babl", SAMPLES * ITERATIONS * BPP);

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
  test_end ("gegl_buffer_sample near+ba", SAMPLES * ITERATIONS * BPP);

  test_start ();
  for (i = 0; i < ITERATIONS; i++)
  {
    int j;
    float px[4] = {0.2, 0.4, 0.1, 0.5};
    GeglSampler *sampler = gegl_buffer_sampler_new (buffer, format,
                                                    GEGL_SAMPLER_NEAREST);

    for (j = 0; j < SAMPLES; j ++)
    {
      int x = rands[j*2];
      int y = rands[j*2+1];
      gegl_sampler_get (sampler, x, y, NULL, (void*)&px[0], GEGL_ABYSS_NONE);
    }

    g_object_unref (sampler);
  }
  test_end ("gegl_sampler_get nearest", SAMPLES * ITERATIONS * BPP);

  test_start ();
  for (i = 0; i < ITERATIONS; i++)
  {
    int j;
    float px[4] = {0.2, 0.4, 0.1, 0.5};
    GeglSampler *sampler = gegl_buffer_sampler_new (buffer, format, 
                                                    GEGL_SAMPLER_NEAREST);
    GeglSamplerGetFun sampler_get_fun = gegl_sampler_get_fun (sampler);

    for (j = 0; j < SAMPLES; j ++)
    {
      int x = rands[j*2];
      int y = rands[j*2+1];
      sampler_get_fun (sampler, x, y, NULL, (void*)&px[0], GEGL_ABYSS_NONE);
    }

    g_object_unref (sampler);
  }
  test_end ("sampler_get_fun nearest", SAMPLES * ITERATIONS * BPP);


  test_start ();
  for (i = 0; i < ITERATIONS; i++)
  {
    int j;
    float px[4] = {0.2, 0.4, 0.1, 0.5};
    GeglSampler *sampler = gegl_buffer_sampler_new (buffer, format2, 
                                                    GEGL_SAMPLER_NEAREST);
    GeglSamplerGetFun sampler_get_fun = gegl_sampler_get_fun (sampler);

    for (j = 0; j < SAMPLES; j ++)
    {
      int x = rands[j*2];
      int y = rands[j*2+1];
      sampler_get_fun (sampler, x, y, NULL, (void*)&px[0], GEGL_ABYSS_NONE);
    }

    g_object_unref (sampler);
  }
  test_end ("sampler_get_fun nearest+babl", SAMPLES * ITERATIONS * BPP);


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
                          GEGL_SAMPLER_LINEAR, GEGL_ABYSS_NONE); 
    }
  }
  test_end ("gegl_buffer_sample linear", SAMPLES * ITERATIONS * BPP);

  test_start ();
  for (i = 0; i < ITERATIONS; i++)
  {
    int j;
    float px[4] = {0.2, 0.4, 0.1, 0.5};
    GeglSampler *sampler = gegl_buffer_sampler_new (buffer, format, 
                                                    GEGL_SAMPLER_LINEAR);

    for (j = 0; j < SAMPLES; j ++)
    {
      int x = rands[j*2];
      int y = rands[j*2+1];
      gegl_sampler_get (sampler, x, y, NULL, (void*)&px[0], GEGL_ABYSS_NONE);
    }

    g_object_unref (sampler);
  }
  test_end ("gegl_sampler_get linear", SAMPLES * ITERATIONS * BPP);

  test_start ();
  for (i = 0; i < ITERATIONS; i++)
  {
    int j;
    float px[4] = {0.2, 0.4, 0.1, 0.5};
    GeglSampler *sampler = gegl_buffer_sampler_new (buffer, format, 
                                                    GEGL_SAMPLER_LINEAR);
    GeglSamplerGetFun sampler_get_fun = gegl_sampler_get_fun (sampler);

    for (j = 0; j < SAMPLES; j ++)
    {
      int x = rands[j*2];
      int y = rands[j*2+1];
      sampler_get_fun (sampler, x, y, NULL, (void*)&px[0], GEGL_ABYSS_NONE);
    }

    g_object_unref (sampler);
  }
  test_end ("sampler_get_fun linear", SAMPLES * ITERATIONS * BPP);

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
                          GEGL_SAMPLER_CUBIC, GEGL_ABYSS_NONE); 
    }
  }
  test_end ("gegl_buffer_sample cubic", SAMPLES * ITERATIONS * BPP);

  test_start ();
  for (i = 0; i < ITERATIONS; i++)
  {
    int j;
    float px[4] = {0.2, 0.4, 0.1, 0.5};
    GeglSampler *sampler = gegl_buffer_sampler_new (buffer, format, 
                                                    GEGL_SAMPLER_CUBIC);

    for (j = 0; j < SAMPLES; j ++)
    {
      int x = rands[j*2];
      int y = rands[j*2+1];
      gegl_sampler_get (sampler, x, y, NULL, (void*)&px[0], GEGL_ABYSS_NONE);
    }

    g_object_unref (sampler);
  }
  test_end ("gegl_sampler_get cubic", SAMPLES * ITERATIONS * BPP);

  test_start ();
  for (i = 0; i < ITERATIONS; i++)
  {
    int j;
    float px[4] = {0.2, 0.4, 0.1, 0.5};
    GeglSampler *sampler = gegl_buffer_sampler_new (buffer, format, 
                                                    GEGL_SAMPLER_CUBIC);
    GeglSamplerGetFun sampler_get_fun = gegl_sampler_get_fun (sampler);

    for (j = 0; j < SAMPLES; j ++)
    {
      int x = rands[j*2];
      int y = rands[j*2+1];
      sampler_get_fun (sampler, x, y, NULL, (void*)&px[0], GEGL_ABYSS_NONE);
    }

    g_object_unref (sampler);
  }
  test_end ("sampler_get_fun cubic", SAMPLES * ITERATIONS * BPP);

  test_start ();
  for (i = 0; i < ITERATIONS; i++)
  {
    int j;
    float px[4] = {0.2, 0.4, 0.1, 0.5};
    GeglSampler *sampler = gegl_buffer_sampler_new (buffer, format, 
                                                    GEGL_SAMPLER_NOHALO);

    for (j = 0; j < SAMPLES; j ++)
    {
      int x = rands[j*2];
      int y = rands[j*2+1];
      gegl_sampler_get (sampler, x, y, NULL, (void*)&px[0], GEGL_ABYSS_NONE);
    }

    g_object_unref (sampler);
  }
  test_end ("gegl_sampler_get nohalo", SAMPLES * ITERATIONS * BPP);

  test_start ();
  for (i = 0; i < ITERATIONS; i++)
  {
    int j;
    float px[4] = {0.2, 0.4, 0.1, 0.5};
    GeglSampler *sampler = gegl_buffer_sampler_new (buffer, format, 
                                                    GEGL_SAMPLER_LOHALO);

    for (j = 0; j < SAMPLES; j ++)
    {
      int x = rands[j*2];
      int y = rands[j*2+1];
      gegl_sampler_get (sampler, x, y, NULL, (void*)&px[0], GEGL_ABYSS_NONE);
    }

    g_object_unref (sampler);
  }
  test_end ("gegl_sampler_get lohalo", SAMPLES * ITERATIONS * BPP);

  }



  g_free (buf);
  g_object_unref (buffer);

  return 0;
}
