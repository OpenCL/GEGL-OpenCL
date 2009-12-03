#include <gegl.h>
#include <gegl/gegl-utils.h>
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

static inline int gegl_float_16_indext (float f)
{
  union
  {
    float          f;
    unsigned int   i;
    unsigned short s[2];
  } u;
  u.f = f;
  return u.s[1];
}



gint main (int argc, gchar **argv)
{
  gfloat val;
  gfloat foo;
  gdouble sum = 0;
  gint    count=0;
  GeglLookup *lookup;
  gint ticks;
  
  lookup = gegl_lookup_new (wrapped_sqrt, NULL);
  ticks = gegl_ticks ();
  for (val = 0; val < 1.0; val+=0.0000001)
    foo = gegl_lookup (lookup, val);
  ticks = gegl_ticks ()-ticks;
  g_print ("First run: %i\n", ticks);

  ticks = gegl_ticks ();
  for (val = 0; val < 1.0; val+=0.0000001)
    foo = gegl_lookup (lookup, val);
  ticks = gegl_ticks ()-ticks;
  g_print ("Second run: %i\n", ticks);

  ticks = gegl_ticks ();
  for (val = 0; val < 1.0; val+=0.0000001)
    foo = gegl_lookup (lookup, val);
  ticks = gegl_ticks ()-ticks;
  g_print ("Third run: %i\n", ticks);


  ticks = gegl_ticks ();
  for (val = 0; val < 1.0; val+=0.0000001)
    foo = sqrt (val);
  ticks = gegl_ticks ()-ticks;
  g_print ("Just sqrt: %i\n", ticks);
  gegl_lookup_free (lookup);

  {
    /* stack allocated */
    GeglLookup lookup = {passthrough, NULL, };
    for (val = 0.0, sum=0.0, count=0; val < 1.0; val+=0.000001, count++)
      sum += fabs (val-gegl_lookup (&lookup, val));
    g_printf ("Average error in range 0.0-1.0: %f\n", sum/count);
      foo = sqrt (val);
  }

  return 0;
}
