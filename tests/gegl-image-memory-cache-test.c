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

#include "image/gegl-memory-cache.h"
#include "gegl-image-mock-cache-entry.h"
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

static void
test_gegl_memory_cache_properties(Test * test)
{
  GeglMemoryCache* mem_cache = g_object_new (GEGL_TYPE_MEMORY_CACHE,
				  "soft_limit", G_GINT64_CONSTANT(1024),
				  "hard_limit", G_GINT64_CONSTANT(4096),
				  "persistent", FALSE,
				  NULL);
  GObject * object = G_OBJECT (mem_cache);
  guint64 soft_limit;
  guint64 hard_limit;
  gboolean persistent;
  g_object_get (object,
		"soft_limit", &soft_limit,
		"hard_limit", &hard_limit,
		"persistent", &persistent,
		NULL);
  ct_test(test, soft_limit == 1024);
  ct_test(test, hard_limit == 4096);
  ct_test(test, persistent == FALSE);
  g_object_unref (mem_cache);
}

static void
test_gegl_memory_cache_caching(Test* test)
{
  GeglMemoryCache* mem_cache = g_object_new (GEGL_TYPE_MEMORY_CACHE,
				   "soft_limit", G_GINT64_CONSTANT(1024),
				   "hard_limit", G_GINT64_CONSTANT(4096),
				   "persistent", FALSE,
				   NULL);
  GeglCache * cache = GEGL_CACHE (mem_cache);

  GArray * entries = g_array_new (FALSE, TRUE, sizeof(GeglMockCacheEntry*));
  GArray * ids = g_array_new (FALSE, TRUE, sizeof(gsize));
  gint i,j,k;
  GeglMockCacheEntry * mock_entry;
  for (i=0;i<8;i++)
    {
      gint j;
      mock_entry = gegl_mock_cache_entry_new (1024);
      g_array_insert_val (entries, i, mock_entry);
      for (j=0;j<1024;j++)
	{
	  mock_entry->data[j] = i;
	}
    }
  for (i=0;i<8;i++)
    {
      GeglCacheEntry * entry;
      gsize id;
      gint results;
      entry = g_array_index (entries, GeglCacheEntry *, i);
      id=0;
      results = gegl_cache_put (cache, entry, &id);
      ct_test (test, results == GEGL_PUT_SUCCEEDED);
      /* Normally one would unref the entry after putting it in.  but
       * we want to test some things so we'll keep a reference.
       */
      /*
	g_object_unref (entry);
      */
      g_array_insert_val (ids, i, id);
      if (i == 3)
	{
	  /*
	    At this point, the cache should be full, with the first
	    four entries fetchable from the cache.
	   */
	  for (j=0;j<=i;j++)
	    {
	      GeglMockCacheEntry * mock_entry;
	      gboolean data_ok=TRUE;
	      id = g_array_index (ids, gsize, j);
	      results = gegl_cache_fetch (cache, id, &entry);
	      mock_entry = GEGL_MOCK_CACHE_ENTRY (entry);
	      ct_test (test, results == GEGL_FETCH_SUCCEEDED);
	      	      
	      for (k=0;k<1024;k++)
		{
		  if (mock_entry->data[k] != j)
		    {
		      data_ok=FALSE;
		      break;
		    }
		}
	      ct_test (test, data_ok == TRUE);
	      gegl_cache_put (cache, entry, &id);
	      g_object_unref (entry);
	    }
	}
      if (i == 4)
	{
	  /*
	    At this point, the first 3 entries should have expired
	    from the cache. (e.g hit the hard_limit and dropping the
	    capaicty to the soft_limit and then added the fifth entry
	  */
	  for (j=0;j<3;j++)
	    {
	      id = g_array_index (ids, gsize, j);
	      results = gegl_cache_fetch (cache, id, &entry);
	      ct_test (test, results == GEGL_FETCH_EXPIRED);
	    }
	  for (j=3;j<=i;j++)
	    {
	      id = g_array_index (ids, gsize, j);
	      results = gegl_cache_fetch (cache, id, &entry);
	      ct_test (test, results == GEGL_FETCH_SUCCEEDED);
	      gegl_cache_put (cache, entry, &id);
	      g_object_unref (entry);
	    }
	}
      if (i == 6)
	{
	  /*
	   * At this point, the 4th, 5th, 6th and 7th entries should
	   * be in the cache.  (that is indices 3, 4, 5, and 6).
	   *
	   * We are going to fetch and put back the 5th entry before
	   * putting in the 8th entry, which will trigger a flushing
	   * of the 3 oldest tiles, which, should be 4th, 6th and 7th,
	   * keeping the 8th and 5th entries.
	   */
	  id = g_array_index (ids, gsize, 4);
	  results = gegl_cache_fetch (cache, id, &entry);
	  ct_test (test, results == GEGL_FETCH_SUCCEEDED);
	  gegl_cache_put (cache, entry, &id);
	}
      if (i == 7)
	{
	  for (j=0;j<=i;j++)
	    {
	      id = g_array_index (ids, gsize, j);
	      results = gegl_cache_fetch (cache, id, &entry);
	      /*fprintf (stderr, "%d:%d  ", j, results);*/
	      if ( (j == 4) || (j == 7))
		{
		  ct_test (test, results == GEGL_FETCH_SUCCEEDED);
		  gegl_cache_put (cache, entry, &id);
		}
	      else
		{
		  ct_test (test, results == GEGL_FETCH_EXPIRED);
		}
	    }
	}
    }
  g_object_unref (cache);
  
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
  g_assert(ct_addTestFun(t, test_gegl_memory_cache_properties));
  g_assert(ct_addTestFun(t, test_gegl_memory_cache_caching));
  return t; 
}
