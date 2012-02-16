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

  char image_name_1 [1000];
  char image_name_2 [1000];
  
  if (argc == 3)
    {
      strcpy(image_name_1, argv[1]);
      strcpy(image_name_2, argv[2]);
    }
  else
    return FAILURE;

  gegl_init (&argc, &argv);

  /* process */
  {
    GeglNode *gegl, *sink;

    gegl = gegl_graph(sink = gegl_node ("gegl:png-save", "path", "out.png", NULL,
                                        gegl_node ("gegl:over", NULL,
                                                   gegl_node ("gegl:over", NULL,
                                                              gegl_node ("gegl:load", "path", image_name_1, NULL),
                                                              gegl_node ("gegl:load", "path", image_name_2, NULL)),
                                                   gegl_node ("gegl:translate", "x", 200.0, "y", 200.0, NULL,
                                                              gegl_node ("gegl:load", "path", image_name_2, NULL)))));
    gegl_node_process (sink);
    g_object_unref (gegl);
  }

  gegl_exit ();

  return retval;
}
