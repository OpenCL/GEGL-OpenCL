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

static gboolean
test_node_reconnect_many (void)
{
  gboolean result = TRUE;

  GeglNode *ptn, *child0, *child1, *child2;

  ptn    = gegl_node_new ();
  child0 = gegl_node_new_child (ptn,
                                "operation", "gegl:nop",
                                NULL);
  child1 = gegl_node_new_child (ptn,
                                "operation", "gegl:nop",
                                NULL);
  child2 = gegl_node_new_child (ptn,
                                "operation", "gegl:nop",
                                NULL);

  gegl_node_link (child0, child1);
  gegl_node_link (child0, child2);

  if (!(child0 == gegl_node_get_producer (child1, "input", NULL)))
    {
      g_warning ("Wrong producer node on child 1");
      result = FALSE;
    }
  
  if (!(child0 == gegl_node_get_producer (child2, "input", NULL)))
    {
      g_warning ("Wrong producer node on child 2");
      result = FALSE;
    }

  gegl_node_set (child0,
                 "operation", "gegl:color",
                 NULL);

  if (!(child0 == gegl_node_get_producer (child1, "input", NULL)))
    {
      g_warning ("Wrong producer node on child 1");
      result = FALSE;
    }

  if (!(child0 == gegl_node_get_producer (child2, "input", NULL)))
    {
      g_warning ("Wrong producer node on child 2");
      result = FALSE;
    }

  return result;
}

#define RUN_TEST(test_name) \
{ \
  if (test_name()) \
    { \
      printf ("" #test_name " ... PASS\n"); \
      tests_passed++; \
    } \
  else \
    { \
      printf ("" #test_name " ... FAIL\n"); \
      tests_failed++; \
    } \
  tests_run++; \
}

int main(int argc, char **argv)
{
  gint tests_run    = 0;
  gint tests_passed = 0;
  gint tests_failed = 0;

  gegl_init (0, NULL);
  g_object_set(G_OBJECT(gegl_config()),
               "swap", "RAM",
               "use-opencl", FALSE,
               NULL);

  RUN_TEST (test_node_reconnect_many)

  gegl_exit ();

  if (tests_passed == tests_run)
    return 0;
  return -1;

  return 0;
}
