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

#include "image/gegl-swap-cache.h"
#include "gegl-image-mock-cache-entry.h"
#include "gegl-image-cache-tests.h"
#include "ctest.h"
#include <string.h>
static void
gegl_swap_cache_new_test (Test * test)
{

  GeglSwapCache* swap_cache = gegl_swap_cache_new ("swap_cache_test.XXXXXX", 50000);
  ct_test(test,swap_cache!=NULL);
  ct_test(test, GEGL_IS_SWAP_CACHE(swap_cache));
  ct_test(test, g_type_parent(GEGL_TYPE_SWAP_CACHE) == GEGL_TYPE_CACHE);
  ct_test(test, !strcmp("GeglSwapCache", g_type_name(GEGL_TYPE_SWAP_CACHE)));

  g_object_unref(swap_cache);

}

static void
setup_prime (Test * test)
{

  GeglCache * cache = GEGL_CACHE(gegl_swap_cache_new ("swap_cache_test.XXXXXX", 50000));
  setup_cache_leaks(cache);
  setup_prime_entries();

}
static void
teardown_prime(Test * test)
{

  teardown_cache_leaks();
  teardown_prime_entries();

}
Test *
create_swap_cache_mem_leaks_test()
{
  Test* t = ct_create("GegSwapCache Test");
  g_assert(ct_addSetUp(t, setup_prime));
  g_assert(ct_addTearDown(t, teardown_prime));

  g_assert(ct_addTestFun(t, gegl_swap_cache_new_test));
  g_assert(ct_addTestFun(t, gegl_cache_leak_test1));
  g_assert(ct_addTestFun(t, gegl_cache_leak_test3));
  g_assert(ct_addTestFun(t, gegl_cache_leak_test4));
  g_assert(ct_addTestFun(t, gegl_cache_leak_test5));
  g_assert(ct_addTestFun(t, gegl_cache_leak_test6));
  return t;
}
