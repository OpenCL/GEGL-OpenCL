#include <glib/gstdio.h>
#include <gegl.h>
#include <gegl-plugin.h>
#include <math.h>

static gfloat wrapped_sqrt (gfloat   in,
                            gpointer data) /* we could have used the user data */
{
  return sqrt (in);
}

static gfloat passthrough (gfloat in, gpointer data)
{
  return in;
}

glong gegl_ticks (void);

#define ITERATIONS 10
#define SAMPLES   10000000

gint main (int argc, gchar **argv)
{
  gint   i,j = 0;
  gfloat val;
  gfloat foo;
  gdouble sum = 0;
  gint    count=0;
  GeglLookup *lookup;
  gint ticks;
  gfloat *rand = g_malloc (SAMPLES * sizeof (gfloat));
  
  for (i=0;i<SAMPLES;i++)
    {
      rand[i]=g_random_double ();
    }

  lookup = gegl_lookup_new (wrapped_sqrt, NULL);
  ticks = gegl_ticks ();
  for (i=0;i<ITERATIONS;i++)
    for (j=0;j<SAMPLES;j++)
      foo = gegl_lookup (lookup, rand[j]);
  ticks = gegl_ticks ()-ticks;
  g_print ("First run:  %i\n", ticks);

  ticks = gegl_ticks ();
  for (i=0;i<ITERATIONS;i++)
    for (j=0;j<SAMPLES;j++)
      foo = gegl_lookup (lookup, rand[j]);
  ticks = gegl_ticks ()-ticks;
  g_print ("Second run: %i\n", ticks);

  ticks = gegl_ticks ();
  for (i=0;i<ITERATIONS;i++)
    for (j=0;j<SAMPLES;j++)
      foo = sqrt (rand[j]);
  ticks = gegl_ticks ()-ticks;
  g_print ("Just sqrt: %i\n", ticks);
  gegl_lookup_free (lookup);

  lookup = gegl_lookup_new (passthrough, NULL);
  for (val = 0.0, sum=0.0, count=0; val < 1.0; val+=0.000001, count++)
    sum += fabs (val-gegl_lookup (lookup, val));
  g_printf ("Average error in range 0.0-1.0: %f\n", sum/count);
  foo = sqrt (val);
  gegl_lookup_free (lookup);

  return 0;
}
