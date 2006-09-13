#ifndef GEGL_OPTIONS
#define GEGL_OPTIONS

#include <glib.h>

typedef enum 
{
  GEGL_RUN_MODE_HELP,
  GEGL_RUN_MODE_INTERACTIVE,
  GEGL_RUN_MODE_PNG,
  GEGL_RUN_MODE_XML
} GeglRunMode;

typedef struct GeglOptions
{
  GeglRunMode  mode;

  const gchar *file;
  const gchar *xml;
  const gchar *output;

  gchar      **rest;

  gboolean     verbose;
  gboolean     stats;

  gfloat       delay;
} GeglOptions;

GeglOptions *
gegl_options_parse (gint    argc,
                    gchar **argv);

#endif
