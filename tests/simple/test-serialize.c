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
 * Copyright (C) 2016 OEyvind Kolaas
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include "gegl.h"
#include "gegl-plugin.h"

#define SUCCESS  0
#define FAILURE -1

typedef struct TestCase {
    gchar *argv_chain;
    gchar *expected_serialization;
    gchar *expected_error;
} TestCase;

TestCase tests[] = {
    {"invert", " gegl:invert-linear", ""},
    {"invert a=b", " gegl:invert-linear", "gegl:invert has no a property, but has "},
    {"invert a=c", " gegl:invert-linear", "gegl:invert has no a property, but has "},
    {"gaussian-blur", " gegl:gaussian-blur", ""},
    {"over aux=[ text string='foo bar' ]",  " svg:src-over aux=[ gegl:text string='foo bar' width=35 height=11 ]", ""},

    {"over aux= [ ",  " svg:src-over", "No such op 'gegl:['"}, 

    /* the following cause undesired warnings on console */
    {"over aux=[ string='foo bar' ]",  " svg:src-over", ""},

    /* the following should have error message */
    {"over aux=[ ",  " svg:src-over", ""},
    {"over aux=[ ]",  " svg:src-over", ""},

    {"exposure foo=2",  " gegl:exposure", "gegl:exposure has no foo property, but has 'exposure', 'offset', 'gamma', "},

#if 0
    {"id=foo over aux=[ ref=foo invert ]",  " id=001 svg:src-over aux=[ ref=001 gegl:invert-linear ]", ""},
#else
    {"id=foo over aux=[ ref=foo invert ]",  " svg:src-over aux=[ gegl:nop gegl:invert-linear ]", ""},
#endif

    /* the following causes crashes */
#if 0
    {"over aux=[]",  " svg:src-over", "No such op 'gegl:['"},  /* should report error message */
#endif
    {NULL, NULL}
};

static int
test_serialize (void)
{
  gint result = SUCCESS;
  gint res = SUCCESS;
  GeglNode *node = gegl_node_new ();
  GeglNode *start = gegl_node_new_child (node, "operation", "gegl:nop", NULL);
  GeglNode *end = gegl_node_new_child (node, "operation", "gegl:nop", NULL);
  int i;

  gegl_node_link_many (start, end, NULL);

  for (i = 0; tests[i].argv_chain; i++)
  {
    GError *error = NULL;
    gchar *serialization = NULL;
    gegl_create_chain (tests[i].argv_chain, start, end,
                    0.0, 500, &error);
    serialization = gegl_serialize (start, gegl_node_get_producer (end, "input", NULL));
    if (strcmp (serialization, tests[i].expected_serialization))
    {
      printf ("%s\nexpected:\n%s\nbut got:\n%s\n", 
                      tests[i].argv_chain,
                      tests[i].expected_serialization,
                      serialization);
      res = FAILURE;
    }
    g_free (serialization);
    if (error)
    {
      if (strcmp (error->message, tests[i].expected_error))
      {
       printf ("%s\nexpected error:\n%s\nbut got error:\n%s\n", 
                      tests[i].argv_chain,
                      tests[i].expected_error,
                      error->message);
       res = FAILURE;
      }
    }
    else if (tests[i].expected_error && tests[i].expected_error[0])
    {
       printf ("%s\ngot success instead of expected error:%s\n",
                      tests[i].argv_chain,
                      tests[i].expected_error);
       res = FAILURE;
    }

    if (res == FAILURE)
    {
      result = FAILURE;
      printf ("failed: %s\n", tests[i].argv_chain);
    }
    else
    {
      printf ("pass: %s\n", tests[i].argv_chain);
    }
  }
  g_object_unref (node);
  
  return result;
}


int main(int argc, char *argv[])
{
  gint result = SUCCESS;

  gegl_init (&argc, &argv);

  if (result == SUCCESS)
    result = test_serialize ();

  gegl_exit ();

  if (result == SUCCESS)
    fprintf (stderr, "\n:)\n");
  else
    fprintf (stderr, "\n:(\n");

  return result;
}
