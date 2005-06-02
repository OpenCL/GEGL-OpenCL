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
 *  Copyright 2003-2004 Daniel S. Rogers
 *
 */

#include <string.h>

#include <glib-object.h>

#include "image/gegl-image-types.h"

#include "image/gegl-null-cache-store.h"
#include "image/gegl-entry-record.h"

#include "gegl-image-mock-cache-entry.h"
#include "gegl-image-cache-store-tests.h"

#include "ctest.h"
#include "csuite.h"


static void
test_null_cache_store_new (Test * test)
{
  GeglNullCacheStore * null_store;

  null_store = gegl_null_cache_store_new (GEGL_DISCARDED);
  ct_test(test, GEGL_IS_NULL_CACHE_STORE(null_store) == TRUE);
  ct_test(test, g_type_parent(GEGL_TYPE_NULL_CACHE_STORE) == GEGL_TYPE_CACHE_STORE);
  ct_test(test, !strcmp("GeglNullCacheStore", g_type_name(GEGL_TYPE_NULL_CACHE_STORE)));
  g_object_unref(null_store);
}

static void
setup (Test * test)
{
  GeglNullCacheStore * null_store;
  null_store = gegl_null_cache_store_new(GEGL_DISCARDED);
  setup_cache_store_tests (GEGL_CACHE_STORE (null_store));
  g_object_unref (null_store);
}

static void
teardown (Test * test)
{
  teardown_cache_store_tests();
}

Test *
create_null_cache_store_test()
{
  Test* t = ct_create("GeglNullCacheStoreTest");


  g_assert(ct_addSetUp(t, setup));
  g_assert(ct_addTearDown(t, teardown));

  g_assert(ct_addTestFun(t, test_null_cache_store_new));
  g_assert(ct_addTestFun(t, test_cache_store_add_remove));
  g_assert(ct_addTestFun(t, test_cache_store_zap));
  g_assert(ct_addTestFun(t, test_cache_store_pop));
  g_assert(ct_addTestFun(t, test_cache_store_peek));

  return t;
}
