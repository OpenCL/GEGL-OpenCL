#include "gegl-init.h"
#include "gegl-types.h"

static gboolean gegl_initialized = FALSE;


static void
gegl_exit (void)
{

}

void
gegl_init (int *argc, char ***argv)
{
  if (gegl_initialized)
    return;



  g_atexit (gegl_exit);
  gegl_initialized = TRUE;
}
