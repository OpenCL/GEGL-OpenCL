/* This file is part of GEGL editor -- a gtk frontend for GEGL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2003, 2004, 2006 Øyvind Kolås
 */

#ifndef GEGL_OPTIONS
#define GEGL_OPTIONS

#include <glib.h>

typedef enum
{
  GEGL_RUN_MODE_HELP,
  GEGL_RUN_MODE_DISPLAY,
  GEGL_RUN_MODE_OUTPUT,
  GEGL_RUN_MODE_XML
} GeglRunMode;

typedef struct _GeglOptions GeglOptions;

struct _GeglOptions
{
  GeglRunMode  mode;

  const gchar *file;
  const gchar *xml;
  const gchar *output;

  GList       *files;

  gchar      **rest;

  gboolean     verbose;
  gboolean     fatal_warnings;

  gboolean     play;
};

GeglOptions *gegl_options_parse (gint    argc,
                                 gchar **argv);

/* used to let the file member traverse the files list back and forth */
gboolean gegl_options_next_file (GeglOptions *o);
gboolean gegl_options_previous_file (GeglOptions *o);

#endif
