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

#include "gegl-image-mock-cache-entry.h"

#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_mock_cache_entry_g_object_new(Test *test)
{
  GeglMockCacheEntry * mock_entry = gegl_mock_cache_entry_new (1024);
  ct_test(test, mock_entry!=NULL);
  ct_test(test, GEGL_IS_CACHE_ENTRY (mock_entry));
  ct_test(test, g_type_parent(GEGL_TYPE_MOCK_CACHE_ENTRY) == GEGL_TYPE_CACHE_ENTRY);
  ct_test(test, g_type_parent(GEGL_TYPE_CACHE_ENTRY) == G_TYPE_OBJECT);
  ct_test(test, !strcmp("GeglCacheEntry", g_type_name(GEGL_TYPE_CACHE_ENTRY)));
  
  g_object_unref(mock_entry);
}

static void
test_mock_cache_entry_flatten_tests(Test *test)
{
  GeglMockCacheEntry * mock_entry = gegl_mock_cache_entry_new (5);
  GeglCacheEntry * cache_entry = GEGL_CACHE_ENTRY (mock_entry);
  gint test_array[] = {0,1,2,3,4};
  gint i;
  memcpy (mock_entry->data, test_array, 5 * sizeof(gint));
  gint * flattened = g_new (gint, 5);
  
  ct_test(test, gegl_cache_entry_flattened_size(cache_entry) == 5 * sizeof(gint));
  gegl_cache_entry_flatten (cache_entry, flattened, sizeof(gint) * 5);
  for (i=0;i<5;i++)
    {
      ct_test (test, flattened[i] == i);
    }
  gegl_cache_entry_unflatten (cache_entry, flattened, sizeof(gint) * 5);
  for (i=0;i<5;i++)
    {
      ct_test (test, (mock_entry->data)[i] == i);
    }

  gegl_cache_entry_discard (cache_entry);
  ct_test(test, mock_entry->data == NULL);
  g_object_unref (mock_entry);
}


Test *
create_mock_cache_entry_test()
{
  Test* t = ct_create("GeglMockCacheEntryTest");
  /*
    g_assert(ct_addSetUp(t, color_test_setup));
    g_assert(ct_addTearDown(t, color_test_teardown));
  */
  g_assert(ct_addTestFun(t, test_mock_cache_entry_g_object_new));
  g_assert(ct_addTestFun(t, test_mock_cache_entry_flatten_tests));
  return t; 
}
