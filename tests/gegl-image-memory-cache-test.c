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

#include "image/gegl-memory-cache.h"

#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_gegl_memory_cache_g_object_new(Test * test)
{
  GeglMemoryCache * mem_cache = g_object_new (GEGL_TYPE_MEMORY_CACHE,
					      "soft_limit", G_GINT64_CONSTANT(1024),
					      "hard_limit", G_GINT64_CONSTANT(4096),
					      "persistent", FALSE,
					      NULL);
  ct_test(test,mem_cache!=NULL);
  ct_test(test, GEGL_IS_MEMORY_CACHE(mem_cache));
  ct_test(test, g_type_parent(GEGL_TYPE_MEMORY_CACHE) == GEGL_TYPE_CACHE);
  ct_test(test, !strcmp("GeglMemoryCache", g_type_name(GEGL_TYPE_MEMORY_CACHE)));
  g_object_unref(mem_cache);
}

Test *
create_memory_cache_test()
{
  Test* t = ct_create("GeglMemoryCacheTest");
  
  /*
    g_assert(ct_addSetUp(t, color_test_setup));
    g_assert(ct_addTearDown(t, color_test_teardown));
  */
  g_assert(ct_addTestFun(t, test_gegl_memory_cache_g_object_new));
  return t; 
}
