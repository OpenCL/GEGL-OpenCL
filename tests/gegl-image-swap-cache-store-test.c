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

#include "image/gegl-swap-cache-store.h"
#include "gegl-image-mock-cache-entry.h"
#include "image/gegl-entry-record.h"
#include "gegl-image-cache-store-tests.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>


static void
test_swap_cache_store_new (Test * test)
{
  GeglSwapCacheStore * swap_store;
  swap_store = gegl_swap_cache_store_new ("swap_cache.XXXXXX");
  ct_test(test, GEGL_IS_SWAP_CACHE_STORE(swap_store) == TRUE);
  ct_test(test, g_type_parent(GEGL_TYPE_SWAP_CACHE_STORE) == GEGL_TYPE_CACHE_STORE);
  ct_test(test, !strcmp("GeglSwapCacheStore", g_type_name(GEGL_TYPE_SWAP_CACHE_STORE)));
  g_object_unref(swap_store);
}

static void
setup (Test * test)
{
  GeglSwapCacheStore * swap_store;
  swap_store = gegl_swap_cache_store_new("swap_cache.XXXXXX");
  setup_cache_store_tests (GEGL_CACHE_STORE (swap_store));
  g_object_unref (swap_store);
}

static void
teardown (Test * test)
{
  teardown_cache_store_tests();
}

Test *
create_swap_cache_store_test()
{
  Test* t = ct_create("GeglSwapCacheStoreTest");


  g_assert(ct_addSetUp(t, setup));
  g_assert(ct_addTearDown(t, teardown));

  g_assert(ct_addTestFun(t, test_swap_cache_store_new));
  g_assert(ct_addTestFun(t, test_cache_store_add_remove));
  g_assert(ct_addTestFun(t, test_cache_store_zap));
  g_assert(ct_addTestFun(t, test_cache_store_size));
  g_assert(ct_addTestFun(t, test_cache_store_pop));
  g_assert(ct_addTestFun(t, test_cache_store_peek));

  return t;
}
