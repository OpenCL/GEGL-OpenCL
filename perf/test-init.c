#include "test-common.h"

gint
main (gint    argc,
      gchar **argv)
{
  long ticks;

  test_start ();
  gegl_init (&argc, &argv);
  gegl_exit ();
  ticks = babl_ticks ()-ticks_start;
  g_print ("@ %s: %.2f seconds\n", "init", (ticks / 1000000.0));
  return 0;
}
