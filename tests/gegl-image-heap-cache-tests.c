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

#include "image/gegl-heap-cache.h"
#include "gegl-image-mock-cache-entry.h"
#include "image/gegl-cache.h"
#include "ctest.h"
#include "string.h"

typedef void (*TestFun) (Test * test);

static const gint num_entries = 20;
static const gint num_primes = 100;
static const gint primes[] =
{
9923, 6833, 3943, 8669, 1259, 4679, 8707, 8887, 9391, 1621,
9829, 6121, 5527, 1907, 3221, 2293, 1933, 4799, 7589, 5501,
1151, 1019, 6983, 4153, 3371, 5443, 1423, 5303, 3119, 6197,
7753, 7883, 1663, 7907, 2851, 2861, 5849, 3613, 1091, 5939,
7549, 7603, 7321, 5323, 2683, 1117, 6379, 3001, 3541, 3067,
9049, 9439, 9551, 1291, 8807, 2381, 9431, 5003, 2777, 7489,
1543, 9839, 4139, 2579, 1861, 2999, 6269, 9311, 4909, 5419,
4201, 2341, 2221, 7477, 4937, 7127, 6841, 4561, 8243, 5483,
9689, 3517, 9851, 6043, 7993, 7247, 8219, 8537, 9257, 7517,
9377, 7963, 6367, 7639, 2699, 7789, 9203, 3847, 4409, 4159
};

static void
gegl_heap_cache_new_test (Test * test)
{
  GeglHeapCache* heap_cache = gegl_heap_cache_new (1024*1024*32, FALSE);
  ct_test(test,heap_cache!=NULL);
  ct_test(test, GEGL_IS_HEAP_CACHE(heap_cache));
  ct_test(test, g_type_parent(GEGL_TYPE_HEAP_CACHE) == GEGL_TYPE_CACHE);
  ct_test(test, !strcmp("GeglHeapCache", g_type_name(GEGL_TYPE_HEAP_CACHE)));
  
  g_object_unref(heap_cache);
}

static GeglMockCacheEntry **
create_const_entries (gint num_entries, gint num_elements)
{
  GeglMockCacheEntry ** entries = g_new (GeglMockCacheEntry *, num_entries);
  gint i=0;
  
  for (i=0;i<num_entries;i++)
    {
      entries[i] = gegl_mock_cache_entry_new (num_elements);
    }
  return entries;
}

static GeglMockCacheEntry **
create_prime_entries (gint num_entires)
{ 
  GeglMockCacheEntry ** entries = g_new (GeglMockCacheEntry *, num_entries);
  gint i=0;
  
  for (i=0;i<num_entries;i++)
    {
      entries[i] = gegl_mock_cache_entry_new (primes[i]);
    }
  return entries;
  
}

static void
free_entries (GeglMockCacheEntry ** entries)
{
  gint i;
  for (i=0;i<num_entries;i++)
    {
      if (entries[i] != NULL)
	{
	  g_object_unref (entries[i]);
	  entries[i] = NULL;
	}
    }
  g_free (entries);
  return;
}

static void
put_entries (GeglCache * cache,
	     GeglMockCacheEntry ** entries,
	     gsize entry_ids[],
	     gint start,
	     gint end,
	     Test * test)
{
  GeglPutResults put_results;
  gint i;
  for (i=start;i<end;i++)
    {
      put_results = gegl_cache_put (cache, GEGL_CACHE_ENTRY(entries[i]), &(entry_ids[i]));
      ct_test (test, put_results == GEGL_PUT_SUCCEEDED);
      g_object_unref (entries[i]);
      entries[i] = NULL;
    }
}

static void
fetch_entries (GeglCache * cache,
	       GeglMockCacheEntry ** entries,
	       gsize entry_ids[],
	       gint start,
	       gint end,
	       Test * test)
{
  GeglFetchResults fetch_results;
  gint i;
  for (i=start;i<end;i++)
    {
      GeglCacheEntry ** entry_handle = (GeglCacheEntry **)(&(entries[i]));
      fetch_results = gegl_cache_fetch (cache, entry_ids[i], entry_handle);
      ct_test (test, fetch_results == GEGL_FETCH_SUCCEEDED);
      ct_test (test, entries[i] != NULL);
    }
}

static void
unfetch_entries (GeglCache * cache,
		 GeglMockCacheEntry ** entries,
		 gsize entry_ids[],
		 gint start,
		 gint end,
		 Test * test)
{
  GeglFetchResults fetch_results;
  gint i;
  for (i=start;i<end;i++)
    {
      fetch_results = gegl_cache_unfetch (cache, entry_ids[i], GEGL_CACHE_ENTRY(entries[i]));
      ct_test (test, fetch_results == GEGL_FETCH_SUCCEEDED);
      ct_test (test, entries[i] != NULL);
      g_object_unref (entries[i]);
      entries[i] = NULL;
    }
}

static void
dirty_entries (GeglCache * cache,
	       GeglMockCacheEntry ** entries,
	       gsize entry_ids[],
	       gint start,
	       gint end,
	       Test * test)
{
  gint i;
  for (i=start;i<end;i++)
    {
      gegl_cache_mark_as_dirty (cache, entry_ids[i]);
    }
}

static void
flush_entries (GeglCache * cache,
	       GeglMockCacheEntry ** entries,
	       gsize entry_ids[],
	       gint start,
	       gint end,
	       Test * test)
{
  gint i;
  for (i=start;i<end;i++)
    {
      gegl_cache_flush (cache, entry_ids[i]);
    }
}


/*
 * We need to test, put, flush, fetch, unfetch, mark_as_dirty.
 *
 * The basic principle here is to stress test the cache; This is
 * what we are going to do.  First create 20 evenly sized
 * MockCacheEntries, and a heap cache capable of holding 20 entries
 * Then do the following tests.  The repeat these tests with the
 * prime sized CacheEntires.  These are all non-expiring tests.
 *
 * put 10;
 * fetch 5;
 * mark_as_dirty 10;
 */

static void
gegl_heap_cache_caching_test1 (GeglMockCacheEntry ** entries, Test * test)
{
  /* put 5;
   * fetch 5;
   * unfetch 5;
   * flush 5;
   */
  GeglCache * cache = GEGL_CACHE(gegl_heap_cache_new (10000 * sizeof(gint) * 20, FALSE));
  gint i;
  gsize entry_ids[20];
  for (i=0;i<20;i++)
    {
      entry_ids[i] = 0;
    }
  put_entries (cache, entries, entry_ids, 0, 5, test);
  fetch_entries (cache, entries, entry_ids, 0, 5, test);
  unfetch_entries (cache, entries, entry_ids, 0, 5, test);
  flush_entries (cache, entries, entry_ids, 0, 5, test);
  g_object_unref (cache);
 
}

static void
gegl_heap_cache_caching_test2 (GeglMockCacheEntry ** entries, Test * test)
{
  
  GeglCache * cache = GEGL_CACHE(gegl_heap_cache_new (10000 * sizeof(gint) * 20, FALSE));
  gint i;
  gsize entry_ids[20];
 /*
  * put 10;
  * fetch 5;
  * put 10 more;
  * unfetch 5;
  * flush 20
  */

  for (i=0;i<20;i++)
    {
      entry_ids[i] = 0;
    }
  put_entries (cache, entries, entry_ids, 0, 10, test);
  fetch_entries (cache, entries, entry_ids, 0, 5, test);
  put_entries (cache, entries, entry_ids, 10, 20, test);
  unfetch_entries (cache, entries, entry_ids, 0, 5, test);
  flush_entries (cache, entries, entry_ids, 0, 20, test);
  g_object_unref (cache);
}

static void
gegl_heap_cache_caching_test3 (GeglMockCacheEntry ** entries, Test * test)
{
  /* put 10;
   * unref cache;
   */
  GeglCache * cache = GEGL_CACHE(gegl_heap_cache_new (10000 * sizeof(gint) * 20, FALSE));
  gint i;
  gsize entry_ids[20];
  for (i=0;i<20;i++)
    {
      entry_ids[i] = 0;
    }
  put_entries (cache, entries, entry_ids, 0, 10, test);
  g_object_unref (cache);
}

static void
gegl_heap_cache_caching_test4 (GeglMockCacheEntry ** entries, Test * test)
{
   /*
    * put 10;
    * fetch 5;
    * unref cache;
    */
  GeglCache * cache = GEGL_CACHE(gegl_heap_cache_new (10000 * sizeof(gint) * 20, FALSE));
  gint i;
  gsize entry_ids[20];
  for (i=0;i<20;i++)
    {
      entry_ids[i] = 0;
    }
  put_entries (cache, entries, entry_ids, 0, 10, test);
  fetch_entries (cache, entries, entry_ids, 0, 5, test);
  g_object_unref (cache);
}

static void
gegl_heap_cache_caching_test5 (GeglMockCacheEntry ** entries, Test * test)
{
  /*
   * put 15;
   * fetch 5;
   * flush 5 (the fetched ones);
   * flush 5 (the unfetched ones);
   * put 5;
   * fetch the remaining 10;
   * unref cache;
   */
  GeglCache * cache = GEGL_CACHE(gegl_heap_cache_new (10000 * sizeof(gint) * 20, FALSE));
  gint i;
  gsize entry_ids[20];
  for (i=0;i<20;i++)
    {
      entry_ids[i] = 0;
    }
  put_entries (cache, entries, entry_ids, 0, 15, test);
  fetch_entries (cache, entries, entry_ids, 0, 5, test);
  flush_entries (cache, entries, entry_ids, 0, 5, test);
  flush_entries (cache, entries, entry_ids, 5, 10, test);
  put_entries (cache, entries, entry_ids, 15, 20, test);
  fetch_entries (cache, entries, entry_ids, 10, 20, test);
  g_object_unref (cache);
  
}

static void
gegl_heap_cache_caching_test6 (GeglMockCacheEntry ** entries, Test * test)
{
  /*
   * put 10;
   * fetch 5;
   * mark_as_dirty 10;
   */
  GeglCache * cache = GEGL_CACHE(gegl_heap_cache_new (10000 * sizeof(gint) * 20, FALSE));
  gint i;
  gsize entry_ids[20];
  for (i=0;i<20;i++)
    {
      entry_ids[i] = 0;
    }
  put_entries (cache, entries, entry_ids, 0, 10, test);
  fetch_entries (cache, entries, entry_ids, 0, 5, test);
  dirty_entries (cache, entries, entry_ids, 0, 10, test);
  g_object_unref (cache);
}

static void
gegl_heap_cache_memory_leaks_test (Test * test)
{
  GeglMockCacheEntry ** entries;
  entries = create_const_entries (20, 10000);
  gegl_heap_cache_caching_test1 (entries, test);
  free_entries (entries);

  entries = create_const_entries (20, 10000);
  gegl_heap_cache_caching_test2 (entries, test);
  free_entries (entries);

  entries = create_const_entries (20, 10000);
  gegl_heap_cache_caching_test3 (entries, test);
  free_entries (entries);

  entries = create_const_entries (20, 10000);
  gegl_heap_cache_caching_test4 (entries, test);
  free_entries (entries);

  entries = create_const_entries (20, 10000);
  gegl_heap_cache_caching_test5 (entries, test);
  free_entries (entries);

  entries = create_const_entries (20, 10000);
  gegl_heap_cache_caching_test6 (entries, test);
  free_entries (entries);

  entries = create_prime_entries (20);
  gegl_heap_cache_caching_test1 (entries, test);
  free_entries (entries);

  entries = create_prime_entries (20);
  gegl_heap_cache_caching_test2 (entries, test);
  free_entries (entries);

  entries = create_prime_entries (20);
  gegl_heap_cache_caching_test3 (entries, test);
  free_entries (entries);

  entries = create_prime_entries (20);
  gegl_heap_cache_caching_test4 (entries, test);
  free_entries (entries);

  entries = create_prime_entries (20);
  gegl_heap_cache_caching_test5 (entries, test);
  free_entries (entries);

  entries = create_prime_entries (20);
  gegl_heap_cache_caching_test6 (entries, test);
  free_entries (entries);

}

Test *
create_cache_mem_leaks_test()
{
  Test* t = ct_create("GegCache Mem Leaks Test");
  /*
    g_assert(ct_addSetUp(t, color_test_setup));
    g_assert(ct_addTearDown(t, color_test_teardown));
  */
  g_assert(ct_addTestFun(t, gegl_heap_cache_new_test));
  g_assert(ct_addTestFun(t, gegl_heap_cache_memory_leaks_test));
  return t; 
}
