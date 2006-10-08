/* This file is part of GEGL editor -- a gtk frontend for GEGL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) 2003, 2004, 2006 Øyvind Kolås
 */

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

  gfloat       delay;
} GeglOptions;

GeglOptions *
gegl_options_parse (gint    argc,
                    gchar **argv);

#endif
