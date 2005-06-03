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

#include <glib-object.h>

#include "image/gegl-image-types.h"

#include "gegl-image-cache-tests.h"


static GeglCache           *cache;
static GeglMockCacheEntry **entries;

const gint num_entries     = 20;
const gint min_cache_size  = 10000 * sizeof (gint) * 20;

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
static const gint num_primes = G_N_ELEMENTS(primes);

static void
free_entries (GeglMockCacheEntry ** entries);

static GeglMockCacheEntry **
create_prime_entries (gint num_entires);

static GeglMockCacheEntry **
create_const_entries (gint num_entries, gint num_elements);

void
setup_cache_leaks (GeglCache * theCache)
{
  g_assert (cache == NULL);
  cache = theCache;
}

void teardown_cache_leaks ()
{
  g_object_unref (cache);
  cache = NULL;
}

void setup_const_entries ()
{
  entries = create_const_entries (20, 10000);
}

void teardown_const_entries()
{
  free_entries(entries);
}

void setup_prime_entries()
{
  entries = create_prime_entries(20);
}
void teardown_prime_entries()
{
  free_entries(entries);
}

GeglMockCacheEntry **
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

GeglMockCacheEntry **
create_prime_entries (gint num_entries)
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

void
gegl_cache_leak_test1 (Test * test)
{
  /* put 5;
   * fetch 5;
   * unfetch 5;
   * flush 5;
   */

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

}

void
gegl_cache_leak_test2 (Test * test)
{


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
}

void
gegl_cache_leak_test3 (Test * test)
{
  /* put 10;
   * unref cache;
   */

  gint i;
  gsize entry_ids[20];
  for (i=0;i<20;i++)
    {
      entry_ids[i] = 0;
    }
  put_entries (cache, entries, entry_ids, 0, 10, test);
}

void
gegl_cache_leak_test4 (Test * test)
{
   /*
    * put 10;
    * fetch 5;
    * unref cache;
    */

  gint i;
  gsize entry_ids[20];
  for (i=0;i<20;i++)
    {
      entry_ids[i] = 0;
    }
  put_entries (cache, entries, entry_ids, 0, 10, test);
  fetch_entries (cache, entries, entry_ids, 0, 5, test);
}

void
gegl_cache_leak_test5 (Test * test)
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

}

void
gegl_cache_leak_test6 (Test * test)
{
  /*
   * put 10;
   * fetch 5;
   * mark_as_dirty 10;
   */

  gint i;
  gsize entry_ids[20];
  for (i=0;i<20;i++)
    {
      entry_ids[i] = 0;
    }
  put_entries (cache, entries, entry_ids, 0, 10, test);
  fetch_entries (cache, entries, entry_ids, 0, 5, test);
  dirty_entries (cache, entries, entry_ids, 0, 10, test);
}

