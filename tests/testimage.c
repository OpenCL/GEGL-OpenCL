/*
 *   This file is part of GEGL.
 *
 *    GEGL is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Foobar is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Foobar; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright 2003 Daniel S. Rogers
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"

extern Test * create_buffer_double_test();
extern Test* create_component_sample_model_test();

int
main (int argc, char *argv[])
{  

  g_log_set_always_fatal (g_log_set_always_fatal (G_LOG_FATAL_MASK) |
                                                 G_LOG_LEVEL_WARNING | 
                                                 G_LOG_LEVEL_CRITICAL);
  g_type_init_with_debug_flags (G_TYPE_DEBUG_OBJECTS);

  gegl_init(&argc, &argv);

  gegl_log_direct("Hello there");

  {
    Suite *suite = cs_create("GeglTestSuite");

#if 1 
    cs_addTest(suite, create_buffer_double_test());
    cs_addTest(suite, create_component_sample_model_test());
#endif

    cs_setStream(suite, stdout);
    cs_run(suite);
    cs_report(suite);
    cs_destroy(suite, TRUE);
  }

  return 0;
}
