/*
 *   This file is part of GEGL.
 *
 *    GEGL is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    GEGL is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with GEGL; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright 2003 Daniel S. Rogers
 *
 */

#include "image/gegl-buffer-double.h"

#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_buffer_double_g_object_new(Test *test) {
	GeglBuffer* buffer_double=g_object_new(GEGL_TYPE_BUFFER_DOUBLE,
					       "num_banks",1,
					       "elements_per_bank",400,
					       NULL);
	ct_test(test,buffer_double!=NULL);
	ct_test(test, GEGL_IS_BUFFER_DOUBLE(buffer_double));
	ct_test(test, g_type_parent(GEGL_TYPE_BUFFER_DOUBLE) == GEGL_TYPE_BUFFER);
	ct_test(test, !strcmp("GeglBufferDouble", g_type_name(GEGL_TYPE_BUFFER_DOUBLE)));
	
	g_object_unref(buffer_double);
}


static void
test_buffer_double_properties(Test* test) {
    GeglBufferDouble* buffer_double=g_object_new(GEGL_TYPE_BUFFER_DOUBLE,
                                                 "num_banks", 42,
                                                 "elements_per_bank", 47,
                                                 NULL);
    GObject* object=G_OBJECT(buffer_double);
    gint num_banks;
    gint elements_per_bank;
    g_object_get(object, "num_banks",&num_banks,NULL);
    ct_test(test, num_banks == 42);

    g_object_get(object, "elements_per_bank",&elements_per_bank,NULL);
    ct_test(test, elements_per_bank == 47);

    g_object_unref(buffer_double);
}

static void
test_buffer_double_get_set(Test* test) {
  GeglBufferDouble* buffer_double=g_object_new(GEGL_TYPE_BUFFER_DOUBLE,
					       "num_banks", 10,
					       "elements_per_bank", 10,
					       NULL);
  GeglBuffer* buffer=GEGL_BUFFER(buffer_double);
  gint bank;
  gint element;

  for (bank=0;bank<10;bank++) {
    for (element=0;element<10;element++) {
      gegl_buffer_set_element_double(buffer,bank,element,bank+element);
    }
  }

  gboolean all_ok=TRUE;

  for (bank=0;bank<10;bank++) {
    for (element=0;element<10;element++) {
      if (gegl_buffer_get_element_double(buffer,bank,element) !=
	  bank+element) {
	all_ok=FALSE;
	break;
      }
    }
    if (all_ok==FALSE) {
      break;
    }
  }

  ct_test(test,all_ok==TRUE);

  g_object_unref(buffer_double);
}

static void
test_buffer_double_factory(Test* test) {
    GeglBufferDouble* buffer_double=
      (GeglBufferDouble*)gegl_buffer_create(TYPE_DOUBLE,
					    "num_banks",1,
					    "elements_per_bank",256,
					    NULL);
    ct_test(test,GEGL_IS_BUFFER_DOUBLE(buffer_double));

    g_object_unref(buffer_double);
}

Test *
create_buffer_double_test()
{
  Test* t = ct_create("GeglBufferDoubleTest");
  /*
    g_assert(ct_addSetUp(t, color_test_setup));
    g_assert(ct_addTearDown(t, color_test_teardown));
  */
  g_assert(ct_addTestFun(t, test_buffer_double_g_object_new));
  g_assert(ct_addTestFun(t, test_buffer_double_factory));
  g_assert(ct_addTestFun(t, test_buffer_double_properties));
  g_assert(ct_addTestFun(t, test_buffer_double_get_set));
  return t;
}
