/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2011 Victor M. de Araujo Oliveira <victormatheus@gmail.com>
 */

#include <string.h>
#include <assert.h>
#include <babl/babl.h>
#include <sys/time.h>

#include "gegl.h"
#include "gegl-types.h"
#include "gegl-utils.h"
#include "gegl-cl.h"

#define SUCCESS 0
#define FAILURE (-1)

gint
main (gint    argc,
      gchar **argv)
{
  gint retval = SUCCESS;

  gfloat brightness = 0.5f;
  gfloat contrast   = 2.0f;

  char image_name[1000] = "GEGL.png";
  if (argc > 1)
    strcpy(image_name, argv[1]);

  gegl_init (&argc, &argv);

  /* process */
  {
    GeglNode *gegl, *sink, *bc,  *load;

    gegl = gegl_graph(sink  = gegl_node ("gegl:png-save", "path", "out.png", NULL,
                        bc  = gegl_node ("gegl:brightness-contrast", "brightness", brightness, "contrast", contrast, NULL,
                      load  = gegl_node ("gegl:load", "path", image_name, NULL))));

    gegl_node_process (sink);
    g_object_unref (gegl);
  }

  gegl_exit ();

  return retval;
}
