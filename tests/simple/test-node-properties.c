/*
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
 * Copyright (C) 2013 Daniel Sabo
 */

#include "config.h"
#include <string.h>
#include <stdio.h>

#include "gegl.h"

#define SUCCESS  0
#define FAILURE -1

int main(int argc, char *argv[])
{
  int           result = SUCCESS;
  GeglNode     *graph  = NULL;
  GeglNode     *node   = NULL;
  GeglColor    *color  = NULL;
  double        x      = -5;
  double        y      = -5;
  char         *name   = NULL;
  gboolean      cache  = TRUE;

  /* Init */
  gegl_init (&argc, &argv);

  color = gegl_color_new ("rgb(0.0, 1.0, 0.0)");

  graph = gegl_node_new ();
  node  = gegl_node_new_child (graph,
                               "operation", "gegl:color",
                               "value",     color,
                               NULL);

  gegl_node_get (node, "operation", &name, NULL);
  
  if (!(!strcmp (name, "gegl:color")))
    {
      result = FAILURE;
      printf ("operation: %s\n", name); 
      goto abort;
    }

  gegl_node_set (node,
                 "operation", "gegl:translate",
                 "x", 50.0,
                 "y", 100.0,
                 NULL);
  
  gegl_node_get (node,
                 "operation", &name,
                 "x", &x,
                 "y", &y,
                 NULL);

  if (!(!strcmp (name, "gegl:translate") &&
        (int)x == 50 && (int)y == 100))
    {
      result = FAILURE;
      printf ("operation: %s\n", name); 
      printf ("x, y: %f, %f\n", x, y);
      goto abort;
    }

  gegl_node_set (node,
                 "x", 5.0,
                 "y", 10.0,
                 NULL);
  
  gegl_node_get (node,
                 "y", &y,
                 "operation", &name,
                 "x", &x,
                 NULL);
  
  if (!(!strcmp (name, "gegl:translate") &&
        (int)x == 5 && (int)y == 10))
    {
      result = FAILURE;
      printf ("operation: %s\n", name); 
      printf ("x, y: %f, %f\n", x, y);
      goto abort;
    }
  
  gegl_node_set (node,
                 "operation", "gegl:nop",
                 NULL);

  gegl_node_get (node, "operation", &name, NULL);
  
  if (!(!strcmp (name, "gegl:nop")))
    {
      result = FAILURE;
      printf ("operation: %s\n", name); 
      goto abort;
    }

  gegl_node_set (node,
                 "operation", "gegl:translate",
                 NULL);
  
  gegl_node_get (node,
                 "operation", &name,
                 "x", &x,
                 "y", &y,
                 NULL);

  if (!(!strcmp (name, "gegl:translate") &&
        (int)x == 0 && (int)y == 0))
    {
      result = FAILURE;
      printf ("operation: %s\n", name); 
      printf ("x, y: %f, %f\n", x, y);
      goto abort;
    }
  
  gegl_node_set (node,
                 "operation", "gegl:translate",
                 "name", "Brian",
                 "dont-cache", FALSE,
                 NULL);
  
  gegl_node_get (node,
                 "name", &name,
                 "dont-cache", &cache,
                 NULL);

  if (!(!strcmp (name, "Brian") &&
        cache == FALSE))
    {
      result = FAILURE;
      printf ("name:  %s\n", name); 
      printf ("cache: %d\n", cache); 
      goto abort;
    }
  
  gegl_node_set (node,
                 "dont-cache", TRUE,
                 "name", "Steve",
                 NULL);
  
  gegl_node_get (node,
                 "name", &name,
                 "dont-cache", &cache,
                 NULL);

  if (!(!strcmp (name, "Steve") &&
        cache == TRUE))
    {
      result = FAILURE;
      printf ("name: %s\n", name); 
      printf ("cache: %d\n", cache); 
      goto abort;
    }

abort:
  /* Cleanup */
  g_object_unref (graph);
  g_object_unref (color);
  gegl_exit ();

  return result;
}
