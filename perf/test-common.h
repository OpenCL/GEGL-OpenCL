#include <stdlib.h>
#include <glib.h>
#include "gegl.h"
#include "gegl/opencl/gegl-cl-init.h"

static long ticks_start;

typedef void (*t_run_perf)(GeglBuffer *buffer);

long babl_ticks (void); /* using babl_ticks instead of gegl_ticks
                           to be able to go further back in time */

void test_start (void);
void test_end (const gchar *id,
               glong        bytes);
void test_end_suffix (const gchar *id,
                      const gchar *suffix,
                      glong        bytes);
GeglBuffer *test_buffer (gint width,
                         gint height,
                         const Babl *format);
void do_bench(const gchar *id,
              GeglBuffer  *buffer,
              t_run_perf   test_func,
              gboolean     opencl);
void bench(const gchar *id,
           GeglBuffer  *buffer,
           t_run_perf   test_func );

void test_start (void)
{
  ticks_start = babl_ticks ();
}

void test_end_suffix (const gchar *id,
                      const gchar *suffix,
                      glong        bytes)
{
  long ticks = babl_ticks ()-ticks_start;
  g_print ("@ %s%s: %.2f megabytes/second\n",
       id, suffix, (bytes / 1024.0 / 1024.0)  / (ticks / 1000000.0));
}

void test_end (const gchar *id,
               glong        bytes)
{
    test_end_suffix (id, "", bytes);
}

/* create a test buffer of random data in -0.5 to 2.0 range 
 */
GeglBuffer *test_buffer (gint width,
                         gint height,
                         const Babl *format)
{
  GeglRectangle bound = {0, 0, width, height};
  GeglBuffer *buffer;
  gfloat *buf = g_malloc0 (width * height * 16);
  gint i;
  buffer = gegl_buffer_new (&bound, format);
  for (i=0; i < width * height * 4; i++)
    buf[i] = g_random_double_range (-0.5, 2.0);
  gegl_buffer_set (buffer, NULL, 0, babl_format ("RGBA float"), buf, 0);
  g_free (buf);
  return buffer;
}

void do_bench (const gchar *id,
               GeglBuffer  *buffer,
               t_run_perf   test_func,
               gboolean     opencl)
{
  gchar* suffix = "";

  g_object_set(G_OBJECT(gegl_config()),
               "use-opencl", opencl,
               NULL);

  if (opencl) {
    if ( ! gegl_cl_is_accelerated()) {
      g_print("OpenCL is disabled. Skipping OpenCL test\n");
      return;
    }
    suffix = " (OpenCL)";
  }

  // warm up
  test_func(buffer);

#define ITERATIONS 16
  test_start ();
  for (int i=0; i<ITERATIONS; ++i)
    {
      test_func(buffer);
    }
  test_end_suffix (id, suffix, gegl_buffer_get_pixel_count (buffer) * 16 * ITERATIONS);
#undef ITERATIONS
}

void bench (const gchar *id,
            GeglBuffer  *buffer,
            t_run_perf   test_func)
{
  do_bench(id, buffer, test_func, FALSE );
  do_bench(id, buffer, test_func, TRUE );
}

